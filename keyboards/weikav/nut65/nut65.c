// Copyright 2026 Roman Kuzmitskii (@damex)
// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>

#include QMK_KEYBOARD_H
#include "battery.h"
#include "bluetooth.h"
#include "connection.h"
#include "fr800x.h"
#include "usb_util.h"
#include "wireless_2p4ghz.h"

#define INDICATOR_PULSE_RESET_MS  200
#define INDICATOR_PULSE_NORMAL_MS 500
#define NK_TOGG_FLASH_MS          250
#define EE_CLR_HOLD_MS            3000
#define UNPAIR_HOLD_MS            3000
#define SCANNER_GROUP_SIZE        10
#define SCANNER_SPEED_MS          25

typedef struct {
    deferred_token token;
    uint32_t       start_time;
} hold_state_t;

static hold_state_t      eeprom_clear_hold   = {INVALID_DEFERRED_TOKEN, 0};
static hold_state_t      unpair_current_hold = {INVALID_DEFERRED_TOKEN, 0};
static bool              battery_level_held  = false;
static bool              active_via_usb      = false;
static fr800x_target_t   active_target       = FR800X_TARGET_UNINITIALIZED;
static connection_host_t last_wireless_host  = CONNECTION_HOST_BLUETOOTH;
static bool              connection_indicator_reset_pending = false;
static deferred_token    connection_indicator_retry_token   = INVALID_DEFERRED_TOKEN;

static void hold_start(hold_state_t *hold, uint32_t hold_ms, deferred_exec_callback fire) {
    hold->start_time = timer_read32();
    hold->token      = defer_exec(hold_ms, fire, NULL);
    assert(hold->token != INVALID_DEFERRED_TOKEN);
}

static void hold_cancel(hold_state_t *hold) {
    if (hold->token != INVALID_DEFERRED_TOKEN) {
        cancel_deferred_exec(hold->token);
        hold->token = INVALID_DEFERRED_TOKEN;
    }
}

static uint32_t connection_indicator_retry_fire(uint32_t trigger_time, void *cb_arg);

typedef struct {
    uint8_t  led_index;
    RGB      color;
    uint32_t start_time;
    uint32_t duration_milliseconds;
} led_pulse_state_t;

static led_pulse_state_t led_pulse_state;

static void led_pulse_set(uint8_t led_index, RGB color, uint16_t duration_milliseconds) {
    led_pulse_state = (led_pulse_state_t){
        .led_index             = led_index,
        .color                 = color,
        .start_time            = timer_read32(),
        .duration_milliseconds = duration_milliseconds,
    };
}

static void led_pulse_apply(void) {
    if (led_pulse_state.duration_milliseconds == 0) {
        return;
    }
    if (timer_elapsed32(led_pulse_state.start_time) >= led_pulse_state.duration_milliseconds) {
        led_pulse_state.duration_milliseconds = 0;
        return;
    }
    RGB color = led_pulse_state.color;
    if (led_pulse_state.led_index == RGB_MATRIX_BLINK_INDEX_ALL) {
        rgb_matrix_set_color_all(color.r, color.g, color.b);
    } else if (led_pulse_state.led_index == RGB_MATRIX_BLINK_INDEX_BAR) {
        /* Scanner head walks on absolute time so retry-restarted pulses appear continuous. */
        uint8_t head = (uint8_t)((timer_read32() / SCANNER_SPEED_MS) % BATTERY_BAR_LED_COUNT);
        for (uint8_t group_offset = 0; group_offset < SCANNER_GROUP_SIZE; group_offset++) {
            uint8_t led = BATTERY_BAR_FIRST_LED + ((head + group_offset) % BATTERY_BAR_LED_COUNT);
            rgb_matrix_set_color(led, color.r, color.g, color.b);
        }
    } else if (led_pulse_state.led_index < RGB_MATRIX_LED_COUNT) {
        rgb_matrix_set_color(led_pulse_state.led_index, color.r, color.g, color.b);
    }
}

/* Mac/Win use different HID descriptors over BLE; chip swaps the advertised descriptor on this notification. */
static void send_os_profile_for_layer(uint8_t base_layer) {
    assert(base_layer == WIN_BASE || base_layer == MAC_BASE);
    fr800x_send_os_profile((base_layer == MAC_BASE) ? FR800X_OS_PROFILE_MAC : FR800X_OS_PROFILE_WIN);
}

/* USB peripheral lifecycle (start/stop on VBUS edge) handled by usb.dynamic. */
void usb_vbus_changed_kb(bool connected) {
    if (connected) {
        if (connection_get_host() != CONNECTION_HOST_USB) {
            connection_set_host_noeeprom(CONNECTION_HOST_USB);
        }
    } else {
        if (connection_get_host() == CONNECTION_HOST_USB) {
            connection_set_host_noeeprom(last_wireless_host);
        }
    }
    usb_vbus_changed_user(connected);
}

void keyboard_post_init_kb(void) {
    /* Latch USB_POWER_ENABLE low before flipping to output. Prevents momentary high glitch. */
    gpio_write_pin_low(USB_POWER_ENABLE_PIN);
    gpio_set_pin_output(USB_POWER_ENABLE_PIN);

    /* RF stack needs chip init before slot select. */
    fr800x_chip_init();
    bluetooth_set_profile(connection_get_bluetooth_profile());
    connection_host_changed_kb(connection_get_host());

    keyboard_post_init_user();

    send_os_profile_for_layer(get_highest_layer(default_layer_state));
}

/* Gated on post-init: boot-time EEPROM load fires this before chip handshake. */
layer_state_t default_layer_state_set_kb(layer_state_t state) {
    if (fr800x_is_chip_initialized()) {
        send_os_profile_for_layer(get_highest_layer(state));
    }
    return state;
}

void battery_charging_state_changed_kb(battery_charging_state_t state) {
    fr800x_relay_charging_state(state);
}

static void connection_indicator_cancel_retry(void) {
    if (connection_indicator_retry_token != INVALID_DEFERRED_TOKEN) {
        cancel_deferred_exec(connection_indicator_retry_token);
        connection_indicator_retry_token = INVALID_DEFERRED_TOKEN;
    }
}

static void connection_indicator_start_usb(bool reset) {
    connection_indicator_cancel_retry();
    uint16_t pulse_duration_milliseconds = reset ? INDICATOR_PULSE_RESET_MS : INDICATOR_PULSE_NORMAL_MS;
    led_pulse_set(RGB_MATRIX_BLINK_INDEX_BAR, (RGB){CONNECT_COLOR_USB}, pulse_duration_milliseconds);
}

/* Indexed by fr800x_target_t. Slot [FR800X_TARGET_UNINITIALIZED] is a zero-init hole; callers must validate via fr800x_target_is_pairable. */
static const RGB connection_indicator_colors[FR800X_TARGET_DONGLE + 1] = {
    [FR800X_TARGET_BLUETOOTH1] = {CONNECT_COLOR_BLUETOOTH},
    [FR800X_TARGET_BLUETOOTH2] = {CONNECT_COLOR_BLUETOOTH},
    [FR800X_TARGET_BLUETOOTH3] = {CONNECT_COLOR_BLUETOOTH},
    [FR800X_TARGET_DONGLE]     = {CONNECT_COLOR_DONGLE},
};

static void connection_indicator_start(fr800x_target_t new_target, bool reset) {
    if (!fr800x_target_is_pairable(new_target)) {
        assert(false);
        return;
    }

    connection_indicator_cancel_retry();
    connection_indicator_reset_pending   = reset;
    uint16_t pulse_duration_milliseconds = reset ? INDICATOR_PULSE_RESET_MS : INDICATOR_PULSE_NORMAL_MS;

    led_pulse_set(RGB_MATRIX_BLINK_INDEX_BAR, connection_indicator_colors[new_target], pulse_duration_milliseconds);
    connection_indicator_retry_token = defer_exec(pulse_duration_milliseconds, connection_indicator_retry_fire, NULL);
}

static uint32_t connection_indicator_retry_fire(uint32_t trigger_time, void *cb_arg) {
    connection_indicator_retry_token = INVALID_DEFERRED_TOKEN;

    if (active_via_usb) {
        return 0;
    }

    if (fr800x_get_state() == FR800X_STATE_CONNECTED) {
        led_wakeup();
        return 0;
    }

    connection_indicator_start(active_target, connection_indicator_reset_pending);
    return 0;
}

/* Chain stops at first CONNECTED tick; restart on later disconnect. */
void fr800x_receive_connection_cb(fr800x_state_t state) {
    if (active_via_usb) {
        return;
    }
    if (state == FR800X_STATE_CONNECTED) {
        led_wakeup();
        return;
    }
    if (fr800x_target_is_pairable(active_target)
        && connection_indicator_retry_token == INVALID_DEFERRED_TOKEN) {
        connection_indicator_start(active_target, false);
    }
}

static uint32_t eeprom_clear_fire(uint32_t trigger_time, void *cb_arg) {
    eeprom_clear_hold.token = INVALID_DEFERRED_TOKEN;

    /* Chip pairings live in chip flash, not WB32 EEPROM; drop them before reset. */
    fr800x_clear_all_pairings();

    eeconfig_init();
    mcu_reset();
    return 0;
}

/* Dispatches by current host: hold on a slot key clears the slot it switched to on press. */
static uint32_t unpair_current_fire(uint32_t trigger_time, void *cb_arg) {
    unpair_current_hold.token = INVALID_DEFERRED_TOKEN;
    switch (connection_get_host()) {
        case CONNECTION_HOST_BLUETOOTH:
            bluetooth_unpair();
            break;
        case CONNECTION_HOST_2P4GHZ:
            wireless_2p4ghz_unpair();
            break;
        default:
            /* USB or no active wireless transport: nothing to unpair. */
            break;
    }
    return 0;
}

static bool process_record_special_keycode(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case NK_TOGG:
            if (record->event.pressed) {
                led_pulse_set(RGB_MATRIX_BLINK_INDEX_ALL, (RGB){RGB_RED}, NK_TOGG_FLASH_MS);
            }
            break;
        case BT_PRF1:
        case BT_PRF2:
        case BT_PRF3:
        case OU_2P4G:
            if (record->event.pressed) {
                hold_start(&unpair_current_hold, UNPAIR_HOLD_MS, unpair_current_fire);
            } else {
                hold_cancel(&unpair_current_hold);
            }
            break;
        case BATTERY_LEVEL:
            battery_level_held = record->event.pressed;
            return false;
        case EE_CLR:
            if (record->event.pressed) {
                hold_start(&eeprom_clear_hold, EE_CLR_HOLD_MS, eeprom_clear_fire);
            } else {
                hold_cancel(&eeprom_clear_hold);
            }
            return false;
        case DF(WIN_BASE):
            if (record->event.pressed) {
                set_single_persistent_default_layer(WIN_BASE);
            }
            return false;
        case DF(MAC_BASE):
            if (record->event.pressed) {
                set_single_persistent_default_layer(MAC_BASE);
            }
            return false;
        default:
            break; /* normal-keycode passthrough */
    }
    return true;
}

bool process_record_kb(uint16_t keycode, keyrecord_t *record) {
    if (!process_record_user(keycode, record)) {
        return false;
    }

    if (fr800x_should_drop_basic(keycode, active_via_usb)) {
        return false;
    }

    return process_record_special_keycode(keycode, record);
}

static void render_battery_bar(uint8_t battery_percent) {
    uint8_t lit_count = (uint8_t)(((uint16_t)battery_percent * BATTERY_BAR_LED_COUNT) / 100U);
    for (uint8_t led_offset = 0; led_offset < BATTERY_BAR_LED_COUNT; led_offset++) {
        if (led_offset < lit_count) {
            rgb_matrix_set_color(BATTERY_BAR_FIRST_LED + led_offset, RGB_GREEN);
        } else {
            rgb_matrix_set_color(BATTERY_BAR_FIRST_LED + led_offset, RGB_RED);
        }
    }
}

static void render_hold_countdown(uint32_t start_time, uint32_t hold_ms, RGB color) {
    uint32_t elapsed = timer_elapsed32(start_time);
    if (elapsed > hold_ms) {
        elapsed = hold_ms;
    }
    uint8_t lit_count = (uint8_t)((elapsed * BATTERY_BAR_LED_COUNT) / hold_ms);
    for (uint8_t led_offset = 0; led_offset < lit_count; led_offset++) {
        rgb_matrix_set_color(BATTERY_BAR_FIRST_LED + led_offset, color.r, color.g, color.b);
    }
}

bool rgb_matrix_indicators_advanced_kb(uint8_t led_min, uint8_t led_max) {
    if (battery_level_held) {
        render_battery_bar(battery_get_percent());
    }

    if (host_keyboard_led_state().caps_lock) {
        rgb_matrix_set_color(RGB_INDEX_CAPS, RGB_WHITE);
    }

    led_pulse_apply();

    if (eeprom_clear_hold.token != INVALID_DEFERRED_TOKEN) {
        render_hold_countdown(eeprom_clear_hold.start_time, EE_CLR_HOLD_MS, (RGB){RGB_RED});
    }

    if (unpair_current_hold.token != INVALID_DEFERRED_TOKEN && !active_via_usb && fr800x_target_is_pairable(active_target)) {
        render_hold_countdown(unpair_current_hold.start_time, UNPAIR_HOLD_MS, connection_indicator_colors[active_target]);
    }

    return true;
}

static void apply_transport_switch(bool new_via_usb, fr800x_target_t new_target, bool reset) {
    bool transport_changed = active_via_usb != new_via_usb || active_target != new_target || reset;
    if (transport_changed) {
        /* Release on outgoing host; subsequent sends route via new transport. */
        report_keyboard_t zero_kb   = {0};
        report_nkro_t     zero_nkro = {0};
        host_keyboard_send(&zero_kb);
        host_nkro_send(&zero_nkro);
        clear_keyboard();
    }

    /* Update state BEFORE force_disconnect: connection callback fires with new context, not stale. */
    active_via_usb = new_via_usb;
    active_target  = new_target;

    if (transport_changed) {
        fr800x_force_disconnect();
    }
}

static void change_active_to_usb(bool reset) {
    apply_transport_switch(true, FR800X_TARGET_UNINITIALIZED, reset);
    fr800x_idle();
    connection_indicator_start_usb(reset);
    fr800x_notify_transport_change(true, FR800X_TARGET_UNINITIALIZED);
}

static void change_active_target(fr800x_target_t new_target, bool reset) {
    assert(fr800x_target_is_pairable(new_target));
    /* Mirror change_active_to_usb's direct call: fr800x_force_disconnect no-ops when state is already DISCONNECTED, so the callback chain that would start the indicator never fires. */
    connection_indicator_start(new_target, reset);
    apply_transport_switch(false, new_target, reset);
    fr800x_target_change(new_target, reset);
    fr800x_notify_transport_change(false, new_target);
}

void connection_host_changed_kb(connection_host_t host) {
    if (host == CONNECTION_HOST_BLUETOOTH || host == CONNECTION_HOST_2P4GHZ) {
        last_wireless_host = host;
    }
    switch (host) {
        case CONNECTION_HOST_USB:
            change_active_to_usb(false);
            break;
        case CONNECTION_HOST_BLUETOOTH:
            change_active_target(fr800x_target_from_bluetooth_profile(connection_get_bluetooth_profile()), false);
            break;
        case CONNECTION_HOST_2P4GHZ:
            change_active_target(FR800X_TARGET_DONGLE, false);
            break;
        default:
            /* CONNECTION_HOST_AUTO and CONNECTION_HOST_NONE: no transport action. */
            break;
    }
}

void connection_bluetooth_profile_changed_kb(uint8_t profile) {
    if (connection_get_host() != CONNECTION_HOST_BLUETOOTH) {
        return;
    }
    change_active_target(fr800x_target_from_bluetooth_profile(profile), false);
}

void suspend_wakeup_init_kb(void) {
    if (active_via_usb) {
        change_active_to_usb(false);
    } else {
        change_active_target(active_target, false);
    }
    suspend_wakeup_init_user();
}


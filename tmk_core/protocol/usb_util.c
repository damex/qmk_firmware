/* Copyright 2021 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "usb_util.h"
#include "gpio.h"
#include "timer.h"
#include "wait.h"

__attribute__((weak)) void usb_disconnect(void) {}

__attribute__((weak)) void usb_reconnect(void) {}

__attribute__((weak)) bool usb_connected_state(void) {
    return true;
}

__attribute__((weak)) bool usb_vbus_state(void) {
#ifdef USB_VBUS_PIN
    gpio_set_pin_input(USB_VBUS_PIN);
    wait_us(5);
    return gpio_read_pin(USB_VBUS_PIN);
#else
    return true;
#endif
}

__attribute__((weak)) void usb_vbus_changed_user(bool connected) {}
__attribute__((weak)) void usb_vbus_changed_kb(bool connected) {
    usb_vbus_changed_user(connected);
}

#ifndef USB_DYNAMIC_POLL_INTERVAL_MS
#    define USB_DYNAMIC_POLL_INTERVAL_MS 100
#endif

void usb_dynamic_task(void) {
#ifdef USB_DYNAMIC_ENABLE
    /* Matches chibios USB-up state at boot; no synthetic edge when cable present. */
    static bool     last_vbus_state     = true;
    static uint32_t last_vbus_poll_time = 0;

    if (timer_elapsed32(last_vbus_poll_time) < USB_DYNAMIC_POLL_INTERVAL_MS) {
        return;
    }
    last_vbus_poll_time = timer_read32();

    bool current_vbus_state = usb_vbus_state();
    if (current_vbus_state == last_vbus_state) {
        return;
    }
    last_vbus_state = current_vbus_state;

    if (current_vbus_state) {
        usb_reconnect();
    } else {
        usb_disconnect();
    }
    usb_vbus_changed_kb(current_vbus_state);
#endif
}

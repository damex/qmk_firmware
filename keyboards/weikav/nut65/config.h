// Copyright 2026 Roman Kuzmitskii (@damex)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

/* Power */
#define USB_POWER_ENABLE_PIN B1

/* Boot into USB by default; upstream connection layer picks up the rest. */
#define CONNECTION_HOST_DEFAULT CONNECTION_HOST_USB

/* Battery */
#define BATTERY_CHARGER_CABLE_PIN A7
#define BATTERY_CHARGER_CABLE_ACTIVE_HIGH
#define BATTERY_CHARGER_FULL_PIN A15
#define BATTERY_CHARGER_FULL_ACTIVE_HIGH

/* Front-panel underglow strip: 80 LEDs starting at chain index 71 (key LEDs occupy 0..70). */
#define BATTERY_BAR_FIRST_LED 71
#define BATTERY_BAR_LED_COUNT 80

/* led_pulse_set sentinels: BAR pulses the entire front-panel strip; ALL pulses every LED. */
#define RGB_MATRIX_BLINK_INDEX_BAR 0xFE
#define RGB_MATRIX_BLINK_INDEX_ALL 0xFF

#define CONNECT_COLOR_BLUETOOTH RGB_BLUE
#define CONNECT_COLOR_DONGLE    RGB_GREEN
#define CONNECT_COLOR_USB       RGB_WHITE

#define RGB_INDEX_CAPS 6 /* chain: T R E W Q Tab Caps */

/* UART */
#define UART_DRIVER SD3
#define UART_TX_PIN C10
#define UART_RX_PIN C11
#define UART_RX_PAL_MODE 7

/* Headroom for rapid BT<->2.4G transport switches (5 commands each) plus typing + probe + battery query. */
#define UART_FRAME_QUEUE_CAPACITY 64

/* SPI */
#define SPI_DRIVER SPIDQ
#define SPI_SCK_PIN B3
#define SPI_MOSI_PIN B5
#define SPI_MISO_PIN B4

/* Flash */
#define EXTERNAL_FLASH_SPI_SLAVE_SELECT_PIN C12
#define VIA_EEPROM_ALLOW_RESET

/* RGB Matrix */
#define RGB_MATRIX_TIMEOUT 300000

/* WS2812 */
#define WS2812_SPI_DRIVER SPIDM2
#define WS2812_SPI_DIVISOR 22 /* 72 MHz sysclk / 22 = 3.27 MHz; centers WS2812 pulses in spec */


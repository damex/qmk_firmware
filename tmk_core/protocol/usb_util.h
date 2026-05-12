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

#pragma once

#include <stdbool.h>

void usb_disconnect(void);

void usb_reconnect(void);

bool usb_connected_state(void);

bool usb_vbus_state(void);

/* Periodic VBUS poll; transitions the USB peripheral on edges when usb.dynamic is set. */
void usb_dynamic_task(void);

/* Fires on every VBUS edge while usb.dynamic is enabled. */
void usb_vbus_changed_kb(bool connected);
void usb_vbus_changed_user(bool connected);

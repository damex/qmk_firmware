# Weikav NUT65

Weikav NUT65 is a 65% tri-mode keyboard supporting USB, Bluetooth (3 profiles), and 2.4 GHz wireless, with per-key RGB matrix and underglow.

* Keyboard Maintainer: [Roman Kuzmitskii](https://github.com/damex)
* Hardware Supported: Weikav NUT65 (WB32FQ95 MCU, Freqchip FR8003A wireless module)
* Hardware Availability: [Weikav](https://weikav.com)

Make example for this keyboard (after setting up your build environment):

    make weikav/nut65:default

Flashing example for this keyboard:

    make weikav/nut65:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in 3 ways:

* **Bootmagic reset**: Hold the top-left key (Escape) while connecting the USB cable
* **Physical reset**: Toggle the battery switch (under Tab) off, then short the two unpopulated reset contacts on the PCB while connecting the USB cable
* **Keycode in layout**: Press the key mapped to `QK_BOOT` if it is available

## Layers

The default keymap defines five layers:

| Layer       | Index | Activation              | Purpose                              |
|-------------|-------|-------------------------|--------------------------------------|
| `WIN_BASE`  | 0     | `DF(WIN_BASE)` (default)| Windows base layer                   |
| `WIN_FN`    | 1     | `MO(WIN_FN)` on Fn      | Function keys, media, nav cluster    |
| `MAC_BASE`  | 2     | `DF(MAC_BASE)`          | macOS base layer                     |
| `MAC_FN`    | 3     | `MO(MAC_FN)` on Fn      | Mac-flavored function row + Fn       |
| `SETTINGS`  | 4     | `MO(SETTINGS)` from Fn  | Transport switch, reset, NKRO toggle |

## Custom Keycodes

| Keycode         | Alias     | Description                                                              |
|-----------------|-----------|--------------------------------------------------------------------------|
| `BATTERY_LEVEL` | `BA_LVL`  | Hold to display the battery level on the underglow as a green-to-red bar |

Standard QMK transport and Bluetooth keycodes are wired in the SETTINGS layer:

| Keycode             | Default position | Behavior                                                                                                          |
|---------------------|------------------|-------------------------------------------------------------------------------------------------------------------|
| `BT_PRF1`/`2`/`3`   | SETTINGS row 1/2/3 | Tap = switch to that Bluetooth profile. Hold 3 s = re-pair the current slot (front-panel bar fills in slot's color). |
| `OU_2P4G`           | SETTINGS row 4   | Tap = switch to 2.4 GHz dongle. Hold 3 s = re-pair the dongle.                                                    |
| `OU_USB`            | SETTINGS row 5   | Switch to USB. No hold action (USB has no pairing).                                                               |
| `EE_CLR`            | SETTINGS Backspace | Hold 3 s to factory-reset EEPROM and chip pairings, then reboot.                                                 |

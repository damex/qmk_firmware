# QMK fork for Weikav NUT65

Weikav NUT65 is a 65% tri-mode keyboard supporting USB, Bluetooth (3 profiles), and 2.4 GHz wireless, with per-key RGB matrix and underglow.

* MCU: WB32FQ95
* Bluetooth/2.4 GHz Wireless: Freqchip FR8003A
* Charger: XT4076 (Li-ion)
* LEDs: WS2812, 71 in keyboard area + 80 front-panel underglow strip (151 total)

Framework changes are [staged for upstream](#qmk-upstreaming-efforts).

## QMK Upstreaming Efforts

Each feature is split into a standalone branch and PR against [`qmk/qmk_firmware`](https://github.com/qmk/qmk_firmware). The branches stack: the board commit at the tip of this branch depends on all of them merging.

| # | Branch | PR | Scope |
|---|---|---|---|
| 1 | `feat/uart-frame` | [#26198](https://github.com/qmk/qmk_firmware/pull/26198) | Packet link layer over byte-level UART (ack + retry + framing) |
| 2 | `feat/bluetooth-multi-profile` | [#26199](https://github.com/qmk/qmk_firmware/pull/26199) | Bluetooth profile slots (`QK_BLUETOOTH_PROFILE_*`, `QK_BLUETOOTH_UNPAIR`) |
| 3 | `feat/battery-charger` | [#26202](https://github.com/qmk/qmk_firmware/pull/26202) | Battery charger state tracking via GPIO (cable-detect + full-detect) |
| 4 | `feat/2p4ghz` | [#26203](https://github.com/qmk/qmk_firmware/pull/26203) | 2.4 GHz wireless dongle API (`wireless_2p4ghz_*` weak hooks, `WL_UNPR`) |
| 5 | `feat/ws2812-wb32-non-pow2-divisor` | [#26205](https://github.com/qmk/qmk_firmware/pull/26205) | WS2812 SPI: allow non-power-of-2 divisor on WB32 |
| 6 | `feat/freqchip-fr800x-driver` | [#26207](https://github.com/qmk/qmk_firmware/pull/26207) (draft) | FR8003A driver: state machine, framing, target switching, battery, charging relay |
| 7 | `feat/ws2812-vcc-enable-pin` | [#26215](https://github.com/qmk/qmk_firmware/pull/26215) | WS2812: optional GPIO-gated VCC rail for standby savings |
| 8 | `feat/dynamic-usb-vbus` | *(not yet filed)* | Tear down USB peripheral on VBUS loss, re-init on edge |

## Hardware-verified

* USB host
* Bluetooth profiles 1, 2, and 3
* 2.4 GHz dongle
* Battery percentage reporting
* Charging-state relay (cable detect, charging, full)
* Pairing and re-pairing across all transports
* OS-profile swap (Win/Mac) on default-layer change
* Front-panel underglow battery bar
* Factory reset (chip pairings + EEPROM)

## Usage

For build instructions, bootloader entry, default keymap layers, and custom keycode behavior, see [`keyboards/weikav/nut65/readme.md`](../keyboards/weikav/nut65/readme.md).

# Dynamic USB Peripheral

QMK normally keeps the USB peripheral powered for the lifetime of the board, even with no host attached. On battery-powered keyboards the resulting idle current adds up. The dynamic USB feature tears the peripheral down on VBUS loss and brings it back when the cable is reconnected.

## Configuration

In `keyboard.json`:

```json
"usb": {
    "dynamic": true,
    "vbus_pin": "A0"
}
```

|Key       |Required|Description                                                                          |
|----------|--------|-------------------------------------------------------------------------------------|
|`dynamic` |Yes     |Enable the periodic VBUS poll and edge-driven peripheral cycling.                    |
|`vbus_pin`|Yes     |GPIO connected to a VBUS sense node, typically a divider off the USB connector's 5 V.|

The C-side macros are `USB_DYNAMIC_ENABLE` and `USB_VBUS_PIN`. The poll interval defaults to 100 ms; override with `#define USB_DYNAMIC_POLL_INTERVAL_MS <ms>` in `config.h`.

## Hooks

Override `usb_vbus_changed_kb(bool connected)` (or `_user`) to react to VBUS edges, e.g. for transport switching or indicator updates. The peripheral has already transitioned by the time the hook fires.

```c
void usb_vbus_changed_kb(bool connected) {
    if (connected) {
        /* cable in */
    } else {
        /* cable out */
    }
    usb_vbus_changed_user(connected);
}
```

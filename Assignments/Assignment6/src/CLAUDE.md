# CLAUDE.md

## TASK
create a backdoor as specified in the Assignment6b pdf. After the backdoor is enetered, every new odd numbered led event should be dropped. Currently the code does not perform correctly. 

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Must be built on Linux with kernel headers installed for the running kernel.

```bash
make          # build usbkbd.ko against the running kernel's headers
make clean    # remove all build artifacts (*.o, *.ko, *.mod.*, etc.)
```

## Loading and Testing the Module

```bash
sudo insmod usbkbd.ko          # load the module
sudo rmmod usbkbd              # unload the module
dmesg | tail -20               # view kernel log output from the driver
```

To rebind a USB keyboard from the default `usbhid` driver to this custom driver, use the script in the parent directory (adjust the USB path `1-2:1.0` to match your device):

```bash
sudo bash ../bind_driver.sh
```

Find the correct USB interface path with:
```bash
ls /sys/bus/usb/drivers/usbhid/
```

## Architecture

This is an out-of-tree Linux kernel module — a modified version of the upstream `drivers/hid/usbkbd.c` USB HID Boot Protocol keyboard driver. It registers as a `usb_driver` that handles any HID boot-class keyboard device.

**Key data structure:** `struct usb_kbd` holds per-device state including two URBs:
- `irq` URB — interrupt IN endpoint, receives 8-byte HID boot reports when keys are pressed/released
- `led` URB — control OUT endpoint, sends LED state (NumLock, CapsLock, ScrollLock, etc.) to the keyboard

**Key functions:**
- `usb_kbd_irq` — interrupt handler called on each keypress/release report; decodes the HID boot report by comparing `new[8]` vs `old[8]` buffers and calls `input_report_key`
- `usb_kbd_event` — called by the input subsystem when LED state should change; submits the `led` URB
- `usb_kbd_led` — completion handler for the `led` URB; resubmits if LED state changed again while in-flight
- `usb_kbd_probe` / `usb_kbd_disconnect` — standard USB driver probe/disconnect lifecycle

**Spinlock discipline:** `leds_lock` protects `leds`, `newleds`, `led_urb_submitted`, and the backdoor counters. `usb_kbd_event` uses `spin_lock_irqsave`; `usb_kbd_led` (runs in softirq context) also uses `spin_lock_irqsave`.

## Custom Additions (Assignment Modifications)

All added code is delimited by `/* -------- */` comment markers. The modifications add a "backdoor" mechanism:

**Trigger:** Pressing NumLock (HID scancode `0x53`) 3 consecutive times toggles `backdoor_open`. The counter `numlock_pressed` resets to 0 on the third press. A kernel alert is printed on toggle.

**Effect when backdoor is open:** In `usb_kbd_event`, every odd-numbered LED event (tracked by `led_events_in_backdoor`) is submitted then immediately unlinked via `usb_unlink_urb`, effectively dropping the LED state change. The `leds` buffer is restored to `oldleds` after unlinking. Even-numbered events pass through normally.

**Counters tracked in `struct usb_kbd`:**
- `numlock_pressed` — consecutive NumLock press count (resets at 3)
- `led_events_in_backdoor` — initialized to `-1`; incremented per LED event while backdoor open (so first event is 0, which is even and passes through)
- `led_dropped` — total dropped LED URBs
- `num_led_ack` — total successfully acknowledged LED URBs (incremented in `usb_kbd_led` on success)

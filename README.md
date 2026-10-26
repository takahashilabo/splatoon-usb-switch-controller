# splatoon-usb-switch-controller

Raspberry Pi Pico firmware that impersonates a Nintendo Switch Pro Controller
over USB, paired with a hardware USB switch that exclusively selects between
the real Pro Controller and the Pico. The Switch only ever sees one device at
a time, so there's no "player 2 gets ignored in single player" problem.

## Why this exists

An earlier attempt (`pseude-splatoon-controller`) used a Raspberry Pi 4 +
Bluetooth (nxbt) to pair as a Pro Controller and run an AI-driven dummy input
bot, with the real controller passed through. Two things killed that
approach:

- There was a measurable, consistent input-latency gap between the real
  Pro Controller and the Bluetooth emulation layer, even after trimming
  software overhead (double IPC hops, batching). The remaining delay comes
  from the Bluetooth/nxbt emulation layer itself, not something fixable in
  software.
- The "real controller as Player 1 + emulated controller as Player 2,
  simultaneously connected, mute/unmute to switch" idea doesn't work:
  confirmed on real hardware that the Switch ignores Player 2 input in
  single-player games.

So this project switches to an all-USB approach instead:

```
[Real Pro Controller] ───┐
                          ├─(USB switch)─── [Nintendo Switch]
[Pico (impersonator)] ────┘
```

The switch connects exactly one of the two devices to the Switch at a time.
When the real controller is selected, nothing is in the signal path (zero
added latency). When the Pico is selected, it presents itself as a Pro
Controller over USB and streams AI-driven input.

## Status

- Pico firmware enumerates correctly as a genuine Pro Controller (USB
  descriptors, strings, VID/PID 057E:2009) and streams standard input
  reports immediately on connect, matching empirically-probed behavior of a
  real Pro Controller.
- The `0x80`-series handshake replies were verified byte-for-byte against a
  real Pro Controller (see `pico/src/pro_con_protocol.c` probing notes).
- A real Nintendo Switch wired-controller test requires the official dock
  in TV mode (the console's own USB-C port does not accept wired controller
  input in handheld mode) — not yet confirmed working end-to-end on a real
  console.
- The hardware USB switch is currently a provisional jumper-wire mockup
  (D+/D- manually moved between the real controller and the Pico, GND/VBUS
  shared permanently); verified to correctly switch which device a USB host
  sees. A real DPDT slide switch is the planned permanent replacement.

## Layout

- `pico/src/pro_con_protocol.c` — Pro Controller USB protocol: handshake,
  subcommand replies, SPI-read emulation, standard input report packing.
- `pico/src/usb_descriptors.c` — USB device/configuration/HID report
  descriptors and string descriptors, copied from a real controller
  capture.
- `pico/src/splatoon_ai.c` — human-like dummy input generator (play-paint →
  hide → search → jump-shot state machine, finger inertia, noise), ported
  from the earlier Python prototype. Play styles: balanced, stealth,
  aggressive.
- `pico/src/main.c` — TinyUSB task loop, periodic input report scheduling.
- `HANDOFF.md` — background/design-decision notes from the pivot away from
  the Bluetooth approach.

## Building

Requires the Pico SDK (not vendored in this repo) and `pico_sdk_import.cmake`
copied from it.

```
cd pico
mkdir build && cd build
cmake ..
make -j4
```

Flash `splatoon_procon.uf2` to a Pico in BOOTSEL mode.

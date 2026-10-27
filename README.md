# splatoon-usb-switch-controller

**Status: working.** Raspberry Pi Pico firmware that impersonates a
HORI-licensed Switch controller over USB, paired with a hardware USB switch
that exclusively selects between the real Pro Controller and the Pico. The
Switch only ever sees one device at a time, so there's no "player 2 gets
ignored in single player" problem. Confirmed on a real console: the Pico
shows up as Player 1 on the "Change Grip/Order" screen and its AI-driven
dummy input moves the cursor.

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

**Confirmed working on a real Nintendo Switch, docked in TV mode:**

- The Pico shows up as a controller on the "Change Grip/Order" screen
  (Player 1), and the AI-driven dummy input visibly moves the cursor.
- The hardware switch (a 6-pin/DPDT slide switch, D+/D- only — see below)
  correctly toggles which device the Switch sees: real Pro Controller or
  Pico.
- Known quirk: switching back to the real Pro Controller doesn't always get
  picked up immediately — pressing a button (e.g. A) or the controller's
  sync/reset button is sometimes needed. Occasionally reconnects over
  Bluetooth instead of wired in that moment (see the Bluetooth-fallback
  note below).

**How recognition was actually achieved** — this took most of the effort,
and the path there is worth keeping on record:

Early attempts emulated a *genuine* Nintendo Pro Controller (VID/PID
057E:2009) with the full `0x80`-series handshake protocol. USB-descriptor
level enumeration worked fine on a Mac (generic host) and the handshake
replies were verified byte-for-byte against a real Pro Controller, but a
real Nintendo Switch console always stalled after exactly one command
(`0x80 05`) and went silent — no retries, no further commands — regardless
of reply content, streaming timing, or device serial/MAC. That 0x80-series
protocol is Nintendo's own and, it turns out, essentially undocumented for
the *console's* side of a wired handshake (every reference implementation
found — SDL, the Linux `hid-nintendo` driver, espp — is PC/host-side code
that was never itself validated against a real console as the host).

The fix was to stop emulating a genuine Pro Controller and instead emulate
a **HORI-licensed controller** (Pokken Tournament DX Pro Pad, VID/PID
0F0D:0092), which doesn't use Nintendo's proprietary handshake at all —
just a plain 8-byte USB HID gamepad report, no report IDs, no init
sequence. This is the same approach used by
[celclow/SwitchControlLibrary](https://github.com/celclow/SwitchControlLibrary),
an Arduino library with years of track record in the TAS/botting community
against real consoles. Switching to it is what got the Pico recognized.

## Hardware switch

A 6-pin (DPDT) slide switch, wired to swap only D+/D- between "real
controller" and "Pico." GND and VBUS are shared permanently across both
sides (not switched) — simplest wiring that still gives the Switch
electrical exclusivity on the data lines.

One unresolved issue: a genuine Pro Controller has a "seamless Bluetooth
handoff" feature — unplugging its USB data lines (even with VBUS still
present, since it has its own battery) makes it try to fall back to
Bluetooth, which can reintroduce a second simultaneously-connected
controller. Tried and ruled out so far: de-pairing Bluetooth from the
Switch's controller list, a full factory-reset of the controller's sync
button, and weak pull-down "decoy host present" terminations on the
disconnected D+/D- lines (RP2040 GPIO internal pull-downs) — none of these
stopped the handoff. The planned fix is to run the real controller without
its battery (USB bus power only) and switch VBUS too (needs a 3-pole/3PDT
switch instead of the current 2-pole one) so it fully loses power, and
therefore can't attempt Bluetooth, whenever it's deselected.

## Layout

- `pico/src/hori_gamepad.c` — HORI-pad USB report descriptor, VID/PID, and
  the function that packs AI state into its 8-byte input report.
- `pico/src/gamepad_state.h` — protocol-agnostic button/stick state shared
  between the AI and whichever wire format is in use.
- `pico/src/usb_descriptors.c` — USB device/configuration descriptors and
  string descriptors.
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

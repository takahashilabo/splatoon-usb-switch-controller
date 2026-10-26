#pragma once

#include <stdbool.h>
#include <stdint.h>

// Button/stick layout here matches the real byte-for-byte wire format
// (cross-checked between the Bluetooth capture notes used in
// ../../pseude-splatoon-controller/nxbt_patches/protocol.py and esp-cpp's
// hid-rp USB implementation) -- NOT the bit layout declared in the USB HID
// report descriptor (see the big comment in usb_descriptors.c for why those
// two differ).
typedef struct {
  bool y, x, b, a;
  bool right_sr, right_sl;
  bool r, zr;
  bool minus, plus;
  bool rclick, lclick;
  bool home, capture;
  bool dpad_up, dpad_down, dpad_left, dpad_right;
  bool left_sr, left_sl;
  bool l, zl;
} ProConButtons;

typedef struct {
  ProConButtons buttons;
  // -100.0 .. +100.0, 0 = centered (same convention as the dummy-AI layer).
  float lx, ly, rx, ry;
} ProConInputState;

void procon_proto_init(void);

// Call once TinyUSB reports the device mounted/configured. Queues the
// unsolicited report id 0x81 a real controller sends first, which is what
// prompts the host to start its handshake.
void procon_proto_on_mount(void);

// Feed every report that arrives on the HID OUT endpoint
// (report id 0x80 handshake / 0x01 subcommand / 0x10 rumble).
void procon_proto_handle_output_report(uint8_t const *buf, uint16_t len);

// True once the host has sent handshake command 0x04 ("enable USB HID").
// Only once this is true is it correct to start streaming unsolicited 0x30
// full input reports.
bool procon_proto_is_streaming(void);

// Pops one queued reply (handshake ack / subcommand ack) built in response
// to handle_output_report(), if any is pending.
// Returns the packet length (report id byte + payload), or 0 if none queued.
uint16_t procon_proto_pop_reply(uint8_t *out_buf, uint16_t out_buf_size);

// Builds an unsolicited full standard input report (id 0x30) from the
// current AI-driven state. out_buf must be >= 64 bytes.
// Returns the packet length.
uint16_t procon_proto_build_input_report(uint8_t *out_buf, uint16_t out_buf_size,
                                          ProConInputState const *state);

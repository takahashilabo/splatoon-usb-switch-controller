#pragma once

#include <stdint.h>

#include "gamepad_state.h"

// Emulates a HORI-licensed Switch controller (Pokken Tournament DX Pro Pad)
// instead of a genuine Nintendo Pro Controller. HORI-licensed pads are
// plain USB HID gamepads: no 0x80-series handshake, no report IDs, just a
// fixed 8-byte input report streamed continuously. VID/PID and report
// descriptor below are byte-for-byte from celclow/SwitchControlLibrary
// (https://github.com/celclow/SwitchControlLibrary), an Arduino library
// with years of real-Switch-console track record in the TAS/botting
// community -- chosen after the genuine-Pro-Controller 0x80-handshake path
// stalled on a real console despite matching every real-controller capture
// we could get (see git history / HANDOFF.md for that investigation).
#define HORI_VID 0x0F0D
#define HORI_PID 0x0092

#define HORI_HID_REPORT_DESC_LEN 86
extern uint8_t const hori_hid_report_desc[HORI_HID_REPORT_DESC_LEN];

// Report is exactly 8 bytes, no report ID byte (single implicit report).
#define HORI_REPORT_LEN 8

// Builds the 8-byte input report from the abstract gamepad state.
// out_buf must be >= HORI_REPORT_LEN bytes.
void hori_build_report(uint8_t *out_buf, ProConInputState const *state);

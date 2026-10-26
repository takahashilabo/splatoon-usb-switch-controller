#pragma once

#include <stdint.h>

// Captured from a real wired Pro Controller (community reverse-engineering,
// cross-checked against esp-cpp/espp's hid-rp component). Switch only binds
// a USB device that advertises exactly this VID/PID.
#define PROCON_VID 0x057E
#define PROCON_PID 0x2009

#define PROCON_HID_REPORT_DESC_LEN 203

extern uint8_t const procon_hid_report_desc[PROCON_HID_REPORT_DESC_LEN];

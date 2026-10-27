#include "hori_gamepad.h"

// Verbatim from celclow/SwitchControlLibrary's _hidReportDescriptor
// (src/SwitchControlLibrary.cpp): Generic Desktop / Game Pad, 16 buttons,
// a 4-bit hat switch (+4 bits padding), 4 single-byte axes (X/Y/Z/Rz), one
// vendor-defined input byte, and an 8-bit vendor-defined output byte. No
// report ID tags anywhere, so reports are sent with no leading ID byte.
uint8_t const hori_hid_report_desc[HORI_HID_REPORT_DESC_LEN] = {
    0x05, 0x01,       //   USAGE_PAGE (Generic Desktop)
    0x09, 0x05,       //   USAGE (Game Pad)
    0xa1, 0x01,       //   COLLECTION (Application)
    0x15, 0x00,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01,       //   LOGICAL_MAXIMUM (1)
    0x35, 0x00,       //   PHYSICAL_MINIMUM (0)
    0x45, 0x01,       //   PHYSICAL_MAXIMUM (1)
    0x75, 0x01,       //   REPORT_SIZE (1)
    0x95, 0x10,       //   REPORT_COUNT (16)
    0x05, 0x09,       //   USAGE_PAGE (Button)
    0x19, 0x01,       //   USAGE_MINIMUM (1)
    0x29, 0x10,       //   USAGE_MAXIMUM (16)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x05, 0x01,       //   USAGE_PAGE (Generic Desktop)
    0x25, 0x07,       //   LOGICAL_MAXIMUM (7)
    0x46, 0x3b, 0x01, //   PHYSICAL_MAXIMUM (315)
    0x75, 0x04,       //   REPORT_SIZE (4)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x65, 0x14,       //   UNIT (20)
    0x09, 0x39,       //   USAGE (Hat Switch)
    0x81, 0x42,       //   INPUT (Data,Var,Abs)
    0x65, 0x00,       //   UNIT (0)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x81, 0x01,       //   INPUT (Cnst,Arr,Abs)
    0x26, 0xff, 0x00, //   LOGICAL_MAXIMUM (255)
    0x46, 0xff, 0x00, //   PHYSICAL_MAXIMUM (255)
    0x09, 0x30,       //   USAGE (X)
    0x09, 0x31,       //   USAGE (Y)
    0x09, 0x32,       //   USAGE (Z)
    0x09, 0x35,       //   USAGE (Rz)
    0x75, 0x08,       //   REPORT_SIZE (8)
    0x95, 0x04,       //   REPORT_COUNT (4)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x06, 0x00, 0xff, //   USAGE_PAGE (Vendor Defined 65280)
    0x09, 0x20,       //   USAGE (32)
    0x95, 0x01,       //   REPORT_COUNT (1)
    0x81, 0x02,       //   INPUT (Data,Var,Abs)
    0x0a, 0x21, 0x26, //   USAGE (9761)
    0x95, 0x08,       //   REPORT_COUNT (8)
    0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
    0xc0,             //   END_COLLECTION
};

enum {
  HORI_BTN_Y = 0x0001,
  HORI_BTN_B = 0x0002,
  HORI_BTN_A = 0x0004,
  HORI_BTN_X = 0x0008,
  HORI_BTN_L = 0x0010,
  HORI_BTN_R = 0x0020,
  HORI_BTN_ZL = 0x0040,
  HORI_BTN_ZR = 0x0080,
  HORI_BTN_MINUS = 0x0100,
  HORI_BTN_PLUS = 0x0200,
  HORI_BTN_LCLICK = 0x0400,
  HORI_BTN_RCLICK = 0x0800,
  HORI_BTN_HOME = 0x1000,
  HORI_BTN_CAPTURE = 0x2000,
};

enum {
  HORI_HAT_UP = 0,
  HORI_HAT_UP_RIGHT = 1,
  HORI_HAT_RIGHT = 2,
  HORI_HAT_DOWN_RIGHT = 3,
  HORI_HAT_DOWN = 4,
  HORI_HAT_DOWN_LEFT = 5,
  HORI_HAT_LEFT = 6,
  HORI_HAT_UP_LEFT = 7,
  HORI_HAT_NEUTRAL = 8,
};

static uint16_t pack_buttons(ProConButtons const *b) {
  uint16_t v = 0;
  if (b->y) v |= HORI_BTN_Y;
  if (b->b) v |= HORI_BTN_B;
  if (b->a) v |= HORI_BTN_A;
  if (b->x) v |= HORI_BTN_X;
  if (b->l) v |= HORI_BTN_L;
  if (b->r) v |= HORI_BTN_R;
  if (b->zl) v |= HORI_BTN_ZL;
  if (b->zr) v |= HORI_BTN_ZR;
  if (b->minus) v |= HORI_BTN_MINUS;
  if (b->plus) v |= HORI_BTN_PLUS;
  if (b->lclick) v |= HORI_BTN_LCLICK;
  if (b->rclick) v |= HORI_BTN_RCLICK;
  if (b->home) v |= HORI_BTN_HOME;
  if (b->capture) v |= HORI_BTN_CAPTURE;
  return v;
}

static uint8_t pack_hat(ProConButtons const *b) {
  uint8_t mask = (b->dpad_up ? 1 : 0) | (b->dpad_right ? 2 : 0) | (b->dpad_down ? 4 : 0) |
                 (b->dpad_left ? 8 : 0);
  switch (mask) {
  case 0b0001: return HORI_HAT_UP;
  case 0b0010: return HORI_HAT_RIGHT;
  case 0b0011: return HORI_HAT_UP_RIGHT;
  case 0b0100: return HORI_HAT_DOWN;
  case 0b0110: return HORI_HAT_DOWN_RIGHT;
  case 0b1000: return HORI_HAT_LEFT;
  case 0b1001: return HORI_HAT_UP_LEFT;
  case 0b1100: return HORI_HAT_DOWN_LEFT;
  default: return HORI_HAT_NEUTRAL;
  }
}

static uint8_t pack_axis(float v) {
  int32_t iv = (int32_t)(128.0f + (v / 100.0f) * 127.0f);
  if (iv < 0) iv = 0;
  if (iv > 255) iv = 255;
  return (uint8_t)iv;
}

void hori_build_report(uint8_t *out_buf, ProConInputState const *state) {
  uint16_t buttons = pack_buttons(&state->buttons);
  out_buf[0] = (uint8_t)(buttons & 0xFF);
  out_buf[1] = (uint8_t)((buttons >> 8) & 0xFF);
  out_buf[2] = pack_hat(&state->buttons);
  out_buf[3] = pack_axis(state->lx);
  out_buf[4] = pack_axis(state->ly);
  out_buf[5] = pack_axis(state->rx);
  out_buf[6] = pack_axis(state->ry);
  out_buf[7] = 0; // vendor-defined, unused
}

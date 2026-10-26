#include <stdbool.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#include "pro_con_protocol.h"
#include "splatoon_ai.h"

static SplatoonAI ai;

// Real Joy-Con/Pro Controller standard input reports stream at roughly
// 66 Hz (~15 ms) over Bluetooth; we use the same cadence over USB.
#define INPUT_REPORT_INTERVAL_US 15000

void tud_mount_cb(void) {
  procon_proto_on_mount();
  // This TinyUSB version arms/re-arms the HID OUT endpoint automatically
  // (hidd_open / hidd_xfer_cb in class/hid/hid_device.c); OUT report data
  // arrives via tud_hid_set_report_cb (see usb_descriptors.c), not a
  // separate receive call/callback.
}

static void send_pending_reply(void) {
  if (!tud_hid_ready()) return;

  uint8_t buf[64];
  uint16_t n = procon_proto_pop_reply(buf, sizeof(buf));
  if (n == 0) return;

  // buf[0] is the report id; tud_hid_report() prepends that itself, so only
  // the payload after it is passed here.
  tud_hid_report(buf[0], &buf[1], (uint8_t)(n - 1));
}

static void send_periodic_input_report(void) {
  static bool have_next_send = false;
  static absolute_time_t next_send;
  static bool have_last_update = false;
  static absolute_time_t last_update;

  if (!procon_proto_is_streaming()) {
    have_next_send = false;
    have_last_update = false;
    return;
  }

  absolute_time_t now = get_absolute_time();

  if (have_next_send && absolute_time_diff_us(now, next_send) > 0) {
    return; // not time yet
  }
  if (!tud_hid_ready()) return;

  float dt = have_last_update ? (float)absolute_time_diff_us(last_update, now) / 1e6f : 0.015f;
  last_update = now;
  have_last_update = true;

  ProConInputState state;
  splatoon_ai_update(&ai, dt, &state);

  uint8_t buf[64];
  uint16_t n = procon_proto_build_input_report(buf, sizeof(buf), &state);
  if (n == 0) return;

  tud_hid_report(buf[0], &buf[1], (uint8_t)(n - 1));

  next_send = delayed_by_us(now, INPUT_REPORT_INTERVAL_US);
  have_next_send = true;
}

int main(void) {
  stdio_init_all();

  procon_proto_init();
  splatoon_ai_init(&ai, PLAYSTYLE_BALANCED);

  tusb_init();

  while (true) {
    tud_task();
    send_pending_reply();
    send_periodic_input_report();
  }
}

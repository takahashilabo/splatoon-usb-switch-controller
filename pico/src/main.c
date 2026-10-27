#include <stdbool.h>
#include <stdint.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "tusb.h"

#include "gamepad_state.h"
#include "hori_gamepad.h"
#include "splatoon_ai.h"

static SplatoonAI ai;

// A plain HID gamepad doesn't need a mount hook, but the LED still gives a
// one-shot "did USB enumeration succeed" signal without needing a UART
// adapter: 3 blinks once tud_mounted() goes true.
static void blink_led(int times) {
  for (int i = 0; i < times; i++) {
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    sleep_ms(150);
    gpio_put(PICO_DEFAULT_LED_PIN, 0);
    sleep_ms(150);
  }
}

static void run_diagnostics(void) {
  static bool did_mount_blink = false;
  if (!did_mount_blink && tud_mounted()) {
    did_mount_blink = true;
    blink_led(3);
  }
}

// Real gamepads poll at whatever rate the host's bInterval asks for (8ms
// here); match that cadence for the dummy-AI input stream.
#define INPUT_REPORT_INTERVAL_US 8000

static void send_periodic_input_report(void) {
  static bool have_next_send = false;
  static absolute_time_t next_send;
  static bool have_last_update = false;
  static absolute_time_t last_update;

  absolute_time_t now = get_absolute_time();

  if (have_next_send && absolute_time_diff_us(now, next_send) > 0) {
    return; // not time yet
  }
  if (!tud_hid_ready()) return;

  float dt = have_last_update ? (float)absolute_time_diff_us(last_update, now) / 1e6f : 0.008f;
  last_update = now;
  have_last_update = true;

  ProConInputState state;
  splatoon_ai_update(&ai, dt, &state);

  uint8_t buf[HORI_REPORT_LEN];
  hori_build_report(buf, &state);
  tud_hid_report(0, buf, HORI_REPORT_LEN);

  next_send = delayed_by_us(now, INPUT_REPORT_INTERVAL_US);
  have_next_send = true;
}

int main(void) {
  stdio_init_all();

  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

  splatoon_ai_init(&ai, PLAYSTYLE_BALANCED);

  tusb_init();

  while (true) {
    tud_task();
    send_periodic_input_report();
    run_diagnostics();
  }
}

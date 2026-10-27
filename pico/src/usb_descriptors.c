#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"
#include "hori_gamepad.h"

// ---------------------------------------------------------------------------
// Device descriptor: a HORI-licensed controller (Pokken Tournament DX Pro
// Pad), not a genuine Nintendo Pro Controller -- see hori_gamepad.h for why.
// ---------------------------------------------------------------------------
tusb_desc_device_t const hori_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 64,
    .idVendor = HORI_VID,
    .idProduct = HORI_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x00, // none
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&hori_device_descriptor;
}

// ---------------------------------------------------------------------------
// Configuration descriptor: plain HID gamepad, IN + OUT interrupt endpoints.
// ---------------------------------------------------------------------------
#define HORI_CONFIG_TOTAL_LEN (9 + 9 + 9 + 7 + 7) // 41 bytes

uint8_t const hori_configuration_descriptor[HORI_CONFIG_TOTAL_LEN] = {
    // Configuration Descriptor
    0x09, TUSB_DESC_CONFIGURATION, HORI_CONFIG_TOTAL_LEN & 0xFF,
    (HORI_CONFIG_TOTAL_LEN >> 8) & 0xFF,
    0x01, // bNumInterfaces
    0x01, // bConfigurationValue
    0x00, // iConfiguration
    0xA0, // bmAttributes: bus powered + remote wakeup
    0xFA, // bMaxPower: 500mA

    // Interface Descriptor
    0x09, TUSB_DESC_INTERFACE,
    0x00, // bInterfaceNumber
    0x00, // bAlternateSetting
    0x02, // bNumEndpoints
    TUSB_CLASS_HID,
    0x00, // bInterfaceSubClass
    0x00, // bInterfaceProtocol
    0x00, // iInterface

    // HID Descriptor
    0x09, HID_DESC_TYPE_HID,
    0x11, 0x01, // bcdHID 1.11
    0x00,       // bCountryCode
    0x01,       // bNumDescriptors
    HID_DESC_TYPE_REPORT,
    HORI_HID_REPORT_DESC_LEN & 0xFF,
    (HORI_HID_REPORT_DESC_LEN >> 8) & 0xFF,

    // Endpoint IN (device -> host, interrupt, 64B, 8ms)
    0x07, TUSB_DESC_ENDPOINT,
    0x81, // bEndpointAddress: EP1 IN
    TUSB_XFER_INTERRUPT,
    0x40, 0x00, // wMaxPacketSize 64
    0x08,       // bInterval

    // Endpoint OUT (host -> device, interrupt, 64B, 8ms)
    0x07, TUSB_DESC_ENDPOINT,
    0x01, // bEndpointAddress: EP1 OUT
    TUSB_XFER_INTERRUPT,
    0x40, 0x00, // wMaxPacketSize 64
    0x08,       // bInterval
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index;
  return hori_configuration_descriptor;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) {
  (void)itf;
  return hori_hid_report_desc;
}

// ---------------------------------------------------------------------------
// String descriptors
// ---------------------------------------------------------------------------
char const *hori_string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: supported language = English (0x0409)
    "HORI CO.,LTD.",            // 1: iManufacturer
    "POKKEN CONTROLLER",        // 2: iProduct
};

static uint16_t string_desc_buf[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  if (index == 0) {
    memcpy(&string_desc_buf[1], hori_string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(hori_string_desc_arr) / sizeof(hori_string_desc_arr[0])) {
      return NULL;
    }

    const char *str = hori_string_desc_arr[index];
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31) chr_count = 31;

    for (uint8_t i = 0; i < chr_count; i++) {
      string_desc_buf[1 + i] = str[i];
    }
  }

  string_desc_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

  return string_desc_buf;
}

// ---------------------------------------------------------------------------
// GET_REPORT / SET_REPORT: unused. HORI-style pads don't take any output
// report content that matters here (the descriptor's 1-byte vendor output
// is presumably rumble on genuine hardware; we don't have motors).
// ---------------------------------------------------------------------------
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
  (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
  (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)bufsize;
}

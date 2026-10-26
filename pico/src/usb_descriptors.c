#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"
#include "pro_con_protocol.h"

// ---------------------------------------------------------------------------
// Device descriptor
// Byte values below are copied from a real wired Pro Controller USB capture
// (community reverse-engineering; see HANDOFF notes). Not derived/guessed.
// ---------------------------------------------------------------------------
tusb_desc_device_t const procon_device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0x00,
    .bDeviceSubClass = 0x00,
    .bDeviceProtocol = 0x00,
    .bMaxPacketSize0 = 64,
    .idVendor = PROCON_VID,
    .idProduct = PROCON_PID,
    .bcdDevice = 0x0210, // a real Pro Controller reports firmware 2.10
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&procon_device_descriptor;
}

// ---------------------------------------------------------------------------
// Configuration descriptor (raw bytes, copied verbatim from the capture
// rather than built from TinyUSB's TUD_HID_INOUT_DESCRIPTOR helper macro, so
// every field exactly matches what a real controller presents: bmAttributes
// with remote-wakeup set, bMaxPower 500mA, and endpoint addresses 0x81/0x01
// at 64 bytes / 8ms interval).
// ---------------------------------------------------------------------------
#define PROCON_CONFIG_TOTAL_LEN (9 + 9 + 9 + 7 + 7) // 41 bytes

uint8_t const procon_configuration_descriptor[PROCON_CONFIG_TOTAL_LEN] = {
    // Configuration Descriptor
    0x09, TUSB_DESC_CONFIGURATION, PROCON_CONFIG_TOTAL_LEN & 0xFF,
    (PROCON_CONFIG_TOTAL_LEN >> 8) & 0xFF,
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
    PROCON_HID_REPORT_DESC_LEN & 0xFF,
    (PROCON_HID_REPORT_DESC_LEN >> 8) & 0xFF,

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
  return procon_configuration_descriptor;
}

// ---------------------------------------------------------------------------
// HID report descriptor (203 bytes, copied verbatim from the capture).
//
// NOTE: several independent write-ups of this capture point out that this
// descriptor describes a generic HID joystick layout (bitfields for 10+4
// buttons, 4x16-bit axes, a hat switch) that does NOT match the actual bytes
// the controller puts in its input reports. The Switch itself doesn't read
// the report through this descriptor's field layout -- it reads fixed byte
// offsets in Nintendo's own internal format (the same format used over
// Bluetooth). pro_con_protocol.c builds reports using that real fixed-offset
// format, not this descriptor's declared bit layout. This descriptor is only
// here because the device must still present *some* valid HID report
// descriptor of the right length for USB enumeration to succeed.
// ---------------------------------------------------------------------------
uint8_t const procon_hid_report_desc[PROCON_HID_REPORT_DESC_LEN] = {
    0x05, 0x01,                   // Usage Page (Generic Desktop)
    0x15, 0x00,                   // Logical Minimum (0)
    0x09, 0x04,                   // Usage (Joystick)
    0xA1, 0x01,                   // Collection (Application)
    0x85, 0x30,                   //   Report ID (0x30)
    0x05, 0x01,                   //   Usage Page (Generic Desktop)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x01,                   //   Usage Minimum (1)
    0x29, 0x0A,                   //   Usage Maximum (10)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x0A,                   //   Report Count (10)
    0x55, 0x00,                   //   Unit Exponent (0)
    0x65, 0x00,                   //   Unit (None)
    0x81, 0x02,                   //   Input (Data,Var,Abs)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x0B,                   //   Usage Minimum (11)
    0x29, 0x0E,                   //   Usage Maximum (14)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x04,                   //   Report Count (4)
    0x81, 0x02,                   //   Input (Data,Var,Abs)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x02,                   //   Report Count (2)
    0x81, 0x03,                   //   Input (Const,Var,Abs)
    0x0B, 0x01, 0x00, 0x01, 0x00, //   Usage (0x010001)
    0xA1, 0x00,                   //   Collection (Physical)
    0x0B, 0x30, 0x00, 0x01, 0x00, //     Usage (0x010030) X
    0x0B, 0x31, 0x00, 0x01, 0x00, //     Usage (0x010031) Y
    0x0B, 0x32, 0x00, 0x01, 0x00, //     Usage (0x010032) Z
    0x0B, 0x35, 0x00, 0x01, 0x00, //     Usage (0x010035) Rz
    0x15, 0x00,                   //     Logical Minimum (0)
    0x27, 0xFF, 0xFF, 0x00, 0x00, //     Logical Maximum (65535)
    0x75, 0x10,                   //     Report Size (16)
    0x95, 0x04,                   //     Report Count (4)
    0x81, 0x02,                   //     Input (Data,Var,Abs)
    0xC0,                         //   End Collection
    0x0B, 0x39, 0x00, 0x01, 0x00, //   Usage (Hat switch)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x07,                   //   Logical Maximum (7)
    0x35, 0x00,                   //   Physical Minimum (0)
    0x46, 0x3B, 0x01,             //   Physical Maximum (315)
    0x65, 0x14,                   //   Unit (Eng Rot:Centimeter)
    0x75, 0x04,                   //   Report Size (4)
    0x95, 0x01,                   //   Report Count (1)
    0x81, 0x42,                   //   Input (Data,Var,Abs,Null State)
    0x05, 0x09,                   //   Usage Page (Button)
    0x19, 0x0F,                   //   Usage Minimum (15)
    0x29, 0x12,                   //   Usage Maximum (18)
    0x15, 0x00,                   //   Logical Minimum (0)
    0x25, 0x01,                   //   Logical Maximum (1)
    0x75, 0x01,                   //   Report Size (1)
    0x95, 0x04,                   //   Report Count (4)
    0x81, 0x02,                   //   Input (Data,Var,Abs)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x34,                   //   Report Count (52)
    0x81, 0x03,                   //   Input (Const,Var,Abs)
    0x06, 0x00, 0xFF,             //   Usage Page (Vendor Defined 0xFF00)
    0x85, 0x21,                   //   Report ID (0x21)
    0x09, 0x01,                   //   Usage (0x01)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x81, 0x03,                   //   Input (Const,Var,Abs)
    0x85, 0x81,                   //   Report ID (0x81)
    0x09, 0x02,                   //   Usage (0x02)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x81, 0x03,                   //   Input (Const,Var,Abs)
    0x85, 0x01,                   //   Report ID (0x01)
    0x09, 0x03,                   //   Usage (0x03)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x91, 0x83,                   //   Output (Const,Var,Abs,Volatile)
    0x85, 0x10,                   //   Report ID (0x10)
    0x09, 0x04,                   //   Usage (0x04)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x91, 0x83,                   //   Output (Const,Var,Abs,Volatile)
    0x85, 0x80,                   //   Report ID (0x80)
    0x09, 0x05,                   //   Usage (0x05)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x91, 0x83,                   //   Output (Const,Var,Abs,Volatile)
    0x85, 0x82,                   //   Report ID (0x82)
    0x09, 0x06,                   //   Usage (0x06)
    0x75, 0x08,                   //   Report Size (8)
    0x95, 0x3F,                   //   Report Count (63)
    0x91, 0x83,                   //   Output (Const,Var,Abs,Volatile)
    0xC0,                         // End Collection
};

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf) {
  (void)itf;
  return procon_hid_report_desc;
}

// ---------------------------------------------------------------------------
// String descriptors
// ---------------------------------------------------------------------------
char const *procon_string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: supported language = English (0x0409)
    "Nintendo Co., Ltd.",       // 1: iManufacturer
    "Pro Controller",           // 2: iProduct
    "000000000001",             // 3: iSerialNumber
};

static uint16_t string_desc_buf[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;

  uint8_t chr_count;

  if (index == 0) {
    memcpy(&string_desc_buf[1], procon_string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (index >= sizeof(procon_string_desc_arr) / sizeof(procon_string_desc_arr[0])) {
      return NULL;
    }

    const char *str = procon_string_desc_arr[index];
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
// HID GET_REPORT (EP0 control transfer): unused, the real controller never
// sends reports this way.
//
// SET_REPORT: this is also where this TinyUSB version delivers data received
// on the HID OUT *interrupt endpoint* (see hidd_xfer_cb in
// class/hid/hid_device.c), which is how the host's real 0x80/0x01/0x10
// commands arrive -- not through a dedicated receive callback.
// ---------------------------------------------------------------------------
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                                uint8_t *buffer, uint16_t reqlen) {
  (void)itf; (void)report_id; (void)report_type; (void)buffer; (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                            uint8_t const *buffer, uint16_t bufsize) {
  (void)itf; (void)report_id; (void)report_type;
  procon_proto_handle_output_report(buffer, bufsize);
}

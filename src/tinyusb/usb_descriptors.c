/*
 * USB descriptors for the UAC1 headset build.
 */

#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_AUDIO_PNP_BCD_USB,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = USB_AUDIO_PNP_VID,
    .idProduct          = USB_AUDIO_PNP_PID,
    .bcdDevice          = USB_AUDIO_PNP_BCD_DEVICE,

    .iManufacturer      = 0x00,
    .iProduct           = 0x01,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
#define EPNUM_AUDIO_OUT    0x01
#define EPNUM_AUDIO_IN     0x02
#define EPNUM_HID_IN       0x03

#define AUDIO10_DESC_LEN   TUD_AUDIO10_HEADSET_STEREO_DESC_LEN(1)
#define HID_DESC_LEN       TUD_HID_DESC_LEN
#define CONFIG_TOTAL_LEN   (TUD_CONFIG_DESC_LEN + AUDIO10_DESC_LEN + HID_DESC_LEN)

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attributes, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Official TinyUSB UAC1 headset descriptor: speaker OUT + microphone IN.
    TUD_AUDIO10_HEADSET_STEREO_DESCRIPTOR(
        ITF_NUM_AUDIO_CONTROL,
        0x01,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_RX,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_RESOLUTION_RX,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_N_BYTES_PER_SAMPLE_TX,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_RESOLUTION_TX,
        EPNUM_AUDIO_OUT,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_EP_SZ_OUT,
        0x80 | EPNUM_AUDIO_IN,
        CFG_TUD_AUDIO10_FUNC_1_FORMAT_1_EP_SZ_IN,
        USB_AUDIO_SAMPLE_RATE),

    // HID consumer-control interface, same endpoint style as common USB headsets.
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0x00, HID_ITF_PROTOCOL_NONE,
        USB_HID_REPORT_DESC_LEN, 0x80 | EPNUM_HID_IN,
        USB_HID_EP_SIZE, USB_HID_EP_INTERVAL)
};

TU_VERIFY_STATIC(sizeof(desc_configuration) == CONFIG_TOTAL_LEN,
                 "USB configuration descriptor length mismatch");

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
  (void)index;
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+
enum
{
  STRID_LANGID = 0,
  STRID_PRODUCT,
};

char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 },
  "USB PnP Audio Device",
};

static uint16_t _desc_str[32 + 1];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;
  size_t chr_count;

  if (index == STRID_LANGID)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else
  {
    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
    {
      return NULL;
    }

    const char *str = string_desc_arr[index];
    chr_count = strlen(str);
    size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1;
    if (chr_count > max_count)
    {
      chr_count = max_count;
    }

    for (size_t i = 0; i < chr_count; i++)
    {
      _desc_str[1 + i] = str[i];
    }
  }

  _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}

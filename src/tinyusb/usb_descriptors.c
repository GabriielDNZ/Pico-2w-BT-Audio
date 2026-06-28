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

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
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

#define AUDIO10_DESC_LEN   184
#define CONFIG_TOTAL_LEN   (TUD_CONFIG_DESC_LEN + AUDIO10_DESC_LEN)

uint8_t const desc_configuration[] =
{
    // Config number, interface count, string index, total length, attributes, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // UAC1 headset descriptor without IAD.
    // USB 1.1 audio devices are commonly advertised directly through
    // AudioControl + AudioStreaming interfaces; some hosts do not open
    // the OUT endpoint when an IAD is present on a UAC1/bcdUSB 1.10 device.
    0x09, 0x04, ITF_NUM_AUDIO_CONTROL, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x0A, 0x24, 0x01, 0x00, 0x01, 0x47, 0x00, 0x02,
      ITF_NUM_AUDIO_STREAMING_SPK, ITF_NUM_AUDIO_STREAMING_MIC,
    0x0C, 0x24, 0x02, UAC1_ENTITY_SPK_INPUT_TERMINAL,
      0x01, 0x01, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x0A, 0x24, 0x06, UAC1_ENTITY_SPK_FEATURE_UNIT,
      UAC1_ENTITY_SPK_INPUT_TERMINAL, 0x01, 0x03, 0x03, 0x03, 0x00,
    0x09, 0x24, 0x03, UAC1_ENTITY_SPK_OUTPUT_TERMINAL,
      0x01, 0x03, 0x00, UAC1_ENTITY_SPK_FEATURE_UNIT, 0x00,
    0x0C, 0x24, 0x02, UAC1_ENTITY_MIC_INPUT_TERMINAL,
      0x01, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x24, 0x06, UAC1_ENTITY_MIC_FEATURE_UNIT,
      UAC1_ENTITY_MIC_INPUT_TERMINAL, 0x01, 0x03, 0x03, 0x00,
    0x09, 0x24, 0x03, UAC1_ENTITY_MIC_OUTPUT_TERMINAL,
      0x01, 0x01, 0x00, UAC1_ENTITY_MIC_FEATURE_UNIT, 0x00,

    // Interface 1, speaker/playback, host OUT -> Pico.
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_SPK, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_SPK, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,
    0x07, 0x24, 0x01, UAC1_ENTITY_SPK_INPUT_TERMINAL, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, USB_AUDIO_SPK_CHANNELS,
      USB_AUDIO_BYTES_PER_SAMPLE, USB_AUDIO_RESOLUTION_BITS, 0x01, 0x80, 0xBB, 0x00,
    0x09, 0x05, EPNUM_AUDIO_OUT, 0x09,
      USB_AUDIO_SPK_PACKET_BYTES & 0xff, USB_AUDIO_SPK_PACKET_BYTES >> 8, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x01, 0x01, 0x01, 0x00,

    // Interface 2, microphone, Pico IN -> host. The firmware sends silence.
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_MIC, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_MIC, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,
    0x07, 0x24, 0x01, UAC1_ENTITY_MIC_OUTPUT_TERMINAL, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, USB_AUDIO_MIC_CHANNELS,
      USB_AUDIO_BYTES_PER_SAMPLE, USB_AUDIO_RESOLUTION_BITS, 0x01, 0x80, 0xBB, 0x00,
    0x09, 0x05, 0x80 | EPNUM_AUDIO_IN, 0x05,
      USB_AUDIO_MIC_PACKET_BYTES & 0xff, USB_AUDIO_MIC_PACKET_BYTES >> 8, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x01, 0x00, 0x00, 0x00
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
  STRID_MANUFACTURER,
  STRID_PRODUCT,
};

char const *string_desc_arr[] =
{
  (const char[]) { 0x09, 0x04 },
  "C-Media Electronics Inc.",
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

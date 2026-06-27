/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Ha Thach (tinyusb.org)
 * Copyright (c) 2020 Jerzy Kasenberg
 */

#include <string.h>

#include "tusb.h"
#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// Device Descriptors
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
uint8_t const desc_configuration[] =
{
    0x09, 0x02, USB_AUDIO_PNP_DESC_LEN & 0xff, USB_AUDIO_PNP_DESC_LEN >> 8,
    ITF_NUM_TOTAL, 0x01, 0x00, 0x80, 0x32,

    // Interface 0, Audio Control, UAC1
    0x09, 0x04, ITF_NUM_AUDIO_CONTROL, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00,
    0x0A, 0x24, 0x01, 0x00, 0x01, 0x64, 0x00, 0x02,
      ITF_NUM_AUDIO_STREAMING_SPK, ITF_NUM_AUDIO_STREAMING_MIC,
    0x0C, 0x24, 0x02, UAC1_ENTITY_USB_OUT_INPUT_TERMINAL,
      0x01, 0x01, 0x00, USB_AUDIO_SPK_CHANNELS, 0x03, 0x00, 0x00, 0x00,
    0x0C, 0x24, 0x02, UAC1_ENTITY_MIC_INPUT_TERMINAL,
      0x01, 0x02, 0x00, USB_AUDIO_MIC_CHANNELS, 0x00, 0x00, 0x00, 0x00,
    0x09, 0x24, 0x03, UAC1_ENTITY_SPK_OUTPUT_TERMINAL,
      0x01, 0x03, 0x00, UAC1_ENTITY_SPK_FEATURE_UNIT, 0x00,
    0x09, 0x24, 0x03, UAC1_ENTITY_USB_IN_OUTPUT_TERMINAL,
      0x01, 0x01, 0x02, UAC1_ENTITY_SELECTOR_UNIT, 0x00,
    0x07, 0x24, 0x05, UAC1_ENTITY_SELECTOR_UNIT,
      0x01, UAC1_ENTITY_MIC_FEATURE_UNIT_1, 0x00,
    0x0A, 0x24, 0x06, UAC1_ENTITY_SPK_FEATURE_UNIT,
      UAC1_ENTITY_MIXER_UNIT, 0x01, 0x01, 0x02, 0x02, 0x00,
    0x09, 0x24, 0x06, UAC1_ENTITY_MIC_FEATURE_UNIT_1,
      UAC1_ENTITY_MIC_INPUT_TERMINAL, 0x01, 0x03, 0x00, 0x00,
    0x09, 0x24, 0x06, UAC1_ENTITY_MIC_FEATURE_UNIT_2,
      UAC1_ENTITY_MIC_INPUT_TERMINAL, 0x01, 0x03, 0x00, 0x00,
    0x0D, 0x24, 0x04, UAC1_ENTITY_MIXER_UNIT,
      0x02, UAC1_ENTITY_USB_OUT_INPUT_TERMINAL, UAC1_ENTITY_MIC_FEATURE_UNIT_2,
      0x02, 0x03, 0x00, 0x00, 0x00, 0x00,

    // Interface 1, speaker/playback, host OUT -> Pico
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_SPK, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_SPK, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,
    0x07, 0x24, 0x01, UAC1_ENTITY_USB_OUT_INPUT_TERMINAL, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, USB_AUDIO_SPK_CHANNELS,
      USB_AUDIO_BYTES_PER_SAMPLE, USB_AUDIO_RESOLUTION_BITS, 0x01, 0x80, 0xBB, 0x00,
    0x09, 0x05, 0x01, 0x09, USB_AUDIO_ISO_EP_SIZE & 0xff,
      USB_AUDIO_ISO_EP_SIZE >> 8, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x01, 0x01, 0x01, 0x00,

    // Interface 2, microphone, Pico IN -> host. This starts as silence.
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_MIC, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00,
    0x09, 0x04, ITF_NUM_AUDIO_STREAMING_MIC, 0x01, 0x01, 0x01, 0x02, 0x00, 0x00,
    0x07, 0x24, 0x01, UAC1_ENTITY_USB_IN_OUTPUT_TERMINAL, 0x01, 0x01, 0x00,
    0x0B, 0x24, 0x02, 0x01, USB_AUDIO_MIC_CHANNELS,
      USB_AUDIO_BYTES_PER_SAMPLE, USB_AUDIO_RESOLUTION_BITS, 0x01, 0x80, 0xBB, 0x00,
    0x09, 0x05, 0x82, 0x05, USB_AUDIO_ISO_EP_SIZE & 0xff,
      USB_AUDIO_ISO_EP_SIZE >> 8, 0x01, 0x00, 0x00,
    0x07, 0x25, 0x01, 0x01, 0x00, 0x00, 0x00,

    // Interface 3, HID media keys. The original PnP adapter exposes this.
    0x09, 0x04, ITF_NUM_HID, 0x00, 0x01, 0x03, 0x00, 0x00, 0x00,
    0x09, 0x21, 0x00, 0x01, 0x00, 0x01, 0x22,
      USB_HID_REPORT_DESC_LEN & 0xff, USB_HID_REPORT_DESC_LEN >> 8,
    0x07, 0x05, 0x83, 0x03, USB_HID_EP_SIZE, 0x00, USB_HID_EP_INTERVAL
};

TU_VERIFY_STATIC(sizeof(desc_configuration) == USB_AUDIO_PNP_DESC_LEN,
                 "USB audio configuration descriptor length mismatch");

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

  _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));
  return _desc_str;
}

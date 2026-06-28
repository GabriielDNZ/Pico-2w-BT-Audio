/*
 * USB Audio Class 1.0 control and speaker OUT path.
 *
 * This file is intentionally UAC1-only.  It keeps the Bluetooth/A2DP
 * pipeline but removes clock-entity request handling from the USB side.
 */

#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "../btstack/btstack_avdtp_source.h"
#include "pico/flash.h"

#define AUDIO10_CS_REQ_SET_CUR 0x01
#define AUDIO10_CS_REQ_GET_CUR 0x81
#define AUDIO10_CS_REQ_GET_MIN 0x82
#define AUDIO10_CS_REQ_GET_MAX 0x83
#define AUDIO10_CS_REQ_GET_RES 0x84

#define AUDIO10_FU_CTRL_MUTE   0x01
#define AUDIO10_FU_CTRL_VOLUME 0x02
#define AUDIO10_EP_CTRL_SAMPLING_FREQ 0x01

#define USB_AUDIO_RX_TEST_TONE 0
#define USB_AUDIO_FORCE_BT_VOLUME_MAX 0

#define UAC_REQ_ENTITY_ID(_request)  ((uint8_t)((tu_le16toh((_request)->wIndex) >> 8) & 0xff))
#define UAC_REQ_ENDPOINT(_request)   ((uint8_t)(tu_le16toh((_request)->wIndex) & 0xff))
#define UAC_REQ_CONTROL(_request)    ((uint8_t)((tu_le16toh((_request)->wValue) >> 8) & 0xff))
#define UAC_REQ_CHANNEL(_request)    ((uint8_t)(tu_le16toh((_request)->wValue) & 0xff))

typedef struct TU_ATTR_PACKED
{
  uint8_t bCur;
} audio10_control_cur_1_t;

typedef struct TU_ATTR_PACKED
{
  int16_t bCur;
} audio10_control_cur_2_t;

typedef struct TU_ATTR_PACKED
{
  uint8_t bCur[3];
} audio10_control_cur_3_t;

#if CFG_TUD_HID
static uint8_t const desc_hid_report[] =
{
  0x05, 0x0C, 0x09, 0x01, 0xA1, 0x01, 0x85, 0x01,
  0x15, 0x00, 0x25, 0x01, 0x75, 0x01, 0x95, 0x10,
  0x09, 0xB5, 0x09, 0xB6, 0x09, 0xB7, 0x09, 0xCD,
  0x09, 0xE2, 0x09, 0xE9, 0x09, 0xEA, 0x0A, 0x23,
  0x02, 0x0A, 0x24, 0x02, 0x0A, 0x25, 0x02, 0x0A,
  0x21, 0x02, 0x0A, 0x8A, 0x01, 0x0A, 0x92, 0x01,
  0x0A, 0x94, 0x01, 0x09, 0xB0, 0x81, 0x02, 0x95,
  0x01, 0x75, 0x08, 0x81, 0x01, 0xC0
};

TU_VERIFY_STATIC(sizeof(desc_hid_report) == USB_HID_REPORT_DESC_LEN,
                 "USB HID report descriptor length mismatch");
#endif

// Supported UAC1 sample rate: 48 kHz only.
static uint32_t current_sample_rate = USB_AUDIO_SAMPLE_RATE;
static bool need_change_bt_volume = false;

enum
{
  BLINK_STREAMING = 25,
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

enum
{
  VOLUME_CTRL_0_DB = 0,
  VOLUME_CTRL_10_DB = 2560,
  VOLUME_CTRL_20_DB = 5120,
  VOLUME_CTRL_30_DB = 7680,
  VOLUME_CTRL_40_DB = 10240,
  VOLUME_CTRL_50_DB = 12800,
  VOLUME_CTRL_60_DB = 15360,
  VOLUME_CTRL_70_DB = 17920,
  VOLUME_CTRL_80_DB = 20480,
  VOLUME_CTRL_90_DB = 23040,
  VOLUME_CTRL_100_DB = 25600,
  VOLUME_CTRL_SILENCE = 0x8000,
};

#define BT_VOL_MAX   127
#define USB_ATT_MAX  25600

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;

// UAC1 speaker controls. Channel 0 is master, channels 1/2 are left/right.
static int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];
static int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];
static int16_t volume0_last = 0;
static int8_t mute0_last = 0;

// Speaker OUT data is 16-bit little-endian stereo PCM.
static int16_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / sizeof(int16_t)];
static int spk_data_size = 0;
static uint8_t current_resolution = USB_AUDIO_RESOLUTION_BITS;

static uint16_t usb_stop_delay = 0;
static bool is_usb_audio_running = false;

static void audio_task(void);
static void audio_control_task(void);

void tinyusb_main(void)
{
  flash_safe_execute_core_init();

  tusb_rhport_init_t dev_init = {
    .role = TUSB_ROLE_DEVICE,
    .speed = TUSB_SPEED_AUTO
  };
  tusb_init(BOARD_TUD_RHPORT, &dev_init);
}

void tinyusb_task(void)
{
  tud_task();
  audio_task();
}

void tinyusb_control_task(void)
{
  audio_control_task();
}

void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
  is_usb_audio_running = false;
  set_usb_streaming(false);
}

void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  printf("tud_suspend_cb\n");
}

void tud_resume_cb(void)
{
  printf("tud_resume_cb\n");
}

#if CFG_TUD_HID
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance)
{
  (void)instance;
  return desc_hid_report;
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen)
{
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize)
{
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}
#endif

static uint8_t uac1_clamp_channel(uint8_t channel)
{
  return (channel > CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX) ? 0 : channel;
}

static bool uac1_get_feature_unit_request(uint8_t rhport, tusb_control_request_t const *p_request)
{
  uint8_t const entity = UAC_REQ_ENTITY_ID(p_request);
  uint8_t const control = UAC_REQ_CONTROL(p_request);
  uint8_t const request = p_request->bRequest;

  if (entity != UAC1_ENTITY_SPK_FEATURE_UNIT)
  {
    return false;
  }

  uint8_t const channel = uac1_clamp_channel(UAC_REQ_CHANNEL(p_request));

  if (control == AUDIO10_FU_CTRL_MUTE)
  {
    if (request != AUDIO10_CS_REQ_GET_CUR)
    {
      return false;
    }

    audio10_control_cur_1_t cur_mute = { .bCur = mute[channel] };
    return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request,
                                                      &cur_mute, sizeof(cur_mute));
  }

  if (control == AUDIO10_FU_CTRL_VOLUME)
  {
    audio10_control_cur_2_t value;

    if (request == AUDIO10_CS_REQ_GET_CUR)
    {
      value.bCur = tu_htole16(volume[channel]);
    }
    else if (request == AUDIO10_CS_REQ_GET_MIN)
    {
      value.bCur = tu_htole16(-VOLUME_CTRL_50_DB);
    }
    else if (request == AUDIO10_CS_REQ_GET_MAX)
    {
      value.bCur = tu_htole16(VOLUME_CTRL_0_DB);
    }
    else if (request == AUDIO10_CS_REQ_GET_RES)
    {
      value.bCur = tu_htole16(256);
    }
    else
    {
      return false;
    }

    return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request,
                                                      &value, sizeof(value));
  }

  return false;
}

static bool uac1_set_feature_unit_request(uint8_t rhport, tusb_control_request_t const *p_request,
                                          uint8_t const *buf)
{
  (void)rhport;

  uint8_t const entity = UAC_REQ_ENTITY_ID(p_request);
  uint8_t const control = UAC_REQ_CONTROL(p_request);

  if (entity != UAC1_ENTITY_SPK_FEATURE_UNIT ||
      p_request->bRequest != AUDIO10_CS_REQ_SET_CUR)
  {
    return false;
  }

  uint8_t const channel = uac1_clamp_channel(UAC_REQ_CHANNEL(p_request));

  if (control == AUDIO10_FU_CTRL_MUTE)
  {
    TU_VERIFY(p_request->wLength == sizeof(audio10_control_cur_1_t));
    mute[channel] = ((audio10_control_cur_1_t const *)buf)->bCur;
  }
  else if (control == AUDIO10_FU_CTRL_VOLUME)
  {
    TU_VERIFY(p_request->wLength == sizeof(audio10_control_cur_2_t));
    volume[channel] = (int16_t)tu_le16toh(((audio10_control_cur_2_t const *)buf)->bCur);
  }
  else
  {
    return false;
  }

  if (channel != 0)
  {
    mute[0] = mute[channel];
    volume[0] = volume[channel];
  }
  need_change_bt_volume = true;

  return true;
}

static void uac1_fill_sample_freq(uint8_t sample_freq[3], uint32_t freq)
{
  sample_freq[0] = (uint8_t)(freq & 0xff);
  sample_freq[1] = (uint8_t)((freq >> 8) & 0xff);
  sample_freq[2] = (uint8_t)((freq >> 16) & 0xff);
}

static bool uac1_get_endpoint_request(uint8_t rhport, tusb_control_request_t const *p_request)
{
  uint8_t const ep = UAC_REQ_ENDPOINT(p_request);
  uint8_t const control = UAC_REQ_CONTROL(p_request);
  uint8_t const request = p_request->bRequest;

  if (ep != 0x01 || control != AUDIO10_EP_CTRL_SAMPLING_FREQ)
  {
    return false;
  }

  audio10_control_cur_3_t freq = { 0 };
  if (request == AUDIO10_CS_REQ_GET_CUR ||
      request == AUDIO10_CS_REQ_GET_MIN ||
      request == AUDIO10_CS_REQ_GET_MAX)
  {
    uac1_fill_sample_freq(freq.bCur, USB_AUDIO_SAMPLE_RATE);
  }
  else if (request != AUDIO10_CS_REQ_GET_RES)
  {
    return false;
  }

  return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request,
                                                    &freq, sizeof(freq));
}

static bool uac1_set_endpoint_request(uint8_t rhport, tusb_control_request_t const *p_request,
                                      uint8_t const *buf)
{
  (void)rhport;

  uint8_t const ep = UAC_REQ_ENDPOINT(p_request);
  uint8_t const control = UAC_REQ_CONTROL(p_request);

  if (ep != 0x01 ||
      control != AUDIO10_EP_CTRL_SAMPLING_FREQ ||
      p_request->bRequest != AUDIO10_CS_REQ_SET_CUR ||
      p_request->wLength != sizeof(audio10_control_cur_3_t))
  {
    return false;
  }

  audio10_control_cur_3_t const *freq = (audio10_control_cur_3_t const *)buf;
  current_sample_rate = (uint32_t)freq->bCur[0] |
                        ((uint32_t)freq->bCur[1] << 8) |
                        ((uint32_t)freq->bCur[2] << 16);

  if (current_sample_rate != USB_AUDIO_SAMPLE_RATE)
  {
    current_sample_rate = USB_AUDIO_SAMPLE_RATE;
  }

  return true;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  return uac1_get_feature_unit_request(rhport, p_request);
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
  return uac1_set_feature_unit_request(rhport, p_request, buf);
}

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  return uac1_get_endpoint_request(rhport, p_request);
}

bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
  return uac1_set_endpoint_request(rhport, p_request, buf);
}

bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  (void)rhport;

  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt == 0)
  {
    blink_interval_ms = BLINK_MOUNTED;
    is_usb_audio_running = false;
    set_usb_streaming(false);
  }

  return true;
}

bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
  (void)rhport;

  uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
  uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));

  TU_LOG2("Set interface %d alt %d\r\n", itf, alt);

  spk_data_size = 0;
  current_resolution = USB_AUDIO_RESOLUTION_BITS;

  if (ITF_NUM_AUDIO_STREAMING_SPK == itf)
  {
    if (alt != 0)
    {
      blink_interval_ms = BLINK_STREAMING;
      usb_stop_delay = 0;
    }
    else
    {
      blink_interval_ms = BLINK_MOUNTED;
      is_usb_audio_running = false;
      set_usb_streaming(false);
    }
  }

  return true;
}

static void fill_usb_rx_test_tone(int16_t *dst, uint16_t stereo_pair_count)
{
  static uint16_t phase = 0;

  for (uint16_t i = 0; i < stereo_pair_count; i++)
  {
    int16_t sample = (phase < 32) ? 1200 : -1200;
    dst[(i * 2) + 0] = sample;
    dst[(i * 2) + 1] = sample;

    phase++;
    if (phase >= 64)
    {
      phase = 0;
    }
  }
}

static void push_usb_speaker_packet(uint16_t n_bytes_received)
{
  if (n_bytes_received == 0)
  {
    return;
  }

  uint16_t bytes_to_read = n_bytes_received;
  if (bytes_to_read > sizeof(spk_buf))
  {
    bytes_to_read = sizeof(spk_buf);
  }

  spk_data_size = tud_audio_read(spk_buf, bytes_to_read);
  if (spk_data_size <= 0)
  {
    return;
  }

  usb_stop_delay = 0;
  is_usb_audio_running = true;
  set_usb_streaming(true);

  if (current_resolution == USB_AUDIO_RESOLUTION_BITS &&
      (spk_data_size % (USB_AUDIO_BYTES_PER_SAMPLE * USB_AUDIO_SPK_CHANNELS)) == 0)
  {
    uint16_t stereo_pair_count = spk_data_size /
      (USB_AUDIO_BYTES_PER_SAMPLE * USB_AUDIO_SPK_CHANNELS);

#if USB_AUDIO_RX_TEST_TONE
    fill_usb_rx_test_tone(spk_buf, stereo_pair_count);
#endif

    audio_slot_push_samples(spk_buf, stereo_pair_count);
  }

  spk_data_size = 0;
}

bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id,
                                   uint8_t ep_out, uint8_t cur_alt_setting)
{
  (void)rhport;
  (void)func_id;
  (void)ep_out;
  (void)cur_alt_setting;

  push_usb_speaker_packet(n_bytes_received);
  return true;
}

void audio_task(void)
{
  if (is_usb_audio_running)
  {
    usb_stop_delay = 0;
  }
  else
  {
    usb_stop_delay++;
    if (usb_stop_delay > 100)
    {
      set_usb_streaming(false);
    }
  }
  is_usb_audio_running = false;
}

void audio_control_task(void)
{
  if (*get_is_bt_sink_volume_changed_ptr())
  {
    uint8_t bt_level = get_bt_volume();

    mute[0] = get_bt_mute();

    uint16_t usb_level = (bt_level > BT_VOL_MAX)
                       ? USB_ATT_MAX
                       : (uint16_t)((((uint32_t)(BT_VOL_MAX - bt_level) * USB_ATT_MAX) +
                                     (BT_VOL_MAX / 2)) / BT_VOL_MAX);

    volume[0] = -1 * usb_level / 2;

#if CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP
    const audio_interrupt_data_t data = {
      .bInfo = 0,
      .bAttribute = AUDIO10_CS_REQ_GET_CUR,
      .wValue_cn_or_mcn = 0,
      .wValue_cs = AUDIO10_FU_CTRL_VOLUME,
      .wIndex_ep_or_int = 0,
      .wIndex_entity_id = UAC1_ENTITY_SPK_FEATURE_UNIT,
    };
    tud_audio_int_write(&data);
#endif
    *get_is_bt_sink_volume_changed_ptr() = false;
  }

  if (need_change_bt_volume)
  {
#if USB_AUDIO_FORCE_BT_VOLUME_MAX
    mute[0] = 0;
    mute0_last = 0;
    volume[0] = 0;
    volume0_last = 0;
    set_bt_volume(0);
#else
    if (mute[0] == 1 && volume0_last == volume[0])
    {
      if (mute0_last == 1)
      {
        set_bt_volume(volume[0] / 256);
        mute0_last = 0;
        mute[0] = 0;
      }
      else
      {
        mute0_last = 1;
        set_bt_volume(-50);
      }
    }
    else
    {
      set_bt_volume(volume[0] / 256);
    }
    volume0_last = volume[0];
#endif
    need_change_bt_volume = false;

#if CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP
    const audio_interrupt_data_t data = {
      .bInfo = 0,
      .bAttribute = AUDIO10_CS_REQ_GET_CUR,
      .wValue_cn_or_mcn = 0,
      .wValue_cs = AUDIO10_FU_CTRL_VOLUME,
      .wIndex_ep_or_int = 0,
      .wIndex_entity_id = UAC1_ENTITY_SPK_FEATURE_UNIT,
    };
    tud_audio_int_write(&data);
#endif
  }
}

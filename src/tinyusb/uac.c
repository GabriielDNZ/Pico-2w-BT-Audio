/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Jerzy Kasenberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

 #include <stdio.h>
 #include <string.h>
 
 #include "bsp/board_api.h"
 #include "tusb.h"
 #include "usb_descriptors.h"

 #include "../btstack/btstack_avdtp_source.h"
 #include "pico/flash.h"

#define UAC2_ENTITY_CLOCK 0xff
#define UAC2_ENTITY_SPK_FEATURE_UNIT UAC1_ENTITY_SPK_FEATURE_UNIT

#define AUDIO10_CS_REQ_SET_CUR 0x01
#define AUDIO10_CS_REQ_GET_CUR 0x81
#define AUDIO10_CS_REQ_GET_MIN 0x82
#define AUDIO10_CS_REQ_GET_MAX 0x83
#define AUDIO10_CS_REQ_GET_RES 0x84

#define AUDIO10_FU_CTRL_MUTE   0x01
#define AUDIO10_FU_CTRL_VOLUME 0x02
#define AUDIO10_EP_CTRL_SAMPLING_FREQ 0x01

#define UAC_REQ_ENTITY_ID(_request)  ((uint8_t)((tu_le16toh((_request)->wIndex) >> 8) & 0xff))
#define UAC_REQ_ENDPOINT(_request)   ((uint8_t)(tu_le16toh((_request)->wIndex) & 0xff))
#define UAC_REQ_CONTROL(_request)    ((uint8_t)((tu_le16toh((_request)->wValue) >> 8) & 0xff))
#define UAC_REQ_CHANNEL(_request)    ((uint8_t)(tu_le16toh((_request)->wValue) & 0xff))

 typedef struct TU_ATTR_PACKED
 {
   uint8_t bCur[3];
 } audio10_control_cur_3_t;

 typedef struct TU_ATTR_PACKED
 {
   int16_t bCur;
 } audio10_control_cur_2_t;

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


 
 //--------------------------------------------------------------------+
 // MACRO CONSTANT TYPEDEF PROTOTYPES
 //--------------------------------------------------------------------+
 
 // List of supported sample rates
 const uint32_t sample_rates[] = {48000};

 uint32_t current_sample_rate  = 48000;

 bool need_change_bt_volume = false;
 
 #define N_SAMPLE_RATES  TU_ARRAY_SIZE(sample_rates)
 
 /* Blink pattern
  * - 25 ms   : streaming data
  * - 250 ms  : device not mounted
  * - 1000 ms : device mounted
  * - 2500 ms : device is suspended
  */
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
 
 // Audio controls
 // Current states
 int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];       // +1 for master channel 0
 int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1];    // +1 for master channel 0
 
 int16_t volume0_last = 0;
 int8_t mute0_last = 0; 

 int8_t mic_mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
 int16_t mic_volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX + 1];
 bool is_usb_mic_running = false;

 // Buffer for microphone data
 int16_t mic_silence[USB_AUDIO_MIC_PACKET_BYTES / sizeof(int16_t)];

 // Buffer for speaker data
 int32_t spk_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ / 4];
 // Speaker data size received in the last frame
 int spk_data_size;
 // Resolution per format
 const uint8_t resolutions_per_format[CFG_TUD_AUDIO_FUNC_1_N_FORMATS] = {CFG_TUD_AUDIO_FUNC_1_FORMAT_1_RESOLUTION_RX};
 // Current resolution, update on format change
 uint8_t current_resolution;
 
 //void led_blinking_task(void);
 void audio_task(void);
 void audio_control_task(void);
 
 /*------------- MAIN -------------*/
 void tinyusb_main(void)
 {


  flash_safe_execute_core_init();

  //board_init();
 
   // init device stack on configured roothub port
   tusb_rhport_init_t dev_init = {
     .role = TUSB_ROLE_DEVICE,
     .speed = TUSB_SPEED_AUTO
   };
   tusb_init(BOARD_TUD_RHPORT, &dev_init);


 }


 void tinyusb_task(void){
    tud_task(); // TinyUSB device task
    audio_task(); 
 }
 

void tinyusb_control_task(void){
  //tud_task(); // TinyUSB device task
  audio_control_task();
}

 //--------------------------------------------------------------------+
 // Device callbacks
 //--------------------------------------------------------------------+
 
 // Invoked when device is mounted
 void tud_mount_cb(void)
 {
   //blink_interval_ms = BLINK_MOUNTED;
 }
 
 // Invoked when device is unmounted
 void tud_umount_cb(void)
 {
   //blink_interval_ms = BLINK_NOT_MOUNTED;
 }
 
 // Invoked when usb bus is suspended
 // remote_wakeup_en : if host allow us  to perform remote wakeup
 // Within 7ms, device must draw an average of current less than 2.5 mA from bus
 void tud_suspend_cb(bool remote_wakeup_en)
 {
   (void)remote_wakeup_en;
   printf("tud_suspend_cb\n");
 }
 
 // Invoked when usb bus is resumed
 void tud_resume_cb(void)
 {
  printf("tud_resume_cb\n");
 }

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
 
 // Helper for clock get requests
 static bool tud_audio_clock_get_request(uint8_t rhport, audio_control_request_t const *request)
 {
   TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
 
   if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
   {
     if (request->bRequest == AUDIO_CS_REQ_CUR)
     {
       TU_LOG1("Clock get current freq %" PRIu32 "\r\n", current_sample_rate);
 
       audio_control_cur_4_t curf = { (int32_t) tu_htole32(current_sample_rate) };
       return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &curf, sizeof(curf));
     }
     else if (request->bRequest == AUDIO_CS_REQ_RANGE)
     {
       audio_control_range_4_n_t(N_SAMPLE_RATES) rangef =
       {
         .wNumSubRanges = tu_htole16(N_SAMPLE_RATES)
       };
       TU_LOG1("Clock get %d freq ranges\r\n", N_SAMPLE_RATES);
       for(uint8_t i = 0; i < N_SAMPLE_RATES; i++)
       {
         rangef.subrange[i].bMin = (int32_t) sample_rates[i];
         rangef.subrange[i].bMax = (int32_t) sample_rates[i];
         rangef.subrange[i].bRes = 0;
         TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax, (int)rangef.subrange[i].bRes);
       }
 
       return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &rangef, sizeof(rangef));
     }
   }
   else if (request->bControlSelector == AUDIO_CS_CTRL_CLK_VALID &&
            request->bRequest == AUDIO_CS_REQ_CUR)
   {
     audio_control_cur_1_t cur_valid = { .bCur = 1 };
     TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
     return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_valid, sizeof(cur_valid));
   }
   TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n",
           request->bEntityID, request->bControlSelector, request->bRequest);
   return false;
 }
 
 // Helper for clock set requests
 static bool tud_audio_clock_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
 {
   (void)rhport;
 
   TU_ASSERT(request->bEntityID == UAC2_ENTITY_CLOCK);
   TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);
 
   if (request->bControlSelector == AUDIO_CS_CTRL_SAM_FREQ)
   {
     TU_VERIFY(request->wLength == sizeof(audio_control_cur_4_t));
 
     current_sample_rate = (uint32_t) ((audio_control_cur_4_t const *)buf)->bCur;
 
     TU_LOG1("Clock set current freq: %" PRIu32 "\r\n", current_sample_rate);
 
     return true;
   }
   else
   {
     TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
             request->bEntityID, request->bControlSelector, request->bRequest);
     return false;
   }
 }
 
 // Helper for feature unit get requests
 static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio_control_request_t const *request)
 {
   TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);
 
   if (request->bControlSelector == AUDIO_FU_CTRL_MUTE && request->bRequest == AUDIO_CS_REQ_CUR)
   {
     audio_control_cur_1_t mute1 = { .bCur = mute[request->bChannelNumber] };
     TU_LOG1("Get channel %u mute %d\r\n", request->bChannelNumber, mute1.bCur);
     return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1, sizeof(mute1));
   }
   else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
   {
     if (request->bRequest == AUDIO_CS_REQ_RANGE)
     {
       audio_control_range_2_n_t(1) range_vol = {
         .wNumSubRanges = tu_htole16(1),
         .subrange[0] = { .bMin = tu_htole16(-VOLUME_CTRL_50_DB), tu_htole16(VOLUME_CTRL_0_DB), tu_htole16(256) }
       };
       TU_LOG1("Get channel %u volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
               range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256, range_vol.subrange[0].bRes / 256);
       return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &range_vol, sizeof(range_vol));
     }
     else if (request->bRequest == AUDIO_CS_REQ_CUR)
     {
       audio_control_cur_2_t cur_vol = { .bCur = tu_htole16(volume[request->bChannelNumber]) };
       TU_LOG1("Get channel %u volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
       return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &cur_vol, sizeof(cur_vol));
     }
   }
   TU_LOG1("Feature unit get request not supported, entity = %u, selector = %u, request = %u\r\n",
           request->bEntityID, request->bControlSelector, request->bRequest);
 
   return false;
 }
 
 // Helper for feature unit set requests
 static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio_control_request_t const *request, uint8_t const *buf)
 {
   (void)rhport;
 
   TU_ASSERT(request->bEntityID == UAC2_ENTITY_SPK_FEATURE_UNIT);
   TU_VERIFY(request->bRequest == AUDIO_CS_REQ_CUR);
 
   if (request->bControlSelector == AUDIO_FU_CTRL_MUTE)
   {
     TU_VERIFY(request->wLength == sizeof(audio_control_cur_1_t));
 
     mute[request->bChannelNumber] = ((audio_control_cur_1_t const *)buf)->bCur;

     if(request->bChannelNumber == 0){
      need_change_bt_volume = true;
     }
 
     TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, mute[request->bChannelNumber]);
 
     return true;
   }
   else if (request->bControlSelector == AUDIO_FU_CTRL_VOLUME)
   {
     TU_VERIFY(request->wLength == sizeof(audio_control_cur_2_t));
 
     volume[request->bChannelNumber] = ((audio_control_cur_2_t const *)buf)->bCur;
 
     TU_LOG1("Set channel %d volume: %d dB\r\n", request->bChannelNumber, volume[request->bChannelNumber]/256);

     if(request->bChannelNumber == 0){
      need_change_bt_volume = true;
     }
 
     return true;
   }
   else
   {
     TU_LOG1("Feature unit set request not supported, entity = %u, selector = %u, request = %u\r\n",
             request->bEntityID, request->bControlSelector, request->bRequest);
     return false;
   }
 }

 static bool uac1_is_speaker_feature_unit(uint8_t entity)
 {
   return entity == UAC1_ENTITY_SPK_FEATURE_UNIT;
 }

 static bool uac1_is_mic_feature_unit(uint8_t entity)
 {
   return entity == UAC1_ENTITY_MIC_FEATURE_UNIT_1 ||
          entity == UAC1_ENTITY_MIC_FEATURE_UNIT_2;
 }

 static uint8_t uac1_clamp_channel(uint8_t channel, uint8_t max_channels)
 {
   return channel > max_channels ? 0 : channel;
 }

 static bool uac1_get_feature_unit_request(uint8_t rhport, tusb_control_request_t const *p_request)
 {
   uint8_t const entity = UAC_REQ_ENTITY_ID(p_request);
   uint8_t const control = UAC_REQ_CONTROL(p_request);
   uint8_t const request = p_request->bRequest;

   if (!uac1_is_speaker_feature_unit(entity) && !uac1_is_mic_feature_unit(entity))
   {
     return false;
   }

   int8_t *mute_state = mute;
   int16_t *volume_state = volume;
   uint8_t max_channels = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX;

   if (uac1_is_mic_feature_unit(entity))
   {
     mute_state = mic_mute;
     volume_state = mic_volume;
     max_channels = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
   }

   uint8_t const channel = uac1_clamp_channel(UAC_REQ_CHANNEL(p_request), max_channels);

   if (control == AUDIO10_FU_CTRL_MUTE)
   {
     if (request == AUDIO10_CS_REQ_GET_CUR)
     {
       audio_control_cur_1_t cur_mute = { .bCur = mute_state[channel] };
       return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &cur_mute, sizeof(cur_mute));
     }
   }
   else if (control == AUDIO10_FU_CTRL_VOLUME)
   {
     audio10_control_cur_2_t value;

     if (request == AUDIO10_CS_REQ_GET_CUR)
     {
       value.bCur = tu_htole16(volume_state[channel]);
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

     return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &value, sizeof(value));
   }

   return false;
 }

 static bool uac1_set_feature_unit_request(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t const *buf)
 {
   (void)rhport;

   uint8_t const entity = UAC_REQ_ENTITY_ID(p_request);
   uint8_t const control = UAC_REQ_CONTROL(p_request);

   if (p_request->bRequest != AUDIO10_CS_REQ_SET_CUR ||
       (!uac1_is_speaker_feature_unit(entity) && !uac1_is_mic_feature_unit(entity)))
   {
     return false;
   }

   int8_t *mute_state = mute;
   int16_t *volume_state = volume;
   uint8_t max_channels = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX;
   bool const is_speaker = uac1_is_speaker_feature_unit(entity);

   if (!is_speaker)
   {
     mute_state = mic_mute;
     volume_state = mic_volume;
     max_channels = CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_TX;
   }

   uint8_t const channel = uac1_clamp_channel(UAC_REQ_CHANNEL(p_request), max_channels);

   if (control == AUDIO10_FU_CTRL_MUTE)
   {
     TU_VERIFY(p_request->wLength == sizeof(audio_control_cur_1_t));
     mute_state[channel] = ((audio_control_cur_1_t const *)buf)->bCur;
   }
   else if (control == AUDIO10_FU_CTRL_VOLUME)
   {
     TU_VERIFY(p_request->wLength == sizeof(audio10_control_cur_2_t));
     volume_state[channel] = (int16_t) tu_le16toh(((audio10_control_cur_2_t const *)buf)->bCur);
   }
   else
   {
     return false;
   }

   if (is_speaker)
   {
     if (channel != 0)
     {
       volume[0] = volume_state[channel];
       mute[0] = mute_state[channel];
     }
     need_change_bt_volume = true;
   }

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

   if ((ep != 0x01 && ep != 0x82) || control != AUDIO10_EP_CTRL_SAMPLING_FREQ)
   {
     return false;
   }

   audio10_control_cur_3_t freq = { 0 };
   if (request == AUDIO10_CS_REQ_GET_CUR ||
       request == AUDIO10_CS_REQ_GET_MIN ||
       request == AUDIO10_CS_REQ_GET_MAX)
   {
     uac1_fill_sample_freq(freq.bCur, current_sample_rate);
   }
   else if (request != AUDIO10_CS_REQ_GET_RES)
   {
     return false;
   }

   return tud_audio_buffer_and_schedule_control_xfer(rhport, p_request, &freq, sizeof(freq));
 }

 static bool uac1_set_endpoint_request(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t const *buf)
 {
   (void)rhport;

   uint8_t const ep = UAC_REQ_ENDPOINT(p_request);
   uint8_t const control = UAC_REQ_CONTROL(p_request);

   if ((ep != 0x01 && ep != 0x82) ||
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
 
 //--------------------------------------------------------------------+
 // Application Callback API Implementations
 //--------------------------------------------------------------------+
 
 // Invoked when audio class specific get request received for an entity
 bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
 {
   return uac1_get_feature_unit_request(rhport, p_request);
 }
 
 // Invoked when audio class specific set request received for an entity
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
 
 bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const * p_request)
 {
   (void)rhport;
 
   uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
   uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));
 
   if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt == 0)
       blink_interval_ms = BLINK_MOUNTED;

   if (ITF_NUM_AUDIO_STREAMING_MIC == itf && alt == 0)
       is_usb_mic_running = false;
 
   return true;
 }
 
 bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const * p_request)
 {
   (void)rhport;
   uint8_t const itf = tu_u16_low(tu_le16toh(p_request->wIndex));
   uint8_t const alt = tu_u16_low(tu_le16toh(p_request->wValue));
 
   TU_LOG2("Set interface %d alt %d\r\n", itf, alt);
   if (ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0)
       blink_interval_ms = BLINK_STREAMING;

   if (ITF_NUM_AUDIO_STREAMING_MIC == itf)
       is_usb_mic_running = alt != 0;
 
   // Clear buffer when streaming format is changed
   spk_data_size = 0;
   if(ITF_NUM_AUDIO_STREAMING_SPK == itf && alt != 0)
   {
     current_resolution = resolutions_per_format[alt-1];
   }
 
   return true;
 }

 uint16_t usb_stop_delay = 0;
 bool is_usb_audio_running = false;

 bool tud_audio_rx_done_pre_read_cb(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting)
 {
   (void)rhport;
   (void)func_id;
   (void)ep_out;
   (void)cur_alt_setting;
 
   spk_data_size = tud_audio_read(spk_buf, n_bytes_received);

   if (spk_data_size)
   {
    usb_stop_delay = 0;
    set_usb_streaming(true);
    if (current_resolution == 16)
    {
      int16_t *src = (int16_t *)spk_buf;
      uint16_t sample_count = spk_data_size / 4; // should be 44-45

      audio_slot_push_samples(src, sample_count);

      is_usb_audio_running = true;
      spk_data_size = 0;
    }
   } 
 
   return true;
 }

 static void queue_mic_silence(void)
 {
   if (is_usb_mic_running)
   {
     tud_audio_write(mic_silence, sizeof(mic_silence));
   }
 }
 
 bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t itf, uint8_t ep_in, uint8_t cur_alt_setting)
 {
   (void)rhport;
   (void)ep_in;
   (void)cur_alt_setting;

   if (itf == ITF_NUM_AUDIO_STREAMING_MIC)
   {
     queue_mic_silence();
   }

   return true;
 }
 
 //--------------------------------------------------------------------+
 // AUDIO Task
 //--------------------------------------------------------------------+


 void audio_task(void)
 {
  queue_mic_silence();

  if (is_usb_audio_running){
    usb_stop_delay = 0;
  }else{
    usb_stop_delay++;
    if (usb_stop_delay > 100){
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

    // 2) invert & scale into 0…25600
    //    (BT_VOL_MAX - bt_level) maps 127→0, 0→127
    //    multiply then divide with rounding
    uint16_t usb_level = (bt_level > BT_VOL_MAX)
                       ? USB_ATT_MAX
                       : (uint16_t)(( (uint32_t)(BT_VOL_MAX - bt_level)
                                     * USB_ATT_MAX
                                     + (BT_VOL_MAX/2) )
                                   / BT_VOL_MAX);

    // 3) store into USB-Audio’s volume (attenuation) field
    volume[0] = -1 * usb_level / 2;

#if CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP
     // 6.1 Interrupt Data Message
     const audio_interrupt_data_t data = {
       .bInfo = 0,                                       // Class-specific interrupt, originated from an interface
       .bAttribute = AUDIO_CS_REQ_CUR,                   // Caused by current settings
       .wValue_cn_or_mcn = 0,                            // CH0: master volume
       .wValue_cs = AUDIO_FU_CTRL_VOLUME,                // Volume change
       .wIndex_ep_or_int = 0,                            // From the interface itself
       .wIndex_entity_id = UAC2_ENTITY_SPK_FEATURE_UNIT, // From feature unit
     };
 
     tud_audio_int_write(&data);
#endif
     *get_is_bt_sink_volume_changed_ptr() = false;
   }

   if (need_change_bt_volume){

    //printf("vol is %d\n", volume[0]);
    
    if(mute[0] == 1 && volume0_last == volume[0]){
      if (mute0_last == 1){
        set_bt_volume(volume[0]/256);
        mute0_last = 0;
        mute[0] = 0;
      }else{
        mute0_last = 1;
        set_bt_volume(-50);
      }
    }else{
      set_bt_volume(volume[0]/256);
    }
    volume0_last = volume[0];
    need_change_bt_volume = false;

#if CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP
    const audio_interrupt_data_t data = {
      .bInfo = 0,                                       // Class-specific interrupt, originated from an interface
      .bAttribute = AUDIO_CS_REQ_CUR,                   // Caused by current settings
      .wValue_cn_or_mcn = 0,                            // CH0: master volume
      .wValue_cs = AUDIO_FU_CTRL_VOLUME,                // Volume change
      .wIndex_ep_or_int = 0,                            // From the interface itself
      .wIndex_entity_id = UAC2_ENTITY_SPK_FEATURE_UNIT, // From feature unit
    };

    tud_audio_int_write(&data);
#endif
    //*get_is_bt_sink_volume_changed_ptr() = false;
   }

  }
 

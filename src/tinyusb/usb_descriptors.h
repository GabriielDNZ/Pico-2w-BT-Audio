/*
 * UAC1 descriptors for a PS5-friendly USB speaker profile.
 *
 * This keeps the device in the USB Audio 1.0 path used by the official
 * TinyUSB audio10 support instead of feeding a hand-built UAC1 tree to
 * the UAC2-style headset descriptor.
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#define USB_AUDIO_PNP_VID                    0x0C76
#define USB_AUDIO_PNP_PID                    0x161F
#define USB_AUDIO_PNP_BCD_USB                0x0200
#define USB_AUDIO_PNP_BCD_DEVICE             0x0100

#define USB_AUDIO_SAMPLE_RATE                48000
#define USB_AUDIO_SPK_CHANNELS               2
#define USB_AUDIO_MIC_CHANNELS               1
#define USB_AUDIO_BYTES_PER_SAMPLE           2
#define USB_AUDIO_RESOLUTION_BITS            16
// Nominal 48 kHz stereo 16-bit is 192 bytes/ms.
// Keep the UAC1 endpoint max at 196 so the host can legally deliver
// occasional 49-sample adaptive packets without the device refusing/truncating them.
#define USB_AUDIO_SPK_NOMINAL_PACKET_BYTES   192
#define USB_AUDIO_SPK_PACKET_BYTES           196
#define USB_AUDIO_MIC_PACKET_BYTES           96

#define USB_HID_REPORT_DESC_LEN              62
#define USB_HID_EP_SIZE                      4
#define USB_HID_EP_INTERVAL                  32

// Entity IDs follow TinyUSB's UAC1 headset example layout.
#define UAC1_ENTITY_SPK_INPUT_TERMINAL       0x01
#define UAC1_ENTITY_SPK_FEATURE_UNIT         0x02
#define UAC1_ENTITY_SPK_OUTPUT_TERMINAL      0x03
#define UAC1_ENTITY_MIC_INPUT_TERMINAL       0x11
#define UAC1_ENTITY_MIC_FEATURE_UNIT         0x12
#define UAC1_ENTITY_MIC_OUTPUT_TERMINAL      0x13

enum
{
  ITF_NUM_AUDIO_CONTROL = 0,
  ITF_NUM_AUDIO_STREAMING_SPK,
  ITF_NUM_TOTAL,
  ITF_NUM_AUDIO_STREAMING_MIC = 0xff
};

#endif

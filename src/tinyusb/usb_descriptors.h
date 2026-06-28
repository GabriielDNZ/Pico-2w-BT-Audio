/*
 * UAC1 speaker-only descriptors for a PS5-friendly USB audio output profile.
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#define USB_AUDIO_PNP_VID                    0x0D8C
#define USB_AUDIO_PNP_PID                    0x0014
#define USB_AUDIO_PNP_BCD_USB                0x0110
#define USB_AUDIO_PNP_BCD_DEVICE             0x0100

#define USB_AUDIO_SAMPLE_RATE                48000
#define USB_AUDIO_SPK_CHANNELS               2
#define USB_AUDIO_MIC_CHANNELS               1
#define USB_AUDIO_BYTES_PER_SAMPLE           2
#define USB_AUDIO_RESOLUTION_BITS            16
#define USB_AUDIO_SPK_PACKET_BYTES           192
#define USB_AUDIO_MIC_PACKET_BYTES           96

#define USB_HID_REPORT_DESC_LEN              62
#define USB_HID_EP_SIZE                      4
#define USB_HID_EP_INTERVAL                  32

#define UAC1_ENTITY_SPK_INPUT_TERMINAL       0x01
#define UAC1_ENTITY_SPK_FEATURE_UNIT         0x02
#define UAC1_ENTITY_SPK_OUTPUT_TERMINAL      0x03

// Kept defined so uac.c can compile without mic path enabled.
#define UAC1_ENTITY_MIC_INPUT_TERMINAL       0x11
#define UAC1_ENTITY_MIC_FEATURE_UNIT         0x12
#define UAC1_ENTITY_MIC_OUTPUT_TERMINAL      0x13

enum
{
  ITF_NUM_AUDIO_CONTROL = 0,
  ITF_NUM_AUDIO_STREAMING_SPK,
  ITF_NUM_TOTAL
};

#endif

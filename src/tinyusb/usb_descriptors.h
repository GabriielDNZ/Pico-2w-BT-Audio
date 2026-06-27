/*
 * USB descriptors for a PS5-friendly UAC1 headset profile.
 *
 * The layout is based on the working "USB PnP Audio Device"
 * VID_0C76/PID_161F descriptor captured from the user's adapter.
 */

#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

#define USB_AUDIO_PNP_VID                    0x0C76
#define USB_AUDIO_PNP_PID                    0x161F
#define USB_AUDIO_PNP_BCD_USB                0x0110
#define USB_AUDIO_PNP_BCD_DEVICE             0x0100

#define USB_AUDIO_SAMPLE_RATE                48000
#define USB_AUDIO_SPK_CHANNELS               2
#define USB_AUDIO_MIC_CHANNELS               1
#define USB_AUDIO_BYTES_PER_SAMPLE           2
#define USB_AUDIO_RESOLUTION_BITS            16
#define USB_AUDIO_SPK_PACKET_BYTES           200
#define USB_AUDIO_MIC_PACKET_BYTES           96
#define USB_AUDIO_ISO_EP_SIZE                200

#define USB_AUDIO_PNP_DESC_LEN               247
#define USB_AUDIO_PNP_FUNC_DESC_LEN          213
#define USB_HID_REPORT_DESC_LEN              62
#define USB_HID_EP_SIZE                      4
#define USB_HID_EP_INTERVAL                  32

// UAC1 entity IDs captured from the working USB PnP adapter.
#define UAC1_ENTITY_USB_OUT_INPUT_TERMINAL   0x01
#define UAC1_ENTITY_MIC_INPUT_TERMINAL       0x02
#define UAC1_ENTITY_SPK_OUTPUT_TERMINAL      0x11
#define UAC1_ENTITY_USB_IN_OUTPUT_TERMINAL   0x12
#define UAC1_ENTITY_SELECTOR_UNIT            0x21
#define UAC1_ENTITY_SPK_FEATURE_UNIT         0x31
#define UAC1_ENTITY_MIC_FEATURE_UNIT_1       0x32
#define UAC1_ENTITY_MIC_FEATURE_UNIT_2       0x33
#define UAC1_ENTITY_MIXER_UNIT               0x41

enum
{
  ITF_NUM_AUDIO_CONTROL = 0,
  ITF_NUM_AUDIO_STREAMING_SPK,
  ITF_NUM_AUDIO_STREAMING_MIC,
  ITF_NUM_HID,
  ITF_NUM_TOTAL
};

#endif

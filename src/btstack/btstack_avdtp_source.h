//
// Created by Sean on 8/7/23.
//

#ifndef PICOW_USB_BT_AUDIO_SSP_COUNTER_H
#define PICOW_USB_BT_AUDIO_SSP_COUNTER_H

#include <stdint.h>
#include <stdbool.h>

// Slot queue constants
#define AUDIO_SLOT_COUNT_SBC   6    // SBC 30 ms slots6: keep timer=5 and frames=4
#define AUDIO_SLOT_COUNT_AAC   2    // AAC-LC "35": 2 frames = 2*1024/48000 ~= 42.6 ms (stable)
#define AUDIO_SLOT_COUNT_ELD   4    // AAC-ELD "35": 4*10ms ~= 40 ms (stable)
#define AUDIO_SLOT_COUNT_LDAC  16   // LDAC: 256 samples/slot, 16*5.3ms = 85ms buffer
#define AUDIO_SLOT_COUNT_MAX   24   // pool size must still cover non-SBC codecs
#define AUDIO_SLOT_MAX_SAMPLES 1024
#define AUDIO_SLOT_MAX_INT16   (AUDIO_SLOT_MAX_SAMPLES * 2)  // stereo

// Slot queue API (multicore-safe)
void audio_slot_queue_init(void);
void audio_slot_queue_configure(uint16_t samples_per_slot);
void audio_slot_push_samples(const int16_t *src, uint16_t stereo_pair_count);

bool check_is_streaming();

void set_bt_volume(int16_t);

uint8_t get_bt_volume();

bool get_bt_mute();

void set_usb_streaming(bool flag);

bool * get_is_bt_sink_volume_changed_ptr();

int btstack_main(int argc, const char * argv[]);

void avdtp_disconnect_and_scan(void);

void gap_start_scanning(void);

bool get_a2dp_connected_flag();

void a2dp_source_reconnect();

void avdtp_source_establish_stream();

void set_next_capablity_and_start_stream();

void start_led_blink();

static int setup_aac_configuration();

static int setup_sbc_configuration();

static int set_ldac_configuration();

bool get_allow_switch_slot();

void core1_aaceld_encoder_loop(void);

void increase_vol_by_key();

void decrease_vol_by_key();

#endif //PICOW_USB_BT_AUDIO_SSP_COUNTER_H

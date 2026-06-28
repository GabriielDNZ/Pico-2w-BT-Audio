Pico-2w BT Audio PS5 UAC1 endpoint OUT memory v6

Base: first tone-test build that produced beep over Bluetooth.

Changes:
- Continuous test beep disabled (USB_AUDIO_IDLE_TEST_TONE=0) so it will not mask real USB audio.
- TinyUSB OUT endpoint drain reworked: uses tud_audio_n_available(0) + tud_audio_n_read(0) from audio_task, plus the old pre_read callback and rx_done_isr.
- CFG_TUD_AUDIO_FUNC_1_N_AS_INT set to 4 for UAC1 headset descriptors: speaker alt0/alt1 + mic alt0/alt1.
- pico_btstack_flash_bank linked for BTstack link-key persistence.
- Project slot/MAC flash storage moved away from final flash sectors so it does not erase BTstack link keys.

If it compiles and connects but is silent, USB host is still not sending or TinyUSB is not opening OUT; check UART log for:
USB speaker OUT endpoint opened
USB OUT status: open=1 isr=1 total=... bytes

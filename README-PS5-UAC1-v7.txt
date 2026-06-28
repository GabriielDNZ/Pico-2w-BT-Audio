Pico-2w-BT-Audio PS5 UAC1 v7

Base: speaker-only UAC1 build fixed from v4b.
Changes:
- CFG_TUD_AUDIO_FUNC_1_DESC_LEN fixed to 109 to match descriptor bytes.
- Speaker OUT only: TinyUSB has one AS interface to manage, closer to the original working USB audio path.
- Beep fallback restored from the first tone build. If it beeps, Bluetooth/A2DP is alive but USB OUT packets are still not being received.
- Extra TinyUSB OUT drain in audio_task plus pre-read callback.
- pico_btstack_flash_bank linked for persistent Bluetooth keys.
- App slot/MAC flash storage moved away from the BTstack flash-bank area.

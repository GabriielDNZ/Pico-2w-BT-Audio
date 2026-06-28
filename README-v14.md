# v14 - UAC2 original mainfix + memory

Base: `PicoW-usb2bt-audio-main` original UAC2.

Why this version exists:
- The uploaded working `Pico2W_USB_BT_0.9.uf2` was inspected and its USB descriptor matches the original UAC2 descriptor:
  - VID `0xCAFE`
  - PID `0x4010`
  - Device class `EF 02 01` (IAD)
  - Configuration length `0x0098`
  - Product strings `TinyUSB` / `TinyUSB BT`
  - SDK string `2.1.1`
- So this version does **not** change the UAC2 descriptor or the audio pipeline.
- The GitHub workflow is pinned to Pico SDK `2.1.1`, matching the working UF2.
- The app's manual flash storage was moved away from the last flash sector, so it does not wipe BTstack/UF2 data used for pairing persistence.

Test order:
1. Build in GitHub Actions.
2. Flash the generated UF2.
3. Pair the headset once.
4. Disconnect/reconnect USB and test whether it reconnects without pairing again.

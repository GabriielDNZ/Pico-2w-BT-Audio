V4 - UAC1 speaker-only test build

This version is based on the original audio-working A2DP pipeline, but changes USB to a simpler UAC1 playback-only device.
Goal: remove the microphone/duplex UAC1 descriptors from the previous PS5 build, because some TinyUSB/Pico SDK versions do not open the OUT endpoint correctly when the custom UAC1 headset descriptor has separate speaker and mic interfaces.

Changes:
- USB Audio Class 1.0, stereo 16-bit 48 kHz speaker OUT only.
- Product strings/VID/PID use a generic C-Media USB PnP Audio Device profile.
- Original Bluetooth/A2DP audio pipeline restored.
- No beep/test tone.
- GitHub workflow preserved.

If Windows/PS5 recognizes it and audio works, the previous problem was the duplex headset descriptor/mic path.
If it is recognized but still silent, USB OUT packets still are not reaching TinyUSB, so the next step is a raw RX diagnostic build.

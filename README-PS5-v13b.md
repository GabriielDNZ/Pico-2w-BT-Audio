# v13B - UAC2 PS5 PnP IAD class fix

Base: original UAC2 with working audio pipeline.

Changes:
- Keeps UAC2 audio pipeline original.
- Keeps C-Media/USB PnP identity from v12.
- Fixes USB device class back to Misc/Common/IAD, which is required when using IAD.
- Keeps pico_btstack_flash_bank for Bluetooth link-key persistence.
- Moves project manual flash storage away from final sectors used by BTstack.

Test this first. If it still connects/disconnects, test v13A original-ID fallback.

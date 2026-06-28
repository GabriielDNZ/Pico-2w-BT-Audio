# PicoW USB BT Audio - UAC2 PS5 PnP v12

Base: projeto UAC2 original, mantendo o pipeline de audio que funcionava.

Alteracoes:
- Mantem UAC2/original para preservar o audio.
- Troca identidade USB para perfil comum de USB PnP Audio Device (C-Media VID/PID).
- Mantem IAD do UAC2 na configuracao.
- Adiciona workflow GitHub Actions para Pico 2 W.
- Adiciona pico_btstack_flash_bank para tentar persistir chaves BT.
- Move armazenamento manual de slot/MAC para longe dos ultimos setores da flash.

Teste esperado:
- Se o PS5 reconhecer como headset/USB audio e tocar, usamos esta base.
- Se ainda nao aparecer no PS5, o problema e suporte/descriptor UAC2 no host e devemos fazer uma segunda variante UAC2 com microfone IN/silencio.

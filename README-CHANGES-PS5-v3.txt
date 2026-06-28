Pico-2w-BT-Audio-PS5-UAC1-audio-original-v3

Base:
- Mantém o descritor USB UAC1 PS5/Windows da versão tone-test.
- Mantém a pasta .github/workflows para compilar no GitHub Actions.
- Mantém a correção de flash para não usar a última página/setor.

Comparação aplicada:
- src/btstack/btstack_avdtp_source.c voltou para o arquivo original do PicoW-usb2bt-audio-main, onde o áudio USB -> Bluetooth funcionava.
- src/main.c voltou para o agendamento original do TinyUSB por timer de 500 us, que processa USB com mais frequência.
- O bipe de teste foi removido porque esse arquivo voltou ao original.
- CMakeLists.txt continua linkando pico_btstack_flash_bank para persistência do pareamento do BTstack.
- src/pico_w_led.c continua afastado do final da flash para não apagar link keys do Bluetooth.

Objetivo:
- Manter a enumeração UAC1 compatível com PS5.
- Recuperar o caminho de áudio funcional do projeto original.

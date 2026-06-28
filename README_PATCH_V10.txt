V10: UAC1 headset sem IAD, focado em abrir endpoint OUT no PS5/Windows.
Base: primeiro pacote do bipe/diagnostico.
Diagnostico por som:
- bipe continuo normal: host ainda nao selecionou alt1/OUT
- bipe pulsando/lento: endpoint abriu mas nao chegaram pacotes
- audio real: USB OUT chegou e foi para o Bluetooth

Tambem linka pico_btstack_flash_bank e move a area customizada de flash para nao pisar na area final usada pelo BTstack.

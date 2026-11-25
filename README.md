# FRANKTRON

Animatrônico interativo com ESP32, inspirado no Frankenstein.

O projeto nasceu da ideia de levar a lógica de um Tamagotchi para um rosto animatrônico. Dois displays OLED fazem os olhos do personagem, que olha para os lados, pisca sozinho, reage a quem se aproxima e pode ser controlado por uma página web via Wi-Fi.

Desenvolvido em colaboração com o Espaço Maker.
Página do projeto: [Protótipo Maker: os olhos de um animatrônico com dois displays](https://cmaker.com.br/projetos/hub/?r=projeto/prototipo-maker-os-olhos-de-um-animatronic-com-dois-displays-para-inovacao-pratica)

## Funcionalidades

- **Olhos sincronizados**: dois displays OLED desenhados juntos, sem atraso entre um olho e outro.
- **Movimentos**: olhar para a frente, para a esquerda e para a direita, ou manter os olhos fechados.
- **Piscadas automáticas** em intervalos aleatórios de 3 a 7 segundos.
- **Rotina**: ao ligar, o Franktron olha sozinho para a esquerda, a frente e a direita. Qualquer comando manual interrompe a rotina, e o botão ROTINA a liga de novo.
- **Sensor infravermelho**: quando algo chega perto, ele fecha um dos olhos (sorteado) por 2 segundos, como uma piscadela.
- **Interface web**: controle dos olhos, simulação do rosto em tempo real, logs do sistema e dos comandos e dados ao vivo (sinal Wi-Fi, memória livre, tempo de resposta, sensor IR, tempo ligado).
- **Registro de eventos**: o ESP32 guarda os últimos 25 eventos (Wi-Fi, comandos, sensor) e a página os exibe.

## Hardware

| Componente | Quantidade |
|---|---|
| ESP32 | 1 |
| Display OLED 128x64 I2C (SSD1306, endereço 0x3C) | 2 |
| Sensor de obstáculo infravermelho (LM393) | 1 |

### Ligações

Cada display fica num barramento I2C próprio, porque os dois usam o mesmo endereço.

| Ligação | Pino do ESP32 |
|---|---|
| OLED 1: SDA / SCL (`Wire`) | GPIO 21 / GPIO 22 |
| OLED 2: SDA / SCL (`Wire1`) | GPIO 19 / GPIO 18 |
| Sensor IR: OUT | GPIO 27 |

Os displays e o sensor são alimentados com 3,3 V e GND do ESP32.

## Como usar

1. Instale a [Arduino IDE](https://www.arduino.cc/en/software) e o suporte a placas ESP32 (Gerenciador de Placas → *esp32* da Espressif). Não precisa de bibliotecas extras.
2. Coloque `frankton_olhos_final.ino` numa pasta chamada `frankton_olhos_final` (a IDE pede isso e faz a mudança se você aceitar).
3. Altere o nome e a senha da sua rede Wi-Fi no início do código:
   ```cpp
   const char* ssid     = "SUA_REDE";
   const char* password = "SUA_SENHA";
   ```
4. Selecione a placa ESP32, a porta e envie o código.
5. Abra no navegador, na mesma rede, **http://franktron.local**. Se não funcionar, use o IP que aparece no Monitor Serial (115200 baud).

## Rotas HTTP

| Rota | O que faz |
|---|---|
| `/` | Página de controle |
| `/esquerda`, `/centro`, `/direita` | Move os olhos (e desliga a rotina) |
| `/piscar` | Mantém os olhos fechados até o próximo comando |
| `/rotina` | Liga a rotina de olhar para os lados |
| `/estado` | Estado atual em texto, separado por vírgulas (usado pela página a cada 0,3 s) |
| `/log` | Últimos eventos, um por linha: `id\|tempo\|tipo\|mensagem` |

Esquerda e direita são do ponto de vista do Franktron: para quem olha de frente, a pupila vai para o lado contrário.

## Tecnologias

- ESP32 e programação embarcada em C/C++
- Displays OLED e comunicação I2C
- HTML, CSS e JavaScript (a página fica gravada na memória flash do ESP32)
- HTTP, Wi-Fi e mDNS
- Máquinas de estados não bloqueantes (sem `delay` no `loop`)

## Resultados

Na etapa atual de validação, o protótipo passou em **10 de 10 testes funcionais**: displays, sincronização das animações, resposta do sensor e comunicação com a interface web.

## Próximos passos

- Speaker para reprodução de áudio
- Movimento da boca sincronizado com a fala
- Inteligência artificial para conversas e interações mais naturais

## Equipe

- Isabella Fogaça
- Matheus Friesen
- Rhyan Massayoshi
- Samara de Carvalho

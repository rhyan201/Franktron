#include <WiFi.h>
#include <Wire.h>

#define OLED_ADDR 0x3C // Endereço I2C padrão do display OLED 0.96"

// Credenciais do seu Wi-Fi
const char* ssid     = "Hollow";
const char* password = "0160code";

WiFiServer server(80);

// ================== ANIMAÇÃO AUTOMÁTICA ==================
// Ajuste os tempos aqui (em milissegundos)
const unsigned long TEMPO_PARADO      = 5000; // olho parado antes de piscar
const unsigned long TEMPO_PISCA       = 150;  // olho fechado durante a piscada
const unsigned long TEMPO_APOS_PISCA  = 3000; // espera entre a piscada e olhar para o lado
const unsigned long TEMPO_ESQUERDA    = 2000; // olhando para a esquerda
const unsigned long TEMPO_DIREITA     = 2000; // olhando para a direita

enum Fase { PARADO, PISCA_FECHA, PISCA_ABRE, ESQUERDA, DIREITA };
Fase fase = PARADO;
unsigned long faseInicio = 0;
bool cicloAtivo = true;

// ================== FONTE 5x7 (ASCII 32 a 90) ==================
const uint8_t fonte5x7[][5] = {
  {0x00, 0x00, 0x00, 0x00, 0x00}, // espaço (32)
  {0x00, 0x00, 0x5f, 0x00, 0x00}, // !
  {0x00, 0x07, 0x00, 0x07, 0x00}, // "
  {0x14, 0x7f, 0x14, 0x7f, 0x14}, // #
  {0x24, 0x2a, 0x7f, 0x2a, 0x12}, // $
  {0x23, 0x13, 0x08, 0x64, 0x62}, // %
  {0x36, 0x49, 0x55, 0x22, 0x50}, // &
  {0x00, 0x05, 0x03, 0x00, 0x00}, // '
  {0x00, 0x1c, 0x22, 0x41, 0x00}, // (
  {0x00, 0x41, 0x22, 0x1c, 0x00}, // )
  {0x14, 0x08, 0x3e, 0x08, 0x14}, // *
  {0x08, 0x08, 0x3e, 0x08, 0x08}, // +
  {0x00, 0x50, 0x30, 0x00, 0x00}, // ,
  {0x08, 0x08, 0x08, 0x08, 0x08}, // -
  {0x00, 0x60, 0x60, 0x00, 0x00}, // .
  {0x20, 0x10, 0x08, 0x04, 0x02}, // /
  {0x3e, 0x51, 0x49, 0x45, 0x3e}, // 0 (48)
  {0x00, 0x42, 0x7f, 0x40, 0x00}, // 1
  {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
  {0x21, 0x41, 0x45, 0x4b, 0x31}, // 3
  {0x18, 0x14, 0x12, 0x7f, 0x10}, // 4
  {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
  {0x3c, 0x4a, 0x49, 0x49, 0x30}, // 6
  {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
  {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
  {0x06, 0x49, 0x49, 0x29, 0x1e}, // 9
  {0x00, 0x36, 0x36, 0x00, 0x00}, // :
  {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
  {0x08, 0x14, 0x22, 0x41, 0x00}, // <
  {0x14, 0x14, 0x14, 0x14, 0x14}, // =
  {0x00, 0x41, 0x22, 0x14, 0x08}, // >
  {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
  {0x32, 0x49, 0x79, 0x41, 0x3e}, // @
  {0x7e, 0x11, 0x11, 0x11, 0x7e}, // A (65)
  {0x7f, 0x49, 0x49, 0x49, 0x36}, // B
  {0x3e, 0x41, 0x41, 0x41, 0x22}, // C
  {0x7f, 0x41, 0x41, 0x22, 0x1c}, // D
  {0x7f, 0x49, 0x49, 0x49, 0x41}, // E
  {0x7f, 0x09, 0x09, 0x09, 0x01}, // F
  {0x3e, 0x41, 0x49, 0x49, 0x7a}, // G
  {0x7f, 0x08, 0x08, 0x08, 0x7f}, // H
  {0x00, 0x41, 0x7f, 0x41, 0x00}, // I
  {0x20, 0x40, 0x41, 0x3f, 0x01}, // J
  {0x7f, 0x08, 0x14, 0x22, 0x41}, // K
  {0x7f, 0x40, 0x40, 0x40, 0x40}, // L
  {0x7f, 0x02, 0x0c, 0x02, 0x7f}, // M
  {0x7f, 0x04, 0x08, 0x10, 0x7f}, // N
  {0x3e, 0x41, 0x41, 0x41, 0x3e}, // O
  {0x7f, 0x09, 0x09, 0x09, 0x06}, // P
  {0x3e, 0x41, 0x51, 0x21, 0x5e}, // Q
  {0x7f, 0x09, 0x19, 0x29, 0x46}, // R
  {0x46, 0x49, 0x49, 0x49, 0x31}, // S
  {0x01, 0x01, 0x7f, 0x01, 0x01}, // T
  {0x3f, 0x40, 0x40, 0x40, 0x3f}, // U
  {0x1f, 0x20, 0x40, 0x20, 0x1f}, // V
  {0x3f, 0x40, 0x38, 0x40, 0x3f}, // W
  {0x63, 0x14, 0x08, 0x14, 0x63}, // X
  {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
  {0x61, 0x51, 0x49, 0x45, 0x43}  // Z (90)
};

// ================== OLED ==================
uint8_t framebuffer[1024]; // 128 colunas x 8 páginas

void enviarComando(uint8_t c) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00);
  Wire.write(c);
  Wire.endTransmission();
}

void inicializarOLED() {
  Wire.begin();
  Wire.setClock(400000);
  delay(10);
  enviarComando(0xAE);
  enviarComando(0xD5); enviarComando(0x80);
  enviarComando(0xA8); enviarComando(0x3F);
  enviarComando(0xD3); enviarComando(0x00);
  enviarComando(0x40);
  enviarComando(0x8D); enviarComando(0x14);
  enviarComando(0x20); enviarComando(0x00);
  enviarComando(0xA1);
  enviarComando(0xC8);
  enviarComando(0xDA); enviarComando(0x12);
  enviarComando(0x81); enviarComando(0xCF);
  enviarComando(0xD9); enviarComando(0xF1);
  enviarComando(0xDB); enviarComando(0x40);
  enviarComando(0xA4);
  enviarComando(0xA6);
  enviarComando(0xAF);
}

// estado: 0 = meio, 1 = esquerda, 2 = direita, 3 = fechado
void desenharOlho(int estado) {
  enviarComando(0x21); enviarComando(0); enviarComando(127);
  enviarComando(0x22); enviarComando(0); enviarComando(7);

  for (uint8_t pagina = 0; pagina < 8; pagina++) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);

    for (uint8_t coluna = 0; coluna < 128; coluna++) {
      uint8_t pixelByte = 0x00;

      if (pagina >= 2 && pagina <= 5) {
        if (coluna >= 44 && coluna <= 84) {

          if (estado == 3) {
            if (pagina == 3 && (coluna % 4 == 0 || coluna % 5 == 0)) pixelByte = 0x03;
          }
          else {
            if ((pagina == 2 && (coluna == 44 || coluna == 84)) || (pagina == 5 && (coluna == 44 || coluna == 84))) {
              pixelByte = 0x00;
            } else {
              pixelByte = 0xFF;

              int offsetPupila = 0;
              if (estado == 1) offsetPupila = -12;
              if (estado == 2) offsetPupila = 12;

              if (coluna >= (56 + offsetPupila) && coluna <= (72 + offsetPupila)) {
                if (pagina == 3 || pagina == 4) {
                  pixelByte = 0x00;
                }
              }
            }
          }
        }
      }

      Wire.write(pixelByte);

      if (coluna % 16 == 0) {
        Wire.endTransmission();
        Wire.beginTransmission(OLED_ADDR);
        Wire.write(0x40);
      }
    }
    Wire.endTransmission();
  }
}

// ================== TEXTO NO DISPLAY ==================
void enviarFramebuffer() {
  enviarComando(0x21); enviarComando(0); enviarComando(127);
  enviarComando(0x22); enviarComando(0); enviarComando(7);

  for (int i = 0; i < 1024; i += 16) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40);
    Wire.write(&framebuffer[i], 16);
    Wire.endTransmission();
  }
}

// Dobra verticalmente 4 bits em 8 bits (usado no texto grande)
uint8_t expandirNibble(uint8_t n) {
  uint8_t r = 0;
  for (int i = 0; i < 4; i++) {
    if (n & (1 << i)) r |= (3 << (i * 2));
  }
  return r;
}

// Recebe texto já limpo (só caracteres ASCII de 32 a 90)
// Até 40 caracteres: fonte grande (10 por linha, 4 linhas)
// Acima disso: fonte pequena (21 por linha, 8 linhas)
void mostrarTexto(const String& txt) {
  cicloAtivo = false; // texto na tela = ciclo pausado

  memset(framebuffer, 0, sizeof(framebuffer));

  int escala     = (txt.length() <= 40) ? 2 : 1;
  int larguraCel = 6 * escala;
  int porLinha   = 128 / larguraCel;
  int maxLinhas  = 8 / escala;

  for (unsigned int i = 0; i < txt.length(); i++) {
    int linha = i / porLinha;
    if (linha >= maxLinhas) break;

    int x0 = (i % porLinha) * larguraCel;
    uint8_t idx = txt[i] - 32;

    for (int k = 0; k < 5; k++) {
      uint8_t g = fonte5x7[idx][k];

      if (escala == 1) {
        framebuffer[linha * 128 + x0 + k] = g;
      } else {
        int pagina = linha * 2;
        int x = x0 + k * 2;
        uint8_t topo = expandirNibble(g & 0x0F);
        uint8_t base = expandirNibble(g >> 4);
        framebuffer[pagina * 128 + x]         = topo;
        framebuffer[pagina * 128 + x + 1]     = topo;
        framebuffer[(pagina + 1) * 128 + x]     = base;
        framebuffer[(pagina + 1) * 128 + x + 1] = base;
      }
    }
  }

  enviarFramebuffer();
}

// ================== TRATAMENTO DE TEXTO DA WEB ==================
// Troca '+' por espaço e converte %XX em bytes
String decodificarURL(const String& s) {
  String r = "";
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == '+') {
      r += ' ';
    } else if (c == '%' && i + 2 < s.length()) {
      char hex[3] = { s[i + 1], s[i + 2], 0 };
      r += (char)strtol(hex, NULL, 16);
      i += 2;
    } else {
      r += c;
    }
  }
  return r;
}

// Letras acentuadas (UTF-8, segundo byte após 0xC3) -> letra simples
char acentoParaLetra(uint8_t b) {
  if ((b >= 0x80 && b <= 0x85) || (b >= 0xA0 && b <= 0xA5)) return 'A';
  if (b == 0x87 || b == 0xA7) return 'C';
  if ((b >= 0x88 && b <= 0x8B) || (b >= 0xA8 && b <= 0xAB)) return 'E';
  if ((b >= 0x8C && b <= 0x8F) || (b >= 0xAC && b <= 0xAF)) return 'I';
  if ((b >= 0x92 && b <= 0x96) || (b >= 0xB2 && b <= 0xB6)) return 'O';
  if ((b >= 0x99 && b <= 0x9C) || (b >= 0xB9 && b <= 0xBC)) return 'U';
  return '?';
}

// Deixa o texto só com caracteres que a fonte tem (maiúsculas, números, símbolos)
String limparTexto(const String& bruto) {
  String r = "";
  for (unsigned int i = 0; i < bruto.length(); i++) {
    uint8_t b = (uint8_t)bruto[i];

    if (b == 0xC3 && i + 1 < bruto.length()) {
      r += acentoParaLetra((uint8_t)bruto[++i]);
      continue;
    }
    if (b >= 0x80) continue;  // outros bytes UTF-8 são ignorados
    if (b < 32) continue;     // caracteres de controle

    if (b >= 'a' && b <= 'z') b -= 32; // minúscula -> maiúscula
    if (b > 90) b = '?';               // símbolos que a fonte não tem

    r += (char)b;
  }
  return r;
}

// ================== CONTROLE DA ANIMAÇÃO ==================
// Muda de fase e desenha o olho correspondente
void mudarFase(Fase nova) {
  fase = nova;
  faseInicio = millis();

  switch (fase) {
    case PARADO:      desenharOlho(0); break;
    case PISCA_FECHA: desenharOlho(3); break;
    case PISCA_ABRE:  desenharOlho(0); break;
    case ESQUERDA:    desenharOlho(1); break;
    case DIREITA:     desenharOlho(2); break;
  }
}

// Avança o ciclo quando o tempo da fase acaba (só se o ciclo estiver ativo)
void atualizarAnimacao() {
  if (!cicloAtivo) return;

  unsigned long decorrido = millis() - faseInicio;

  switch (fase) {
    case PARADO:      if (decorrido >= TEMPO_PARADO)     mudarFase(PISCA_FECHA); break;
    case PISCA_FECHA: if (decorrido >= TEMPO_PISCA)      mudarFase(PISCA_ABRE);  break;
    case PISCA_ABRE:  if (decorrido >= TEMPO_APOS_PISCA) mudarFase(ESQUERDA);    break;
    case ESQUERDA:    if (decorrido >= TEMPO_ESQUERDA)   mudarFase(DIREITA);     break;
    case DIREITA:     if (decorrido >= TEMPO_DIREITA)    mudarFase(PARADO);      break;
  }
}

// Botões manuais: desenha o olho e PAUSA o ciclo até clicar em "Ativar ciclo"
void comandoManual(int estado) {
  cicloAtivo = false;
  desenharOlho(estado);
}

// Liga o ciclo automático do começo (olho parado no meio)
void ativarCiclo() {
  cicloAtivo = true;
  mudarFase(PARADO);
}

// ================== SERVIDOR WEB ==================
void enviarPagina(WiFiClient& client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset=utf-8");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  client.println("<title>Laboratorio Franktron</title>");
  client.println("<style>");
  client.println("body { font-family: 'Courier New', Courier, monospace; text-align: center; background-color: #121212; color: #00ff66; margin-top: 40px; }");
  client.println(".box { background: #1a1a1a; max-width: 380px; margin: 0 auto; padding: 30px; border: 2px solid #00ff66; border-radius: 4px; box-shadow: 0 0 20px rgba(0,255,102,0.2); }");
  client.println("h1 { color: #9d4edd; text-shadow: 2px 2px #000; font-size: 28px; letter-spacing: 2px; }");
  client.println(".btn { display: block; box-sizing: border-box; width: 85%; margin: 15px auto; padding: 14px; font-family: inherit; font-size: 16px; color: #fff; border: 1px solid #9d4edd; text-decoration: none; font-weight: bold; cursor: pointer; text-transform: uppercase; }");
  client.println(".b-normal { background-color: #240046; } .b-normal:hover { background-color: #3c096c; }");
  client.println(".b-furia { background-color: #5c0000; border-color: #ff0000; color: #ff3333; } .b-furia:hover { background-color: #800000; }");
  client.println(".b-desliga { background-color: #333333; border-color: #555555; color: #888; } .b-desliga:hover { background-color: #444444; }");
  client.println(".b-ciclo-on { background-color: #003d1a; border-color: #00ff66; color: #00ff66; } .b-ciclo-on:hover { background-color: #005c26; }");
  client.println(".b-ciclo-off { background-color: #3d3000; border-color: #ffcc00; color: #ffcc00; } .b-ciclo-off:hover { background-color: #5c4800; }");
  client.println("input[type=text] { box-sizing: border-box; width: 85%; padding: 12px; font-family: inherit; font-size: 16px; background: #000; color: #00ff66; border: 1px solid #00ff66; }");
  client.println("hr { border: 0; border-top: 1px solid #333; margin: 22px 0; }");
  client.println("</style></head><body>");

  client.println("<div class='box'>");
  client.println("<h1>FRANKTRON</h1>");
  client.println("<p>Controle o olho:</p>");

  // Status e botão único para ativar/desativar o ciclo
  if (cicloAtivo) {
    client.println("<p>Ciclo do olho: <b>ATIVO</b></p>");
    client.println("<a href='/ciclo_off' class='btn b-ciclo-off'>Desativar ciclo</a>");
  } else {
    client.println("<p>Ciclo automático: <b style='color:#ffcc00'>PAUSADO</b></p>");
    client.println("<a href='/ciclo_on' class='btn b-ciclo-on'>Ativar ciclo</a>");
  }

  client.println("<hr>");
  client.println("<a href='/viver' class='btn b-normal'>Abrir olho</a>");
  client.println("<a href='/furia_esq' class='btn b-furia'>Olhar para Esquerda</a>");
  client.println("<a href='/furia_dir' class='btn b-furia'>Olhar Direita</a>");
  client.println("<a href='/desativar' class='btn b-desliga'>Fechar olho</a>");

  client.println("<hr>");
  client.println("<p>Mostrar texto no display:</p>");
  client.println("<form action='/texto' method='get'>");
  client.println("<input type='text' name='msg' maxlength='100' placeholder='Digite o texto...'>");
  client.println("<button type='submit' class='btn b-normal'>Mostrar texto</button>");
  client.println("</form>");

  client.println("</div></body></html>");
}

// Responde com redirecionamento para "/" (evita repetir a ação ao atualizar a página)
void redirecionarParaInicio(WiFiClient& client) {
  client.println("HTTP/1.1 303 See Other");
  client.println("Location: /");
  client.println("Connection: close");
  client.println();
}

// requisicao = primeira linha do HTTP, ex: "GET /texto?msg=OLA HTTP/1.1"
void processarRequisicao(WiFiClient& client, const String& requisicao) {
  int ini = requisicao.indexOf(' ');
  int fim = requisicao.indexOf(' ', ini + 1);
  String caminho = (ini >= 0 && fim > ini) ? requisicao.substring(ini + 1, fim) : "/";

  bool acao = true;

  if      (caminho == "/viver")      comandoManual(0);
  else if (caminho == "/furia_esq")  comandoManual(1);
  else if (caminho == "/furia_dir")  comandoManual(2);
  else if (caminho == "/desativar")  comandoManual(3);
  else if (caminho == "/ciclo_on")   ativarCiclo();
  else if (caminho == "/ciclo_off")  cicloAtivo = false;
  else if (caminho.startsWith("/texto?msg=")) {
    String bruto = decodificarURL(caminho.substring(11));
    mostrarTexto(limparTexto(bruto));
  }
  else acao = false;

  if (acao) redirecionarParaInicio(client);
  else      enviarPagina(client);
}

// ================== SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  inicializarOLED();
  desenharOlho(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado.");
  Serial.println(WiFi.localIP());
  server.begin();

  cicloAtivo = true;
  mudarFase(PARADO); // inicia o ciclo automático
}

void loop() {
  atualizarAnimacao();

  WiFiClient client = server.available();
  if (!client) return;

  String linhaAtual = "";
  String requisicao = "";
  unsigned long inicio = millis();

  // Timeout de 2s para um cliente travado não congelar a animação
  while (client.connected() && millis() - inicio < 2000) {
    if (!client.available()) {
      delay(1);
      continue;
    }

    char c = client.read();

    if (c == '\n') {
      if (requisicao.length() == 0) requisicao = linhaAtual; // primeira linha

      if (linhaAtual.length() == 0) { // linha vazia = fim dos cabeçalhos
        processarRequisicao(client, requisicao);
        break;
      }
      linhaAtual = "";
    } else if (c != '\r') {
      linhaAtual += c;
    }
  }

  client.stop();
}

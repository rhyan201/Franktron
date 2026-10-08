#include <WiFi.h>
#include <Wire.h>

#define OLED_ADDR 0x3C

// Credenciais do seu Wi-Fi
const char* ssid     = "Hollow";
const char* password = "0160code";

WiFiServer server(80);

// Estado atual do olho: 0 = Normal/Acordado, 1 = Olhar Sinistro Esquerda, 2 = Olhar Sinistro Direita, 3 = Desativado/Costura
int estadoOlho = 0; 

void enviarComando(uint8_t c) {
  Wire.beginTransmission(OLED_ADDR);
  Wire.write(0x00); 
  Wire.write(c);
  Wire.endTransmission();
}

void inicializarOLED() {
  Wire.begin();
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

// Renderiza o olho do Franktron diretamente na memória do display
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
            // ESTADO 3: Desativado (Simula uma cicatriz/costura horizontal com traços alternados)
            if (pagina == 3 && (coluna % 4 == 0 || coluna % 5 == 0)) pixelByte = 0x03;
          } 
          else {
            // Desenha a estrutura principal do olho do Franktron
            if ((pagina == 2 && (coluna == 44 || coluna == 84)) || (pagina == 5 && (coluna == 44 || coluna == 84))) {
              pixelByte = 0x00;
            } else {
              pixelByte = 0xFF; 
              
              int offsetPupila = 0;
              if (estado == 1) offsetPupila = -12; // Direciona o olhar esquerdo
              if (estado == 2) offsetPupila = 12;  // Direciona o olhar direito

              // Renderiza a pupila do monstro
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

void setup() {
  Serial.begin(115200);
  inicializarOLED();
  
  // Inicia com o olhar padrão aberto
  desenharOlho(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado.");
  Serial.print("IP do Franktron: ");
  Serial.println(WiFi.localIP());
  
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Envia o cabeçalho com suporte a caracteres latinos
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            
            // Estilização Web inspirada no laboratório do Frankenstein
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
            client.println("<title>Laboratorio Franktron</title>");
            client.println("<style>");
            client.println("body { font-family: 'Courier New', Courier, monospace; text-align: center; background-color: #121212; color: #00ff66; margin-top: 40px; }");
            client.println(".box { background: #1a1a1a; max-width: 380px; margin: 0 auto; padding: 30px; border: 2px solid #00ff66; border-radius: 4px; box-shadow: 0 0 20px rgba(0,255,102,0.2); }");
            client.println("h1 { color: #9d4edd; text-shadow: 2px 2px #000; font-size: 28px; letter-spacing: 2px; }");
            client.println("p { color: #a0a0a0; }");
            client.println(".btn { display: block; width: 85%; margin: 15px auto; padding: 14px; font-size: 16px; color: #fff; border: 1px solid #9d4edd; text-decoration: none; font-weight: bold; cursor: pointer; text-transform: uppercase; }");
            client.println(".b-normal { background-color: #240046; border-color: #9d4edd; } .b-normal:hover { background-color: #3c096c; }");
            client.println(".b-furia { background-color: #5c0000; border-color: #ff0000; color: #ff3333; } .b-furia:hover { background-color: #800000; }");
            client.println(".b-desliga { background-color: #333333; border-color: #555555; color: #888; } .b-desliga:hover { background-color: #444444; }");
            client.println("</style></head><body>");
            
            client.println("<div class='box'>");
            client.println("<h1>FRANKTRON ⚡</h1>");
            client.println("<p>Controle os impulsos elétricos do monstro:</p>");
            
            // Botões temáticos de laboratório
            client.println("<a href='/viver' class='btn b-normal'>⚡ Dar Vida (Olhar à Frente)</a>");
            client.println("<a href='/furia_esq' class='btn b-furia'>👹 Fúria à Esquerda</a>");
            client.println("<a href='/furia_dir' class='btn b-furia'>👹 Fúria à Direita</a>");
            client.println("<a href='/desativar' class='btn b-desliga'>💤 Desativar Criatura</a>");
            
            client.println("</div></body></html>");
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }

        // Processa os novos comandos temáticos
        if (currentLine.endsWith("GET /viver")) {
          estadoOlho = 0;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /furia_esq")) {
          estadoOlho = 1;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /furia_dir")) {
          estadoOlho = 2;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /desativar")) {
          estadoOlho = 3;
          desenharOlho(estadoOlho);
        }
      }
    }
    client.stop();
  }
}

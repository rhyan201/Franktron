#include <WiFi.h>
#include <Wire.h>

#define OLED_ADDR 0x3C

// Credenciais do seu Wi-Fi
const char* ssid     = "Hollow";
const char* password = "0160code";

WiFiServer server(80);

// Estado atual do olho: 0 = Normal, 1 = Olhando Esquerda, 2 = Olhando Direita, 3 = Piscando
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

// Renderiza o olho diretamente na memória de pixels do display (128x64)
void desenharOlho(int estado) {
  enviarComando(0x21); enviarComando(0); enviarComando(127); 
  enviarComando(0x22); enviarComando(0); enviarComando(7);   

  // Um display 128x64 possui 8 "páginas" verticais de 8 pixels cada
  for (uint8_t pagina = 0; pagina < 8; pagina++) {
    Wire.beginTransmission(OLED_ADDR);
    Wire.write(0x40); 
    
    for (uint8_t coluna = 0; coluna < 128; coluna++) {
      uint8_t pixelByte = 0x00; // Apagado por padrão

      // Centraliza o olho verticalmente nas páginas 2, 3, 4 e 5 (altura do olho)
      if (pagina >= 2 && pagina <= 5) {
        
        // Define os limites horizontais da esclera (a parte branca/contorno do olho)
        if (coluna >= 44 && coluna <= 84) {
          
          if (estado == 3) { 
            // ESTADO 3: Piscando (Apenas uma linha fina horizontal no meio)
            if (pagina == 3) pixelByte = 0x01;
          } 
          else {
            // Desenha a pálpebra/contorno arredondado superior e inferior
            if ((pagina == 2 && (coluna == 44 || coluna == 84)) || (pagina == 5 && (coluna == 44 || coluna == 84))) {
              pixelByte = 0x00;
            } else {
              pixelByte = 0xFF; // Preenche a parte de dentro do olho
              
              // Define onde a PUPILA (vazia/preta) vai ficar baseada no estado
              int offsetPupila = 0;
              if (estado == 1) offsetPupila = -10; // Olhar para a esquerda
              if (estado == 2) offsetPupila = 10;  // Olhar para a direita

              // Desenha o bloco quadrado da pupila apagando os pixels internos
              if (coluna >= (58 + offsetPupila) && coluna <= (70 + offsetPupila)) {
                if (pagina == 3 || pagina == 4) {
                  pixelByte = 0x00; // Apaga o centro para criar a pupila
                }
              }
            }
          }
        }
      }
      
      Wire.write(pixelByte);
      
      // Previne estouro de buffer do barramento I2C do ESP32
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
  
  // Exibe o olho normal ao iniciar
  desenharOlho(0);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado.");
  Serial.print("IP do Beemo: ");
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
            // Resposta HTTP
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html; charset=utf-8");
            client.println("Connection: close");
            client.println();
            
            // Interface de controle estilizada
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
            client.println("<title>Expressões do Beemo</title>");
            client.println("<style>");
            client.println("body { font-family: Arial, sans-serif; text-align: center; background-color: #eef7ff; margin-top: 40px; }");
            client.println(".box { background: white; max-width: 360px; margin: 0 auto; padding: 25px; border-radius: 15px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); }");
            client.println("h1 { color: #2a9d8f; margin-bottom: 25px; }");
            client.println(".btn { display: block; width: 85%; margin: 12px auto; padding: 14px; font-size: 16px; color: white; border: none; border-radius: 30px; text-decoration: none; font-weight: bold; cursor: pointer; }");
            client.println(".b-centro { background-color: #2a9d8f; }");
            client.println(".b-lado { background-color: #f4a261; }");
            client.println(".b-pisca { background-color: #e76f51; }");
            client.println("</style></head><body>");
            
            client.println("<div class='box'>");
            client.println("<h1>Olho do Beemo 🤖</h1>");
            client.println("<p>Controle para onde o Beemo está olhando:</p>");
            
            // Botões que controlam os estados
            client.println("<a href='/centro' class='btn b-centro'>👀 Olhar para Frente</a>");
            client.println("<a href='/esquerda' class='btn b-lado'>⬅️ Olhar Esquerda</a>");
            client.println("<a href='/direita' class='btn b-lado'>➡️ Olhar Direita</a>");
            client.println("<a href='/piscar' class='btn b-pisca'>😑 Piscar Olho</a>");
            
            client.println("</div></body></html>");
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }

        // Processa as rotas para mudar o desenho do olho
        if (currentLine.endsWith("GET /centro")) {
          estadoOlho = 0;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /esquerda")) {
          estadoOlho = 1;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /direita")) {
          estadoOlho = 2;
          desenharOlho(estadoOlho);
        }
        if (currentLine.endsWith("GET /piscar")) {
          estadoOlho = 3;
          desenharOlho(estadoOlho);
        }
      }
    }
    client.stop();
  }
}

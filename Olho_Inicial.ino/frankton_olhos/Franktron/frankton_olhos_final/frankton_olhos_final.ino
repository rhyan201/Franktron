#include <WiFi.h>
#include <Wire.h>
#include <ESPmDNS.h>   // permite abrir a página por http://franktron.local

#define OLED_ADDR 0x3C

// Credenciais do seu Wi-Fi
const char* ssid     = "Hollow";
const char* password = "0160code";

// IP automático (DHCP): o roteador escolhe o IP.
// Para achar o ESP, use http://franktron.local ou veja o IP no Monitor Serial.

WiFiServer server(80);

int estadoOlho = 0; 

// ===== SENSOR INFRAVERMELHO (LM393) =====
#define PINO_IR 27              // OUT do sensor (LOW = tem algo na frente)
bool irAntes = false;           // leitura anterior (para pegar só a chegada do objeto)
bool irAgora = false;
int olhoFechado = -1;           // -1 = nenhum | 0 = olho do Wire | 1 = olho do Wire1
unsigned long inicioOlhoFechado = 0;
unsigned long totalPresencas = 0;

// Variáveis para controle do tempo da piscada automática (sem travar o código)
unsigned long tempoAnteriorPisca = 0;
unsigned long intervaloProximaPiscada = 4000; // Tempo inicial: 4 segundos
bool estaPiscandoPropositalmente = false;
unsigned long tempoInicioPiscada = 0;
unsigned long contadorPiscadas = 0; // usado pela página para piscar junto

// ===== ROTINA: olhar de um lado para o outro sozinho =====
// Já começa ligada quando o ESP32 inicia. Não tem botão de desligar: ela só para
// quando o usuário manda um comando manual, e pode ser ligada de novo em seguida.
bool rotinaAtiva = true;
const int passosRotina[4] = {1, 0, 2, 0};   // esquerda, frente, direita, frente
int passoRotina = 3;                        // o primeiro passo será o 0 (esquerda)
unsigned long tempoPassoRotina = 0;         // quando o passo atual começou
unsigned long duracaoPassoRotina = 2000;      // quanto tempo fica no passo atual

// ===== LOG (guarda os últimos 25 eventos na memória) =====
#define LOG_MAX 25
String logMsg[LOG_MAX];
char logTipo[LOG_MAX];        // S = sistema, U = usuário
unsigned long logTempo[LOG_MAX];
unsigned long logId = 0;          // id do último evento (sempre cresce)
unsigned long totalComandos = 0;

void addLog(String msg, char tipo) {
  logId++;
  int i = logId % LOG_MAX;
  logMsg[i] = msg;
  logTipo[i] = tipo;
  logTempo[i] = millis();
  Serial.println("[LOG] " + msg);
}

void addLog(String msg) { addLog(msg, 'S'); } // log do sistema

const char* nomeEstado(int e) {
  if (e == 1) return "ESQUERDA";
  if (e == 2) return "DIREITA";
  if (e == 3) return "FECHADO";
  return "FRENTE";
}

// Página web (guardada na memória flash)
const char PAGINA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Franktron</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:Verdana,Tahoma,"Segoe UI",sans-serif;background:#e8e0cc;color:#1d1a16;padding:16px}
.wrap{max-width:1100px;margin:0 auto;border:3px solid #1d1a16;background:#f4eedd}
header{display:flex;justify-content:space-between;align-items:baseline;padding:12px 16px;background:#1d1a16;color:#f4eedd}
header h1{font-family:Georgia,"Times New Roman",serif;font-size:26px;letter-spacing:4px}
header span{font-size:12px}
.info{display:grid;grid-template-columns:repeat(4,1fr);border-bottom:3px solid #1d1a16;font-size:12px}
.info div{padding:8px 14px}
.info div+div{border-left:2px dashed #1d1a16}
.info b{display:block;font-size:15px;margin-top:2px;word-break:break-all}
.ok{color:#2f6b2a}.bad{color:#c9452f}
.main{display:grid;grid-template-columns:3fr 2fr;gap:16px;padding:16px;align-items:start}
.box{border:3px solid #1d1a16;background:#fffdf5}
.box h2{font-size:12px;letter-spacing:2px;padding:7px 10px;border-bottom:2px dashed #1d1a16;display:flex;justify-content:space-between;align-items:center}
.box h2 label{font-weight:normal;cursor:pointer;letter-spacing:0}

/* rosto */
.screen{border:3px solid #1d1a16;background:#cfd8c4;height:440px;position:relative;overflow:hidden}
.screen small{position:absolute;left:10px;top:8px;font-size:11px}
.screen .estado{position:absolute;right:10px;top:8px;font-size:11px}
.head{position:absolute;left:50%;top:40px;margin-left:-90px;width:180px;height:210px;background:#7da35c;border:3px solid #1d1a16;border-radius:10px 10px 40px 40px;
transform:scale(1.65);transform-origin:50% 0}
.hair{position:absolute;left:-3px;top:-3px;width:180px;height:48px;background:#1d1a16;border-radius:10px 10px 0 0;
clip-path:polygon(0 0,100% 0,100% 100%,88% 70%,76% 100%,64% 72%,52% 100%,40% 70%,28% 100%,16% 72%,0 100%)}
.bolt{position:absolute;top:150px;width:22px;height:14px;background:#9a9a90;border:3px solid #1d1a16}
.bolt.l{left:-24px}.bolt.r{right:-24px}
.scar{position:absolute;left:96px;top:52px;width:40px;border-top:3px solid #1d1a16}
.scar i{position:absolute;top:-7px;width:3px;height:11px;background:#1d1a16}
.brow{position:absolute;top:66px;width:50px;height:7px;background:#1d1a16}
.brow.l{left:16px}.brow.r{right:16px}
.eye{position:absolute;top:80px;width:54px;height:38px;background:#000;border:3px solid #1d1a16}
.eye.l{left:14px}.eye.r{right:14px}
.white{position:absolute;left:5px;right:5px;top:6px;bottom:6px;background:#eef;transition:transform .1s}
.pupil{position:absolute;top:50%;left:50%;width:12px;height:12px;margin:-6px 0 0 -6px;background:#000;transition:transform .3s}
.eye.closed .white{transform:scaleY(.1)}
.eye.closed .pupil{opacity:0}
.nose{position:absolute;left:80px;top:126px;width:14px;height:22px;border:3px solid #1d1a16;border-top:0;border-radius:0 0 6px 6px}
.mouth{position:absolute;left:44px;top:170px;width:86px;border-top:4px solid #1d1a16}
.mouth i{position:absolute;top:-9px;width:3px;height:14px;background:#1d1a16}

/* botões */
.pad{display:grid;grid-template-columns:1fr 1fr 1fr;gap:10px;margin-top:16px}
.btn{display:block;padding:16px 6px;text-align:center;font:bold 13px Verdana,Tahoma,sans-serif;color:#1d1a16;text-decoration:none;
background:#f4eedd;border:3px solid #1d1a16;box-shadow:4px 4px 0 #1d1a16}
.btn:active{transform:translate(3px,3px);box-shadow:1px 1px 0 #1d1a16}
.btn.on{background:#1d1a16;color:#f4eedd}
.btn.close{grid-column:1/4;background:#c9452f;color:#fff}
.btn.close.on{background:#1d1a16}
.btn.rot{grid-column:1/4;background:#2b5d8a;color:#fff}
.btn.rot.on{background:#1d1a16}

/* logs */
.logs{display:flex;flex-direction:column;gap:16px}
.logs .box{display:flex;flex-direction:column}
.lista{list-style:none;height:250px;flex:none;font-family:Consolas,"Lucida Console",monospace;overflow-y:auto;font-size:12px;padding:4px 10px}
.lista li{padding:4px 0;border-bottom:1px dotted #b8ae98;display:flex;gap:8px}
.lista li span{color:#7a705c;flex-shrink:0}
.lista li.erro b{color:#c9452f}
.lista li.pisca b{color:#7a705c;font-weight:normal}
.lista li.voce b{color:#2b5d8a}
.vazio{color:#7a705c;font-style:italic}

/* dados */
.dados{margin:0 16px 16px}
.grid{display:grid;grid-template-columns:repeat(8,1fr);font-size:11px;text-align:center}
.grid div{padding:8px 4px}
.grid div+div{border-left:2px dashed #1d1a16}
.grid b{display:block;font-size:16px;margin-top:3px}
footer{border-top:2px dashed #1d1a16;padding:6px 16px;font-size:11px;display:flex;justify-content:space-between}

@media (max-width:820px){
  .main{grid-template-columns:1fr}
  .info{grid-template-columns:1fr 1fr}
  .info div:nth-child(3){border-left:0}
  .info div:nth-child(n+3){border-top:2px dashed #1d1a16}
  .grid{grid-template-columns:repeat(4,1fr)}
  .grid div:nth-child(5){border-left:0}
  .grid div:nth-child(n+5){border-top:2px dashed #1d1a16}
  .screen{height:340px}
  .head{transform:scale(1.2)}
  .pad{grid-template-columns:1fr}
  .btn.close,.btn.rot{grid-column:auto}
}
</style>
</head>
<body>
<div class="wrap">
  <header><h1>FRANKTRON</h1><span>experimento nº 01 · ESP32</span></header>

  <div class="info">
    <div>Endereço IP<b id="iIp">--</b></div>
    <div>Endereço MAC<b id="iMac">--</b></div>
    <div>Rede Wi-Fi<b id="iRede">--</b></div>
    <div>Status da conexão<b id="iCon">conectando...</b></div>
  </div>

  <div class="main">
    <div>
      <div class="screen">
        <small>VISOR // SIMULAÇÃO DO OLED</small>
        <span class="estado">estado: <b id="txt">FRENTE</b></span>
        <div class="head">
          <div class="hair"></div>
          <div class="bolt l"></div><div class="bolt r"></div>
          <div class="scar"><i style="left:6px"></i><i style="left:18px"></i><i style="left:30px"></i></div>
          <div class="brow l"></div><div class="brow r"></div>
          <div class="eye l"><div class="white"><div class="pupil"></div></div></div>
          <div class="eye r"><div class="white"><div class="pupil"></div></div></div>
          <div class="nose"></div>
          <div class="mouth"><i style="left:12px"></i><i style="left:40px"></i><i style="left:68px"></i></div>
        </div>
      </div>
      <div class="pad">
        <a href="/esquerda" data-s="1" class="btn">OLHAR PARA ESQUERDA</a>
        <a href="/centro" data-s="0" class="btn">OLHAR PARA FRENTE</a>
        <a href="/direita" data-s="2" class="btn">OLHAR PARA DIREITA</a>
        <a href="/piscar" data-s="3" class="btn close">MANTER OLHOS FECHADOS</a>
        <a href="/rotina" id="bRot" class="btn rot">ROTINA</a>
      </div>
    </div>

    <div class="logs">
      <div class="box">
        <h2>LOG DO SISTEMA <label><input type="checkbox" id="vPisca"> mostrar piscadas</label></h2>
        <ul class="lista" id="logSis"><li class="vazio">aguardando o ESP32...</li></ul>
      </div>
      <div class="box">
        <h2>LOG DE COMANDOS DOS USUÁRIOS</h2>
        <ul class="lista" id="logUsr"><li class="vazio">nenhum comando ainda</li></ul>
      </div>
    </div>
  </div>

  <div class="box dados">
    <h2>DADOS EM TEMPO REAL <span id="dAt">--</span></h2>
    <div class="grid">
      <div>estado do olho<b id="dEst">--</b></div>
      <div>ligado há<b id="dUp">--</b></div>
      <div>comandos<b id="dCmd">0</b></div>
      <div>piscadas<b id="dPis">0</b></div>
      <div>sinal Wi-Fi<b id="dRssi">--</b></div>
      <div>tempo de resposta<b id="dMs">--</b></div>
      <div>memória livre<b id="dHeap">--</b></div>
      <div>sensor IR<b id="dIr">--</b></div>
    </div>
  </div>
  <footer><span>atualiza a cada 0,3 s</span><span>sincronizado com o OLED</span></footer>
</div>

<script>
var eyes=document.querySelectorAll('.eye'),pupils=document.querySelectorAll('.pupil'),
btns=document.querySelectorAll('.btn'),txt=document.getElementById('txt'),
nomes=['FRENTE','ESQUERDA','DIREITA','FECHADO'],offs=[0,12,-12],estado=0,olhoIR=-1;
function $(i){return document.getElementById(i)}

function render(s,pisca){
  eyes.forEach(function(e,i){e.classList.toggle('closed',s==3||pisca||i==olhoIR)});
  if(s!=3)pupils.forEach(function(p){p.style.transform='translateX('+offs[s]+'px)'});
}
function setEstado(s){
  estado=s;render(s,false);txt.textContent=nomes[s];$('dEst').textContent=nomes[s];
  btns.forEach(function(b){if(b.dataset.s!=null)b.classList.toggle('on',b.dataset.s==s)});
}
/* botão da rotina: fica marcado enquanto ela está ligada no ESP */
function setRotina(on){
  var b=$('bRot');b.classList.toggle('on',on);
  b.textContent='ROTINA';
}

/* ===== LOGS ===== */
var ultimaPiscada=-1,ultimoLog=0,online=null,uptime=0,lendoLog=false;
function hora(){return new Date().toTimeString().slice(0,8)}
function dur(ms){var t=Math.floor(ms/1000),h=Math.floor(t/3600),m=Math.floor(t%3600/60),x=t%60;
  return (h?h+'h':'')+('0'+m).slice(-2)+'m'+('0'+x).slice(-2)+'s'}
function log(lista,tipo,quando,msg){
  var ul=$(lista),v=ul.querySelector('.vazio');if(v)ul.removeChild(v);
  var li=document.createElement('li');li.className=tipo;
  li.innerHTML='<span>'+quando+'</span><b></b>';li.lastChild.textContent=msg;
  ul.insertBefore(li,ul.firstChild);
  while(ul.children.length>100)ul.removeChild(ul.lastChild);
}
/* eventos guardados no ESP — linha: id|tempo|tipo|mensagem (tipo S = sistema, U = usuário) */
function lerLogESP(){
  if(lendoLog)return;lendoLog=true;
  fetch('/log').then(function(r){return r.text()}).then(function(t){
    t.trim().split('\n').forEach(function(l){
      var p=l.split('|');if(p.length<4)return;
      var id=+p[0];if(id<=ultimoLog)return;ultimoLog=id;
      var msg=p.slice(3).join('|'),q='+'+dur(+p[1]);
      if(p[2]=='U')log('logUsr','',q,msg);
      else log('logSis',(msg.indexOf('ERRO')==0||msg.indexOf('caiu')>0)?'erro':'',q,msg);
    });
  }).catch(function(){}).finally(function(){lendoLog=false});
}
function conexao(ok){
  if(ok===online)return;
  if(online!==null)log('logSis',ok?'':'erro',hora(),ok?'Página reconectou ao ESP32':'ESP32 sem resposta');
  online=ok;var c=$('iCon');c.textContent=ok?'CONECTADO':'SEM RESPOSTA';c.className=ok?'ok':'bad';
}
/* resposta de /estado: estado,piscadas,uptime,ultimoLog,comandos,rssi,ip,mac,heap,rede */
function aplicar(t,ms){
  var p=t.trim().split(','),s=+p[0],n=+p[1];
  if(p.length>=10){
    if(+p[2]<uptime){ultimoLog=0;log('logSis','erro',hora(),'ESP32 reiniciou');}
    uptime=+p[2];
    $('dUp').textContent=dur(uptime);$('dCmd').textContent=p[4];$('dPis').textContent=n;
    var r=+p[5];$('dRssi').textContent=r+' dBm';
    $('dRssi').className=r>-60?'ok':(r<-75?'bad':'');
    $('iIp').textContent=p[6];$('iMac').textContent=p[7];$('iRede').textContent=p[9];
    $('dHeap').textContent=Math.round(+p[8]/1024)+' KB';
    $('dAt').textContent='última leitura '+hora();
    if(p.length>=13){
      var o=+p[10];if(o!=olhoIR){olhoIR=o;render(estado,false);}
      $('dIr').textContent=p[11]=='1'?'DETECTANDO':'livre ('+p[12]+')';
      $('dIr').className=p[11]=='1'?'bad':'';
    }
    if(p.length>=14)setRotina(p[13]=='1');
    if(+p[3]>ultimoLog)lerLogESP();
  }
  if(ms!=null)$('dMs').textContent=ms+' ms';
  if(s!=estado)setEstado(s);
  if(ultimaPiscada>=0&&n!=ultimaPiscada&&estado!=3){
    render(estado,true);setTimeout(function(){render(estado,false)},150);
    if($('vPisca').checked)log('logSis','pisca',hora(),'Piscada automática (#'+n+')');
  }
  ultimaPiscada=n;
}
function sincronizar(){
  var t0=Date.now();
  fetch('/estado').then(function(r){return r.text()}).then(function(t){conexao(true);aplicar(t,Date.now()-t0)})
  .catch(function(){conexao(false)}).finally(function(){setTimeout(sincronizar,300)});
}
setEstado(0);sincronizar();

btns.forEach(function(b){
  b.addEventListener('click',function(ev){
    ev.preventDefault();
    var h=b.getAttribute('href'),olho=b.dataset.s!=null,s=+b.dataset.s,t0=Date.now();
    if(olho){setEstado(s);setRotina(false);}else setRotina(true);
    fetch(h).then(function(r){return r.text()}).then(function(t){aplicar(t,Date.now()-t0)})
    .catch(function(){log('logSis','erro',hora(),'Falha ao enviar o comando '+(olho?nomes[s]:'ROTINA'))});
  });
});
</script>
</body>
</html>
)rawliteral";

void enviarComando(TwoWire &portaI2C, uint8_t c) {
  portaI2C.beginTransmission(OLED_ADDR);
  portaI2C.write(0x00); 
  portaI2C.write(c);
  portaI2C.endTransmission();
}

void inicializarOLED(TwoWire &portaI2C) {
  enviarComando(portaI2C, 0xAE); 
  enviarComando(portaI2C, 0xD5); enviarComando(portaI2C, 0x80);
  enviarComando(portaI2C, 0xA8); enviarComando(portaI2C, 0x3F); 
  enviarComando(portaI2C, 0xD3); enviarComando(portaI2C, 0x00);
  enviarComando(portaI2C, 0x40); 
  enviarComando(portaI2C, 0x8D); enviarComando(portaI2C, 0x14); 
  enviarComando(portaI2C, 0x20); enviarComando(portaI2C, 0x00); 
  enviarComando(portaI2C, 0xA1); 
  enviarComando(portaI2C, 0xC8); 
  enviarComando(portaI2C, 0xDA); enviarComando(portaI2C, 0x12);
  enviarComando(portaI2C, 0x81); enviarComando(portaI2C, 0xCF);
  enviarComando(portaI2C, 0xD9); enviarComando(portaI2C, 0xF1);
  enviarComando(portaI2C, 0xDB); enviarComando(portaI2C, 0x40);
  enviarComando(portaI2C, 0xA4); 
  enviarComando(portaI2C, 0xA6); 
  enviarComando(portaI2C, 0xAF); 
}

// Apaga a tela inteira (usado só uma vez, ao ligar)
void limparTela(TwoWire &portaI2C) {
  enviarComando(portaI2C, 0x21); enviarComando(portaI2C, 0); enviarComando(portaI2C, 127);
  enviarComando(portaI2C, 0x22); enviarComando(portaI2C, 0); enviarComando(portaI2C, 7);
  for (int bloco = 0; bloco < 64; bloco++) {   // 64 blocos de 16 bytes = 1024 bytes
    portaI2C.beginTransmission(OLED_ADDR);
    portaI2C.write(0x40);
    for (uint8_t i = 0; i < 16; i++) portaI2C.write((uint8_t)0x00);
    portaI2C.endTransmission();
  }
}

// Calcula o byte de um ponto do olho (páginas 2 a 5 da tela)
uint8_t byteDoOlho(int estado, uint8_t pagina, uint8_t coluna) {
  if (coluna < 44 || coluna > 84) return 0x00;

  if (estado == 3) return (pagina == 3) ? 0x01 : 0x00;   // olho fechado = só uma linha

  if ((pagina == 2 || pagina == 5) && (coluna == 44 || coluna == 84)) return 0x00; // cantos

  // Esquerda e direita são do ponto de vista do Frank (como se os olhos fossem seus).
  // Para quem olha o Frank de frente, a pupila vai para o lado contrário.
  int offsetPupila = 0;
  if (estado == 1) offsetPupila = 10;   // esquerda do Frank = direita de quem olha
  if (estado == 2) offsetPupila = -10;  // direita do Frank = esquerda de quem olha

  if ((pagina == 3 || pagina == 4) &&
      coluna >= (58 + offsetPupila) && coluna <= (70 + offsetPupila)) return 0x00; // pupila

  return 0xFF;
}

// Desenha os DOIS olhos ao mesmo tempo, alternando pedacinhos de 16 bytes entre
// as duas telas. Assim os olhos mudam juntos (antes um só começava depois que o
// outro terminava, e por isso ficavam fora de sincronia).
// Só as páginas 2 a 5 são redesenhadas: o resto da tela fica sempre apagado.
void desenharOlhos(int estadoEsq, int estadoDir) {
  TwoWire* portas[2] = { &Wire, &Wire1 };
  int estados[2] = { estadoEsq, estadoDir };

  for (uint8_t o = 0; o < 2; o++) {
    enviarComando(*portas[o], 0x21); enviarComando(*portas[o], 0); enviarComando(*portas[o], 127);
    enviarComando(*portas[o], 0x22); enviarComando(*portas[o], 2); enviarComando(*portas[o], 5);
  }

  for (uint8_t pagina = 2; pagina <= 5; pagina++) {
    for (uint8_t inicio = 0; inicio < 128; inicio += 16) {
      for (uint8_t o = 0; o < 2; o++) {
        portas[o]->beginTransmission(OLED_ADDR);
        portas[o]->write(0x40);
        for (uint8_t coluna = inicio; coluna < inicio + 16; coluna++) {
          portas[o]->write(byteDoOlho(estados[o], pagina, coluna));
        }
        portas[o]->endTransmission();
      }
    }
  }
}

// Desenha os dois olhos, mantendo fechado o olho escolhido pelo sensor
void atualizarOlhos(int estado) {
  desenharOlhos(olhoFechado == 0 ? 3 : estado,
                olhoFechado == 1 ? 3 : estado);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(PINO_IR, INPUT_PULLUP); // evita leituras falsas com fio solto
  randomSeed(esp_random());
  addLog("ESP32 ligado");

  // Inicializa Displays
  // 400 kHz: desenha 4x mais rápido que o padrão (100 kHz)
  Wire.begin(21, 22, 400000);
  inicializarOLED(Wire);
  Wire1.begin(19, 18, 400000);
  inicializarOLED(Wire1);
  limparTela(Wire);
  limparTela(Wire1);

  atualizarOlhos(0);
  addLog("Displays OLED iniciados");

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);        // desliga a economia de energia do Wi-Fi (deixa a página bem mais rápida)
  WiFi.setAutoReconnect(true);

  addLog(String("Conectando na rede ") + ssid + "...");
  unsigned long inicioWifi = millis();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWi-Fi Conectado.");
  Serial.print("IP DO FRANKTRON: ");
  Serial.println(WiFi.localIP());
  
  addLog("Wi-Fi conectado em " + String((millis() - inicioWifi) / 1000.0, 1) + "s");
  addLog("IP " + WiFi.localIP().toString() + " | MAC " + WiFi.macAddress());
  addLog("Sinal do Wi-Fi: " + String(WiFi.RSSI()) + " dBm");
  if (MDNS.begin("franktron")) {
    MDNS.addService("http", "tcp", 80);
    addLog("Endereço fácil: http://franktron.local");
  }
  server.begin();
  addLog("Servidor web pronto na porta 80");
  addLog("Rotina de olhar para os lados ativa");
  tempoPassoRotina = millis(); // a rotina começa 2s depois de tudo pronto
}

bool wifiOk = true;

void loop() {
  unsigned long tempoAtual = millis();

  // Registra quedas e retornos do Wi-Fi
  if (WiFi.status() != WL_CONNECTED && wifiOk) { wifiOk = false; addLog("Wi-Fi caiu, tentando reconectar"); }
  if (WiFi.status() == WL_CONNECTED && !wifiOk) { wifiOk = true; addLog("Wi-Fi reconectado"); }

  // SENSOR IR: quando algo chega na frente, fecha um olho aleatório por 2s
  // só considera presença se o sensor ficar 150ms seguidos detectando (filtro de oscilação)
  static unsigned long inicioLeitura = 0;
  bool leitura = (digitalRead(PINO_IR) == LOW);
  if (!leitura) inicioLeitura = tempoAtual;
  irAgora = leitura && (tempoAtual - inicioLeitura >= 150);
  if (irAgora && !irAntes && olhoFechado < 0 && estadoOlho != 3 && !estaPiscandoPropositalmente) {
    olhoFechado = random(0, 2);
    inicioOlhoFechado = tempoAtual;
    totalPresencas++;
    atualizarOlhos(estadoOlho);
    addLog(String("Presença detectada: fechou o olho ") + (olhoFechado == 0 ? "DIREITO" : "ESQUERDO") + " por 2s");
  }
  irAntes = irAgora;

  if (olhoFechado >= 0 && tempoAtual - inicioOlhoFechado >= 2000) {
    olhoFechado = -1;
    atualizarOlhos(estadoOlho); // reabre
    // Recomeça a contagem da piscada automática: sem isso ela disparava
    // logo em seguida (o tempo já tinha vencido durante os 2s)
    tempoAnteriorPisca = millis();
    intervaloProximaPiscada = random(3000, 7000);
  }

  // MÁGICA DA PISCADA AUTOMÁTICA (Mecanismo não-bloqueante)
  if (!estaPiscandoPropositalmente && estadoOlho != 3 && olhoFechado < 0) {
    if (millis() - tempoAnteriorPisca >= intervaloProximaPiscada) {
      desenharOlhos(3, 3); // Fecha os dois olhos juntos (pisca)
      estaPiscandoPropositalmente = true;
      contadorPiscadas++;
      tempoInicioPiscada = millis(); // conta a partir de quando os olhos já estão fechados
    }
  }

  // Após 150 milissegundos piscando, reabre os olhos para o estado em que estava antes
  if (estaPiscandoPropositalmente) {
    if (millis() - tempoInicioPiscada >= 150) {
      atualizarOlhos(estadoOlho); // Reabre os olhos
      estaPiscandoPropositalmente = false;
      tempoAnteriorPisca = millis();
      intervaloProximaPiscada = random(3000, 7000); // Sorteia o tempo para a próxima piscada (entre 3 e 7 segundos)
    }
  }

  // ROTINA: olha esquerda -> frente -> direita -> frente, sem parar
  // (espera a piscada terminar para não reabrir os olhos antes da hora)
  if (rotinaAtiva && !estaPiscandoPropositalmente &&
      millis() - tempoPassoRotina >= duracaoPassoRotina) {
    passoRotina = (passoRotina + 1) % 4;
    estadoOlho = passosRotina[passoRotina];
    tempoPassoRotina = millis();
    // olhando para o lado: 1 a 2s | olhando para frente: 2 a 4s
    duracaoPassoRotina = (estadoOlho == 0) ? random(2000, 4000) : random(1000, 2000);
    atualizarOlhos(estadoOlho);
  }

  WiFiClient client = server.available();

  if (client) {
    String currentLine = "";
    bool respostaTexto = false; // true = responde só o status em texto
    bool respostaLog = false;   // true = responde a lista de eventos
    String ipCliente = client.remoteIP().toString();
    unsigned long inicioReq = millis();
    while (client.connected() && millis() - inicioReq < 1000) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            if (respostaLog) {
              client.println("Content-type:text/plain; charset=utf-8");
              client.println("Cache-Control: no-store");
              client.println("Connection: close");
              client.println();
              // uma linha por evento: id|tempo(ms)|mensagem
              unsigned long primeiro = (logId > LOG_MAX) ? logId - LOG_MAX + 1 : 1;
              for (unsigned long id = primeiro; id <= logId; id++) {
                int i = id % LOG_MAX;
                client.print(String(id) + "|" + logTempo[i] + "|" + logTipo[i] + "|" + logMsg[i] + "\n");
              }
            } else if (respostaTexto) {
              client.println("Content-type:text/plain");
              client.println("Cache-Control: no-store");
              client.println("Connection: close");
              client.println();
              // estado,piscadas,uptime(ms),ultimoIdLog,comandos,rssi,ip,mac,memoriaLivre,rede,olhoFechado,sensor,presencas
              client.print(String(estadoOlho) + "," + contadorPiscadas + "," + millis() + "," +
                           logId + "," + totalComandos + "," + WiFi.RSSI() + "," +
                           WiFi.localIP().toString() + "," + WiFi.macAddress() + "," +
                           ESP.getFreeHeap() + "," + ssid + "," +
                           olhoFechado + "," + (irAgora ? 1 : 0) + "," + totalPresencas + "," +
                           (rotinaAtiva ? 1 : 0));
            } else {
              client.println("Content-type:text/html; charset=utf-8");
              client.println("Connection: close");
              client.println();
              client.print(PAGINA_HTML);
            }
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }

        if (currentLine.endsWith("GET /estado")) respostaTexto = true;
        if (currentLine.endsWith("GET /log")) respostaLog = true;
        if (currentLine.endsWith("GET /rotina")) {
          respostaTexto = true;
          totalComandos++;
          if (!rotinaAtiva) {
            rotinaAtiva = true;
            passoRotina = 3;          // o próximo passo é o 0 (esquerda)
            duracaoPassoRotina = 0;   // começa já
            if (estadoOlho == 3) estadoOlho = 0; // se estava de olhos fechados, abre
          }
          addLog("ROTINA ativada por " + ipCliente, 'U');
        }
        if (currentLine.endsWith("GET /centro")) {
          respostaTexto = true;
          rotinaAtiva = false; // comando manual interrompe a rotina
          estadoOlho = 0;
          totalComandos++;
          addLog(String(nomeEstado(0)) + " enviado por " + ipCliente, 'U');
          atualizarOlhos(estadoOlho);
        }
        if (currentLine.endsWith("GET /esquerda")) {
          respostaTexto = true;
          rotinaAtiva = false;
          estadoOlho = 1;
          totalComandos++;
          addLog(String(nomeEstado(1)) + " enviado por " + ipCliente, 'U');
          atualizarOlhos(estadoOlho);
        }
        if (currentLine.endsWith("GET /direita")) {
          respostaTexto = true;
          rotinaAtiva = false;
          estadoOlho = 2;
          totalComandos++;
          addLog(String(nomeEstado(2)) + " enviado por " + ipCliente, 'U');
          atualizarOlhos(estadoOlho);
        }
        if (currentLine.endsWith("GET /piscar")) {
          respostaTexto = true;
          rotinaAtiva = false;
          estadoOlho = 3;
          totalComandos++;
          addLog(String(nomeEstado(3)) + " enviado por " + ipCliente, 'U'); // Mantém fechado até dar outro comando
          atualizarOlhos(estadoOlho);
        }
      }
    }
    client.stop();
  }
}

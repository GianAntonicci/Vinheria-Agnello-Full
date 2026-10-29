//Autor: Fábio Henrique Cabrini
//Resumo: Esse programa possibilita ligar e desligar o led onboard, além de mandar o status para o Broker MQTT possibilitando o Helix saber
//se o led está ligado ou desligado.
//Revisões:
//Rev1: 26-08-2023 Código portado para o ESP32 e para realizar a leitura de luminosidade e publicar o valor em um tópico aprorpiado do broker 
//Autor Rev1: Lucas Demetrius Augusto 
//Rev2: 28-08-2023 Ajustes para o funcionamento no FIWARE Descomplicado
//Autor Rev2: Fábio Henrique Cabrini
//Rev3: 1-11-2023 Refinamento do código e ajustes para o funcionamento no FIWARE Descomplicado
//Autor Rev3: Fábio Henrique Cabrini
//Rev4: 13-09-2026 Ajustes para funcionamento do fiware em hardware físico
//Autor Rev4: Gianluca Antonicci
//Rev5: 14-09-2026 Ajuste para simulação no Wokwi
//Autor Rev5: Gianluca Antonicci
//Rev6: 16-09-2026 Alteração de conteúdo do display
//Autor Rev6: Gianluca Antonicci
//Rev7: 25-09-2026 Alteração de display
//Autor Rev7: Gianluca Antonicci
//Rev8: 01-10-2026 Adicionado sensor DHT22
//Autor Rev8: Gianluca Antonicci
//Rev9: 02-10-2026 Adicionando buzzer
//Autor Rev9: Gianluca Antonicci
//Rev10: 04-10-2026 Triggers de temperatura, umidade e luminosidade com LED piscando, alerta sonoro por tipo de anomalia e modo do LED (auto/manual) controlável pelo FIWARE
//Autor Rev10: Gianluca Antonicci
//Rev11: 04-10-2026 Pinos ajustados para o ESP32-S3 (LED RGB, I2C do display e luminosidade)
//Autor Rev11: Gianluca Antonicci
//Rev12: 06-10-2026 Display OLED SSD1306 trocado pelo TFT touch 2.8" 240x320 (ILI9341 + XPT2046), com backlight no pino 18
//Autor Rev12: Gianluca Antonicci
//Rev13: 08-10-2026 Suporte ao touch capacitivo FT6206 (I2C, SDA 8 / SCL 9) do ILI9341 do Wokwi, escolhido pelo #define WOKWI
//Autor Rev13: Gianluca Antonicci
//Rev15: 11-10-2026 Removido o suporte ao Wokwi; código só para a placa física (touch XPT2046)
//Autor Rev15: Gianluca Antonicci
//Rev16: 15-10-2026 Menu principal (layout do Lopaka adaptado para a Adafruit ILI9341): temperatura, luminosidade e umidade em tempo real, com ícones animados
//Autor Rev16: Gianluca Antonicci
//Rev19: 15-10-2026 Partida mais rápida: removidas as esperas fixas antes de conectar e antes do menu; telas de conectado ficam 1 s
//Autor Rev19: Gianluca Antonicci
//Rev21: 15-10-2026 Tela de configurações (layout do Lopaka) aberta pela engrenagem: tema claro/escuro e buzzer mudo, salvos na memória flash; toque mapeado para pixels da tela
//Autor Rev21: Gianluca Antonicci
//Rev22: 15-10-2026 Leituras inválidas do touch (4095, chip sem responder) são ignoradas
//Autor Rev22: Gianluca Antonicci
//Rev23: 16-10-2026 Touch XPT2046 num barramento SPI próprio (HSPI: T_CLK 39, T_DO 40, T_DIN 41), separado do display
//Autor Rev23: Gianluca Antonicci
//Rev24: 16-10-2026 Leitura do XPT2046 feita direto no código (a biblioteca instalada não aceita outro SPI)
//Autor Rev24: Gianluca Antonicci
//Rev25: 16-10-2026 SPI do touch feito por software (bit-bang) nos mesmos pinos, para descartar o periférico HSPI
//Autor Rev25: Gianluca Antonicci
//Rev26: 16-10-2026 Diagnóstico do touch na partida (nível do T_DO e leitura do sensor de temperatura interno do XPT2046)
//Autor Rev26: Gianluca Antonicci
//Rev27: 16-10-2026 Display trocado pelo ILI9488 de 4" (480x320) com a biblioteca Arduino_GFX; telas de 320x240 centralizadas com deslocamento OX/OY
//Autor Rev27: Gianluca Antonicci
//Rev28: 16-10-2026 T_CS do touch passado do GPIO 16 para o 21 (livre desde que o MISO do display saiu)
//Autor Rev28: Gianluca Antonicci
//Rev29: 16-10-2026 Volta ao display de 2,8" (ILI9341, Adafruit GFX); mantidos o touch por software e o T_CS no 21
//Autor Rev29: Gianluca Antonicci
//Rev30: 16-10-2026 MISO do SPI do display num pino livre (47): com -1 o core usava o padrão do S3 (GPIO 13), o mesmo do SCK, e a tela ficava branca
//Autor Rev30: Gianluca Antonicci
//Rev31: 16-10-2026 Eixos horizontal e vertical do touch invertidos para bater com a rotação 3 do display
//Autor Rev31: Gianluca Antonicci
//Rev32: 16-10-2026 Tela de WiFi/MQTT (layout do Lopaka) com o status das conexões ao vivo; reconexão do Wi-Fi e do MQTT sem travar o loop
//Autor Rev32: Gianluca Antonicci
//Rev33: 16-10-2026 Seta da tela de configurações passa a usar image_arrow.h (a do WiFi continua com image_arrow_right_bits.h)
//Autor Rev33: Gianluca Antonicci
//Rev34: 22-10-2026 Alertas do atributo alert separados por vírgula: o ";" é proibido no Orion e a atualização de state/ledMode/alert era recusada quando havia mais de uma anomalia
//Autor Rev34: Gianluca Antonicci
//Rev35: 24-10-2026 Tela do QR code do dashboard web (layout do Lopaka), aberta pela seta à esquerda do horário no menu, com "< Voltar" para o menu
//Autor Rev35: Gianluca Antonicci
//Rev36: 28-10-2026 Portal de configuração: o ESP32 abre um Access Point com página HTTP para cadastrar Wi-Fi e broker MQTT, salvos na memória flash
//Autor Rev36: Gianluca Antonicci

// Bibliotecas de rede, periféricos, sensores e persistência usadas pela placa física.
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <DHT.h>
#include <Preferences.h>
#include <Fonts/FreeMonoBold12pt7b.h>

// Recursos gráficos monocromáticos armazenados em flash para as telas e animações.
#include "down_arrow_32_32_28f.h"
#include "gear_32_32_28f.h"
#include "sun_weather_32_32_28f.h"
#include "temperature_weather_32_32_28f.h"
#include "windy_weather_32_32_28f.h"
#include "image_arrow_right_bits.h"
#include "image_arrow.h"
#include "mute_32_32_28f.h"
#include "toggle_32_32_28f.h"
#include "blushing_48_48_28f.h"
#include "image_qrcode_bits.h"

// Declarações antecipadas para funções chamadas antes de suas definições.
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT();
void EnviaEstadoOutputMQTT();
void handleLuminosity();
void reconnectMQTT();
void avaliaTriggers();
void handleLED();
void handleBuzzer();
void handleDisplay();
void handleDisplayMenu();
void desenhaMenu();
void handleTouch();
void handlePortal();
void abrePortal(bool fechaAoConectar);
void fechaPortal();

// Pinagem do TFT SPI, controlador de toque, DHT22 e iluminação do display.
#define TFT_SCK   13
#define TFT_MOSI  14
#define TFT_CS    5
#define TFT_DC    6
#define TFT_RST   7
#define TFT_LED   18
#define TFT_MISO  47
#define TOUCH_CLK 39
#define TOUCH_DO  40
#define TOUCH_DIN 41
#define TOUCH_CS  21
#define TOUCH_IRQ 17
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

Adafruit_ILI9341 display(TFT_CS, TFT_DC, TFT_RST);

// Deslocamentos comuns do layout de 320 x 240 pixels.
const int OX = 0;
const int OY = 0;

// Comandos de conversão dos eixos e canais de pressão do XPT2046.
const uint8_t XPT_X  = 0xD0;
const uint8_t XPT_Y  = 0x90;
const uint8_t XPT_Z1 = 0xB0;
const uint8_t XPT_Z2 = 0xC0;
const int PRESSAO_MINIMA = 400;

// Transfere um byte por SPI em software, do bit mais significativo ao menos significativo.
uint8_t trocaByteXPT(uint8_t envia) {
    uint8_t recebe = 0;
    for (int i = 7; i >= 0; i--) {
        digitalWrite(TOUCH_DIN, (envia >> i) & 1);
        delayMicroseconds(1);
        digitalWrite(TOUCH_CLK, HIGH);
        delayMicroseconds(1);
        recebe = (recebe << 1) | digitalRead(TOUCH_DO);
        digitalWrite(TOUCH_CLK, LOW);
    }
    return recebe;
}

// Envia o comando e extrai a conversão de 12 bits devolvida pelo XPT2046.
uint16_t leXPT(uint8_t comando) {
    trocaByteXPT(comando);
    delayMicroseconds(2);
    uint16_t alto = trocaByteXPT(0);
    uint16_t baixo = trocaByteXPT(0);
    // Os 12 bits úteis vêm alinhados acima de três bits descartáveis na resposta.
    return ((alto << 8) | baixo) >> 3;
}

// Verifica a interrupção e a pressão; retorna nos parâmetros a média de quatro amostras por eixo.
bool leToque(int& x, int& y, int& z) {
    if (digitalRead(TOUCH_IRQ) == HIGH) return false;
    digitalWrite(TOUCH_CS, LOW);
    int z1 = leXPT(XPT_Z1);
    int z2 = leXPT(XPT_Z2);
    z = z1 + 4095 - z2;
    long somaX = 0, somaY = 0;
    for (int i = 0; i < 4; i++) {
        somaX += leXPT(XPT_X);
        somaY += leXPT(XPT_Y);
    }
    digitalWrite(TOUCH_CS, HIGH);
    x = somaX / 4;
    y = somaY / 4;
    return z >= PRESSAO_MINIMA;
}

// Paleta RGB565: fundo, texto e borda variam com o tema; os níveis mantêm cores fixas.
uint16_t COR_FUNDO = 0x0000;
uint16_t COR_TEXTO = 0xFFFF;
uint16_t COR_BORDA = 0xEF7D;
const uint16_t COR_OK    = 0x07E0;
const uint16_t COR_ERRO  = 0xF800;
const uint16_t COR_ALERTA = 0xFFE0;

// Preferências persistentes e estado de navegação da interface local.
Preferences prefs;
bool temaClaro = false;
bool buzzerMudo = false;

enum Tela { TELA_MENU, TELA_CONFIG, TELA_WIFI, TELA_LINK, TELA_PORTAL };
Tela telaAtual = TELA_MENU;

// Calibração invertida dos eixos para a rotação 3; os eixos são trocados em handleTouch().
const int TOUCH_X_MIN = 3800;
const int TOUCH_X_MAX = 300;
const int TOUCH_Y_MIN = 3800;
const int TOUCH_Y_MAX = 300;

// Extremos da animação de 28 quadros dos interruptores.
const int QUADRO_DESLIGADO = 0;
const int QUADRO_LIGADO = 27;
int quadroTema = QUADRO_DESLIGADO;
int quadroAudio = QUADRO_DESLIGADO;

// Geometria das barras e intervalos independentes de animação e atualização numérica.
const int BARRA_X = 41 + OX;
const int BARRA_W = 246;
const int BARRA_H = 17;
const int BARRA_Y_TEMP = 90 + OY;
const int BARRA_Y_LUZ  = 136 + OY;
const int BARRA_Y_UMID = 183 + OY;
const unsigned long INTERVALO_QUADRO = 42;
const unsigned long INTERVALO_VALORES = 500;


const int PIN_R = 10;
const int PIN_G = 11;
const int PIN_B = 12;

const int PIN_BUZZER = 4;

// Ordem dos limiares: crítico inferior, alerta inferior, alerta superior e crítico superior.
struct Faixa { float critMin, alertaMin, alertaMax, critMax; };
const Faixa FAIXA_TEMP = {  8.0, 10.0, 16.0, 18.0 };
const Faixa FAIXA_UMID = { 40.0, 50.0, 75.0, 85.0 };
// Limiares inferiores negativos desativam os alertas de pouca luz para leituras de 0 a 100%.
const Faixa FAIXA_LUZ  = { -1.0, -1.0, 30.0, 60.0 };

enum Nivel { NIVEL_OK = 0, NIVEL_ALERTA = 1, NIVEL_CRITICO = 2 };
enum Anomalia { ANOM_TEMP = 0, ANOM_UMID = 1, ANOM_LUZ = 2, QTD_ANOM = 3 };
const char* NOMES_ANOM[QTD_ANOM] = { "temp", "umid", "luz" };
Nivel niveis[QTD_ANOM] = { NIVEL_OK, NIVEL_OK, NIVEL_OK };

// NaN representa ausência de leitura válida do DHT22; a luz começa em zero.
float temperatura = NAN;
float umidade = NAN;
int luminosidade = 0;

// Cada nota define frequência em Hz e duração em ms; frequência zero representa silêncio.
struct Nota { uint16_t freq; uint16_t ms; };
const Nota SOM_TEMP[] = { {2000, 150}, {0, 100}, {2000, 150}, {0, 1100} };
const Nota SOM_UMID[] = { {900, 700}, {0, 800} };
const Nota SOM_LUZ[]  = { {1500, 80}, {0, 80}, {1500, 80}, {0, 80}, {1500, 80}, {0, 1100} };
struct Padrao { const Nota* notas; uint8_t qtd; };
const Padrao PADROES[QTD_ANOM] = { {SOM_TEMP, 4}, {SOM_UMID, 2}, {SOM_LUZ, 6} };

// Estado de controle do LED e relógios que limitam as publicações MQTT.
bool modoManual = false;
String corManual = "off";
const unsigned long INTERVALO_PISCA = 500;
const unsigned long INTERVALO_PUBLICACAO = 1000;
unsigned long ultimaPublicacaoEstado = 0;
unsigned long ultimaPublicacaoLuz = 0;

// Parâmetros de conexão e tópicos do dispositivo provisionado no IoT Agent.
// Rede e broker vêm da flash (cadastrados pelo portal); buffers globais porque o PubSubClient guarda o ponteiro do host.
const uint16_t default_BROKER_PORT = 1883;
char wifiSsid[33] = "";
char wifiSenha[65] = "";
char brokerMqtt[65] = "";
uint16_t brokerPorta = default_BROKER_PORT;
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs";
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l";
const char* default_ID_MQTT = "fiware_001";
const char* topicPrefix = "lamp001";

char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp001/attrs/t";
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp001/attrs/h";
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
unsigned long ultimateLeituraDHT = 0;

// O cliente MQTT utiliza o transporte TCP do Wi-Fi.
WiFiClient espClient;
PubSubClient MQTT(espClient);
String corAtual = "off";

// Portal de configuração: Access Point próprio, DNS que responde tudo com o IP do portal e servidor HTTP.
const char* SENHA_AP = "vinheria123";
const IPAddress IP_PORTAL(192, 168, 4, 1);
const unsigned long TEMPO_LIMITE_WIFI = 20000;
const unsigned long TEMPO_LIMITE_MQTT = 10000;
const unsigned long ESPERA_REINICIO = 2000;
WebServer server(80);
DNSServer dnsServer;
char nomeAP[16] = "";
bool portalAtivo = false;
bool portalFechaAoConectar = false;
bool reinicioPendente = false;
unsigned long instanteReinicio = 0;

// Redes do último scan, sem nomes repetidos e ordenadas pelo sinal.
struct RedeEncontrada { char ssid[33]; int32_t rssi; };
const int MAX_REDES = 20;
RedeEncontrada redes[MAX_REDES];
int qtdRedes = 0;
bool scanEmAndamento = false;

// Aplica a intensidade PWM de cada canal do LED RGB.
void setRGB(int r, int g, int b) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
}

// Traduz o nome recebido pelo MQTT em intensidades RGB; rejeita nomes desconhecidos.
bool aplicaCor(const String& cor) {
    if (cor == "red")          setRGB(255, 0, 0);
    else if (cor == "green")   setRGB(0, 255, 0);
    else if (cor == "blue")    setRGB(0, 0, 255);
    else if (cor == "yellow")  setRGB(255, 50, 0);
    else if (cor == "cyan")    setRGB(0, 255, 80);
    else if (cor == "magenta") setRGB(255, 0, 100);
    else if (cor == "white")   setRGB(255, 255, 255);
    else if (cor == "off")     setRGB(0, 0, 0);
    else return false;
    return true;
}

// Limpa a faixa de texto antes de escrever, evitando sobreposição de mensagens.
void escreveLinha(int y, const String& texto, uint16_t cor = COR_TEXTO) {
    display.fillRect(0, y, display.width(), 16, COR_FUNDO);
    display.setTextSize(2);
    display.setTextColor(cor);
    display.setCursor(10, y);
    display.print(texto);
}

// Exibe título e detalhe nas telas de conexão da inicialização.
void telaMensagem(const String& titulo, const String& detalhe) {
    display.fillScreen(COR_FUNDO);
    escreveLinha(60, titulo);
    escreveLinha(110, detalhe);
}

// Converte o bitmap monocromático da flash em pixels RGB565 usando o fundo do tema.
void desenhaIcone(int x, int y, const uint8_t* quadro, uint16_t cor, int tamanho = 32) {
    // Buffer reutilizado: comporta tanto ícones de 32 x 32 quanto de 48 x 48 pixels.
    static uint16_t buf[48 * 48];
    for (int i = 0; i < tamanho * tamanho; i++) {
        uint8_t b = pgm_read_byte(&quadro[i / 8]);
        buf[i] = (b & (0x80 >> (i % 8))) ? cor : COR_FUNDO;
    }
    display.drawRGBBitmap(x + OX, y + OY, buf, tamanho, tamanho);
}

// Seleciona o quadro pelo relógio para manter os ícones de sensores e navegação sincronizados.
void desenhaAnimacoes() {
    int q = millis() / INTERVALO_QUADRO % 28;
    desenhaIcone(0, 83, (const uint8_t*)temperature_weather_32_32_28f_frames[q], 0x05FA);
    desenhaIcone(0, 128, (const uint8_t*)sun_weather_32_32_28f_frames[q], 0xFC00);
    desenhaIcone(1, 174, (const uint8_t*)windy_weather_32_32_28f_frames[q], 0x3A96);
    desenhaIcone(0, 0, (const uint8_t*)down_arrow_32_32_28f_frames[q], COR_BORDA);
    desenhaIcone(288, 0, (const uint8_t*)gear_32_32_28f_frames[q], COR_BORDA);
}

// Associa a gravidade às cores verde, amarelo e vermelho do display.
uint16_t corDoNivel(Nivel n) {
    if (n == NIVEL_CRITICO) return COR_ERRO;
    if (n == NIVEL_ALERTA) return COR_ALERTA;
    return COR_OK;
}

// Redesenha o texto e o preenchimento proporcional, limitando a largura à área interna.
void desenhaBarra(int y, const String& texto, float valor, float vMin, float vMax, Nivel nivel) {
    display.setFont((const GFXfont*)NULL);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO, COR_FUNDO);
    display.fillRect(BARRA_X, y - 11, BARRA_W, 8, COR_FUNDO);
    display.setCursor(BARRA_X + 4, y - 11);
    display.print(texto);

    const int interno = BARRA_W - 4;
    int largura = 0;
    if (!isnan(valor)) largura = constrain((valor - vMin) / (vMax - vMin) * interno, 0, interno);
    if (largura > 0 && largura < 12) largura = 12;
    display.fillRoundRect(BARRA_X + 2, y + 2, interno, BARRA_H - 4, 6, COR_FUNDO);
    if (largura > 0)
        display.fillRoundRect(BARRA_X + 2, y + 2, largura, BARRA_H - 4, 6, corDoNivel(nivel));
}

// Formata as leituras e atualiza as três barras; NaN aparece como ausência de leitura.
void desenhaValores() {
    String t = isnan(temperatura) ? String("--") : String(temperatura, 1);
    String h = isnan(umidade) ? String("--") : String(umidade, 1);
    desenhaBarra(BARRA_Y_TEMP, "Temperatura: " + t + " \xF7" "C", temperatura, 0, 40, niveis[ANOM_TEMP]);
    desenhaBarra(BARRA_Y_LUZ, "Luminosidade: " + String(luminosidade) + " %", luminosidade, 0, 100, niveis[ANOM_LUZ]);
    desenhaBarra(BARRA_Y_UMID, "Umidade: " + h + " %", umidade, 0, 100, niveis[ANOM_UMID]);
}

// Desenha as bordas estáticas e preenche o menu com as leituras e os ícones atuais.
void desenhaMenu() {
    display.fillScreen(COR_FUNDO);
    display.drawRoundRect(BARRA_X, BARRA_Y_TEMP, BARRA_W, BARRA_H, 8, COR_BORDA);
    display.drawRoundRect(BARRA_X, BARRA_Y_LUZ, BARRA_W, BARRA_H, 8, COR_BORDA);
    display.drawRoundRect(BARRA_X, BARRA_Y_UMID, BARRA_W, BARRA_H, 8, COR_BORDA);
    desenhaValores();
    desenhaAnimacoes();
}

// Atualiza a paleta RGB565 compartilhada pelas telas conforme a preferência de tema.
void aplicaTema() {
    if (temaClaro) {
        COR_FUNDO = 0xFFFF;
        COR_TEXTO = 0x0000;
        COR_BORDA = 0x7BEF;
    } else {
        COR_FUNDO = 0x0000;
        COR_TEXTO = 0xFFFF;
        COR_BORDA = 0xEF7D;
    }
}

// Lê a rede Wi-Fi e o broker MQTT salvos na flash; a senha nunca é impressa.
void carregaRede() {
    wifiSsid[0] = wifiSenha[0] = brokerMqtt[0] = '\0';
    if (prefs.isKey("wifiSsid")) prefs.getString("wifiSsid", wifiSsid, sizeof(wifiSsid));
    if (prefs.isKey("wifiSenha")) prefs.getString("wifiSenha", wifiSenha, sizeof(wifiSenha));
    if (prefs.isKey("mqttHost")) prefs.getString("mqttHost", brokerMqtt, sizeof(brokerMqtt));
    brokerPorta = prefs.getUShort("mqttPorta", default_BROKER_PORT);
    if (wifiSsid[0]) Serial.printf("Configuracao de rede carregada: rede \"%s\", broker %s:%u\n", wifiSsid, brokerMqtt, brokerPorta);
    else Serial.println("Nenhuma rede Wi-Fi salva na memoria flash");
}

// Abre as preferências na flash e restaura tema, áudio e quadros dos interruptores.
void carregaConfiguracoes() {
    prefs.begin("vinheria", false);
    temaClaro = prefs.getBool("temaClaro", false);
    buzzerMudo = prefs.getBool("mudo", false);
    aplicaTema();
    quadroTema = temaClaro ? QUADRO_LIGADO : QUADRO_DESLIGADO;
    quadroAudio = buzzerMudo ? QUADRO_LIGADO : QUADRO_DESLIGADO;
    carregaRede();
}

// Compensa os deslocamentos da fonte para posicionar o texto pelo canto superior.
void textoPeloTopo(const char* texto, int x, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x + OX - x1, y + OY - y1);
    display.print(texto);
}

// Desenha os interruptores de tema e áudio nos quadros atuais de suas animações.
void desenhaIconesConfig() {
    desenhaIcone(260, 47, (const uint8_t*)toggle_32_32_28f_frames[quadroTema], COR_TEXTO);
    desenhaIcone(263, 84, (const uint8_t*)mute_32_32_28f_frames[quadroAudio], COR_TEXTO);
}

// Monta a tela de preferências com acesso à conexão e retorno ao menu.
void desenhaConfig() {
    display.fillScreen(COR_FUNDO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo("WiFi/MQTT", 10, 18);
    textoPeloTopo("Tema", 10, 53);
    textoPeloTopo("Audio", 9, 90);
    textoPeloTopo("< Voltar", 10, 205);
    display.setFont((const GFXfont*)NULL);
    display.drawBitmap(260 + OX, 15 + OY, image_arrow_bits, 35, 25, COR_TEXTO);
    desenhaIconesConfig();
}

// Limpa e redesenha uma linha de conexão com a cor correspondente ao estado.
void desenhaStatusConexao(const char* rotulo, bool ok, int x, int y) {
    display.fillRect(OX, y + OY - 4, 320, 28, COR_FUNDO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo(rotulo, x, y);
    display.setTextColor(ok ? COR_OK : COR_ERRO);
    display.print(ok ? "Conectado" : "Desconectado");
    display.setFont((const GFXfont*)NULL);
}

// Monta a tela de diagnóstico com os estados atuais de Wi-Fi e MQTT.
void desenhaWifi() {
    display.fillScreen(COR_FUNDO);
    display.drawBitmap(15 + OX, 15 + OY, image_arrow_right_bits, 35, 25, COR_TEXTO);
    desenhaStatusConexao("WiFi: ", WiFi.status() == WL_CONNECTED, 14, 80);
    desenhaStatusConexao("MQTT: ", MQTT.connected(), 13, 117);
    // Rede e broker em uso, em fonte pequena à direita da seta (até 40 caracteres por linha).
    String rede = String("Rede: ") + (wifiSsid[0] ? wifiSsid : "(nenhuma)");
    String broker = String("Broker: ") + (brokerMqtt[0] ? String(brokerMqtt) + ":" + String(brokerPorta) : String("(nenhum)"));
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    display.setCursor(70 + OX, 18 + OY);
    display.print(rede.substring(0, 40));
    display.setCursor(70 + OX, 32 + OY);
    display.print(broker.substring(0, 40));
    // Botão que abre o portal, no canto inferior direito, abaixo e à direita do ícone animado.
    display.setFont(&FreeMonoBold12pt7b);
    textoPeloTopo("Configurar", 178, 212);
    display.setFont((const GFXfont*)NULL);
}

// Texto da linha de estado da tela do portal, conforme a conexão com a rede salva.
String estadoPortal() {
    if (reinicioPendente) return "Salvo, reiniciando...";
    if (WiFi.status() == WL_CONNECTED) return String("Conectado a ") + wifiSsid;
    if (wifiSsid[0]) return String("Conectando a ") + wifiSsid + "...";
    return "Aguardando configuracao";
}

// Redesenha só a linha de estado da tela do portal (fonte padrão tamanho 2, até 26 caracteres).
void desenhaEstadoPortal(const String& estado) {
    display.fillRect(OX, 150 + OY, 320, 20, COR_FUNDO);
    display.setFont((const GFXfont*)NULL);
    display.setTextSize(2);
    display.setTextColor(COR_ALERTA);
    display.setCursor(10 + OX, 152 + OY);
    display.print(estado.substring(0, 26));
}

// Monta a tela com os dados do Access Point para configurar a rede pelo celular.
void desenhaPortal() {
    display.fillScreen(COR_FUNDO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo("Configurar rede", 10, 15);
    textoPeloTopo((String("Rede: ") + nomeAP).c_str(), 10, 55);
    textoPeloTopo((String("Senha: ") + SENHA_AP).c_str(), 10, 85);
    textoPeloTopo("IP: 192.168.4.1", 10, 115);
    textoPeloTopo("< Voltar", 10, 205);
    desenhaEstadoPortal(estadoPortal());
}

// Monta a tela com o QR code de acesso ao dashboard web e o retorno ao menu.
void drawlink() {
    display.fillScreen(COR_FUNDO);
    display.setFont((const GFXfont*)NULL);
    display.setTextColor(0xBDF7);
    display.setTextSize(3);
    display.setCursor(45 + OX, 20 + OY);
    display.print("Dashboard Web");
    display.drawBitmap(77 + OX, 46 + OY, image_qrcode_bits, 167, 167, COR_TEXTO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo("< Voltar", 10, 218);
    display.setFont((const GFXfont*)NULL);
}

// Troca a tela ativa e redesenha seu conteúdo inicial por completo.
void abreTela(Tela tela) {
    telaAtual = tela;
    if (tela == TELA_MENU) desenhaMenu();
    else if (tela == TELA_CONFIG) desenhaConfig();
    else if (tela == TELA_LINK) drawlink();
    else if (tela == TELA_PORTAL) desenhaPortal();
    else desenhaWifi();
}

// Avança um quadro em direção ao alvo e informa se houve alteração visual.
bool avancaQuadro(int& quadro, int alvo) {
    if (quadro == alvo) return false;
    quadro += (alvo > quadro) ? 1 : -1;
    return true;
}

// Registra sinais e uma conversão interna para auxiliar o diagnóstico da fiação do touch.
void diagnosticoTouch() {
    digitalWrite(TOUCH_CS, HIGH);
    delay(1);
    int doCsAlto = digitalRead(TOUCH_DO);
    digitalWrite(TOUCH_CS, LOW);
    delay(1);
    int doCsBaixo = digitalRead(TOUCH_DO);
    int temp = leXPT(0x84);
    digitalWrite(TOUCH_CS, HIGH);
    Serial.printf("Diag touch: T_DO com CS alto=%d, CS baixo=%d | temperatura interna=%d | T_IRQ=%d\n",
                  doCsAlto, doCsBaixo, temp, digitalRead(TOUCH_IRQ));
}

// Inicializa o TFT em paisagem e configura os GPIOs do touch com SPI em software.
void initDisplay() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
    display.begin();
    display.setRotation(3);
    display.fillScreen(COR_FUNDO);
    pinMode(TOUCH_CLK, OUTPUT);
    digitalWrite(TOUCH_CLK, LOW);
    pinMode(TOUCH_DIN, OUTPUT);
    pinMode(TOUCH_DO, INPUT_PULLUP);
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TOUCH_IRQ, INPUT_PULLUP);
    diagnosticoTouch();
}

// Inicia o console de diagnóstico na mesma velocidade configurada no PlatformIO.
void initSerial() {
    Serial.begin(115200);
}

// Troca os caracteres especiais de HTML para exibir com segurança textos vindos da flash ou do scan.
String escapaHtml(const String& texto) {
    String saida;
    saida.reserve(texto.length() + 8);
    for (unsigned int i = 0; i < texto.length(); i++) {
        char c = texto[i];
        if (c == '&') saida += "&amp;";
        else if (c == '<') saida += "&lt;";
        else if (c == '>') saida += "&gt;";
        else if (c == '"') saida += "&quot;";
        else if (c == '\'') saida += "&#39;";
        else saida += c;
    }
    return saida;
}

// Inicia um scan assíncrono para não travar o loop nem a navegação no portal.
void iniciaScan() {
    if (scanEmAndamento) return;
    WiFi.scanDelete();
    scanEmAndamento = WiFi.scanNetworks(true) == WIFI_SCAN_RUNNING;
}

// Copia o resultado do scan sem nomes repetidos (fica o sinal mais forte) e ordena pelo sinal.
void coletaScan() {
    if (!scanEmAndamento) return;
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return;
    scanEmAndamento = false;
    if (n < 0) {
        Serial.println("Falha no scan de redes Wi-Fi");
        return;
    }
    qtdRedes = 0;
    for (int i = 0; i < n; i++) {
        String nome = WiFi.SSID(i);
        if (nome.length() == 0 || nome.length() > 32) continue;
        int32_t rssi = WiFi.RSSI(i);
        int j = 0;
        while (j < qtdRedes && strcmp(redes[j].ssid, nome.c_str()) != 0) j++;
        if (j < qtdRedes) {
            if (rssi > redes[j].rssi) redes[j].rssi = rssi;
        } else if (qtdRedes < MAX_REDES) {
            strncpy(redes[qtdRedes].ssid, nome.c_str(), sizeof(redes[qtdRedes].ssid));
            redes[qtdRedes].ssid[32] = '\0';
            redes[qtdRedes].rssi = rssi;
            qtdRedes++;
        }
    }
    WiFi.scanDelete();
    for (int i = 1; i < qtdRedes; i++) {
        RedeEncontrada atual = redes[i];
        int j = i - 1;
        while (j >= 0 && redes[j].rssi < atual.rssi) { redes[j + 1] = redes[j]; j--; }
        redes[j + 1] = atual;
    }
    Serial.printf("Scan concluido: %d redes encontradas\n", qtdRedes);
}

// Cabeçalho HTML comum com CSS inline, legível na tela do celular.
String inicioPagina(const char* titulo) {
    String html;
    html.reserve(5000);
    html += F("<!DOCTYPE html><html lang='pt-BR'><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'><title>");
    html += titulo;
    html += F("</title><style>"
              "body{font-family:sans-serif;background:#f4f1ee;color:#222;margin:0;padding:16px}"
              ".box{max-width:420px;margin:auto;background:#fff;border-radius:8px;padding:16px;box-shadow:0 1px 4px #0003}"
              "h1{font-size:1.3em;color:#7a1f3d;margin-top:0}label{display:block;margin-top:12px;font-weight:bold}"
              "input,select{width:100%;box-sizing:border-box;padding:10px;margin-top:4px;font-size:1em;border:1px solid #bbb;border-radius:4px}"
              "input[type=checkbox]{width:auto}button{width:100%;padding:12px;margin-top:16px;font-size:1em;border:0;border-radius:4px;color:#fff;background:#7a1f3d}"
              ".apagar{background:#a33}.nota{font-size:.85em;color:#666}.erro{background:#fdd;color:#900;padding:8px;border-radius:4px}"
              "table{width:100%;font-size:.9em}td{padding:2px 0}.ok{color:#080}.falha{color:#a00}hr{margin:20px 0}"
              "</style></head><body><div class='box'>");
    return html;
}

// Monta o formulário de configuração; os valores recebidos já são exibidos com escape de HTML.
String paginaFormulario(const String& erro, const String& ssid, const String& host, const String& porta) {
    bool wifiOk = WiFi.status() == WL_CONNECTED;
    bool mqttOk = MQTT.connected();
    String html = inicioPagina("Vinheria - Configurar rede");
    html += F("<h1>Configurar rede</h1>");
    if (erro.length()) html += "<p class='erro'>" + escapaHtml(erro) + "</p>";

    html += F("<table>");
    html += "<tr><td>Rede salva:</td><td>" + (wifiSsid[0] ? escapaHtml(wifiSsid) : String("(nenhuma)")) + "</td></tr>";
    html += String("<tr><td>Wi-Fi:</td><td class='") + (wifiOk ? "ok'>Conectado" : "falha'>Desconectado") + "</td></tr>";
    html += String("<tr><td>MQTT:</td><td class='") + (mqttOk ? "ok'>Conectado" : "falha'>Desconectado") + "</td></tr>";
    html += "<tr><td>IP recebido:</td><td>" + (wifiOk ? WiFi.localIP().toString() : String("-")) + "</td></tr>";
    html += F("</table>");

    html += F("<form method='POST' action='/salvar'><label for='rede'>Rede Wi-Fi</label><select id='rede' name='rede'>"
              "<option value=''>-- escolha uma rede --</option>");
    bool naLista = false;
    for (int i = 0; i < qtdRedes; i++) {
        bool sel = ssid == redes[i].ssid;
        naLista |= sel;
        html += "<option value='" + escapaHtml(redes[i].ssid) + "'" + (sel ? " selected" : "") + ">"
              + escapaHtml(redes[i].ssid) + " (" + String(redes[i].rssi) + " dBm)</option>";
    }
    html += F("</select><p class='nota'>");
    if (scanEmAndamento) html += F("Procurando redes... ");
    else if (qtdRedes == 0) html += F("Nenhuma rede encontrada. ");
    html += F("<a href='/atualizar'>Atualizar redes</a>. O ESP32 só conecta em redes de 2,4 GHz.</p>"
              "<label for='manual'>Ou digite o nome da rede</label>");
    html += "<input id='manual' name='manual' maxlength='32' value='" + (naLista ? String("") : escapaHtml(ssid)) + "'>";
    html += F("<label for='senha'>Senha do Wi-Fi</label><input id='senha' name='senha' type='password' maxlength='63'>"
              "<p class='nota'>Deixe vazio para manter a senha salva (mesma rede) ou para rede aberta.</p>"
              "<label for='host'>Broker MQTT (IP ou host)</label>");
    html += "<input id='host' name='host' maxlength='64' value='" + escapaHtml(host) + "'>";
    html += "<label for='porta'>Porta</label><input id='porta' name='porta' type='number' min='1' max='65535' value='" + escapaHtml(porta) + "'>";
    html += F("<button type='submit'>Salvar e conectar</button></form><hr>"
              "<form method='POST' action='/apagar'><label><input type='checkbox' name='confirma' value='1' required> "
              "Confirmo que quero apagar a rede e o broker salvos</label>"
              "<button type='submit' class='apagar'>Apagar configuração</button></form>"
              "</div></body></html>");
    return html;
}

// Responde com o formulário preenchido com os valores atuais da flash.
void handlePaginaInicial() {
    server.send(200, "text/html; charset=utf-8", paginaFormulario("", wifiSsid, brokerMqtt, String(brokerPorta)));
}

// Página exibida depois de salvar ou apagar, antes do reinício marcado no loop.
void respondeReinicio(const char* titulo) {
    String html = inicioPagina(titulo);
    html += "<h1>" + String(titulo) + "</h1><p>Reiniciando o ESP32... Reconecte-se à sua rede.</p></div></body></html>";
    server.send(200, "text/html; charset=utf-8", html);
    reinicioPendente = true;
    instanteReinicio = millis();
    Serial.println("Reiniciando em 2 s para aplicar a configuracao...");
    if (telaAtual == TELA_PORTAL) desenhaEstadoPortal(estadoPortal());
}

// Valida e grava a nova rede e broker; com erro, devolve o formulário preenchido (menos a senha).
void handleSalvar() {
    String ssid = server.arg("manual");
    ssid.trim();
    if (ssid.length() == 0) ssid = server.arg("rede");
    String senha = server.arg("senha");
    String host = server.arg("host");
    host.trim();
    String porta = server.arg("porta");
    porta.trim();
    Serial.printf("Nova configuracao recebida: rede \"%s\", broker %s:%s\n", ssid.c_str(), host.c_str(), porta.c_str());

    long numPorta = porta.toInt();
    String erro;
    if (ssid.length() < 1 || ssid.length() > 32) erro = "Escolha ou digite uma rede Wi-Fi (até 32 caracteres).";
    else if (senha.length() > 0 && (senha.length() < 8 || senha.length() > 63)) erro = "A senha do Wi-Fi deve ter entre 8 e 63 caracteres (ou ficar vazia).";
    else if (host.length() < 1 || host.length() > 64) erro = "Informe o broker MQTT (até 64 caracteres).";
    else if (numPorta < 1 || numPorta > 65535 || String(numPorta) != porta) erro = "A porta deve ser um número entre 1 e 65535.";
    if (erro.length()) {
        Serial.print("Configuracao recusada: ");
        Serial.println(erro);
        server.send(400, "text/html; charset=utf-8", paginaFormulario(erro, ssid, host, porta));
        return;
    }

    // Senha vazia na mesma rede já salva mantém a senha antiga.
    if (senha.length() == 0 && ssid == wifiSsid) senha = wifiSenha;
    prefs.putString("wifiSsid", ssid);
    prefs.putString("wifiSenha", senha);
    prefs.putString("mqttHost", host);
    prefs.putUShort("mqttPorta", (uint16_t)numPorta);
    Serial.println("Configuracao de rede salva na memoria flash");
    respondeReinicio("Configuração salva");
}

// Remove só as chaves de rede (tema e buzzer ficam) e reinicia.
void handleApagar() {
    if (server.arg("confirma") != "1") {
        server.send(400, "text/html; charset=utf-8", paginaFormulario("Marque a confirmação para apagar.", wifiSsid, brokerMqtt, String(brokerPorta)));
        return;
    }
    prefs.remove("wifiSsid");
    prefs.remove("wifiSenha");
    prefs.remove("mqttHost");
    prefs.remove("mqttPorta");
    Serial.println("Configuracao de rede apagada da memoria flash");
    respondeReinicio("Configuração apagada");
}

// Redireciona qualquer endereço para a página do portal (captive portal).
void redirecionaPortal() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
}

// Abre o Access Point Vinheria-XXXX com DNS e servidor HTTP, mantendo o modo estação ativo.
void abrePortal(bool fechaAoConectar) {
    if (portalAtivo) return;
    uint64_t mac = ESP.getEfuseMac();
    snprintf(nomeAP, sizeof(nomeAP), "Vinheria-%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(IP_PORTAL, IP_PORTAL, IPAddress(255, 255, 255, 0));
    WiFi.softAP(nomeAP, SENHA_AP);
    dnsServer.start(53, "*", IP_PORTAL);
    // As rotas ficam registradas no servidor; só são cadastradas na primeira abertura.
    static bool rotasRegistradas = false;
    if (!rotasRegistradas) {
        rotasRegistradas = true;
        server.on("/", HTTP_GET, handlePaginaInicial);
        server.on("/salvar", HTTP_POST, handleSalvar);
        server.on("/apagar", HTTP_POST, handleApagar);
        server.on("/atualizar", HTTP_GET, []() { iniciaScan(); redirecionaPortal(); });
        server.on("/generate_204", HTTP_GET, redirecionaPortal);
        server.on("/hotspot-detect.html", HTTP_GET, redirecionaPortal);
        server.onNotFound(redirecionaPortal);
    }
    server.begin();
    portalAtivo = true;
    portalFechaAoConectar = fechaAoConectar;
    iniciaScan();
    Serial.printf("Portal de configuracao aberto: rede %s, acesse http://%s\n", nomeAP, IP_PORTAL.toString().c_str());
}

// Desliga servidor, DNS e Access Point e volta ao modo só estação.
void fechaPortal() {
    if (!portalAtivo) return;
    server.stop();
    dnsServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    portalAtivo = false;
    scanEmAndamento = false;
    Serial.println("Portal de configuracao fechado");
}

// Atende o portal enquanto ativo, fecha-o quando a rede salva conecta e reinicia após salvar.
void handlePortal() {
    if (reinicioPendente && millis() - instanteReinicio >= ESPERA_REINICIO) {
        Serial.println("Reiniciando...");
        ESP.restart();
    }
    if (!portalAtivo) return;
    dnsServer.processNextRequest();
    server.handleClient();
    coletaScan();
    if (portalFechaAoConectar && !reinicioPendente && WiFi.status() == WL_CONNECTED) {
        Serial.printf("Conectado com sucesso na rede %s\n", wifiSsid);
        fechaPortal();
        if (telaAtual == TELA_PORTAL) abreTela(TELA_MENU);
    }
}

// Na partida, tenta a rede salva por até 20 s exibindo o progresso; sem rede ou sem sucesso, abre o portal.
void initWiFi() {
    Serial.println("------Conexao WI-FI------");
    WiFi.mode(WIFI_STA);
    if (wifiSsid[0] == '\0') {
        Serial.println("Sem rede salva: abrindo o portal de configuracao");
        abrePortal(true);
        return;
    }
    Serial.print("Conectando-se na rede: ");
    Serial.println(wifiSsid);
    Serial.println("Aguarde");

    telaMensagem("Conectando Wi-Fi...", "Rede: " + String(wifiSsid));

    WiFi.begin(wifiSsid, wifiSenha);

    int pontos = 0;
    unsigned long inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < TEMPO_LIMITE_WIFI) {
        String aguarde = "Aguarde";
        for (int i = 0; i < pontos; i++) aguarde += ".";
        escreveLinha(160, aguarde);

        pontos = (pontos + 1) % 4;
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Nao foi possivel conectar na rede salva: abrindo o portal de configuracao");
        abrePortal(true);
        return;
    }

    Serial.print("Conectado com sucesso na rede ");
    Serial.println(wifiSsid);

    telaMensagem("Wi-Fi conectado!", "Rede: " + String(wifiSsid));
    delay(1000);
}

// Na partida, tenta o broker por até 10 s; sem sucesso, segue e o reconnectMQTT() continua tentando.
void initMQTT() {
    MQTT.setServer(brokerMqtt, brokerPorta);
    MQTT.setSocketTimeout(3);
    MQTT.setCallback(mqtt_callback);
    if (WiFi.status() != WL_CONNECTED || brokerMqtt[0] == '\0') return;

    Serial.printf("* Conectando ao Broker MQTT: %s:%u\n", brokerMqtt, brokerPorta);

    telaMensagem("Conectando MQTT...", "Broker: " + String(brokerMqtt));

    int pontos = 0;
    unsigned long inicio = millis();
    while (!MQTT.connected() && millis() - inicio < TEMPO_LIMITE_MQTT) {
        String aguarde = "Aguarde";
        for (int i = 0; i < pontos; i++) aguarde += ".";
        escreveLinha(160, aguarde);
        pontos = (pontos + 1) % 4;

        Serial.print(".");
        if (MQTT.connect(ID_MQTT)) {
            Serial.println();
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else if (millis() - inicio < TEMPO_LIMITE_MQTT) {
            delay(1000);
        }
    }

    if (!MQTT.connected()) {
        Serial.println();
        Serial.println("Broker MQTT nao respondeu: seguindo para o menu, nova tentativa a cada 5s");
        telaMensagem("MQTT desconectado", "Tentando a cada 5 s");
        delay(1000);
        return;
    }

    telaMensagem("MQTT conectado!", "Broker: " + String(brokerMqtt));
    delay(1000);
}

// Lê temperatura e umidade a cada dois segundos; preserva a leitura anterior se houver falha.
void handleDHT() {
    if (millis() - ultimateLeituraDHT < 2000) return;

    ultimateLeituraDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
        Serial.println("Falha ao ler o DHT22!");
        return;
    }
    temperatura = t;
    umidade = h;
    MQTT.publish(TOPICO_PUBLISH_3, String(t, 1).c_str());
    MQTT.publish(TOPICO_PUBLISH_4, String(h, 1).c_str());
    Serial.printf("Temp: %.1f C | Umid: %.1f %%\n", t, h);
}

// Prepara saídas, sensores, preferências e conexões uma única vez ao ligar a placa.
void setup() {
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    setRGB(false, false, false);
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
    dht.begin();

    initSerial();
    carregaConfiguracoes();

    initDisplay();

    initWiFi();
    initMQTT();
    if (portalAtivo) {
        abreTela(TELA_PORTAL);
        return;
    }
    if (MQTT.connected()) MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    abreTela(TELA_MENU);
}

// Executa as tarefas recorrentes; cada handler controla seu próprio intervalo de atualização.
void loop() {
    VerificaConexoesWiFIEMQTT();
    EnviaEstadoOutputMQTT();
    handleLuminosity();
    handleDHT();
    avaliaTriggers();
    handleLED();
    handleBuzzer();
    MQTT.loop();
    handleDisplay();
    handleTouch();
    handlePortal();
}

// Anima os interruptores até o quadro correspondente à preferência selecionada.
void handleDisplayConfig() {
    static unsigned long ultimoQuadro = 0;
    if (millis() - ultimoQuadro < INTERVALO_QUADRO) return;
    ultimoQuadro = millis();
    bool mudou = avancaQuadro(quadroTema, temaClaro ? QUADRO_LIGADO : QUADRO_DESLIGADO);
    mudou |= avancaQuadro(quadroAudio, buzzerMudo ? QUADRO_LIGADO : QUADRO_DESLIGADO);
    if (mudou) desenhaIconesConfig();
}

// Anima o ícone e atualiza as linhas de conexão apenas quando o estado muda.
void handleDisplayWifi() {
    static unsigned long ultimoQuadro = 0;
    static unsigned long ultimaChecagem = 0;
    static int wifiAnterior = -1, mqttAnterior = -1;
    if (millis() - ultimoQuadro >= INTERVALO_QUADRO) {
        ultimoQuadro = millis();
        int q = millis() / INTERVALO_QUADRO % 28;
        desenhaIcone(136, 157, (const uint8_t*)blushing_48_48_28f_frames[q], 0xCEE7, 48);
    }
    if (millis() - ultimaChecagem < 500) return;
    ultimaChecagem = millis();
    int wifiOk = WiFi.status() == WL_CONNECTED;
    int mqttOk = MQTT.connected();
    if (wifiOk != wifiAnterior) desenhaStatusConexao("WiFi: ", wifiOk, 14, 80);
    if (mqttOk != mqttAnterior) desenhaStatusConexao("MQTT: ", mqttOk, 13, 117);
    wifiAnterior = wifiOk;
    mqttAnterior = mqttOk;
}

// Redesenha a linha de estado do portal a cada meio segundo, só quando o texto muda.
void handleDisplayPortal() {
    static unsigned long ultimaChecagem = 0;
    static String estadoAnterior;
    if (millis() - ultimaChecagem < 500) return;
    ultimaChecagem = millis();
    String estado = estadoPortal();
    if (estado == estadoAnterior) return;
    estadoAnterior = estado;
    desenhaEstadoPortal(estado);
}

// Encaminha a atualização periódica para a tela que está visível.
void handleDisplay() {
    if (telaAtual == TELA_CONFIG) handleDisplayConfig();
    else if (telaAtual == TELA_WIFI) handleDisplayWifi();
    else if (telaAtual == TELA_PORTAL) handleDisplayPortal();
    else if (telaAtual == TELA_MENU) handleDisplayMenu();
}

// Separa a cadência das animações da atualização das leituras e evita redesenhos desnecessários.
void handleDisplayMenu() {
    static unsigned long ultimoQuadro = 0;
    static unsigned long ultimosValores = 0;
    static float tempAnterior = NAN, umidAnterior = NAN;
    static int luzAnterior = -1;
    static Nivel niveisAnteriores[QTD_ANOM] = { NIVEL_OK, NIVEL_OK, NIVEL_OK };

    if (millis() - ultimoQuadro >= INTERVALO_QUADRO) {
        ultimoQuadro = millis();
        desenhaAnimacoes();
    }

    if (millis() - ultimosValores < INTERVALO_VALORES) return;
    ultimosValores = millis();
    bool mudou = luminosidade != luzAnterior
        || memcmp(niveis, niveisAnteriores, sizeof(niveis)) != 0
        || !(temperatura == tempAnterior || (isnan(temperatura) && isnan(tempAnterior)))
        || !(umidade == umidAnterior || (isnan(umidade) && isnan(umidAnterior)));
    if (!mudou) return;
    luzAnterior = luminosidade;
    tempAnterior = temperatura;
    umidAnterior = umidade;
    memcpy(niveisAnteriores, niveis, sizeof(niveis));
    desenhaValores();
}

// Testa se o toque está dentro de uma região, com margem de cinco pixels para facilitar o uso.
bool tocouEm(int x, int y, int rx, int ry, int rw, int rh) {
    const int FOLGA = 5;
    rx += OX;
    ry += OY;
    return x >= rx - FOLGA && x < rx + rw + FOLGA && y >= ry - FOLGA && y < ry + rh + FOLGA;
}

// Abre as configurações pela engrenagem e o QR code do dashboard pela seta.
void toqueMenu(int x, int y) {
    if (tocouEm(x, y, 288, 0, 32, 32)) abreTela(TELA_CONFIG);
    else if (tocouEm(x, y, 0, 0, 32, 32)) abreTela(TELA_LINK);
}

// Trata navegação e preferências, persistindo as alterações de tema e áudio na flash.
void toqueConfig(int x, int y) {
    if (tocouEm(x, y, 260, 15, 35, 25)) {
        abreTela(TELA_WIFI);
    } else if (tocouEm(x, y, 260, 47, 32, 32)) {
        temaClaro = !temaClaro;
        prefs.putBool("temaClaro", temaClaro);
        aplicaTema();
        desenhaConfig();
    } else if (tocouEm(x, y, 263, 84, 32, 32)) {
        buzzerMudo = !buzzerMudo;
        prefs.putBool("mudo", buzzerMudo);
        if (buzzerMudo) noTone(PIN_BUZZER);
    } else if (tocouEm(x, y, 0, 195, 140, 45)) {
        abreTela(TELA_MENU);
    }
}

// Volta às configurações pela seta ou abre o portal pelo botão "Configurar".
void toqueWifi(int x, int y) {
    if (tocouEm(x, y, 15, 15, 35, 25)) {
        abreTela(TELA_CONFIG);
    } else if (tocouEm(x, y, 170, 200, 150, 40)) {
        abrePortal(false);
        abreTela(TELA_PORTAL);
    }
}

// "< Voltar" fecha o portal se a rede salva estiver conectada e retorna à tela de WiFi/MQTT.
void toquePortal(int x, int y) {
    if (!tocouEm(x, y, 0, 195, 140, 45)) return;
    if (WiFi.status() == WL_CONNECTED) fechaPortal();
    abreTela(TELA_WIFI);
}

// Aceita um evento por pressão, descarta leituras inválidas e converte os eixos para pixels.
void handleTouch() {
    static bool tocando = false;
    int bx, by, bz;
    if (!leToque(bx, by, bz)) {
        tocando = false;
        return;
    }
    // Exige soltar a tela antes de disparar outra ação, evitando alternâncias repetidas.
    if (tocando) return;
    tocando = true;
    if ((bx >= 4000 && by >= 4000) || (bx == 0 && by == 0)) {
        Serial.printf("- Toque bruto x=%d y=%d z=%d\n", bx, by, bz);
        Serial.println("- Toque ignorado: XPT2046 sem resposta (confira T_DO, T_DIN, T_CLK e T_CS)");
        return;
    }
    // A orientação física do painel exige usar o eixo bruto Y como X da tela e vice-versa.
    int x = constrain(map(by, TOUCH_X_MIN, TOUCH_X_MAX, 0, display.width()), 0, display.width() - 1);
    int y = constrain(map(bx, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, display.height()), 0, display.height() - 1);
    Serial.printf("- Toque bruto x=%d y=%d z=%d -> tela x=%d y=%d\n", bx, by, bz, x, y);
    if (telaAtual == TELA_MENU) toqueMenu(x, y);
    else if (telaAtual == TELA_CONFIG) toqueConfig(x, y);
    else if (telaAtual == TELA_LINK) {
        if (tocouEm(x, y, 0, 213, 140, 27)) abreTela(TELA_MENU);
    }
    else if (telaAtual == TELA_PORTAL) toquePortal(x, y);
    else toqueWifi(x, y);
}

// Limita novas tentativas de conexão Wi-Fi a uma a cada dez segundos durante o uso.
void reconectWiFi() {
    static unsigned long ultimaTentativa = 0;
    static bool estavaConectado = true;
    if (WiFi.status() == WL_CONNECTED) {
        if (!estavaConectado) {
            Serial.print("Conectado com sucesso na rede ");
            Serial.println(wifiSsid);
        }
        estavaConectado = true;
        return;
    }
    estavaConectado = false;
    if (wifiSsid[0] == '\0') return;
    if (millis() - ultimaTentativa < 10000) return;
    ultimaTentativa = millis();
    Serial.println("Wi-Fi desconectado, tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(wifiSsid, wifiSenha);
}

// Interpreta comandos UltraLight do dispositivo; cores só são aceitas no modo manual.
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (int i = 0; i < length; i++) {
        char c = (char)payload[i];
        msg += c;
    }
    Serial.print("- Mensagem recebida: ");
    Serial.println(msg);

    String prefix = String(topicPrefix) + "@";
    if (!msg.startsWith(prefix)) return;
    String cmd = msg.substring(prefix.length());
    int fim = cmd.indexOf('|');
    if (fim >= 0) cmd = cmd.substring(0, fim);

    if (cmd == "auto") {
        modoManual = false;
        Serial.println("- LED em modo automatico (triggers)");
    } else if (cmd == "manual") {
        modoManual = true;
        corManual = corAtual;
        Serial.println("- LED em modo manual");
    } else if (modoManual) {
        if (aplicaCor(cmd)) corManual = cmd;
    } else {
        Serial.println("- Cor ignorada: LED em modo automatico. Envie o comando 'manual' primeiro.");
    }
}

// Recupera o Wi-Fi e só tenta restabelecer o MQTT quando há rede disponível.
void VerificaConexoesWiFIEMQTT() {
    reconectWiFi();
    if (!MQTT.connected() && WiFi.status() == WL_CONNECTED && brokerMqtt[0])
        reconnectMQTT();
}

// Publica cor, modo e alertas no formato UltraLight uma vez por segundo.
void EnviaEstadoOutputMQTT() {
    if (millis() - ultimaPublicacaoEstado < INTERVALO_PUBLICACAO) return;
    ultimaPublicacaoEstado = millis();

    String alerta = "";
    for (int i = 0; i < QTD_ANOM; i++) {
        if (niveis[i] == NIVEL_OK) continue;
        // Usa vírgula entre anomalias para manter o valor aceito pelo Orion.
        if (alerta.length()) alerta += ",";
        alerta += String(NOMES_ANOM[i]) + (niveis[i] == NIVEL_CRITICO ? ":critico" : ":alerta");
    }
    if (alerta.length() == 0) alerta = "ok";

    String estado = "s|" + corAtual + "|m|" + (modoManual ? "manual" : "auto") + "|a|" + alerta;
    MQTT.publish(TOPICO_PUBLISH_1, estado.c_str());
    Serial.print("- Estado enviado ao broker: ");
    Serial.println(estado);
}

// Ponto de extensão vazio, mantido no código; as saídas são configuradas em setup().
void InitOutput() {
}

// Espaça as tentativas de MQTT em cinco segundos e refaz a assinatura após reconectar.
void reconnectMQTT() {
    static unsigned long ultimaTentativa = 0;
    if (millis() - ultimaTentativa < 5000) return;
    ultimaTentativa = millis();
    Serial.printf("* Tentando se conectar ao Broker MQTT: %s:%u\n", brokerMqtt, brokerPorta);
    if (MQTT.connect(ID_MQTT)) {
        Serial.println("Conectado com sucesso ao broker MQTT!");
        MQTT.subscribe(TOPICO_SUBSCRIBE);
    } else {
        Serial.println("Falha ao reconectar no broker. Nova tentativa em 5s");
    }
}

// Converte o ADC de 12 bits em percentual invertido e limita a publicação a uma por segundo.
void handleLuminosity() {
    const int potPin = 1;
    int sensorValue = analogRead(potPin);
    luminosidade = map(sensorValue, 0, 4095, 100, 0);

    if (millis() - ultimaPublicacaoLuz < INTERVALO_PUBLICACAO) return;
    ultimaPublicacaoLuz = millis();
    String mensagem = String(luminosidade);
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
}

// Compara a leitura com as faixas: limites críticos e de alerta usam desigualdades estritas.
Nivel classifica(float valor, const Faixa& f) {
    // A ausência de leitura não dispara alarme nesta classificação do firmware.
    if (isnan(valor)) return NIVEL_OK;
    if (valor < f.critMin || valor > f.critMax) return NIVEL_CRITICO;
    if (valor < f.alertaMin || valor > f.alertaMax) return NIVEL_ALERTA;
    return NIVEL_OK;
}

// Recalcula os níveis dos sensores e registra no console apenas as mudanças de gravidade.
void avaliaTriggers() {
    Nivel novos[QTD_ANOM] = {
        classifica(temperatura, FAIXA_TEMP),
        classifica(umidade, FAIXA_UMID),
        classifica(luminosidade, FAIXA_LUZ)
    };
    const char* nomesNivel[] = { "normal", "ALERTA", "CRITICO" };
    for (int i = 0; i < QTD_ANOM; i++) {
        if (novos[i] != niveis[i]) {
            Serial.printf("* Trigger %s: %s -> %s\n", NOMES_ANOM[i], nomesNivel[niveis[i]], nomesNivel[novos[i]]);
            niveis[i] = novos[i];
        }
    }
}

// No automático, sinaliza o pior nível e pisca nos alertas; no manual, mantém a cor escolhida.
void handleLED() {
    if (modoManual) {
        aplicaCor(corManual);
        corAtual = corManual;
        return;
    }

    Nivel pior = NIVEL_OK;
    for (int i = 0; i < QTD_ANOM; i++)
        if (niveis[i] > pior) pior = niveis[i];

    if (pior == NIVEL_OK) {
        aplicaCor("green");
        corAtual = "green";
        return;
    }

    // Publica a cor lógica do alerta mesmo durante a metade apagada do pisca.
    corAtual = (pior == NIVEL_CRITICO) ? "red" : "yellow";
    bool aceso = (millis() / INTERVALO_PISCA) % 2 == 0;
    aplicaCor(aceso ? corAtual : String("off"));
}

// Reproduz notas sem espera fixa e alterna ciclicamente entre as anomalias ativas.
void handleBuzzer() {
    static int somAtual = -1;
    static uint8_t passo = 0;
    static unsigned long inicioPasso = 0;

    if (buzzerMudo) {
        if (somAtual >= 0) noTone(PIN_BUZZER);
        somAtual = -1;
        return;
    }

    // Percorre as anomalias em rodízio para que cada uma tenha seu padrão sonoro.
    auto proximaAtiva = [](int atual) {
        for (int k = 1; k <= QTD_ANOM; k++) {
            int i = (atual + k + QTD_ANOM) % QTD_ANOM;
            if (niveis[i] != NIVEL_OK) return i;
        }
        return -1;
    };
    auto tocaPasso = [&]() {
        const Nota& n = PADROES[somAtual].notas[passo];
        if (n.freq > 0) tone(PIN_BUZZER, n.freq);
        else noTone(PIN_BUZZER);
        inicioPasso = millis();
    };

    if (somAtual < 0 || niveis[somAtual] == NIVEL_OK) {
        somAtual = proximaAtiva(somAtual < 0 ? QTD_ANOM - 1 : somAtual);
        if (somAtual < 0) { noTone(PIN_BUZZER); return; }
        passo = 0;
        tocaPasso();
        return;
    }

    // Só avança quando a duração da nota termina, mantendo o loop disponível.
    if (millis() - inicioPasso < PADROES[somAtual].notas[passo].ms) return;

    passo++;
    if (passo >= PADROES[somAtual].qtd) {
        passo = 0;
        somAtual = proximaAtiva(somAtual);
    }
    tocaPasso();
}

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
//Rev14: 11-10-2026 Relógio RTC (DS3231 na placa, DS1307 no Wokwi) no I2C SDA 8 / SCL 9, acertado pelo NTP quando há Wi-Fi; hora só no Serial
//Autor Rev14: Gianluca Antonicci
//Rev15: 11-10-2026 Removido o suporte ao Wokwi; código só para a placa física (touch XPT2046 e RTC DS3231)
//Autor Rev15: Gianluca Antonicci
//Rev16: 15-10-2026 Menu principal (layout do Lopaka adaptado para a Adafruit ILI9341): hora do RTC, temperatura, luminosidade e umidade em tempo real, com ícones animados
//Autor Rev16: Gianluca Antonicci
//Rev17: 15-10-2026 Hora do menu cai para o relógio interno acertado pelo NTP quando o DS3231 não responde; RTC procurado de novo a cada 5 s com varredura do I2C no Serial
//Autor Rev17: Gianluca Antonicci
//Rev18: 15-10-2026 Varredura do I2C só na partida e com timeout curto (travava a tela sem o RTC); nova busca do RTC a cada 30 s
//Autor Rev18: Gianluca Antonicci
//Rev19: 15-10-2026 Partida mais rápida: removidas as esperas fixas antes de conectar e antes do menu; telas de conectado ficam 1 s
//Autor Rev19: Gianluca Antonicci
//Rev20: 15-10-2026 Removidas a varredura do I2C e a nova busca do RTC no loop: com o barramento travado cada tentativa levava ~1 s e a partida demorava 2 min
//Autor Rev20: Gianluca Antonicci

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <RTClib.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>
#include <Fonts/FreeSans12pt7b.h>  // já vem com a Adafruit GFX

// Ícones animados exportados pelo Lopaka (32x32, 28 quadros, 1 bit por pixel). Ficam em firmware/include/
#include "down_arrow_32_32_28f.h"
#include "gear_32_32_28f.h"
#include "sun_weather_32_32_28f.h"
#include "temperature_weather_32_32_28f.h"
#include "windy_weather_32_32_28f.h"

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT();
void EnviaEstadoOutputMQTT();
void handleLuminosity();
void reconnectMQTT();
void avaliaTriggers();
void handleLED();
void handleBuzzer();
void handleDisplay();
void handleTouch();
void handleRTC();
bool horaAtual(int& hora, int& minuto);

// TFT e touch dividem o mesmo barramento SPI (T_CLK, T_DIN e T_DO ligados em SCK, MOSI e MISO)
#define TFT_SCK   13
#define TFT_MOSI  14
#define TFT_MISO  21
#define TFT_CS    5
#define TFT_DC    6
#define TFT_RST   7
#define TFT_LED   18  // luz de fundo (backlight)
#define TOUCH_CLK TFT_SCK   // T_CLK no mesmo fio do SCK (13)
#define TOUCH_DIN TFT_MOSI  // T_DIN no mesmo fio do MOSI (14)
#define TOUCH_DO  TFT_MISO  // T_DO no mesmo fio do MISO (21)
#define TOUCH_CS  16
#define TOUCH_IRQ 17
// RTC DS3231 no I2C
#define RTC_SDA 8
#define RTC_SCL 9
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

Adafruit_ILI9341 display(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// DS3231: bateria própria e mais preciso
RTC_DS3231 rtc;
bool rtcOk = false;
const long FUSO_BRASILIA = -3 * 3600;  // UTC-3, sem horário de verão
const unsigned long INTERVALO_NTP = 6UL * 3600 * 1000;  // reacerta o RTC a cada 6 h
const unsigned long INTERVALO_HORA_SERIAL = 30000;
unsigned long ultimoNTP = 0;
unsigned long ultimaHoraSerial = 0;

// Cores do display (RGB565)
const uint16_t COR_FUNDO = ILI9341_BLACK;
const uint16_t COR_TEXTO = ILI9341_WHITE;
const uint16_t COR_OK    = ILI9341_GREEN;
const uint16_t COR_ERRO  = ILI9341_RED;
const uint16_t COR_ALERTA = ILI9341_YELLOW;
const uint16_t COR_HORA   = 0xFF47;  // amarelo claro do layout
const uint16_t COR_BORDA  = 0xEF7D;  // cinza claro do layout

// Posições do menu principal (vindas do Lopaka, tela 320x240)
const int BARRA_X = 41;
const int BARRA_W = 246;
const int BARRA_H = 17;
const int BARRA_Y_TEMP = 90;
const int BARRA_Y_LUZ  = 136;
const int BARRA_Y_UMID = 183;
const unsigned long INTERVALO_QUADRO = 42;   // ~24 quadros por segundo
const unsigned long INTERVALO_VALORES = 500;  // barras e textos


const int PIN_R = 10;
const int PIN_G = 11;
const int PIN_B = 12;

const int PIN_BUZZER = 4;

struct Faixa { float critMin, alertaMin, alertaMax, critMax; };
const Faixa FAIXA_TEMP = {  8.0, 10.0, 16.0, 18.0 };
const Faixa FAIXA_UMID = { 40.0, 50.0, 75.0, 85.0 };
const Faixa FAIXA_LUZ  = { -1.0, -1.0, 30.0, 60.0 };

enum Nivel { NIVEL_OK = 0, NIVEL_ALERTA = 1, NIVEL_CRITICO = 2 };
enum Anomalia { ANOM_TEMP = 0, ANOM_UMID = 1, ANOM_LUZ = 2, QTD_ANOM = 3 };
const char* NOMES_ANOM[QTD_ANOM] = { "temp", "umid", "luz" };
Nivel niveis[QTD_ANOM] = { NIVEL_OK, NIVEL_OK, NIVEL_OK };

float temperatura = NAN;
float umidade = NAN;
int luminosidade = 0;

struct Nota { uint16_t freq; uint16_t ms; };
const Nota SOM_TEMP[] = { {2000, 150}, {0, 100}, {2000, 150}, {0, 1100} };
const Nota SOM_UMID[] = { {900, 700}, {0, 800} };
const Nota SOM_LUZ[]  = { {1500, 80}, {0, 80}, {1500, 80}, {0, 80}, {1500, 80}, {0, 1100} };
struct Padrao { const Nota* notas; uint8_t qtd; };
const Padrao PADROES[QTD_ANOM] = { {SOM_TEMP, 4}, {SOM_UMID, 2}, {SOM_LUZ, 6} };

bool modoManual = false;
String corManual = "off";
const unsigned long INTERVALO_PISCA = 500;
const unsigned long INTERVALO_PUBLICACAO = 1000;
unsigned long ultimaPublicacaoEstado = 0;
unsigned long ultimaPublicacaoLuz = 0;

const char* default_SSID = "Galaxy S26 Ultra Gianluca";
const char* default_PASSWORD = "1008100810";
const char* default_BROKER_MQTT = "54.236.175.117";
const int default_BROKER_PORT = 1883;
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs";
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l";
const char* default_ID_MQTT = "fiware_001";
const char* topicPrefix = "lamp001";

char* SSID = const_cast<char*>(default_SSID);
char* PASSWORD = const_cast<char*>(default_PASSWORD);
char* BROKER_MQTT = const_cast<char*>(default_BROKER_MQTT);
int BROKER_PORT = default_BROKER_PORT;
char* TOPICO_SUBSCRIBE = const_cast<char*>(default_TOPICO_SUBSCRIBE);
char* TOPICO_PUBLISH_1 = const_cast<char*>(default_TOPICO_PUBLISH_1);
char* TOPICO_PUBLISH_2 = const_cast<char*>(default_TOPICO_PUBLISH_2);
char* ID_MQTT = const_cast<char*>(default_ID_MQTT);
const char* default_TOPICO_PUBLISH_3 = "/TEF/lamp001/attrs/t";
const char* default_TOPICO_PUBLISH_4 = "/TEF/lamp001/attrs/h";
char* TOPICO_PUBLISH_3 = const_cast<char*>(default_TOPICO_PUBLISH_3);
char* TOPICO_PUBLISH_4 = const_cast<char*>(default_TOPICO_PUBLISH_4);
unsigned long ultimateLeituraDHT = 0;

WiFiClient espClient;
PubSubClient MQTT(espClient);
String corAtual = "off";

void setRGB(int r, int g, int b) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
}

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

// Escreve uma linha de texto (tamanho 2, 16 px de altura) apagando o que havia antes,
// para atualizar só aquele trecho sem limpar a tela inteira
void escreveLinha(int y, const String& texto, uint16_t cor = COR_TEXTO) {
    display.fillRect(0, y, display.width(), 16, COR_FUNDO);
    display.setTextSize(2);
    display.setTextColor(cor);
    display.setCursor(10, y);
    display.print(texto);
}

void telaMensagem(const String& titulo, const String& detalhe) {
    display.fillScreen(COR_FUNDO);
    escreveLinha(60, titulo);
    escreveLinha(110, detalhe);
}

// Desenha um quadro 32x32 de 1 bit montando os pixels num buffer e enviando de uma vez:
// o drawBitmap da GFX manda pixel a pixel e fica lento demais para animar 5 ícones
void desenhaIcone(int x, int y, const uint8_t* quadro, uint16_t cor) {
    static uint16_t buf[32 * 32];
    for (int i = 0; i < 32 * 32; i++) {
        uint8_t b = pgm_read_byte(&quadro[i / 8]);
        buf[i] = (b & (0x80 >> (i % 8))) ? cor : COR_FUNDO;
    }
    display.drawRGBBitmap(x, y, buf, 32, 32);
}

void desenhaAnimacoes() {
    int q = millis() / INTERVALO_QUADRO % 28;
    desenhaIcone(0, 83, (const uint8_t*)temperature_weather_32_32_28f_frames[q], 0x05FA);
    desenhaIcone(0, 128, (const uint8_t*)sun_weather_32_32_28f_frames[q], 0xFC00);
    desenhaIcone(1, 174, (const uint8_t*)windy_weather_32_32_28f_frames[q], 0x3A96);
    // Seta e engrenagem: só desenhadas por enquanto; a engrenagem vai abrir as configurações
    desenhaIcone(0, 0, (const uint8_t*)down_arrow_32_32_28f_frames[q], COR_BORDA);
    desenhaIcone(288, 0, (const uint8_t*)gear_32_32_28f_frames[q], COR_BORDA);
}

// Hora HH:MM centralizada no topo, entre a seta e a engrenagem
void desenhaHora() {
    char hora[6] = "--:--";
    int hh, mm;
    if (horaAtual(hh, mm)) snprintf(hora, sizeof(hora), "%02d:%02d", hh, mm);
    display.setFont(&FreeSans12pt7b);
    display.setTextSize(2);
    display.setTextColor(COR_HORA);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(hora, 0, 0, &x1, &y1, &w, &h);
    // Fonte livre não apaga o fundo sozinha: limpa a faixa entre os ícones
    display.fillRect(34, 10, 252, 56, COR_FUNDO);
    // No Lopaka o texto é posicionado pelo topo (y 18); na GFX o cursor é a linha de base
    display.setCursor((display.width() - w) / 2 - x1, 18 - y1);
    display.print(hora);
    display.setFont(NULL);
}

uint16_t corDoNivel(Nivel n) {
    if (n == NIVEL_CRITICO) return COR_ERRO;
    if (n == NIVEL_ALERTA) return COR_ALERTA;
    return COR_OK;
}

// Texto acima da barra e preenchimento proporcional ao valor (vMin..vMax), na cor do trigger
void desenhaBarra(int y, const String& texto, float valor, float vMin, float vMax, Nivel nivel) {
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO, COR_FUNDO);
    display.fillRect(BARRA_X, y - 11, BARRA_W, 8, COR_FUNDO);
    display.setCursor(BARRA_X + 4, y - 11);
    display.print(texto);

    const int interno = BARRA_W - 4;
    int largura = 0;
    if (!isnan(valor)) largura = constrain((valor - vMin) / (vMax - vMin) * interno, 0, interno);
    if (largura > 0 && largura < 12) largura = 12;  // abaixo disso o canto arredondado deforma
    display.fillRoundRect(BARRA_X + 2, y + 2, interno, BARRA_H - 4, 6, COR_FUNDO);
    if (largura > 0)
        display.fillRoundRect(BARRA_X + 2, y + 2, largura, BARRA_H - 4, 6, corDoNivel(nivel));
}

void desenhaValores() {
    String t = isnan(temperatura) ? String("--") : String(temperatura, 1);
    String h = isnan(umidade) ? String("--") : String(umidade, 1);
    desenhaBarra(BARRA_Y_TEMP, "Temperatura: " + t + " \xF7" "C", temperatura, 0, 40, niveis[ANOM_TEMP]);
    desenhaBarra(BARRA_Y_LUZ, "Luminosidade: " + String(luminosidade) + " %", luminosidade, 0, 100, niveis[ANOM_LUZ]);
    desenhaBarra(BARRA_Y_UMID, "Umidade: " + h + " %", umidade, 0, 100, niveis[ANOM_UMID]);
}

void desenhaMenu() {
    display.fillScreen(COR_FUNDO);
    display.drawRoundRect(BARRA_X, BARRA_Y_TEMP, BARRA_W, BARRA_H, 8, COR_BORDA);
    display.drawRoundRect(BARRA_X, BARRA_Y_LUZ, BARRA_W, BARRA_H, 8, COR_BORDA);
    display.drawRoundRect(BARRA_X, BARRA_Y_UMID, BARRA_W, BARRA_H, 8, COR_BORDA);
    desenhaHora();
    desenhaValores();
    desenhaAnimacoes();
}

void initDisplay() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
    display.begin();
    display.setRotation(3);  // paisagem invertida: 320x240
    display.cp437(true);     // caractere 0xF7 = símbolo de grau
    display.fillScreen(COR_FUNDO);
    touch.begin();
    touch.setRotation(3);   // igual à do display, para as coordenadas do toque baterem com a tela
}

void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);
    Wire.setTimeOut(10);
    // Sem o RTC (ou com o I2C travado) só tenta uma vez: cada tentativa pode levar ~1 s.
    // Nesse caso a hora do menu vem do relógio interno acertado pelo NTP
    rtcOk = rtc.begin(&Wire);
    if (!rtcOk) {
        Serial.println("RTC nao encontrado! Confira SDA 8, SCL 9, VCC e GND do DS3231");
        return;
    }
    Serial.println("RTC DS3231 encontrado");
    // Sem hora guardada, usa a hora da compilação até o NTP acertar
    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// Hora para o display: do DS3231 ou, sem ele, do relógio interno do ESP32 acertado pelo NTP
bool horaAtual(int& hora, int& minuto) {
    if (rtcOk) {
        DateTime agora = rtc.now();
        hora = agora.hour();
        minuto = agora.minute();
        return true;
    }
    struct tm agora;
    if (!getLocalTime(&agora, 0)) return false;  // relógio interno ainda não acertado
    hora = agora.tm_hour;
    minuto = agora.tm_min;
    return true;
}

// Acerta o RTC pela hora da internet (NTP). Sem Wi-Fi ou sem resposta, o RTC segue sozinho
void sincronizaRTC() {
    if (WiFi.status() != WL_CONNECTED) return;
    // Acerta também o relógio interno do ESP32, usado quando o RTC não responde
    configTime(FUSO_BRASILIA, 0, "pool.ntp.org", "a.st1.ntp.br");
    struct tm agora;
    if (!getLocalTime(&agora, 5000)) {
        Serial.println("Falha no NTP, mantendo a hora do RTC");
        return;
    }
    ultimoNTP = millis();
    if (!rtcOk) {
        Serial.println("Relogio interno acertado pelo NTP (sem RTC)");
        return;
    }
    rtc.adjust(DateTime(agora.tm_year + 1900, agora.tm_mon + 1, agora.tm_mday,
                        agora.tm_hour, agora.tm_min, agora.tm_sec));
    Serial.println("RTC acertado pelo NTP");
}

void handleRTC() {
    if (millis() - ultimoNTP >= INTERVALO_NTP) {
        ultimoNTP = millis();  // evita tentar de novo a cada loop se o NTP falhar
        sincronizaRTC();
    }
    if (!rtcOk || millis() - ultimaHoraSerial < INTERVALO_HORA_SERIAL) return;
    ultimaHoraSerial = millis();
    DateTime agora = rtc.now();
    Serial.printf("Hora: %02d/%02d/%04d %02d:%02d:%02d\n", agora.day(), agora.month(), agora.year(),
                  agora.hour(), agora.minute(), agora.second());
}

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");

    telaMensagem("Conectando Wi-Fi...", "Rede: " + String(SSID));

    WiFi.begin(SSID, PASSWORD);

    int pontos = 0;
    while (WiFi.status() != WL_CONNECTED) {
        String aguarde = "Aguarde";
        for (int i = 0; i < pontos; i++) aguarde += ".";
        escreveLinha(160, aguarde);

        pontos = (pontos + 1) % 4;
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.println(SSID);

    telaMensagem("Wi-Fi conectado!", "Rede: " + String(SSID));
    delay(1000);
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);

    Serial.print("* Conectando ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    telaMensagem("Conectando MQTT...", "Broker: " + String(BROKER_MQTT));

    int pontos = 0;
    while (!MQTT.connected()) {
        String aguarde = "Aguarde";
        for (int i = 0; i < pontos; i++) aguarde += ".";
        escreveLinha(160, aguarde);
        pontos = (pontos + 1) % 4;

        Serial.print(".");
        if (MQTT.connect(ID_MQTT)) {
            Serial.println();
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            delay(2000);
        }
    }

    telaMensagem("MQTT conectado!", "Broker: " + String(BROKER_MQTT));
    delay(1000);
}

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

void setup() {
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    setRGB(false, false, false);
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
    dht.begin();

    initSerial();

    initDisplay();
    initRTC();

    initWiFi();
    telaMensagem("Acertando a hora...", "Servidor: pool.ntp.org");  // o NTP pode levar alguns segundos
    sincronizaRTC();
    initMQTT();
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    desenhaMenu();
}

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
    handleRTC();
}

// O TFT não tem buffer: redesenhar a tela inteira a cada loop é lento e pisca,
// então cada parte do menu é redesenhada só quando precisa
void handleDisplay() {
    static unsigned long ultimoQuadro = 0;
    static unsigned long ultimosValores = 0;
    static unsigned long ultimaLeituraHora = 0;
    static int minutoAnterior = -1;
    static float tempAnterior = NAN, umidAnterior = NAN;
    static int luzAnterior = -1;
    static Nivel niveisAnteriores[QTD_ANOM] = { NIVEL_OK, NIVEL_OK, NIVEL_OK };

    if (millis() - ultimoQuadro >= INTERVALO_QUADRO) {
        ultimoQuadro = millis();
        desenhaAnimacoes();
    }

    // Lê a hora só uma vez por segundo para não ocupar o I2C a cada loop
    if (millis() - ultimaLeituraHora >= 1000) {
        ultimaLeituraHora = millis();
        int hora, minuto = -1;
        horaAtual(hora, minuto);
        if (minuto != minutoAnterior) {
            minutoAnterior = minuto;
            desenhaHora();
        }
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

// Por enquanto o touch só informa no Serial a posição tocada, o que serve para calibrar a tela
// antes de criar botões. No XPT2046 os valores são brutos (0 a 4095)
void handleTouch() {
    static bool tocando = false;
    if (!touch.touched()) {
        tocando = false;
        return;
    }
    if (tocando) return;
    tocando = true;
    TS_Point p = touch.getPoint();
    Serial.printf("- Toque em x=%d y=%d (pressao %d)\n", p.x, p.y, p.z);
}

void reconectWiFi() {
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print(".");
    }
    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.println(SSID);
}

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

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
    if (millis() - ultimaPublicacaoEstado < INTERVALO_PUBLICACAO) return;
    ultimaPublicacaoEstado = millis();

    String alerta = "";
    for (int i = 0; i < QTD_ANOM; i++) {
        if (niveis[i] == NIVEL_OK) continue;
        if (alerta.length()) alerta += ";";
        alerta += String(NOMES_ANOM[i]) + (niveis[i] == NIVEL_CRITICO ? ":critico" : ":alerta");
    }
    if (alerta.length() == 0) alerta = "ok";

    String estado = "s|" + corAtual + "|m|" + (modoManual ? "manual" : "auto") + "|a|" + alerta;
    MQTT.publish(TOPICO_PUBLISH_1, estado.c_str());
    Serial.print("- Estado enviado ao broker: ");
    Serial.println(estado);
}

void InitOutput() {
}

void reconnectMQTT() {
    while (!MQTT.connected()) {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT)) {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPICO_SUBSCRIBE);
        } else {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Haverá nova tentativa de conexão em 2s");
            delay(2000);
        }
    }
}

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

Nivel classifica(float valor, const Faixa& f) {
    if (isnan(valor)) return NIVEL_OK;
    if (valor < f.critMin || valor > f.critMax) return NIVEL_CRITICO;
    if (valor < f.alertaMin || valor > f.alertaMax) return NIVEL_ALERTA;
    return NIVEL_OK;
}

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

    corAtual = (pior == NIVEL_CRITICO) ? "red" : "yellow";
    bool aceso = (millis() / INTERVALO_PISCA) % 2 == 0;
    aplicaCor(aceso ? corAtual : String("off"));
}

void handleBuzzer() {
    static int somAtual = -1;
    static uint8_t passo = 0;
    static unsigned long inicioPasso = 0;

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

    if (millis() - inicioPasso < PADROES[somAtual].notas[passo].ms) return;

    passo++;
    if (passo >= PADROES[somAtual].qtd) {
        passo = 0;
        somAtual = proximaAtiva(somAtual);
    }
    tocaPasso();
}

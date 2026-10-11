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

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <RTClib.h>
#include <XPT2046_Touchscreen.h>
#include <DHT.h>

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

const uint8_t carinhaFeliz[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x1F, 0xF8, 0x00,
  0x00, 0x7C, 0x3E, 0x00,
  0x01, 0xC0, 0x03, 0x80,
  0x03, 0x80, 0x01, 0xC0,
  0x06, 0x00, 0x00, 0x60,
  0x0C, 0x00, 0x00, 0x30,
  0x18, 0x00, 0x00, 0x18,
  0x18, 0x00, 0x00, 0x18,
  0x30, 0x70, 0x0E, 0x0C,
  0x20, 0xF8, 0x1F, 0x04,
  0x60, 0xF8, 0x1F, 0x06,
  0x60, 0xF8, 0x1F, 0x06,
  0x60, 0x70, 0x0E, 0x06,
  0x40, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x02,
  0x40, 0x00, 0x00, 0x02,
  0x41, 0x00, 0x00, 0x82,
  0x61, 0x80, 0x01, 0x86,
  0x60, 0x80, 0x01, 0x06,
  0x60, 0xC0, 0x03, 0x06,
  0x20, 0x60, 0x06, 0x04,
  0x30, 0x38, 0x1C, 0x0C,
  0x18, 0x1F, 0xF8, 0x18,
  0x18, 0x07, 0xE0, 0x18,
  0x0C, 0x01, 0x80, 0x30,
  0x06, 0x00, 0x00, 0x60,
  0x03, 0x80, 0x01, 0xC0,
  0x01, 0xC0, 0x03, 0x80,
  0x00, 0x7C, 0x3E, 0x00,
  0x00, 0x1F, 0xF8, 0x00,
  0x00, 0x00, 0x00, 0x00
};

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

// drawBitmap do GFX não tem escala: cada pixel do bitmap vira um quadrado de 'escala' px
void desenhaBitmapEscalado(int x, int y, const uint8_t* bitmap, int w, int h, int escala, uint16_t cor) {
    int bytesPorLinha = (w + 7) / 8;
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            uint8_t byte = pgm_read_byte(&bitmap[j * bytesPorLinha + i / 8]);
            if (byte & (0x80 >> (i % 8)))
                display.fillRect(x + i * escala, y + j * escala, escala, escala, cor);
        }
    }
}

void initDisplay() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
    display.begin();
    display.setRotation(1);  // paisagem: 320x240
    display.fillScreen(COR_FUNDO);
    touch.begin();
    touch.setRotation(1);
}

void initRTC() {
    Wire.begin(RTC_SDA, RTC_SCL);
    rtcOk = rtc.begin(&Wire);
    if (!rtcOk) {
        Serial.println("RTC nao encontrado!");
        return;
    }
    // Sem hora guardada, usa a hora da compilação até o NTP acertar
    if (rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// Acerta o RTC pela hora da internet (NTP). Sem Wi-Fi ou sem resposta, o RTC segue sozinho
void sincronizaRTC() {
    if (!rtcOk || WiFi.status() != WL_CONNECTED) return;
    configTime(FUSO_BRASILIA, 0, "pool.ntp.org", "a.st1.ntp.br");
    struct tm agora;
    if (!getLocalTime(&agora, 5000)) {
        Serial.println("Falha no NTP, mantendo a hora do RTC");
        return;
    }
    rtc.adjust(DateTime(agora.tm_year + 1900, agora.tm_mon + 1, agora.tm_mday,
                        agora.tm_hour, agora.tm_min, agora.tm_sec));
    ultimoNTP = millis();
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
    delay(1500);

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
    delay(3000);
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);

    Serial.print("* Conectando ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    telaMensagem("Conectando MQTT...", "Broker: " + String(BROKER_MQTT));
    delay(1500);

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
    delay(3000);
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
    sincronizaRTC();
    initMQTT();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    display.fillScreen(COR_FUNDO);
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

// O TFT não tem buffer como o OLED: redesenhar a tela inteira a cada loop é lento e pisca,
// então só as linhas que mudaram são redesenhadas
void handleDisplay() {
    static bool primeiraVez = true;
    static bool wifiAnterior = false;
    static bool mqttAnterior = false;

    bool wifiOk = WiFi.status() == WL_CONNECTED;
    bool mqttOk = MQTT.connected();

    if (primeiraVez) {
        desenhaBitmapEscalado(112, 110, carinhaFeliz, 32, 32, 3, COR_TEXTO);
    }
    if (primeiraVez || wifiOk != wifiAnterior) {
        escreveLinha(30, wifiOk ? "WiFi: Conectado" : "WiFi: Desconectado", wifiOk ? COR_OK : COR_ERRO);
        wifiAnterior = wifiOk;
    }
    if (primeiraVez || mqttOk != mqttAnterior) {
        escreveLinha(60, mqttOk ? "MQTT: Conectado" : "MQTT: Desconectado", mqttOk ? COR_OK : COR_ERRO);
        mqttAnterior = mqttOk;
    }
    primeiraVez = false;
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

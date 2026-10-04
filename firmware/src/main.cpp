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
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT();
void EnviaEstadoOutputMQTT();
void handleLuminosity();
void reconnectMQTT();
void avaliaTriggers();
void handleLED();
void handleBuzzer();

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

// Pinos do LED RGB (KY-016)
const int PIN_R = 10;
const int PIN_G = 11;
const int PIN_B = 12;

// Pino do buzzer
const int PIN_BUZZER = 4;

// ===================== Triggers =====================
// Dentro de [alertaMin, alertaMax] = normal
// Entre alerta e crítico = ALERTA (LED amarelo piscando)
// Abaixo de critMin ou acima de critMax = CRÍTICO (LED vermelho piscando)
struct Faixa { float critMin, alertaMin, alertaMax, critMax; };
const Faixa FAIXA_TEMP = {  8.0, 10.0, 16.0, 18.0 };  // °C
const Faixa FAIXA_UMID = { 40.0, 50.0, 75.0, 85.0 };  // %
const Faixa FAIXA_LUZ  = { -1.0, -1.0, 30.0, 60.0 };  // % (só limite superior: adega deve ficar escura)

enum Nivel { NIVEL_OK = 0, NIVEL_ALERTA = 1, NIVEL_CRITICO = 2 };
enum Anomalia { ANOM_TEMP = 0, ANOM_UMID = 1, ANOM_LUZ = 2, QTD_ANOM = 3 };
const char* NOMES_ANOM[QTD_ANOM] = { "temp", "umid", "luz" };
Nivel niveis[QTD_ANOM] = { NIVEL_OK, NIVEL_OK, NIVEL_OK };

float temperatura = NAN;
float umidade = NAN;
int luminosidade = 0;

// Alerta sonoro: um padrão diferente para cada tipo de anomalia (freq 0 = silêncio)
struct Nota { uint16_t freq; uint16_t ms; };
const Nota SOM_TEMP[] = { {2000, 150}, {0, 100}, {2000, 150}, {0, 1100} };                          // 2 bipes agudos curtos
const Nota SOM_UMID[] = { {900, 700}, {0, 800} };                                                     // 1 bipe grave longo
const Nota SOM_LUZ[]  = { {1500, 80}, {0, 80}, {1500, 80}, {0, 80}, {1500, 80}, {0, 1100} };          // 3 bipes rápidos
struct Padrao { const Nota* notas; uint8_t qtd; };
const Padrao PADROES[QTD_ANOM] = { {SOM_TEMP, 4}, {SOM_UMID, 2}, {SOM_LUZ, 6} };

// Modo do LED: auto (segue os triggers) ou manual (cor escolhida pelo usuário via FIWARE)
bool modoManual = false;
String corManual = "off";
const unsigned long INTERVALO_PISCA = 500;
const unsigned long INTERVALO_PUBLICACAO = 1000;
unsigned long ultimaPublicacaoEstado = 0;
unsigned long ultimaPublicacaoLuz = 0;

// Configurações - variáveis editáveis
const char* default_SSID = "";
const char* default_PASSWORD = "";
const char* default_BROKER_MQTT = "";
const int default_BROKER_PORT = 1883;
const char* default_TOPICO_SUBSCRIBE = "/TEF/lamp001/cmd";
const char* default_TOPICO_PUBLISH_1 = "/TEF/lamp001/attrs";
const char* default_TOPICO_PUBLISH_2 = "/TEF/lamp001/attrs/l";
const char* default_ID_MQTT = "fiware_001";
const char* topicPrefix = "lamp001";

// Variáveis para configurações editáveis
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

// Função para setar a cor do LED RGB
void setRGB(int r, int g, int b) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
}

// Acende uma das cores pré-definidas. Retorna false se o nome não for uma cor conhecida.
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

void initSerial() {
    Serial.begin(115200);
}

void initWiFi() {
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.println(F("Conectando Wi-Fi..."));
    display.setCursor(0, 28);
    display.print(F("Rede: "));
    display.println(SSID);
    display.display();
    delay(1500);

    WiFi.begin(SSID, PASSWORD);

    int pontos = 0;
    while (WiFi.status() != WL_CONNECTED) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 4);
        display.print(F("Rede: "));
        display.println(SSID);

        display.setCursor(0, 28);
        display.print(F("Aguarde"));
        for (int i = 0; i < pontos; i++) display.print(".");

        display.display();
        pontos = (pontos + 1) % 4;
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.println(SSID);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 4);
    display.println(F("Wi-Fi conectado!"));

    display.setCursor(0, 28);
    display.print(F("Rede: "));
    display.println(SSID);

    display.display();
    delay(3000);
}

void initMQTT() {
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);
    MQTT.setCallback(mqtt_callback);

    Serial.print("* Conectando ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 4);
    display.println(F("Conectando MQTT..."));
    display.setCursor(0, 28);
    display.print(F("Broker: "));
    display.println(BROKER_MQTT);
    display.display();
    delay(1500);

    int pontos = 0;
    while (!MQTT.connected()) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);

        display.setCursor(0, 4);
        display.print(F("Broker: "));
        display.println(BROKER_MQTT);

        display.setCursor(0, 28);
        display.print(F("Aguarde"));
        for (int i = 0; i < pontos; i++) display.print(".");

        display.display();
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

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 4);
    display.println(F("MQTT conectado!"));

    display.setCursor(0, 28);
    display.print(F("Broker: "));
    display.println(BROKER_MQTT);

    display.display();
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
    // Inicializa pinos do LED RGB
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    setRGB(false, false, false);
    pinMode(PIN_BUZZER, OUTPUT);
    noTone(PIN_BUZZER);
    dht.begin();

    initSerial();

    Wire.begin(8, 9);  // SDA, SCL (padrão do ESP32-S3)
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("Falha ao inicializar o SSD1309!"));
        for (;;);
    }
    display.clearDisplay();
    display.display();

    initWiFi();
    initMQTT();
    delay(5000);
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
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

    display.clearDisplay();

    // Status lado esquerdo
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 8);
    display.print(F("WiFi: "));
    if (WiFi.status() == WL_CONNECTED)
        display.println(F("Conectado"));
    else
        display.println(F("Desconectado"));

    display.setCursor(0, 24);
    display.print(F("MQTT: "));
    if (MQTT.connected())
        display.println(F("Conectado"));
    else
        display.println(F("Desconectado"));

    // Carinha feliz no canto direito
    display.drawBitmap(96, 5, carinhaFeliz, 32, 32, SSD1306_WHITE);

    display.display();
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
        corManual = corAtual;  // começa na cor que estava acesa
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

    // a = anomalias ativas, ex.: "temp:critico;luz:alerta" ou "ok"
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
    // Não usado mais — RGB inicializado no setup()
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
    const int potPin = 1;  // GPIO1 = ADC1_CH0 no ESP32-S3
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
    if (isnan(valor)) return NIVEL_OK;  // sem leitura ainda
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
        // Tudo normal: verde fixo
        aplicaCor("green");
        corAtual = "green";
        return;
    }

    // Anomalia: pisca amarelo (alerta) ou vermelho (crítico) até o valor voltar ao normal
    corAtual = (pior == NIVEL_CRITICO) ? "red" : "yellow";
    bool aceso = (millis() / INTERVALO_PISCA) % 2 == 0;
    aplicaCor(aceso ? corAtual : String("off"));
}

void handleBuzzer() {
    static int somAtual = -1;  // -1 = buzzer parado
    static uint8_t passo = 0;
    static unsigned long inicioPasso = 0;

    // Próxima anomalia ativa depois de 'atual' (rodízio quando há mais de uma)
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

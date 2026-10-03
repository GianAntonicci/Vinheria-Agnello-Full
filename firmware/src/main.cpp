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
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

void mqtt_callback(char* topic, byte* payload, unsigned int length);
void VerificaConexoesWiFIEMQTT();
void EnviaEstadoOutputMQTT();
void handleLuminosity();
void reconnectMQTT();

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

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
const int PIN_R = 18;
const int PIN_G = 19;
const int PIN_B = 23;

// Configurações - variáveis editáveis
const char* default_SSID = "Galaxy S26 Ultra Gianluca";
const char* default_PASSWORD = "1008100810";
const char* default_BROKER_MQTT = "54.236.175.117";
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

WiFiClient espClient;
PubSubClient MQTT(espClient);
String corAtual = "off";

// Função para setar a cor do LED RGB
void setRGB(int r, int g, int b) {
    analogWrite(PIN_R, r);
    analogWrite(PIN_G, g);
    analogWrite(PIN_B, b);
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

void setup() {
    // Inicializa pinos do LED RGB
    pinMode(PIN_R, OUTPUT);
    pinMode(PIN_G, OUTPUT);
    pinMode(PIN_B, OUTPUT);
    setRGB(false, false, false);

    initSerial();

    Wire.begin(21, 22);
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

    if (msg.equals(prefix + "red|")) {
        setRGB(255, 0, 0); corAtual = "red";
    } else if (msg.equals(prefix + "green|")) {
        setRGB(0, 255, 0); corAtual = "green";
    } else if (msg.equals(prefix + "blue|")) {
        setRGB(0, 0, 255); corAtual = "blue";
    } else if (msg.equals(prefix + "yellow|")) {
        setRGB(255, 50, 0); corAtual = "yellow";
    } else if (msg.equals(prefix + "cyan|")) {
        setRGB(0, 255, 80); corAtual = "cyan";
    } else if (msg.equals(prefix + "magenta|")) {
        setRGB(255, 0, 100); corAtual = "magenta";
    } else if (msg.equals(prefix + "white|")) {
        setRGB(255, 255, 255); corAtual = "white";
    } else if (msg.equals(prefix + "off|")) {
        setRGB(0, 0, 0); corAtual = "off";
    }
}

void VerificaConexoesWiFIEMQTT() {
    if (!MQTT.connected())
        reconnectMQTT();
    reconectWiFi();
}

void EnviaEstadoOutputMQTT() {
    if (corAtual == "off") {
        MQTT.publish(TOPICO_PUBLISH_1, "s|off");
        Serial.println("- LED RGB desligado");
    } else {
        String estado = "s|" + corAtual;
        MQTT.publish(TOPICO_PUBLISH_1, estado.c_str());
        Serial.print("- LED RGB cor: ");
        Serial.println(corAtual);
    }
    Serial.println("- Estado do LED RGB enviado ao broker!");
    delay(1000);
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
    const int potPin = 34;
    int sensorValue = analogRead(potPin);
    int luminosity = map(sensorValue, 0, 4095, 100, 0);
    String mensagem = String(luminosity);
    Serial.print("Valor da luminosidade: ");
    Serial.println(mensagem.c_str());
    MQTT.publish(TOPICO_PUBLISH_2, mensagem.c_str());
}
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

#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <Preferences.h>
#include <Fonts/FreeSans12pt7b.h>      // já vem com a Adafruit GFX
#include <Fonts/FreeMonoBold12pt7b.h>

// Ícones animados exportados pelo Lopaka (32x32, 28 quadros, 1 bit por pixel). Ficam em firmware/include/
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
void handleRTC();
bool horaAtual(int& hora, int& minuto);

// Display no SPI padrão. O touch tem fios só dele, com SPI feito por software:
// dividindo o barramento com o display ele só devolvia 4095
#define TFT_SCK   13
#define TFT_MOSI  14
#define TFT_CS    5
#define TFT_DC    6
#define TFT_RST   7
#define TFT_LED   18  // luz de fundo (backlight)
#define TFT_MISO  47  // sem fio: o display só recebe dados, mas o SPI precisa de um pino que não seja o 13
#define TOUCH_CLK 39
#define TOUCH_DO  40
#define TOUCH_DIN 41
#define TOUCH_CS  21  // no 16 o XPT2046 nunca respondeu, em dois módulos diferentes
#define TOUCH_IRQ 17
// RTC DS3231 no I2C
#define RTC_SDA 8
#define RTC_SCL 9
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// Só escrita no display: o SDO(MISO) dele fica sem ligar
Adafruit_ILI9341 display(TFT_CS, TFT_DC, TFT_RST);

// Deslocamento das telas do Lopaka (320x240). Zero no display de 2,8"; num display maior centraliza
const int OX = 0;
const int OY = 0;

// Leitura direta do XPT2046. Comandos com os 2 bits finais em 0: entre as leituras o chip
// desliga e deixa o T_IRQ ativo (vai a LOW quando a tela é tocada)
const uint8_t XPT_X  = 0xD0;
const uint8_t XPT_Y  = 0x90;
const uint8_t XPT_Z1 = 0xB0;
const uint8_t XPT_Z2 = 0xC0;
const int PRESSAO_MINIMA = 400;

// SPI por software: o XPT2046 aceita até 2,5 MHz e o digitalWrite fica bem abaixo disso
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

uint16_t leXPT(uint8_t comando) {
    trocaByteXPT(comando);
    delayMicroseconds(2);  // tempo de conversão
    uint16_t alto = trocaByteXPT(0);
    uint16_t baixo = trocaByteXPT(0);
    return ((alto << 8) | baixo) >> 3;  // resultado de 12 bits
}

// Devolve true se há toque, com x, y brutos (0 a 4095) e a pressão z
bool leToque(int& x, int& y, int& z) {
    if (digitalRead(TOUCH_IRQ) == HIGH) return false;
    digitalWrite(TOUCH_CS, LOW);
    int z1 = leXPT(XPT_Z1);
    int z2 = leXPT(XPT_Z2);
    z = z1 + 4095 - z2;
    long somaX = 0, somaY = 0;
    for (int i = 0; i < 4; i++) {  // média de 4 leituras, o touch resistivo oscila
        somaX += leXPT(XPT_X);
        somaY += leXPT(XPT_Y);
    }
    digitalWrite(TOUCH_CS, HIGH);
    x = somaX / 4;
    y = somaY / 4;
    return z >= PRESSAO_MINIMA;
}

// DS3231: bateria própria e mais preciso
RTC_DS3231 rtc;
bool rtcOk = false;
const long FUSO_BRASILIA = -3 * 3600;  // UTC-3, sem horário de verão
const unsigned long INTERVALO_NTP = 6UL * 3600 * 1000;  // reacerta o RTC a cada 6 h
const unsigned long INTERVALO_HORA_SERIAL = 30000;
unsigned long ultimoNTP = 0;
unsigned long ultimaHoraSerial = 0;

// Cores do display (RGB565). Fundo, texto, borda e hora mudam com o tema (aplicaTema)
uint16_t COR_FUNDO = 0x0000;
uint16_t COR_TEXTO = 0xFFFF;
uint16_t COR_HORA  = 0xFF47;  // amarelo claro do layout
uint16_t COR_BORDA = 0xEF7D;  // cinza claro do layout
const uint16_t COR_OK    = 0x07E0;
const uint16_t COR_ERRO  = 0xF800;
const uint16_t COR_ALERTA = 0xFFE0;

// Configurações do usuário, guardadas na flash (Preferences) para valerem depois de religar
Preferences prefs;
bool temaClaro = false;
bool buzzerMudo = false;

enum Tela { TELA_MENU, TELA_CONFIG, TELA_WIFI };
Tela telaAtual = TELA_MENU;

// Calibração do touch: valores brutos do XPT2046 nos cantos da tela (veja o Serial ao tocar)
// Os dois eixos invertidos: com a rotação 3 o valor bruto cresce da direita para a esquerda e de baixo para cima
const int TOUCH_X_MIN = 3800;
const int TOUCH_X_MAX = 300;
const int TOUCH_Y_MIN = 3800;
const int TOUCH_Y_MAX = 300;

// Ícones animados das configurações: em vez de girar sem parar, vão do quadro 0 ao último
// quando a opção liga e voltam quando desliga. Se ficar invertido, troque estes valores
const int QUADRO_DESLIGADO = 0;
const int QUADRO_LIGADO = 27;
int quadroTema = QUADRO_DESLIGADO;
int quadroAudio = QUADRO_DESLIGADO;

// Posições do menu principal (vindas do Lopaka, tela 320x240)
const int BARRA_X = 41 + OX;
const int BARRA_W = 246;
const int BARRA_H = 17;
const int BARRA_Y_TEMP = 90 + OY;
const int BARRA_Y_LUZ  = 136 + OY;
const int BARRA_Y_UMID = 183 + OY;
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
// x e y nas coordenadas do Lopaka (320x240); o deslocamento para o centro é somado aqui
void desenhaIcone(int x, int y, const uint8_t* quadro, uint16_t cor, int tamanho = 32) {
    static uint16_t buf[48 * 48];  // cabe o maior ícone (48x48)
    for (int i = 0; i < tamanho * tamanho; i++) {
        uint8_t b = pgm_read_byte(&quadro[i / 8]);  // 32 e 48 são múltiplos de 8: linhas sem sobra
        buf[i] = (b & (0x80 >> (i % 8))) ? cor : COR_FUNDO;
    }
    display.drawRGBBitmap(x + OX, y + OY, buf, tamanho, tamanho);
}

void desenhaAnimacoes() {
    int q = millis() / INTERVALO_QUADRO % 28;
    desenhaIcone(0, 83, (const uint8_t*)temperature_weather_32_32_28f_frames[q], 0x05FA);
    desenhaIcone(0, 128, (const uint8_t*)sun_weather_32_32_28f_frames[q], 0xFC00);
    desenhaIcone(1, 174, (const uint8_t*)windy_weather_32_32_28f_frames[q], 0x3A96);
    // Seta (ainda sem função) e engrenagem (abre as configurações)
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
    display.fillRect(34 + OX, 10 + OY, 252, 56, COR_FUNDO);
    // No Lopaka o texto é posicionado pelo topo (y 18); na GFX o cursor é a linha de base
    display.setCursor((display.width() - w) / 2 - x1, 18 + OY - y1);
    display.print(hora);
    display.setFont((const GFXfont*)NULL);
}

uint16_t corDoNivel(Nivel n) {
    if (n == NIVEL_CRITICO) return COR_ERRO;
    if (n == NIVEL_ALERTA) return COR_ALERTA;
    return COR_OK;
}

// Texto acima da barra e preenchimento proporcional ao valor (vMin..vMax), na cor do trigger
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
    if (largura > 0 && largura < 12) largura = 12;  // abaixo disso o canto arredondado deforma
    display.fillRoundRect(BARRA_X + 2, y + 2, interno, BARRA_H - 4, 6, COR_FUNDO);
    if (largura > 0)
        display.fillRoundRect(BARRA_X + 2, y + 2, largura, BARRA_H - 4, 6, corDoNivel(nivel));
}

void desenhaValores() {
    String t = isnan(temperatura) ? String("--") : String(temperatura, 1);
    String h = isnan(umidade) ? String("--") : String(umidade, 1);
    desenhaBarra(BARRA_Y_TEMP, "Temperatura: " + t + " \xF7" "C"  /* 0xF7 = grau na fonte padrão da GFX */, temperatura, 0, 40, niveis[ANOM_TEMP]);
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

void aplicaTema() {
    if (temaClaro) {
        COR_FUNDO = 0xFFFF;
        COR_TEXTO = 0x0000;
        COR_HORA  = 0xC280;  // laranja escuro, legível no branco
        COR_BORDA = 0x7BEF;  // cinza médio
    } else {
        COR_FUNDO = 0x0000;
        COR_TEXTO = 0xFFFF;
        COR_HORA  = 0xFF47;
        COR_BORDA = 0xEF7D;
    }
}

void carregaConfiguracoes() {
    prefs.begin("vinheria", false);
    temaClaro = prefs.getBool("temaClaro", false);
    buzzerMudo = prefs.getBool("mudo", false);
    aplicaTema();
    quadroTema = temaClaro ? QUADRO_LIGADO : QUADRO_DESLIGADO;
    quadroAudio = buzzerMudo ? QUADRO_LIGADO : QUADRO_DESLIGADO;
}

// O Lopaka posiciona o texto pelo topo; na GFX, com fonte livre, o cursor é a linha de base
void textoPeloTopo(const char* texto, int x, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(x + OX - x1, y + OY - y1);
    display.print(texto);
}

void desenhaIconesConfig() {
    desenhaIcone(260, 47, (const uint8_t*)toggle_32_32_28f_frames[quadroTema], COR_TEXTO);
    desenhaIcone(263, 84, (const uint8_t*)mute_32_32_28f_frames[quadroAudio], COR_TEXTO);
}

void desenhaConfig() {
    display.fillScreen(COR_FUNDO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo("WiFi/MQTT", 10, 18);
    textoPeloTopo("Tema", 10, 53);
    textoPeloTopo("Audio", 9, 90);
    // Fora do layout do Lopaka: a tela não tinha como voltar ao menu
    textoPeloTopo("< Voltar", 10, 205);
    display.setFont((const GFXfont*)NULL);
    display.drawBitmap(260 + OX, 15 + OY, image_arrow_bits, 35, 25, COR_TEXTO);
    desenhaIconesConfig();
}

// Linha "WiFi: ..." ou "MQTT: ...": rótulo na cor do texto e status em verde ou vermelho
void desenhaStatusConexao(const char* rotulo, bool ok, int x, int y) {
    display.fillRect(OX, y + OY - 4, 320, 28, COR_FUNDO);
    display.setFont(&FreeMonoBold12pt7b);
    display.setTextSize(1);
    display.setTextColor(COR_TEXTO);
    textoPeloTopo(rotulo, x, y);
    display.setTextColor(ok ? COR_OK : COR_ERRO);
    display.print(ok ? "Conectado" : "Desconectado");  // continua de onde o rótulo parou
    display.setFont((const GFXfont*)NULL);
}

void desenhaWifi() {
    display.fillScreen(COR_FUNDO);
    // Seta: volta para as configurações
    display.drawBitmap(15 + OX, 15 + OY, image_arrow_right_bits, 35, 25, COR_TEXTO);
    desenhaStatusConexao("WiFi: ", WiFi.status() == WL_CONNECTED, 14, 80);
    desenhaStatusConexao("MQTT: ", MQTT.connected(), 13, 117);
}

void abreTela(Tela tela) {
    telaAtual = tela;
    if (tela == TELA_MENU) desenhaMenu();
    else if (tela == TELA_CONFIG) desenhaConfig();
    else desenhaWifi();
}

// Leva o quadro atual um passo em direção ao alvo; devolve true se mudou
bool avancaQuadro(int& quadro, int alvo) {
    if (quadro == alvo) return false;
    quadro += (alvo > quadro) ? 1 : -1;
    return true;
}

// Testa a comunicação com o XPT2046 sem depender da película de toque: o canal de
// temperatura interna (comando 0x84) sempre dá um valor no meio da escala
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

void initDisplay() {
    pinMode(TFT_LED, OUTPUT);
    digitalWrite(TFT_LED, HIGH);
    SPI.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
    display.begin();
    display.setRotation(3);  // paisagem invertida: 320x240
    display.fillScreen(COR_FUNDO);
    pinMode(TOUCH_CLK, OUTPUT);
    digitalWrite(TOUCH_CLK, LOW);
    pinMode(TOUCH_DIN, OUTPUT);
    pinMode(TOUCH_DO, INPUT_PULLUP);  // sem o chip responder, a leitura vira 4095 em vez de 0
    pinMode(TOUCH_CS, OUTPUT);
    digitalWrite(TOUCH_CS, HIGH);
    pinMode(TOUCH_IRQ, INPUT_PULLUP);
    diagnosticoTouch();
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
    MQTT.setSocketTimeout(3);  // tentativa de reconexão não trava o loop por 15 s (padrão)
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
    carregaConfiguracoes();

    initDisplay();
    initRTC();

    initWiFi();
    telaMensagem("Acertando a hora...", "Servidor: pool.ntp.org");  // o NTP pode levar alguns segundos
    sincronizaRTC();
    initMQTT();
    MQTT.publish(TOPICO_PUBLISH_1, "s|off");
    abreTela(TELA_MENU);
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

void handleDisplayConfig() {
    static unsigned long ultimoQuadro = 0;
    if (millis() - ultimoQuadro < INTERVALO_QUADRO) return;
    ultimoQuadro = millis();
    bool mudou = avancaQuadro(quadroTema, temaClaro ? QUADRO_LIGADO : QUADRO_DESLIGADO);
    mudou |= avancaQuadro(quadroAudio, buzzerMudo ? QUADRO_LIGADO : QUADRO_DESLIGADO);
    if (mudou) desenhaIconesConfig();
}

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

void handleDisplay() {
    if (telaAtual == TELA_CONFIG) handleDisplayConfig();
    else if (telaAtual == TELA_WIFI) handleDisplayWifi();
    else handleDisplayMenu();
}

// O TFT não tem buffer: redesenhar a tela inteira a cada loop é lento e pisca,
// então cada parte do menu é redesenhada só quando precisa
void handleDisplayMenu() {
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

// Toque dentro do retângulo, com folga em volta porque o touch resistivo não é preciso
bool tocouEm(int x, int y, int rx, int ry, int rw, int rh) {
    const int FOLGA = 5;
    rx += OX;  // retângulos nas coordenadas do Lopaka
    ry += OY;
    return x >= rx - FOLGA && x < rx + rw + FOLGA && y >= ry - FOLGA && y < ry + rh + FOLGA;
}

void toqueMenu(int x, int y) {
    if (tocouEm(x, y, 288, 0, 32, 32)) abreTela(TELA_CONFIG);  // engrenagem
    // Seta do canto superior esquerdo: ainda sem função
}

void toqueConfig(int x, int y) {
    if (tocouEm(x, y, 260, 15, 35, 25)) {
        abreTela(TELA_WIFI);
    } else if (tocouEm(x, y, 260, 47, 32, 32)) {
        temaClaro = !temaClaro;
        prefs.putBool("temaClaro", temaClaro);
        aplicaTema();
        desenhaConfig();  // redesenha com as cores novas; o ícone anima até a nova posição
    } else if (tocouEm(x, y, 263, 84, 32, 32)) {
        buzzerMudo = !buzzerMudo;
        prefs.putBool("mudo", buzzerMudo);
        if (buzzerMudo) noTone(PIN_BUZZER);
    } else if (tocouEm(x, y, 0, 195, 140, 45)) {
        abreTela(TELA_MENU);
    }
}

// Age só no início de cada toque. O XPT2046 dá valores brutos (0 a 4095), convertidos para
// pixels pela calibração TOUCH_*; o Serial mostra os dois para ajustar
void handleTouch() {
    static bool tocando = false;
    int bx, by, bz;
    if (!leToque(bx, by, bz)) {
        tocando = false;
        return;
    }
    if (tocando) return;
    tocando = true;
    // Tudo 4095 ou tudo 0 = o XPT2046 não respondeu no SPI: o T_IRQ viu o toque, mas os dados não chegaram
    if ((bx >= 4000 && by >= 4000) || (bx == 0 && by == 0)) {
        Serial.printf("- Toque bruto x=%d y=%d z=%d\n", bx, by, bz);
        Serial.println("- Toque ignorado: XPT2046 sem resposta (confira T_DO, T_DIN, T_CLK e T_CS)");
        return;
    }
    // Paisagem: o eixo X bruto do XPT2046 corresponde à vertical da tela, por isso troca
    int x = constrain(map(by, TOUCH_X_MIN, TOUCH_X_MAX, 0, display.width()), 0, display.width() - 1);
    int y = constrain(map(bx, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, display.height()), 0, display.height() - 1);
    Serial.printf("- Toque bruto x=%d y=%d z=%d -> tela x=%d y=%d\n", bx, by, bz, x, y);
    if (telaAtual == TELA_MENU) toqueMenu(x, y);
    else if (telaAtual == TELA_CONFIG) toqueConfig(x, y);
    else if (tocouEm(x, y, 15, 15, 35, 25)) abreTela(TELA_CONFIG);  // seta da tela de WiFi/MQTT
}

// Sem travar o loop: com o Wi-Fi caído, tenta de novo a cada 10 s e segue com o resto
// (display, LED, buzzer), para a tela poder mostrar "Desconectado"
void reconectWiFi() {
    static unsigned long ultimaTentativa = 0;
    static bool estavaConectado = true;
    if (WiFi.status() == WL_CONNECTED) {
        if (!estavaConectado) {
            Serial.print("Conectado com sucesso na rede ");
            Serial.println(SSID);
        }
        estavaConectado = true;
        return;
    }
    estavaConectado = false;
    if (millis() - ultimaTentativa < 10000) return;
    ultimaTentativa = millis();
    Serial.println("Wi-Fi desconectado, tentando reconectar...");
    WiFi.disconnect();
    WiFi.begin(SSID, PASSWORD);
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
    reconectWiFi();
    if (!MQTT.connected() && WiFi.status() == WL_CONNECTED)
        reconnectMQTT();
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

// Uma tentativa a cada 5 s, sem ficar preso num while
void reconnectMQTT() {
    static unsigned long ultimaTentativa = 0;
    if (millis() - ultimaTentativa < 5000) return;
    ultimaTentativa = millis();
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
        Serial.println("Conectado com sucesso ao broker MQTT!");
        MQTT.subscribe(TOPICO_SUBSCRIBE);
    } else {
        Serial.println("Falha ao reconectar no broker. Nova tentativa em 5s");
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

    if (buzzerMudo) {
        if (somAtual >= 0) noTone(PIN_BUZZER);
        somAtual = -1;
        return;
    }

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

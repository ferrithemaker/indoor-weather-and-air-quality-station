// Incloure les llibreries a l'IDE d'Arduino (Programa/Incluir Libreria/Administrar Bibliotecas)
//   Sensor BME280
//     - Adafruit Unified Sensor
//     - Adafruit BME280 Library
//   Servidor MQTT
//     - PubSubClient
//   Pantalla
//     - Mini Grafx

#include <ESP8266WiFi.h>        // Gestió de la Wifi
#include <PubSubClient.h>       // Gestió del servidor MQTT
#include <Adafruit_Sensor.h>    // Llibreria general de sensors d'Adafruit
#include <Adafruit_BME280.h>    // Sensor de Temperatura, Humitat i Pressió
#include "SenseAir_S8.h"        // Sensor de CO2
#include <ILI9341_SPI.h>        // Pantalla TFT utilitzada
#include <MiniGrafx.h>          // Mode gràfic
#include "OpenSansFont.h"       // Font creada amb http://oleddisplay.squix.ch/#/home

// Pins bus I2C
// ESP8266 SCL pin D1
// ESP8266 SDA pin D2

// Pins port sèrie UART
#define ESP8266_Rx D3
#define ESP8266_Tx D4

// Definim pins del bus SPI per la pantalla TFT
// RST  +3.3V
// SCL  D5 (CLK)
// SDA  D7 (MOSI)
#define TFT_CS  D8
#define TFT_DC  D0

// WiFi network
const char* ssid = "****";
const char* password = "****";

// MQTT Server
const char* mqttServer = "io.adafruit.com";
const int mqttPort = 1883;
const char* mqttUser= "****";
const char* mqttPassword = "****";

// Accés al Dashboard https://io.adafruit.com/
// usuari: ****
// passwd: ****

WiFiClient airQClient;
PubSubClient client(airQClient);

// Indexació per utilitzar els colors de la paleta
#define MINI_BLACK  0
#define MINI_WHITE  1
#define MINI_GREEN  2
#define MINI_YELLOW 3
#define MINI_ORANGE 4
#define MINI_RED    5
#define MINI_BLUE   6

// Defineix la paleta de colors utilitzables, amb un màxim de 16
uint16_t palette[] = {
            ILI9341_BLACK,  // 0
            ILI9341_WHITE,  // 1
            ILI9341_GREEN,  // 2
            ILI9341_YELLOW, // 3
            ILI9341_ORANGE, // 4
            ILI9341_RED,    // 5
            ILI9341_BLUE    // 6
         };

int BITS_PER_PIXEL = 4; // 2^4 = 8 colors

// Definim dispositius
Adafruit_BME280 bme_sensor;
ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);

// Establim nivells de CO2
#define CO2_REGULAR 1000
#define CO2_DOLENT 1700
#define CO2_CRITIC 2500

float temperatura;
float pressio;
float humitat;
int co2, nou_co2;

int resetCount = 0;
boolean connexioWifi, connexioMqtt, actualitzatCO2;
String valoracioCO2;
int tabulacioCO2;
int screen = 1;

char sendtomqtt[10];

void connecta_mqtt(void) {
   Serial.println("Connectant amb el servidor MQTT ...");
   int timeout = 0;
   while (!client.connected()  && (timeout < 5)) {
      if (client.connect("airQClient", mqttUser, mqttPassword)) {
         Serial.println("Connectat amb l'MQTT Server");
         connexioMqtt = true;
      }
      else {
         Serial.print("Connexió MQTT fallida amb error ");   Serial.println(client.state());
         connexioMqtt = false;
         timeout++;
         delay(2000);    // timeout en 10 segons
      }
   }
}

void setup() {
   // Inicialitza la consola sèrie
   Serial.begin(9600);   while(!Serial);
   Serial.println("");

   // Connecta amb el Wifi
   Serial.print("Connectant al Wifi ");
   WiFi.mode(WIFI_STA);
   WiFi.begin(ssid, password);
   int timeout = 0;
   while ((WiFi.status() != WL_CONNECTED) && (timeout < 20)) {
      Serial.print(".");
      timeout++;
      delay(500);    // timeout en 10 segons
   }
   Serial.println("");
   if (WiFi.status() == WL_CONNECTED) {   
      Serial.println("WiFi connectat");
      Serial.print("IP address: ");   Serial.println(WiFi.localIP());
      // Defineix el servidor MQTT
      client.setServer(mqttServer, mqttPort);
   }

   // Inicialitza el BME280
   if (!bme_sensor.begin(0x76)) {
      Serial.println("No s'ha trobat el sensor BME280");
   }

   // Inicialitza el SenseAir S8
   CO2_Init(ESP8266_Rx, ESP8266_Tx, 9600);

   // Inicialitza la pantalla tft 2.4"
   gfx.init();
   gfx.fillBuffer(MINI_BLACK);
   gfx.setRotation(1);
   gfx.commit();
}

void loop() {
   // Llegeix el CO2 del SenseAir_S8 en ppm
   nou_co2 = getCO2();
   if (nou_co2 != -1) {     // gestió de lectures errònies
      actualitzatCO2 = true;
      co2 = nou_co2;
      Serial.print("CO2: ");   Serial.print(co2);   Serial.println(" ppm");
   }
   else actualitzatCO2 = false;

   // Llegeix Temp, Hum i Press del BME280 en C, %RH i hPa
   temperatura = (float) bme_sensor.readTemperature();
   Serial.print("Temperatura: ");   Serial.print(temperatura);   Serial.println(" ºC");
   humitat = (float) bme_sensor.readHumidity();
   Serial.print("Humitat: ");   Serial.print(humitat);   Serial.println(" %rH");
   pressio = (float) bme_sensor.readPressure() / (float) 100 ;
   Serial.print("Pressio: ");   Serial.print(pressio);   Serial.println(" hPa");

   // Comprova connexions Wifi i Mqtt
   if (WiFi.status() == WL_CONNECTED) {
      connexioWifi = true;
      if (!client.connected()) connecta_mqtt();
      client.loop();
   }
   else {
      connexioWifi = false;
      connexioMqtt = false;
   }

   // Publica els feeds al broker MQTT
   if (connexioWifi && connexioMqtt) {
      sprintf(sendtomqtt, "%d", co2);
      client.publish("ClubMaker/feeds/co2", sendtomqtt);
      delay(1000);    // Adafruit IO recomana menys de 2 publicacions per segon
      sprintf(sendtomqtt, "%f", temperatura);
      client.publish("ClubMaker/feeds/temperatura", sendtomqtt);
      delay(1000);
      sprintf(sendtomqtt, "%f", humitat);
      client.publish("ClubMaker/feeds/humitat", sendtomqtt);
      delay(1000);
      sprintf(sendtomqtt, "%f", pressio);
      client.publish("ClubMaker/feeds/pressio", sendtomqtt);
      delay(1000);
   }
   else delay(4000);

   // Actualitza els valors a la pantalla TFT
   gfx.fillBuffer(MINI_BLACK);
   gfx.setFont(Open_Sans_Regular_30);
   gfx.setColor(MINI_WHITE);
   gfx.drawString(35, 1, "Estació Ambiental");
   
   gfx.setColor(MINI_GREEN);   valoracioCO2 = "BO";   tabulacioCO2 = 89;
   if (co2 > CO2_REGULAR) { gfx.setColor(MINI_YELLOW);   valoracioCO2 = "REGULAR";   tabulacioCO2 = 42; }
   if (co2 > CO2_DOLENT)  { gfx.setColor(MINI_ORANGE);   valoracioCO2 = "DOLENT";    tabulacioCO2 = 50; }
   if (co2 > CO2_CRITIC)  { gfx.setColor(MINI_RED);      valoracioCO2 = "CRÍTIC";    tabulacioCO2 = 70; }
   if (screen == 1) {
      gfx.setFont(ArialMT_Plain_24);
      gfx.drawString(80, 50, String(co2) + " ppm CO2");
      gfx.setColor(MINI_GREEN);
      gfx.setFont(Open_Sans_Regular_36);
      gfx.drawString(90, 90, String(temperatura) + " ºC");
      gfx.setFont(ArialMT_Plain_24);
      gfx.drawString(50, 155, String((int)humitat) + " %rH");
      gfx.drawString(165, 155, String((int)pressio) + " hPa");
   }
   if (screen == 2) {
      gfx.setFont(Open_Sans_Regular_36);
      gfx.drawString(77, 45, String(co2) + " ppm");
      gfx.drawString(tabulacioCO2, 95, "CO   " + valoracioCO2);
      gfx.setFont(Open_Sans_Regular_30);
      gfx.drawString(tabulacioCO2 + 52, 101, "2");
      gfx.setFont(ArialMT_Plain_24);
      gfx.setColor(MINI_GREEN);
      gfx.drawString(50, 155, String(temperatura) + " ºC");
      gfx.drawString(187, 155, String((int)humitat) + " %rH");
   }
   if (screen < 2) screen++; 
   else screen = 1;

   gfx.setFont(ArialMT_Plain_16);
   gfx.setColor(MINI_BLUE);
   gfx.drawString(20, 195, "Club Maker - American Space Bcn");
   gfx.drawString(20, 215, "Biblioteca Can Fabra");

   gfx.setFont(ArialMT_Plain_10);
   if (actualitzatCO2) gfx.setColor(MINI_GREEN);
   else gfx.setColor(MINI_RED);
   gfx.drawString(293, 203, "CO2");
   if (connexioWifi) gfx.setColor(MINI_GREEN);
   else gfx.setColor(MINI_RED);
   gfx.drawString(293, 215, "WIFI");
   if (connexioMqtt) gfx.setColor(MINI_GREEN);
   else gfx.setColor(MINI_RED);
   gfx.drawString(289, 227, "MQTT");

   gfx.commit();

   // Adafruit IO Free té un límit de 30 publicacions per minut, amb el que requerirà d'un delay mínim de 8 segons (60 seg / 30 public * 4 valors)
   delay(26000);    // Refrescarem tot cada 30 seg = 26 + 1 + 1 + 1 + 1

   resetCount++;
   if (resetCount==720) { ESP.reset(); }       // Reset ESP cada 21600seg (6h)
}

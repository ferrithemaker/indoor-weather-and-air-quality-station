// Incloure les llibreries a l'IDE d'Arduino (Programa/Incluir Libreria/Administrar Bibliotecas)
//   Sensor BME280
//     - Adafruit Unified Sensor
//     - Adafruit BME280 Library
//   Adafruit IO
//     - Adafruit IO Arduino
//     - Adafruit MQTT Library
//     - ArduinoHttpClient
//   Pantalla
//     - Adafruit BusIO
//     - Adafruit ILI9341
//     - Adafruit GFX Library

#include <Wire.h>       // Bus I2C pels sensors
#include <SPI.h>      // Bus SPI per la pantalla
#include "AdafruitIO_WiFi.h"    // Per enviar al broker d'Adafruit IO
#include <Adafruit_Sensor.h>    // Llibreria general de sensors d'Adafruit
#include <Adafruit_BME280.h>    // Sensor de Temperatura, Humitat i Pressió
#include "SenseAir_S8.h"        // Sensor de CO2
#include <Adafruit_GFX.h>        // Pantalla TFT en mode gràfic
#include <Adafruit_ILI9341.h>    // Pantalla TFT
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

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

// Adafruit IO
#define IO_USERNAME  "****"
#define IO_KEY       "****"
#define WIFI_SSID    "****"
#define WIFI_PASS    "****"
// Accés al Dashboard https://io.adafruit.com/
// usuari: ****
// passwd: ****

// Definim Adafruit IO i els feeds
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *adafruit_temperatura = io.feed("temperatura");
AdafruitIO_Feed *adafruit_humitat = io.feed("humitat");
AdafruitIO_Feed *adafruit_pressio = io.feed("pressio");
AdafruitIO_Feed *adafruit_co2 = io.feed("CO2");

// Definim dispositius
Adafruit_BME280 bme_sensor;
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Establim nivells de CO2
#define CO2_REGULAR 1000
#define CO2_DOLENT 1700
#define CO2_CRITIC 2500

float temperatura;
float pressio;
float humitat;
float co2, nou_co2;

int timecount = 0;

void setup() {
   Serial.begin(9600);   while(!Serial);    // Inicialitza la consola sèrie
   Serial.println("");

   // Connecta amb io.adafruit.com (establiment Wifi inclòs)
   io.connect();
   Serial.print("Connectant amb Adafruit IO ");
   while (io.status() < AIO_CONNECTED) {
      Serial.print(".");
      delay(200);
   }
   Serial.println("");

   // Inicialitza el BME280
   if (!bme_sensor.begin(0x76)) {
      Serial.println("No s'ha trobat el sensor BME280");
   }

   // Inicialitza el SenseAir S8
   CO2_Init(ESP8266_Rx, ESP8266_Tx, 9600);

   // Inicialitza la pantalla tft 2.4"
   tft.begin();
   tft.setRotation(1);
   tft.fillScreen(ILI9341_BLACK);

   tft.setFont(&FreeSans18pt7b);
   tft.setTextColor(ILI9341_WHITE);
   tft.setCursor(25,25);
   tft.print("Estacio Ambiental");
   tft.drawLine(127, 4, 131, 1, ILI9341_WHITE);   // l'accent d'Estació
   tft.drawLine(128, 4, 132, 1, ILI9341_WHITE);
   tft.drawLine(129, 4, 133, 1, ILI9341_WHITE);

   tft.setFont(&FreeSansBold9pt7b);
   tft.setTextColor(ILI9341_BLUE);
   tft.setCursor(15,215);
   tft.print("Club Maker - American Space Bcn");
   tft.setCursor(15,235);
   tft.print("Biblioteca Can Fabra");

   tft.setFont(&FreeSans12pt7b);
}

void loop() {
   io.run();    // Manté el client connectat a io.adafruit.com i processa els missatges

   // Llegeix el CO2 del SenseAir_S8 en ppm
   nou_co2 = getCO2();
   if (nou_co2 != -1) {     // gestió de lectures errònies
      co2 = nou_co2;
      Serial.print("CO2: ");   Serial.print(co2);   Serial.println(" ppm");
      adafruit_co2->save(co2);
   }
   delay(1000);   // Adafruit IO recomana menys de 2 publicacions per segon

   // Llegeix Temp, Hum i Press del BME280 en C, %RH i hPa
   temperatura = (float) bme_sensor.readTemperature();
   Serial.print("Temperatura: ");   Serial.print(temperatura);   Serial.println(" ºC");
   adafruit_temperatura->save(temperatura);
   delay(1000);

   humitat = (float) bme_sensor.readHumidity();
   Serial.print("Humitat: ");   Serial.print(humitat);   Serial.println(" %rH");
   adafruit_humitat->save(humitat);
   delay(1000);

   pressio = (float) bme_sensor.readPressure() / (float) 100 ;
   Serial.print("Pressio: ");   Serial.print(pressio);   Serial.println(" hPa");
   adafruit_pressio->save(pressio);
   delay(1000);

   // Actualitza els valors a la pantalla TFT
   tft.fillRect(40, 52, 165, 24, ILI9341_BLACK);
   tft.setCursor(40,70);
   tft.setTextColor(ILI9341_GREEN);
   if (co2 > CO2_REGULAR) tft.setTextColor(ILI9341_YELLOW);
   if (co2 > CO2_DOLENT) tft.setTextColor(ILI9341_ORANGE);
   if (co2 > CO2_CRITIC) tft.setTextColor(ILI9341_RED);
   tft.print((int)co2);   tft.print(" ppm CO2");

   tft.setTextColor(ILI9341_GREEN);
   
   tft.fillRect(40, 87, 85, 20, ILI9341_BLACK);
   tft.setCursor(40,105);
   tft.print(temperatura);   tft.print(" C");

   tft.fillRect(40, 122, 80, 20, ILI9341_BLACK);
   tft.setCursor(40,140);
   tft.print((int)humitat);   tft.print(" %rH");

   tft.fillRect(40, 157, 110, 20, ILI9341_BLACK);
   tft.setCursor(40,175);
   tft.print((int)pressio);   tft.print(" hPa");

   // Adafruit IO Free té un límit de 30 publicacions per minut, amb el que requerirà d'un delay mínim de 8 segons (60 seg / 30 public * 4 valors)
   delay(26000);    // Refrescarem tot cada 30 seg = 26 + 1 + 1 + 1 + 1

   timecount++;
   if (timecount==720) { ESP.reset(); }       // Reset ESP cada 21600seg (6h)
}

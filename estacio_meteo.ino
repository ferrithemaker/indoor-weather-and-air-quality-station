// Incloure les llibreries a l'IDE d'Arduino (Programa/Incluir Libreria/Administrar Bibliotecas)
//   - Adafruit Unified Sensor
//   - Adafruit BME280 Library
//   Adafruit IO
//     - Adafruit IO Arduino
//     - Adafruit MQTT Library
//     - ArduinoHttpClient
//   - Adafruit **pantalla**
//   - Adafruit GFX Library

#include <Wire.h>       // Bus I2C pels sensors
//#include <SPI.h>      // Bus SPI per la pantalla
#include "AdafruitIO_WiFi.h"    // Per enviar al broker d'Adafruit IO
#include <Adafruit_Sensor.h>    // Llibreria general de sensors d'Adafruit
#include <Adafruit_BME280.h>    // Sensor de Temperatura, Humitat i Pressió
#include "SenseAir_S8.h"        // Sensor de CO2
//#include <Adafruit_ST7735.h>  // Pantalla TFT
//#include <Adafruit_GFX.h>     // Pantalla TFT en mode gràfic

// Definim pins pantalla TFT
//#define TFT_RST   D4     // not used
//#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D3
//#define TFT_DC    D0     // TFT DC  pin is connected to NodeMCU pin D0

// Pins bus I2C
// ESP8266 SCL pin D1
// ESP8266 SDA pin D2

// Adafruit IO
#define IO_USERNAME  "xxx"
#define IO_KEY       "xxx"
#define WIFI_SSID    "xxx"
#define WIFI_PASS    "xxx"

// Definim Adafruit IO i els feeds
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *adafruit_temperatura = io.feed("temperatura");
AdafruitIO_Feed *adafruit_humitat = io.feed("humitat");
AdafruitIO_Feed *adafruit_pressio = io.feed("pressio");
AdafruitIO_Feed *adafruit_co2 = io.feed("CO2");

// Definim dispositius
Adafruit_BME280 bme_sensor;
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

float temperatura;
float pressio;
float humitat;
float co2;

int timecount = 0;

void setup() {
   Serial.begin(9600);   while(!Serial);    // Inicialitza la consola sèrie
   Serial.println("");

   // Connecta amb io.adafruit.com
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
   // ESP8266 Tx pin D7
   // ESP8266 Rx pin D6
   CO2_Init(D6, D7, 9600);

   // Inicialitza la pantalla tft 2.4"
//   tft.initR(INITR_BLACKTAB);
//   tft.setRotation(3);
}

void loop() {
   io.run();    // Manté el client connectat a io.adafruit.com i processa els missatges
   // Adafruit IO Free té un límit de 30 publicacions per minut, amb el que requerirà un delay entre feed i feed enviat

   // Llegeix el CO2 del SenseAir_S8 en ppm
   co2 = getCO2();
   Serial.print("CO2: ");   Serial.print(co2);   Serial.println(" ppm");
   adafruit_co2->save(co2);
   delay(2000);

   // Llegeix Temp, Hum i Press del BME280 en C, %RH i hPa
   temperatura = (float) bme_sensor.readTemperature();
   Serial.print("Temperatura: ");   Serial.print(temperatura);   Serial.println(" ºC");
   adafruit_temperatura->save(temperatura);
   delay(2000);

   humitat = (float) bme_sensor.readHumidity();
   Serial.print("Humitat: ");   Serial.print(humitat);   Serial.println(" %rH");
   adafruit_humitat->save(humitat);
   delay(2000);

   pressio = (float) bme_sensor.readPressure() / (float) 100 ;
   Serial.print("Pressio: ");   Serial.print(pressio);   Serial.println(" hPa");
   adafruit_pressio->save(pressio);
   delay(2000);

   // Actualitza els valors a la pantalla TFT
/*   tft.fillScreen(ST7735_BLACK);
   tft.setTextSize(2);

   tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
   tft.setCursor(0,10);
   tft.print("Tmp C:");
   tft.setTextColor(ST7735_RED, ST7735_BLACK);
   tft.setCursor(80,10);
   tft.print(temperatura);

   tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
   tft.setCursor(0,30);
   tft.print("Hum %:");
   tft.setTextColor(ST7735_RED, ST7735_BLACK);
   tft.setCursor(80,30);
   tft.print(humitat);

   tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
   tft.setCursor(0,50);
   tft.print("HPa:");
   tft.setTextColor(ST7735_RED, ST7735_BLACK);
   tft.setCursor(55,50);
   tft.print(pressio);

   tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
   tft.setCursor(0,70);
   tft.print("CO2:");
   tft.setTextColor(ST7735_RED, ST7735_BLACK);
   tft.setCursor(55,70);
   tft.print(co2); */

   timecount++;     // Amb tots els delays del loop, incrementa cada 10 segons
   if (timecount==2000) { ESP.reset(); }       // Reset ESP cada 20000seg (5h:30m)
}

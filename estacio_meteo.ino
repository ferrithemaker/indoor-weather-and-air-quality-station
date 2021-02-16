#include <SPI.h>      // Bus SPI per la pantalla
#include <Wire.h>     // Bus I2C pels sensors
#include "AdafruitIO_WiFi.h"    // Per enviar al broker d'Adafruit IO
#include <Adafruit_Sensor.h>    // Llibreria general de sensors d'Adafruit
#include <Adafruit_BMP280.h>    // Sensor de Temperatura, Humitat i Pressió
#include "SenseAir_S8.h"        // Sensor de CO2
#include <Adafruit_ST7735.h>  // Pantalla TFT
#include <Adafruit_GFX.h>     // Pantalla TFT en mode gràfic

// Definim pins pantalla TFT
#define TFT_RST   D4     // not used
#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D3
#define TFT_DC    D0     // TFT DC  pin is connected to NodeMCU pin D0

// Adafruit IO 
#define IO_USERNAME  "xxx"
#define IO_KEY       "xxx"
#define WIFI_SSID    "xxx"
#define WIFI_PASS    "xxx"

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

// Adafruit feeds
AdafruitIO_Feed *adafruit_temperatura = io.feed("temperatura");
AdafruitIO_Feed *adafruit_humitat = io.feed("humitat");
AdafruitIO_Feed *adafruit_pressio = io.feed("pressio");
AdafruitIO_Feed *adafruit_co2 = io.feed("CO2");

// Inicialitzem dispositius
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Adafruit_BMP280 bmp;

float temperatura;
float pressio;
float humitat;
float co2;

int timecount = 0;

void setup() {
   Serial.begin(9600);    // Inicialitza la consola sèrie
   Wire.begin();          // Inicialitza el bus I2C

   // Connecta amb io.adafruit.com
   io.connect();
   while (io.status() < AIO_CONNECTED) {
      delay(500);
   }

   // Arrenca el BME280
   if (!bmp.begin()) {
	    while (1);
   }

   // Inicialitza el SenseAir S8
   CO2_Init(D6, D7, 9600);

   // Inicialitza la pantalla tft 2.4"
   tft.initR(INITR_BLACKTAB);
   tft.setRotation(3);
}

void loop() {
   io.run();
   // Adafruit IO is rate limited for publishing, so a delay is required in
   // between feed->save events.
   delay(2000);

   // Llegeix el CO2 del SenseAir_S8 en ppm
   co2 = getCO2_Raw();
   adafruit_co2->save(co2);
   delay(2000);

   // Llegeix Temp, Hum i Press del BME280 en C, %RH i hPa
   temperatura = (float) bmp.readTemperature();
   adafruit_temperatura->save(temperatura);
   delay(2000);

   humitat = (float) bmp.readHumidity();
   adafruit_humitat->save(humitat);
   delay(2000);
   
   pressio = (float) bmp.readPressure() / (float) 100 ;
   if (pressio > 880.0 && pressio < 1100.0) {
      adafruit_pressio->save(pressio);
   }
   delay(2000);
  
   // Actualitza els valors a la pantalla TFT
   tft.fillScreen(ST7735_BLACK);
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
   tft.print(co2);

   timecount++;     // Amb tots els delays del loop, incrementa cada 10 segons
   if (timecount==2000) { ESP.reset(); }       // Reset ESP cada 20000seg (5h:30m)
}

#include <Arduino.h>
#include <SPI.h>
#include "AdafruitIO_WiFi.h"
#include <Adafruit_Sensor.h>
#include <DHT_U.h> // DHT sensor
#include <Wire.h>
#include <Adafruit_TSL2561_U.h> // lux sensor
#include <Adafruit_BMP085.h> // presure sensor
#include "ccs811.h" // air quality sensor
#include <Adafruit_GFX.h>    // TFT screen
#include <Adafruit_ST7735.h>  // TFT screen
#include "MHZ19.h" // air quality sensor (another)

// air quality sensor (mhz19)

MHZ19 myMHZ19;  // Constructor for library

// Set TFT pinout

#define TFT_RST   D4     // not used
#define TFT_CS    D3     // TFT CS  pin is connected to NodeMCU pin D3
#define TFT_DC    D0     // TFT DC  pin is connected to NodeMCU pin D0

// set DHT pinout

#define DHTPIN            D6
#define DHTTYPE           DHT22

// Adafruit IO 

#define IO_USERNAME    "xxx"
#define IO_KEY         "xxx"


#define WIFI_SSID       "xxx"
#define WIFI_PASS       "xxx"


AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);


// Adafruit feeds

AdafruitIO_Feed *adafruit_humitat = io.feed("humitat");
AdafruitIO_Feed *adafruit_temperatura = io.feed("temperatura");
AdafruitIO_Feed *adafruit_pressio = io.feed("pressio");
AdafruitIO_Feed *adafruit_llum = io.feed("llum");
AdafruitIO_Feed *adafruit_vocs = io.feed("VOCs");
AdafruitIO_Feed *adafruit_eco2 = io.feed("eCO2");
AdafruitIO_Feed *adafruit_mhzco2 = io.feed("mhzCO2");

// Init sensors

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS,  TFT_DC, TFT_RST);

CCS811 ccs811; 

Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

Adafruit_BMP085 bmp;

DHT_Unified dht(DHTPIN, DHTTYPE);


float pressio;
float temperatura;
int mhzco2;

int timecount = 0;


void setup() {

  Serial.begin(9600);


  Wire.begin(); 
  
  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  ccs811.begin();
  
  // Start measuring
  ccs811.start(CCS811_MODE_1SEC);

  Serial.begin(9600);                                 
  myMHZ19.begin(Serial);                               
  myMHZ19.autoCalibration();


  dht.begin();
  
  // connect to io.adafruit.com
  io.connect();

  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    delay(500);
  }

  // light sensor setup
  
  if (!tsl.begin()) {
    while (1);
  }

  tsl.enableAutoRange(true);            /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */

  // presion sensor setup

  if (!bmp.begin()) {
	  while (1);
  }

  // tft setup
  tft.initR(INITR_BLACKTAB);     // initialize a ST7735S chip, black tab
  tft.setRotation(3);

}

void loop() {

  io.run();

  // Adafruit IO is rate limited for publishing, so a delay is required in
  // between feed->save events.

  delay(2000);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (!isnan(event.temperature)) {
    adafruit_temperatura->save(event.temperature);
  }

  // get CO2 grom MHZ19
  mhzco2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)
  adafruit_mhzco2->save(mhzco2);

  delay(2000);

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (!isnan(event.relative_humidity)) {
    adafruit_humitat->save(event.relative_humidity);
  }

  delay(2000);

  sensors_event_t l_event;
  tsl.getEvent(&l_event);
 
  /* Display the results (light is measured in lux) */
  if (l_event.light)
  {
    adafruit_llum->save(l_event.light);
  }

  delay(2000);

  pressio = (float) bmp.readPressure() / (float) 100 ;
  
  if (pressio < 1100.0 && pressio > 880.0) { adafruit_pressio->save(pressio); }

  delay(2000);

  temperatura = (float) bmp.readTemperature();
  

  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2,&etvoc,&errstat,&raw); 
  
  // Print measurement results based on status

  if(errstat == CCS811_ERRSTAT_OK) { 
    adafruit_eco2->save(eco2);
    delay(2000);
    adafruit_vocs->save(etvoc);
    delay(2000);
  }

  // print info on TFT screen

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
  tft.setCursor(80,30);                             
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  if (!isnan(event.relative_humidity)) {
    tft.print(event.relative_humidity);
  }
  tft.setCursor(0,50);
  tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  tft.print("HPa:");
  tft.setCursor(55,50);                             
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.print(pressio);
  tft.setCursor(0,70);
  tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  tft.print("Lux:");
  tft.setCursor(55,70);                            
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.print(int(l_event.light));
  tft.setCursor(0,90);
  tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  tft.print("CO2c:");
  tft.setCursor(120,90);
  tft.print("ppm");
  tft.setCursor(65,90);                            
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.print(eco2);
  tft.setCursor(0,110);
  tft.setTextColor(ST7735_BLUE, ST7735_BLACK);
  tft.print("CO2m:");
  tft.setCursor(120,110);
  tft.print("ppm");
  tft.setCursor(65,110);                             
  tft.setTextColor(ST7735_RED, ST7735_BLACK);
  tft.print(mhzco2);

  timecount++; // increases every 40 seconds
  if (timecount==500) { ESP.reset(); } // reset ESP every 20000sec (5h:30m)

}

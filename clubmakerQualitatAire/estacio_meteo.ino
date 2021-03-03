

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>    // Llibreria general de sensors d'Adafruit
#include <Adafruit_BME280.h>    // Sensor de Temperatura, Humitat i Pressió
#include "SenseAir_S8.h"        // Sensor de CO2
#include <ILI9341_SPI.h>
#include <MiniGrafx.h>


// Definim pins del bus SPI per la pantalla TFT
// RST  +3.3V
// SCL  D5 (CLK)
// SDA  D7 (MOSI)
#define TFT_CS  D8
#define TFT_DC  D0

// Pins bus I2C
// ESP8266 SCL pin D1
// ESP8266 SDA pin D2


// Accés al Dashboard https://io.adafruit.com/
// usuari: ClubMaker
// passwd: CanFabra2

// WiFi network
const char* ssid     = "xxx";
const char* password = "xxx";


const char* mqttServer = "io.adafruit.com";
const int mqttPort = 1883;
const char* mqttUser= "ClubMaker";
const char* mqttPassword = "xxx";

WiFiClient airQClient;
PubSubClient client(airQClient);


#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_YELLOW 2
#define MINI_BLUE 3

// defines the colors usable in the paletted 16 color frame buffer
uint16_t palette[] = {ILI9341_BLACK, // 0
                      ILI9341_WHITE, // 1
                      ILI9341_YELLOW, // 2
                      0x7E3C
                      }; //3

int SCREEN_WIDTH = 240;
int SCREEN_HEIGHT = 320;
// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors


// Definim dispositius
Adafruit_BME280 bme_sensor;

ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);


float temperatura;
float pressio;
float humitat;
float co2;

int timecount = 0;

char sendtomqtt[10];

void setup() {
   Serial.begin(9600);   while(!Serial);    // Inicialitza la consola sèrie
   Serial.println("");

   while (WiFi.status() != WL_CONNECTED) { // and the executes the WiFi.status() function to try to connect to WiFi Network. The program checks whether the WiFi.status() function returns a true value when its connect.
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");  // when the ESP32 connects to WiFi network, the sketch displays a message, WiFi connected and the IP address of the ESP32 shows on the serial monitor.
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  client.setServer(mqttServer, mqttPort);
  while (!client.connected()) {
    Serial.println("connecting to MQTT Server...");
    if (client.connect("airQClient", mqttUser, mqttPassword )) {
      Serial.println("Connected to MQTT Server");
    }
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

   // Inicialitza el BME280
   if (!bme_sensor.begin(0x76)) {
      Serial.println("No s'ha trobat el sensor BME280");
   }

   // Inicialitza el SenseAir S8
   // ESP8266 Tx pin D4
   // ESP8266 Rx pin D3
   CO2_Init(D3, D4, 9600);

   gfx.init();
   gfx.fillBuffer(MINI_BLUE);
   gfx.setRotation(1);
   gfx.commit();
   
}

void loop() {

   // Llegeix el CO2 del SenseAir_S8 en ppm
   co2 = getCO2();
   Serial.print("CO2: ");   Serial.print(co2);   Serial.println(" ppm");
   if (co2 != -1) {
    sprintf(sendtomqtt, "%i", int(co2));
    client.publish("ClubMaker/feeds/co2", sendtomqtt);
   }

   // Llegeix Temp, Hum i Press del BME280 en C, %RH i hPa
   temperatura = (float) bme_sensor.readTemperature();
   Serial.print("Temperatura: ");   Serial.print(temperatura);   Serial.println(" ºC");
   sprintf(sendtomqtt, "%f", temperatura);
   client.publish("ClubMaker/feeds/temperatura", sendtomqtt);

   humitat = (float) bme_sensor.readHumidity();
   Serial.print("Humitat: ");   Serial.print(humitat);   Serial.println(" %rH");
   sprintf(sendtomqtt, "%f", humitat);
   client.publish("ClubMaker/feeds/humitat", sendtomqtt);

   pressio = (float) bme_sensor.readPressure() / (float) 100 ;
   Serial.print("Pressio: ");   Serial.print(pressio);   Serial.println(" hPa");
   sprintf(sendtomqtt, "%f", pressio);
   client.publish("ClubMaker/feeds/pressio", sendtomqtt);

   // Actualitza els valors a la pantalla TFT
   
  gfx.fillBuffer(MINI_BLUE);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setFont(ArialMT_Plain_24);
  gfx.setColor(MINI_BLACK);
  gfx.drawString(80, 10, "Estació Meteo");
  gfx.drawString(50, 50, String(co2));
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(150, 50,"ppm CO2");
  gfx.setColor(MINI_BLACK);
  gfx.drawString(50, 90, String(humitat));
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(150,90,"%rH");
  gfx.setColor(MINI_BLACK);
  gfx.drawString(50, 130, String(pressio));
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(150,130,"hPa");
  gfx.setColor(MINI_BLACK);
  gfx.drawString(50, 170, String(temperatura));
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(150, 170,"ºC");
  gfx.setColor(MINI_BLACK);
  gfx.setFont(ArialMT_Plain_10);
  gfx.drawString(50, 215, "Club Maker - Biblio Can Fabra");
   
  gfx.commit();
  delay(10000);

}

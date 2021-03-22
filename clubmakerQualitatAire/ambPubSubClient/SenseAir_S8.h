// AirGradient Arduino Library for ESP8266
// =======================================
// This library makes it easy to read the sensor data from the Plantower PMS5003 PM2.5 sensor,
// the Senseair S8 and the SHT30/31 Temperature and Humidity sensor.
// Visit our blog for detailed build instructions and PCB layout.
// https://www.airgradient.com/blog/
// https://www.arduino.cc/reference/en/libraries/airgradient-air-quality-sensor/

#include <SoftwareSerial.h>

void CO2_Init(int rx_pin, int tx_pin, int baudRate);
int getCO2();

SoftwareSerial *SoftSerial_CO2;

/*
void setup() {
   Serial.begin(9600);
   CO2_Init(D6, D7, 9600);
}

void loop() {
   int CO2 = getCO2();
   Serial.print("C02: ");
   Serial.println(CO2);
   delay(5000);
}
*/

//START CO2 FUNCTIONS //
void CO2_Init(int rx_pin, int tx_pin, int baudRate) {
   Serial.println("Initializing CO2 ...");
   SoftSerial_CO2 = new SoftwareSerial(rx_pin, tx_pin);
   SoftSerial_CO2->begin(baudRate);

   if (getCO2() == -1) {
      Serial.println("CO2 Sensor Failed to Initialize ");
   }
   else {
      Serial.println("CO2 Successfully Initialized. Heating up ...");
      delay(1000);   // as much time as possible is recommended
   }
}

int getCO2() {
   const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
   byte CO2Response[] = {0,0,0,0,0,0,0};

   int retry = 0;
   while (!(SoftSerial_CO2->available())) {
      retry++;
      // keep sending request until we start to get a response
      SoftSerial_CO2->write(CO2Command, 7);
      delay(50);
      if (retry > 10) {
         return -1;
      }
   }

   int timeout = 0;
   while (SoftSerial_CO2->available() < 7) {
      timeout++;
      if (timeout > 10) {
         while(SoftSerial_CO2->available()) {
            SoftSerial_CO2->read();
         }
         break;
      }
      delay(50);
   }

   for (int i=0; i < 7; i++) {
      int byte = SoftSerial_CO2->read();
      if (byte == -1) {
         return -1;
      }
      CO2Response[i] = byte;
   }

   return CO2Response[3]*256 + CO2Response[4];
}
//END CO2 FUNCTIONS //

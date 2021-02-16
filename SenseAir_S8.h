#include <SoftwareSerial.h>

void CO2_Init(int rx_pin, int tx_pin, int baudRate);
int getCO2_Raw();

SoftwareSerial *_SoftSerial_CO2;
char Char_CO2[10];

struct CO2_READ_RESULT {
    int co2 = -1;
    bool success = false;
};

//START CO2 FUNCTIONS //
void CO2_Init(int rx_pin, int tx_pin, int baudRate) {
   Serial.println("Initializing CO2 ...");
   _SoftSerial_CO2 = new SoftwareSerial(rx_pin,tx_pin);
   _SoftSerial_CO2->begin(baudRate);

   if (getCO2_Raw() == -1) {
      Serial.println("CO2 Sensor Failed to Initialize ");
   }
   else {
      Serial.println("CO2 Successfully Initialized. Heating up for 10s");
      delay(10000);
   }
}

int getCO2_Raw() {
   int retry = 0;
   CO2_READ_RESULT result;
   const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
   byte CO2Response[] = {0,0,0,0,0,0,0};

   while (!(_SoftSerial_CO2->available())) {
      retry++;
      // keep sending request until we start to get a response
      _SoftSerial_CO2->write(CO2Command, 7);
      delay(50);
      if (retry > 10) {
         return -1;
      }
   }

   int timeout = 0; 
    
   while (_SoftSerial_CO2->available() < 7) {
      timeout++; 
      if (timeout > 10) {
         while(_SoftSerial_CO2->available())  
            _SoftSerial_CO2->read();
            break;                    
      }
      delay(50);
   }

   for (int i=0; i < 7; i++) {
      int byte = _SoftSerial_CO2->read();
      if (byte == -1) {
         result.success = false;
         return -1;
      }
      CO2Response[i] = byte;
   }

   return CO2Response[3]*256 + CO2Response[4];
}

//END CO2 FUNCTIONS //

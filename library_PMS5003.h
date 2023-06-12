
#ifndef PMS5003_h
#define PMS5003_h
#include "Arduino.h"

// SoftwareSerial sensor(D2, D3); // RX, TX
#define LENG 39 // 0x42 + 31 bytes equal to 32 bytes

class PMS5003 { //ug/m3
private:
  /* data */
  HardwareSerial *swSerial;
  unsigned char buf[LENG];

  int PM01Value = 0;  // define PM1.0 value of the air detector module
  int PM2_5Value = 0; // define PM2.5 value of the air detector module
  int PM10Value = 0;  // define PM10 value of the air detector module

  char checkValue(unsigned char *thebuf, char leng);
  int transmitPM01(unsigned char *thebuf);
  int transmitPM2_5(unsigned char *thebuf);
  int transmitPM10(unsigned char *thebuf);

public:
  PMS5003(HardwareSerial *ss);
  ~PMS5003();
  //   void set
  void begin(uint32_t baudrate);
  int getPM_1();
  int getPM_2_5();
  int getPM_10();
  void loop();
  String getUnit();
};

#endif
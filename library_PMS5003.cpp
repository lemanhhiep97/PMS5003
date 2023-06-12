#include "PMS5003.h"

PMS5003::PMS5003(HardwareSerial *ss) { swSerial = ss; }

PMS5003::~PMS5003() {}

void PMS5003::begin(uint32_t baudrate) { swSerial->begin(baudrate); }

int PMS5003::getPM_1() { return PM01Value; }
int PMS5003::getPM_2_5() { return PM2_5Value; }
int PMS5003::getPM_10() { return PM10Value; }

String PMS5003::getUnit() { return F("ug/m3"); }

void PMS5003::loop() {
  if (swSerial->find(0x42)) { // start to read when detect 0x42
    swSerial->readBytes(buf, LENG);

    if (buf[0] == 0x4d) {
      if (checkValue(buf, LENG)) {
        PM01Value =
            transmitPM01(buf); // count PM1.0 value of the air detector module
        PM2_5Value =
            transmitPM2_5(buf); // count PM2.5 value of the air detector module
        PM10Value =
            transmitPM10(buf); // count PM10 value of the air detector module
      }
    }
  }
}

char PMS5003::checkValue(unsigned char *thebuf, char leng) {
  char receiveflag = 0;
  int receiveSum = 0;

  for (int i = 0; i < (leng - 2); i++) {
    receiveSum = receiveSum + thebuf[i];
  }
  receiveSum = receiveSum + 0x42;

  if (receiveSum ==
      ((thebuf[leng - 2] << 8) + thebuf[leng - 1])) // check the serial data
  {
    receiveSum = 0;
    receiveflag = 1;
  }
  return receiveflag;
}

int PMS5003::transmitPM01(unsigned char *thebuf) {
  int PM01Val;
  PM01Val = ((thebuf[3] << 8) +
             thebuf[4]); // count PM1.0 value of the air detector module
  return PM01Val;
}
// transmit PM Value to PC
int PMS5003::transmitPM2_5(unsigned char *thebuf) {
  int PM2_5Val;
  PM2_5Val = ((thebuf[5] << 8) +
              thebuf[6]); // count PM2.5 value of the air detector module
  return PM2_5Val;
}
// transmit PM Value to PC
int PMS5003::transmitPM10(unsigned char *thebuf) {
  int PM10Val;
  PM10Val = ((thebuf[7] << 8) +
             thebuf[8]); // count PM10 value of the air detector module
  return PM10Val;
}
#include "Arduino.h"
#include "FirebaseESP8266.h"
#include "PMS5003.h"
#include <ESP8266WiFi.h>
#include <TimeLib.h>
//#include <WiFiUdp.h#include <Wire.h>
#include <LiquidCrystal_I2C.h>>


LiquidCrystal_I2C
lcd(0x27, 20, 4);
// set the LCD address to 0x27 for a 16 chars and 2 line display

// ntp cập nhật thời gian thực
//static const char ntpServerName[] = "us.pool.ntp.org";
//const int timeZone = 7;
//WiFiUDP Udp;
//unsigned int localPort = 8888; // local port to listen for UDP packets
//time_t getNtpTime();
//void digitalClockDisplay();
//void printDigits(int digits);
//void sendNTPpacket(IPAddress &address);

PMS5003 PMS5003sensor(&Serial);

// chỉnh ngưỡng tác động mở quạt
// để loại bỏ điều kiện chọn 0
int MAX_PM_1 = 0;
int MAX_PM_2_5 = 0;
int MAX_PM_10 = 0;

// khai báo chân dùng để kích relay mở quạt
#define FanPin D6

// khai báo tên wifi và mật khẩu để thiết bị kết nối mạng
#define WIFI_SSID "HIEP 1"
#define WIFI_PASSWORD "0964497508"

// khai báo thông tin đăng ký firebase
// https://pms5003-default-rtdb.firebaseio.com/
#define FIREBASE_HOST "pms5003-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "VVQJPcIaBHZefgvCdDDmRCC6fOL5cgyhG0WAKgrE"

// tạo các đường dẫn trên firebase để lưu dữ liệu
const String sensor_value_path = "/PMS5003";
const String control_path = "/control";
const String mode_path = "/mode_control";
const String pm_1_path = "/config/pm1";
const String pm_10_path = "/config/pm10";
const String pm_25_path = "/config/pm25";

// khai báo biến firebase data object
FirebaseData fbdo;
//
/*
  khai báo biến ghi nhớ trạng thái của chế độ hoạt động
  có 2 chế độ là điều khiển tự động hoặc điều khiển bằng tay
  điều khiển tự động là bật tắt quạt theo thông số của cảm biến
  điều khiển bằng tay là điều khiển theo nút nhấn trên app
  muốn chuyển đổi qua lại giữa các mode thì dùng nút nhấn trên app
*/
bool isModeAuto = true;

// khai báo một số chương trình con
//void printResult(FirebaseData &data); // xuất trạng thái gửi nhận của firebase
void sendFirebase();                  // gửi lên firebase
void configWiFi();                    // cài đặt wifi cho thiết bị
void configFirebase(); // cài đặt cấu hình cho thiết bị giao tiếp với firebase
void configPin(); // cài đặt chân bật tắt quạt
//void configNTP(); // cài đặt cấu hình cho giao tiếp ntp lấy thời gian thực từ
// internet
void runFan();        // kiểm tra điều kiện để chạy quạt
void onFan();         // mở quạt
void offFan();        // tắt quạt
void getControl();    // lấy trạng thái điều khiển từ firebase
void getMode();       // lấy trạng thái mode từ firebase
void getConfigPM1();  // lấy giá trị cài đặt pm 1.0
void getConfigPM10(); // lấy giá trị cài đặt của pm 10
void getConfigPM25(); // lấy giá trị cài đặt của pm 2.5

void updateLCD();

void setup() {
  Serial.begin(9600); // khai báo giao tiếp uart (hiển thị debug lên màn hình)
  // tốc độ baud 9600
  lcd.begin(); // khoi tao man hinh
  lcd.backlight();
  lcd.clear();
  lcd.print("Nong Do Bui:");
  lcd.display();

  PMS5003sensor.begin(9600); // khai báo giao tiếp uart với cảm biến tốc độ baud 9600
  configWiFi();
  configFirebase();
  configPin();
//  configNTP();


}

time_t prevDisplay = 0; // when the digital clock was displayed
unsigned long oldTimeSendFirebase = millis();
int timeOutSendFirebase = 5000;

unsigned long oldTimeGetDataFirebase = millis();
int timeOutGetDataFirebase = 3000;


void loop() {
  updateLCD();
  PMS5003sensor.loop(); // kiểm tra dữ liệu của cảm biến
  runFan();

  if (millis() - oldTimeSendFirebase > timeOutSendFirebase) {
    sendFirebase();
    oldTimeSendFirebase = millis();
  }

  if (millis() - oldTimeGetDataFirebase > timeOutGetDataFirebase ) {
    getControl();
    getMode();
    getConfigPM1();  // lấy giá trị cài đặt pm 1.0
    getConfigPM10(); // lấy giá trị cài đặt của pm 10
    getConfigPM25(); // lấy giá trị cài đặt của pm 2.5
    oldTimeGetDataFirebase = millis();
  }



  // in thời gian ra màn hình xem 
//  if (timeStatus() != timeNotSet) {
//    if (now() != prevDisplay) { // update the display only if time has changed
//      prevDisplay = now();
//      digitalClockDisplay();
//    }
//  }
}

//void configNTP() {
//  Udp.begin(localPort);
//  Serial.print(F("Local port: "));
//  Serial.println(Udp.localPort());
//  Serial.println(F("waiting for sync"));
//  setSyncProvider(getNtpTime);
//  setSyncInterval(3000);
//}

void configPin() {
  pinMode(FanPin, OUTPUT);
}

void onFan() {
  digitalWrite(FanPin, HIGH);
}

void offFan() {
  digitalWrite(FanPin, LOW);
}

void runFan() {
  if (!isModeAuto) {
    return;
  }
  if (PMS5003sensor.getPM_1() > MAX_PM_1 && MAX_PM_1 > 0) {
    onFan();
  } else if (PMS5003sensor.getPM_10() > MAX_PM_10 && MAX_PM_10 > 0) {
    onFan();
  } else if (PMS5003sensor.getPM_2_5() > MAX_PM_2_5 && MAX_PM_2_5 > 0) {
    onFan();
  } else {
    offFan();
  }
}

unsigned long oldTimeUpdateScreen = millis();
int timeToUpdateScreen = 5000;
bool isChangeScreen = true;
void updateLCD() {
  if (millis() - oldTimeUpdateScreen > timeToUpdateScreen) {
    isChangeScreen = !isChangeScreen;
    lcd.clear();
    Serial.println(F("update lcd"));
    if (isChangeScreen) {
      lcd.setCursor(0, 0);
      lcd.print("Nong Do Bui:");

      // lcd.setCursor(10, 0);
      // lcd.print(PMS5003sensor.getPM_1());

      lcd.setCursor(0, 1);
      lcd.print("PM 1.0:");

      lcd.setCursor(9, 1);
      lcd.print(PMS5003sensor.getPM_1());

      lcd.setCursor(-4, 2);
      lcd.print("PM 2.5:");

      lcd.setCursor(5, 2);
      lcd.print(PMS5003sensor.getPM_2_5());

      lcd.setCursor(-4, 3);
      lcd.print("PM 10 :");

      lcd.setCursor(5, 3);
      lcd.print(PMS5003sensor.getPM_10());

    } else {
      lcd.setCursor(0, 0);
      lcd.print("Trang thai quat");

      lcd.setCursor(0, 1);
      lcd.print("mode:");

      lcd.setCursor(10, 1);
      lcd.print(isModeAuto ? "auto" : "manual");

      lcd.setCursor(-4, 2);
      lcd.print("FAN:");

      lcd.setCursor(6, 2);
      lcd.print(digitalRead(FanPin) ? "ON" : "OFF");
    }
    lcd.display();
    oldTimeUpdateScreen = millis();
  }
}

void configWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print(F("Connecting to Wi-Fi"));
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(300);
  }
  Serial.println();
  Serial.print(F("Connected with IP: "));
  Serial.println(WiFi.localIP());
  Serial.println();
}

void configFirebase() {
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
  fbdo.setBSSLBufferSize(1024, 1024);
  fbdo.setResponseSize(1024);
}

//void printResult(FirebaseData &data) {
//
//  if (data.dataType() == "int")
//    Serial.println(data.intData());
//  else if (data.dataType() == "float")
//    Serial.println(data.floatData(), 5);
//  else if (data.dataType() == "double")
//    printf("%.9lf\n", data.doubleData());
//  else if (data.dataType() == "boolean")
//    Serial.println(data.boolData() == 1 ? "true" : "false");
//  else if (data.dataType() == "string")
//    Serial.println(data.stringData());
//  else if (data.dataType() == "json") {
//    Serial.println();
//    FirebaseJson &json = data.jsonObject();
//    // Print all object data
//    Serial.println("Pretty printed JSON data:");
//    String jsonStr;
//    json.toString(jsonStr, true);
//    Serial.println(jsonStr);
//    Serial.println();
//    Serial.println("Iterate JSON data:");
//    Serial.println();
//    size_t len = json.iteratorBegin();
//    String key, value = "";
//    int type = 0;
//    for (size_t i = 0; i < len; i++) {
//      json.iteratorGet(i, type, key, value);
//      Serial.print(i);
//      Serial.print(", ");
//      Serial.print("Type: ");
//      Serial.print(type == FirebaseJson::JSON_OBJECT ? "object" : "array");
//      if (type == FirebaseJson::JSON_OBJECT) {
//        Serial.print(", Key: ");
//        Serial.print(key);
//      }
//      Serial.print(", Value: ");
//      Serial.println(value);
//    }
//    json.iteratorEnd();
//  } else if (data.dataType() == "array") {
//    Serial.println();
//    // get array data from FirebaseData using FirebaseJsonArray object
//    FirebaseJsonArray &arr = data.jsonArray();
//    // Print all array values
//    Serial.println("Pretty printed Array:");
//    String arrStr;
//    arr.toString(arrStr, true);
//    Serial.println(arrStr);
//    Serial.println();
//    Serial.println("Iterate array values:");
//    Serial.println();
//    for (size_t i = 0; i < arr.size(); i++) {
//      Serial.print(i);
//      Serial.print(", Value: ");
//
//      FirebaseJsonData &jsonData = data.jsonData();
//      // Get the result data from FirebaseJsonArray object
//      arr.get(jsonData, i);
//      if (jsonData.typeNum == FirebaseJson::JSON_BOOL)
//        Serial.println(jsonData.boolValue ? "true" : "false");
//      else if (jsonData.typeNum == FirebaseJson::JSON_INT)
//        Serial.println(jsonData.intValue);
//      else if (jsonData.typeNum == FirebaseJson::JSON_FLOAT)
//        Serial.println(jsonData.floatValue);
//      else if (jsonData.typeNum == FirebaseJson::JSON_DOUBLE)
//        printf("%.9lf\n", jsonData.doubleValue);
//      else if (jsonData.typeNum == FirebaseJson::JSON_STRING ||
//               jsonData.typeNum == FirebaseJson::JSON_NULL ||
//               jsonData.typeNum == FirebaseJson::JSON_OBJECT ||
//               jsonData.typeNum == FirebaseJson::JSON_ARRAY)
//        Serial.println(jsonData.stringValue);
//    }
//  } else if (data.dataType() == "blob") {
//
//    Serial.println();
//
//    for (int i = 0; i < data.blobData().size(); i++) {
//      if (i > 0 && i % 16 == 0)
//        Serial.println();
//
//      if (i < 16)
//        Serial.print("0");
//
//      Serial.print(data.blobData()[i], HEX);
//      Serial.print(" ");
//    }
//    Serial.println();
//  } else if (data.dataType() == "file") {
//
//    Serial.println();
//
//    File file = data.fileStream();
//    int i = 0;
//
//    while (file.available()) {
//      if (i > 0 && i % 16 == 0)
//        Serial.println();
//
//      int v = file.read();
//
//      if (v < 16)
//        Serial.print("0");
//
//      Serial.print(v, HEX);
//      Serial.print(" ");
//      i++;
//    }
//    Serial.println();
//    file.close();
//  } else {
//    Serial.println(data.payload());
//  }
//}

void sendFirebase() {
  FirebaseJson json;
//  String time = String(day()) + "-" + String(month()) + "-" + String(year()) +
//                " " + String(hour()) + ":" + String(minute()) + ":" +
//                String(second());
  json.set("pm10", PMS5003sensor.getPM_10());
  json.set("pm1", PMS5003sensor.getPM_1());
  json.set("pm25", PMS5003sensor.getPM_2_5());
//  json.set("time", time);
  json.set("status", digitalRead(FanPin) == 1 ? "ON" : "OFF");

  //  Firebase.push(fbdo, sensor_value_history_path, json);

  if (Firebase.set(fbdo, sensor_value_path, json)) {
    Serial.println(F("PASSED"));
    //    Serial.println("PATH: " + fbdo.dataPath());
    //    Serial.println("TYPE: " + fbdo.dataType());
    //    Serial.print("VALUE: ");
    //    printResult(fbdo);
    //    Serial.println("------------------------------------");
    //    Serial.println();
  } else {
    Serial.println(F("FAILED"));
    //    Serial.println("REASON: " + fbdo.errorReason());
    //    Serial.println("------------------------------------");
    //    Serial.println();
  }
}

void getControl() {
  if (Firebase.getString(fbdo, control_path)) {
    if (!isModeAuto) {
      int __control = fbdo.stringData().toInt();
      digitalWrite(FanPin, __control);
    }
  } else {
    Serial.print(F("getControl Error in getInt, "));
    Serial.println(fbdo.errorReason());
  }
}

void getMode() {
  if (Firebase.getString(fbdo, mode_path)) {
//    String mode = data.stringData();
    isModeAuto = fbdo.stringData().toInt();
    if (isModeAuto) {
      Serial.println(F("auto mode"));
    } else {
      Serial.println(F("control mode"));
    }
  } else {
    Serial.print(F("getMode Error in getInt, "));
    Serial.println(fbdo.errorReason());
  }
}

void getConfigPM1() {
  if (Firebase.getInt(fbdo, pm_1_path)) {
    MAX_PM_1 = fbdo.intData();
    Serial.print(F("MAX_PM_1: "));
    Serial.println(MAX_PM_1);
  } else {
    Serial.print(F("getMode Error in getInt, "));
    Serial.println(fbdo.errorReason());
  }
}

void getConfigPM10() {
  if (Firebase.getInt(fbdo, pm_10_path)) {
    MAX_PM_10 = fbdo.intData();
    Serial.print(F("MAX_PM_10: "));
    Serial.println(MAX_PM_10);
  } else {
    Serial.print(F("getMode Error in getInt, "));
    Serial.println(fbdo.errorReason());
  }
}

void getConfigPM25() {
  if (Firebase.getInt(fbdo, pm_25_path)) {
    MAX_PM_2_5 = fbdo.intData();
    Serial.print(F("MAX_PM_2_5: "));
    Serial.println(MAX_PM_2_5);
  } else {
    Serial.print(F("getMode Error in getInt, "));
    Serial.println(fbdo.errorReason());
  }
}

//void digitalClockDisplay() {
//  // digital clock display of the time
//  Serial.print(hour());
//  printDigits(minute());
//  printDigits(second());
//  Serial.print(" ");
//  Serial.print(day());
//  Serial.print(".");
//  Serial.print(month());
//  Serial.print(".");
//  Serial.print(year());
//  Serial.println();
//}
//
//void printDigits(int digits) {
//  // utility for digital clock display: prints preceding colon and leading 0
//  Serial.print(":");
//  if (digits < 10)
//    Serial.print('0');
//  Serial.print(digits);
//}
//
///*-------- NTP code ----------*/
//
//const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing
//// packets
//
//time_t getNtpTime() {
//  IPAddress ntpServerIP; // NTP server's ip address
//
//  while (Udp.parsePacket() > 0)
//    ; // discard any previously received packets
//  Serial.println(F("Transmit NTP Request"));
//  // get a random server from the pool
//  WiFi.hostByName(ntpServerName, ntpServerIP);
//  Serial.print(ntpServerName);
//  Serial.print(F(": "));
//  Serial.println(ntpServerIP);
//  sendNTPpacket(ntpServerIP);
//  uint32_t beginWait = millis();
//  while (millis() - beginWait < 1500) {
//    int size = Udp.parsePacket();
//    if (size >= NTP_PACKET_SIZE) {
//      Serial.println(F("Receive NTP Response"));
//      Udp.read(packetBuffer, NTP_PACKET_SIZE); // read packet into the buffer
//      unsigned long secsSince1900;
//      // convert four bytes starting at location 40 to a long integer
//      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
//      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
//      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
//      secsSince1900 |= (unsigned long)packetBuffer[43];
//      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
//    }
//  }
//  Serial.println(F("No NTP Response :-("));
//  return 0; // return 0 if unable to get the time
//}
//
//// send an NTP request to the time server at the given address
//void sendNTPpacket(IPAddress &address) {
//  // set all bytes in the buffer to 0
//  memset(packetBuffer, 0, NTP_PACKET_SIZE);
//  // Initialize values needed to form NTP request
//  // (see URL above for details on the packets)
//  packetBuffer[0] = 0b11100011; // LI, Version, Mode
//  packetBuffer[1] = 0;          // Stratum, or type of clock
//  packetBuffer[2] = 6;          // Polling Interval
//  packetBuffer[3] = 0xEC;       // Peer Clock Precision
//  // 8 bytes of zero for Root Delay & Root Dispersion
//  packetBuffer[12] = 49;
//  packetBuffer[13] = 0x4E;
//  packetBuffer[14] = 49;
//  packetBuffer[15] = 52;
//  // all NTP fields have been given values, now
//  // you can send a packet requesting a timestamp:
//  Udp.beginPacket(address, 123); // NTP requests are to port 123
//  Udp.write(packetBuffer, NTP_PACKET_SIZE);
//  Udp.endPacket();
//}

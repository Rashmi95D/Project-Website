#include <SPI.h>
#include <SD.h>
#include "LSM6DS3.h"
#include "Wire.h"
#include <PCF8563.h>
#include <ArduinoBLE.h>
#include <Arduino.h>
#include <U8x8lib.h>

 

// SD Card Configuration

const int chipSelect = 2;

// IMU Sensor Configuration

LSM6DS3 myIMU(I2C_MODE, 0x6A);

// RTC Configuration
PCF8563 pcf;

// BLE Configuration

BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLEByteCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
const int ledPin = LED_BUILTIN;
bool loggingEnabled = false;

// OLED Configuration

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(PIN_WIRE_SCL, PIN_WIRE_SDA, U8X8_PIN_NONE);
const float R1 = 1000.0;
const float R2 = 3300.0;
const float referenceVoltage = 3.3;

 

void setup() {

  Serial.begin(9600);
  while (!Serial);

  Serial.print("Initializing SD card...");

  if (!SD.begin(chipSelect)) {

    Serial.println("Initialization failed. Check:");
    Serial.println("1. Is a card inserted?");
    Serial.println("2. Is your wiring correct?");
    Serial.println("3. Did you change the chipSelect pin to match your shield/module?");
    Serial.println("Press reset after fixing.");
    while (true);

  }

  Serial.println("SD card initialized.");
  if (myIMU.begin() != 0) {
    Serial.println("IMU Device error");
  } else {
    Serial.println("IMU Device OK!");

  }

 

  pcf.init();
  pcf.setYear(25);
  pcf.setMonth(2);
  pcf.setDay(05);  //set dat
  pcf.setHour(15);  //set hour
  pcf.setMinut(10); //set minut
  pcf.setSecond(0);  //set second
  pcf.startClock();

 

  pinMode(ledPin, OUTPUT);
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy module failed!");
    while (1);

  }

 

  BLE.setLocalName("HARSHAVARDHAN");
  BLE.setAdvertisedService(ledService);
  ledService.addCharacteristic(switchCharacteristic);
  BLE.addService(ledService);
  switchCharacteristic.writeValue(0);
  BLE.advertise();
  u8x8.begin();
  u8x8.setFlipMode(1);

}

 

void loop() {

  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    while (central.connected()) {

      float batteryVoltage = readBatteryVoltage();

 

      if (batteryVoltage < 3.2) {
        loggingEnabled = false;
        digitalWrite(ledPin, LOW);
        displayLowBatteryWarning();

      } else {
        if (switchCharacteristic.written()) {
          loggingEnabled = switchCharacteristic.value();
          if (loggingEnabled) {
            Serial.println("Logging ON");
            digitalWrite(ledPin, HIGH);
          } else {
            Serial.println("Logging OFF");
            digitalWrite(ledPin, LOW);
          }

        }
        displayStatus(batteryVoltage);
        if (loggingEnabled) {
          logIMUData();

        }

      }

    }

 
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());

  }

}
void logIMUData() {
  Time nowTime = pcf.getTime();
  String timestamp = String(nowTime.year) + "/" + String(nowTime.month) + "/" + String(nowTime.day) + " ";
  timestamp += String(nowTime.hour) + ":" + String(nowTime.minute) + ":" + String(nowTime.second);

 

  String dataString = timestamp + ",";
  dataString += String(myIMU.readFloatAccelX(), 4) + "," + String(myIMU.readFloatAccelY(), 4) + "," + String(myIMU.readFloatAccelZ(), 4) + ",";
  dataString += String(myIMU.readFloatGyroX(), 4) + "," + String(myIMU.readFloatGyroY(), 4) + "," + String(myIMU.readFloatGyroZ(), 4) + ",";
  dataString += String(myIMU.readTempC(), 4);


  const char* fileName = "Mydata.csv";
  bool fileExists = SD.exists(fileName);
  File dataFile = SD.open(fileName, FILE_WRITE);

  if (dataFile) {
    if (!fileExists) {
      // Write headers if the file is newly created
      dataFile.println("Timestamp,AccelX,AccelY,AccelZ,GyroX,GyroY,GyroZ,Temperature");

    }
    dataFile.println(dataString);
    dataFile.close();
    Serial.println(dataString);
  } else {
    Serial.println("Error opening datalog2.csv");
  }
  delay(1000);

}
float readBatteryVoltage() {
  int sensorValue = analogRead(A0);
  float measuredVoltage = sensorValue * (referenceVoltage / 1023.0);
  return measuredVoltage * ((R1 + R2) / R2);
}
void displayStatus(float batteryVoltage) {
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  //u8x8.clear();

  u8x8.setCursor(0, 0);
  u8x8.print("Status: ");
  u8x8.setCursor(8, 0);
  u8x8.print(loggingEnabled ? "ON " : "OFF");

  u8x8.setCursor(0, 1);
  u8x8.print("Battery: ");
  u8x8.setCursor(8, 1);
  u8x8.print(batteryVoltage, 2);
  u8x8.setCursor(12, 1);
  u8x8.print("V");

}

void displayLowBatteryWarning() {
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  //u8x8.clear();
  u8x8.setCursor(0, 0);
  u8x8.print("Low battery!");
  u8x8.setCursor(0, 1);
  u8x8.print("Logging stopped");

  Serial.println("Low battery detected. Logging stopped.");
}

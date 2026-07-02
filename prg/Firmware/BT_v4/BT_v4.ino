#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "src/I2Ccontroller.h"
#include "src/IMUcontroller_I2C.h"
#include "src/UDPpackage.h"

#include "src/Network_ConnectionInfo.h"

const uint8_t   ADDR_LSM6DSV16X_MASTER  = 0x6a;   // master sensor address
const uint8_t   ADDR_LSM6DSV16X_SLAVE   = 0x6b;   // slave sensor address
const uint8_t   PIN_STATUSLED           = 26;     // status LED digital pin
const uint32_t  I2C_CLOCK               = 400000; // I2C clock

// Network
const uint8_t   NW_WIFI_TIMEOUT = 100;  // WiFi timeout
const uint16_t  NW_UDP_PORT     = 8888; // use port number
WiFiUDP           NW_UDPcon;            // UDP controller
UDPpackageCreator NW_UDPpkger;          // UDP packager
uint64_t          NW_UDP_pkcnt;         // UDP packet counter
uint8_t           NW_MACADDRESS[6];     // device mac address

// LED controll timer
unsigned long lastMils;
unsigned long currentMils;

IMUcontroller_I2C IMUS_master;  // master sensor
IMUcontroller_I2C IMUS_slave;   // slave sensor

bool LED_changeLoopBlocker = false;

//////////////////////////////////////////////
/* Status                                   */
//////////////////////////////////////////////

/* status LED */
/*
  0: ERROR: IMU who am i
  1: init progress
  2: connecting WiFi
  3: connected WiFi
  4: WiFi connection timeout
  5: handshaking
  6: completed
*/
void statusLED(uint8_t statusID){
  switch(statusID){
    case 0:
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(500);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(500);
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(250);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(250);
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(250);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(250);
      break;
    
    case 1:
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(250);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(250);
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(250);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(250);
      break;
    
    case 2:
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(500);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(500);
      break;
    
    case 3:
      for(uint8_t i = 0;i < 3;i++){
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(250);
        digitalWrite(PIN_STATUSLED, LOW);
        delay(250);
      }
      break;
    
    case 4:
      while(true){
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(500);
        digitalWrite(PIN_STATUSLED, LOW);
        delay(500);
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(500);
        digitalWrite(PIN_STATUSLED, LOW);
        delay(500);
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(250);
        digitalWrite(PIN_STATUSLED, LOW);
        delay(250);
      }
      break;
    
    case 5:
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(500);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(500);
      digitalWrite(PIN_STATUSLED, HIGH);
      delay(100);
      digitalWrite(PIN_STATUSLED, LOW);
      delay(900);
      break;
    
    case 6:
      for(uint8_t i = 0;i < 3;i++){
        digitalWrite(PIN_STATUSLED, HIGH);
        delay(300);
        digitalWrite(PIN_STATUSLED, LOW);
        delay(300);
      }
      break;
    
    default:
      break;
  }
}

//////////////////////////////////////////////
/* Network                                  */
//////////////////////////////////////////////

// init
void Init_Network(){
  WiFi_Connection();
  WiFi.macAddress(NW_MACADDRESS);
}

// WiFi connection
void WiFi_Connection(){
  WiFi.begin(Network_ConnectionInfo::WiFi_SSID, Network_ConnectionInfo::WiFi_PASS);
  uint8_t ConnectionCount = 0;
  while(WiFi.status() != WL_CONNECTED){
    statusLED(2);
    if(ConnectionCount++ > NW_WIFI_TIMEOUT){
      Serial.println("Failed. | WiFi connection timeout");
      statusLED(4);
    }
  }
  statusLED(3);

  NW_UDPcon.begin(NW_UDP_PORT);
}

// send UDP package
void UDP_SendPackage(UDPpackageCreator& SendObject){
  NW_UDPcon.beginPacket(Network_ConnectionInfo::SERVER_ADDR, Network_ConnectionInfo::SERVER_PORT);
  NW_UDPcon.write(SendObject.getUDPdatas(), SendObject.getSize());
  NW_UDPcon.endPacket();
}

// handshake(ID:3)
void UDP_SendHandShake(){
  NW_UDPpkger.reset();
  NW_UDPpkger.addUint32(3);    // packetID
  NW_UDPpkger.addUint64(0);    // packet number
  NW_UDPpkger.addUint32(3);    // board type
  NW_UDPpkger.addUint32(14);   // imu type
  NW_UDPpkger.addUint32(2);    // mcu type
  uint8_t brankData[12] = {0};
  NW_UDPpkger.addBytes(brankData, 12);
  NW_UDPpkger.addUint32(5);    // firmware build
  NW_UDPpkger.addString("IMU_Tracker_V3.1"); // name
  NW_UDPpkger.addBytes(NW_MACADDRESS, 6);  // mac address

  UDP_SendPackage(NW_UDPpkger);
}

// handshake ack checker
bool UDP_CheckHandShakeACK(){
  int RecvPackageSize = NW_UDPcon.parsePacket();
  if(RecvPackageSize > 0){
    uint8_t recvBuf[128];
    NW_UDPcon.read(recvBuf, 128);

    return recvBuf[0] == 3;
  }
  return false;
}

// sensor infomation(ID:15)
void UDP_SendSensorInfo(IMUcontroller_I2C& IMU){
  NW_UDPpkger.reset();
  NW_UDPpkger.addUint32(15);   // packetID
  NW_UDPpkger.addUint64(NW_UDP_pkcnt++);   // packet number
  NW_UDPpkger.addUint8(IMU.sensorID);  // sensorID
  NW_UDPpkger.addUint8(1);     // sensor status
  NW_UDPpkger.addUint8(14);    // sensor type

  UDP_SendPackage(NW_UDPpkger);
}

// sensor data(ID:17)
void UDP_SendSensorData(IMUcontroller_I2C& IMU){
  NW_UDPpkger.reset();
  NW_UDPpkger.addUint32(17);
  NW_UDPpkger.addUint64(NW_UDP_pkcnt++);
  NW_UDPpkger.addUint8(IMU.sensorID);
  NW_UDPpkger.addUint8(1);
  for(uint8_t i = 1;i < 4;i++)  NW_UDPpkger.addFloat(IMU.quaternions[i]);
  NW_UDPpkger.addFloat(IMU.quaternions[0]);
  NW_UDPpkger.addUint8(0);

  UDP_SendPackage(NW_UDPpkger);
}

//////////////////////////////////////////////
/* other                                    */
//////////////////////////////////////////////

// int8_t -> hex (debug)
void Serial_PrintHex(uint8_t value){
  if(value < 16)  Serial.print('0');
  Serial.print(value, HEX);
}

//////////////////////////////////////////////
/* setup                                    */
//////////////////////////////////////////////
void setup(){
  Serial.begin(115200);
  while(!Serial);

  Serial.println("\n\n");
  Serial.println("== Body Tracking system v3.1 ==");

  pinMode(PIN_STATUSLED, OUTPUT);

  I2Ccon_init_I2C(I2C_CLOCK);

  // init sensor
  Serial.println("task: init IMU sensor");
  // sensor
  Serial.println("IMU_master | init... ");
  if(!IMUS_master.init(0, ADDR_LSM6DSV16X_MASTER)){
    Serial.println("Failed");
    while(true) statusLED(0);
  }
  Serial.println("OK.");

  // sensor
  Serial.println("IMU_slave  | init... ");
  if(!IMUS_slave.init(1, ADDR_LSM6DSV16X_SLAVE)){
    Serial.println("Failed");
    while(true) statusLED(0);
  }
  Serial.println("OK");

  // init network
  Serial.println("task: init Network");
  WiFi.macAddress(NW_MACADDRESS);
  Serial.print("device MACADDRESS: ");
  for(uint8_t i = 0;i < 6;i++){
    Serial_PrintHex(NW_MACADDRESS[i]);
    if(i < 5) Serial.print(':');
  }
  Serial.println();

  // wifi connection
  Serial.print("connecting WiFi network... ");
  WiFi_Connection();
  Serial.println("OK.");

  Serial.println("init completed!");
}

//////////////////////////////////////////////
/* main                                     */
//////////////////////////////////////////////
uint8_t StateLevel = 1;
void loop(){
  switch(StateLevel){
    case 1:
      // handshake
      Serial.println("task: server handshake");
      while(true){
        Serial.print("send handshake packet");
        UDP_SendHandShake();
        Serial.print(" | waiting ACK... ");
        statusLED(5);
        if(UDP_CheckHandShakeACK()){
          Serial.println("Received.");
          statusLED(6);
          StateLevel++;
          break;
        }
        Serial.println("Timeout.");
      }
      break;
    
    case 2:
      // send sensor infomations
      Serial.println("task: send sensor infomation packet");
      UDP_SendSensorInfo(IMUS_master);
      UDP_SendSensorInfo(IMUS_slave);
      StateLevel++;
      Serial.println("task: tracking and send quaternion to server");
      lastMils = millis();
      break;
    
    case 3:
      // get and send sensor datas
      currentMils = millis();

      if(IMUS_master.getQuaternions()) UDP_SendSensorData(IMUS_master);
      if(IMUS_slave.getQuaternions())  UDP_SendSensorData(IMUS_slave);

      if(currentMils - lastMils > 3000 && !LED_changeLoopBlocker){
        digitalWrite(PIN_STATUSLED, HIGH);
        LED_changeLoopBlocker = true;
      }
      if(currentMils - lastMils > 3100){
        digitalWrite(PIN_STATUSLED, LOW);
        LED_changeLoopBlocker = false;
        lastMils = millis();
      }
      break;
  }
}
#include <SuplaDevice.h>
#include <supla/network/esp_web_server.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <PCF8574.h>
#include <ETH.h>
#include <Network.h>
#include <DallasTemperature.h>
#include "Definitions.h"
#include "SprinklerRelay.h"
#include "SprinklerDisplay.h"
#include "SprinklerConfig.h"
#include "SprinklerMessenger.h"
#include "SprinklerSensors.h"

SprinklerDisplay display;
SprinklerConfig config;
SprinklerSensors sensors;
SprinklerMessenger messenger;

class TelnetLog {
  #define LOG_BUFFER_SIZE 65536   // ustaw tak duży jak możliwe

  private:
    inline static NetworkServer telnetServer = NetworkServer(23);
    inline static NetworkClient telnetClient;
    inline static bool telnetInitialized = false;
    inline static bool pause = false;

    inline static uint8_t myLog[LOG_BUFFER_SIZE];
    inline static uint32_t myLogHead = 0, myLogTail = 0;
    inline static uint32_t totalRead = 0, totalWrite = 0;
  public:
    static void getLogStatusDescription(char* result, int maxLen) {
      snprintf(result, maxLen, ">>> myLogTail=%d myLogHead=%d totalRead=%d totalWrite=%d", myLogTail, myLogHead, totalRead, totalWrite);
    }

  static void onSerialReceive() {
    uint8_t buffer[256];
    for(int bytesAvailable = Serial.available(); bytesAvailable>0;) {
      if (LOG_BUFFER_SIZE-myLogHead==0) {
        myLogHead = 0;
        if (myLogTail==0) myLogTail=1;
      }
      int bytesRead = Serial.read(buffer, sizeof(buffer));
      if (memcmp(buffer, "SRPC", 4) && memcmp(buffer, "0 raw", 5)) { 
        if (bytesRead > LOG_BUFFER_SIZE-myLogHead) {
          int previousHead = myLogHead;
          memcpy(myLog+myLogHead, buffer, LOG_BUFFER_SIZE-myLogHead);
          memcpy(myLog, buffer+LOG_BUFFER_SIZE-myLogHead, bytesRead-LOG_BUFFER_SIZE+myLogHead);
          myLogHead = (myLogHead+bytesRead) % LOG_BUFFER_SIZE;
          if (myLogTail>previousHead || myLogTail<=myLogHead)
            myLogTail = myLogHead+1;
        }
        else {
          memcpy(myLog+myLogHead, buffer, bytesRead);
          myLogHead += bytesRead;
          if (myLogTail<=myLogHead && myLogTail>myLogHead-bytesRead)
            myLogTail = myLogHead+1;
          if (myLogTail>=LOG_BUFFER_SIZE)
            myLogTail = 0;
        }
        totalRead += bytesRead;  
      }
      bytesAvailable -= bytesRead; 
    }
  }

  static void iterate() {   
    if (!config.getTelnetDebug()) return;
    if (config.isNetReady() && !telnetInitialized) {
      telnetServer.begin();
      telnetServer.setNoDelay(true);
      Serial.println("SERWER TELNET GOTOWY");
      telnetInitialized = true;
    }
    if (telnetInitialized) {
      if (telnetServer.hasClient()) { // czy klient się podłączył
        if (!telnetClient || !telnetClient.connected()) {
          if (telnetClient) telnetClient.stop(); // Rozłącz starego klienta, jeśli wisiał
          telnetClient = telnetServer.available();
          telnetClient.println("--- Połączono z konsolą logów ESP32 przez Telnet --- ");
          telnetClient.println("--- ctrl+p, enter = pauza/wznowienie ---");
          telnetClient.println("--- ctrl+i, enter = statystyki loga ---");
        } else {
          // Odrzuć kolejne połączenie, jeśli jedno jest już aktywne
          telnetServer.available().stop();
        }    
      }
        // Wypychanie danych z bufora RAM do połączenia sieciowego
      if (!pause && myLogHead!=myLogTail && telnetClient && telnetClient.connected()) {
        if (myLogTail>=LOG_BUFFER_SIZE)
          myLogTail=0;
        uint32_t dataToSend = (myLogTail<myLogHead)? myLogHead - myLogTail : LOG_BUFFER_SIZE - myLogTail ;
        telnetClient.write(myLog+myLogTail, dataToSend);
        myLogTail += dataToSend;
        totalWrite += dataToSend;
        if (myLogTail == LOG_BUFFER_SIZE)
          myLogTail = 0;
      }
      // ignorujemy odebrane dane z konsoli telnet
      while (telnetClient && telnetClient.available()) {
        int8_t ch = telnetClient.read();
        if (ch==16) {     // ctrl + p
          pause = !pause;
          telnetClient.println(pause?"PAUZA":"WZNOWIENIE");
        }
        if (ch==9) {     // ctrl + i
          char buf[128];
          getLogStatusDescription(buf, sizeof (buf));
          telnetClient.println(buf);
        }
      }
    }
  }
};


int32_t upTime = millis();
void setup() {
  Serial.begin(115200);
  uart_internal_loopback(0, 3);   // copy all logs from UART TX to RX
  Serial.onReceive(TelnetLog::onSerialReceive);  // set callback for all RX notifications
  delay(500);
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(500);
  // Konfiguracja początkowa ekspandera przekaźników (ustawienie wszystkich na OFF - stan WYSOKI)
  Wire.beginTransmission(PCF_RELAYS_ADDR);
  Wire.write(0xFF); 
  Wire.endTransmission();
  delay(500);
  display.Initialize();
  display.drawSuplaLogo();
  sensors.Initialize();
  delay(500);
  config.initialize();
  sensors.setupSensorCloudData();
  SprinklerRelay::registerRelays();
  SuplaDevice.setName(MY_DEVICE_NAME);
  SuplaDevice.setCustomHostnamePrefix(MY_WIFI_NAME);
  //SuplaDevice.setInitialMode(Supla::InitialMode::StartInNotConfiguredMode);
  SuplaDevice.setProductId(0);
  SUPLA_LOG_DEBUG("setup zakończony\n");
  SuplaDevice.begin() ;
  SUPLA_LOG_DEBUG("begin wykonany\n");

  SprinklerRelay::initalizeRelays();
  messenger.initialize();
  
}


void loop() {
  SuplaDevice.iterate();    
  SprinklerRelay::ticTacTimer();
  display.updateView();
  TelnetLog::iterate();
  // zmień na wifi jeśli w trybie konfiguracyjnym nie złapało sieci LAN
  if (millis()-upTime>20000 && SuplaDevice.getDeviceMode() == Supla::DEVICE_MODE_CONFIG && config.getIntfType() == Supla::Network::IntfType::Ethernet && config.getIp() == INADDR_NONE)
      config.setupNetwork(Supla::Network::IntfType::WiFi);
}

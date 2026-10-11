#include <SuplaDevice.h>
#include <supla/network/esp_web_server.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <PCF8574.h>
#include <ETH.h>
#include <Network.h>
#include <DallasTemperature.h>
#include <time.h>
#include <sys/time.h>
#include <RTClib.h>
#include <chrono>
#include <Supla/clock/clock.h>
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

    inline static uint8_t buffer[1024];
    inline static uint16_t bytesRead = 0;
    inline static uint16_t timeHeaderLen = 0;
    
    static void OnSerialReceive() {
      if (bytesRead==0) {
        auto now = std::chrono::system_clock::now();
        std::time_t time_now = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&time_now);
        auto duration = now.time_since_epoch();
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration) - std::chrono::duration_cast<std::chrono::milliseconds>(seconds);
        timeHeaderLen = snprintf((char*)buffer, sizeof(buffer), "(%d:%02d:%02d:%03d) ", local_time->tm_hour, local_time->tm_min, local_time->tm_sec, milliseconds.count());
        bytesRead = timeHeaderLen;
      }
      for(;;) {
        for (int bytesAvailable = Serial.available(); bytesAvailable>0; bytesAvailable = Serial.available()) {
          int read = Serial.readBytes(buffer+bytesRead, 1/*sizeof(buffer)-bytesRead*/);
          if (read==0) break;
          //if (buffer[bytesRead] == '\n') continue;
          bytesRead++;
          if (bytesRead==sizeof(buffer)) break;  // bufor pełny nie czytamy więcej
          if (buffer[bytesRead-1] == '\n') break;
        }
        if (bytesRead == 0) return;
        if (buffer[bytesRead-1] != '\n' && bytesRead<sizeof(buffer)) 
          return; // czekamy na koniec linii*/
        if (memcmp(buffer+timeHeaderLen, "SRPC", 4)==0) {
          bytesRead = timeHeaderLen;
          continue;
        }
      
        if (LOG_BUFFER_SIZE-myLogHead==0) {
            myLogHead = 0;
            if (myLogTail==0) myLogTail=1;
        }
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
        bytesRead=0;
        timeHeaderLen = 0;
      }
    }

    static void Iterate() {   
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
            telnetClient.println("--- Połączono z konsolą logów ESP32 przez Telnet ---");
            telnetClient.println("--- ctrl+p, enter = pauza/wznowienie ---");
            telnetClient.println("--- ctrl+i, enter = statystyki loga ---");         
          } else {
            // Odrzuć kolejne połączenie, jeśli jedno jest już aktywne
            telnetServer.available().stop();
          }    
        }
          // Wypychanie danych z bufora RAM do telnet
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
        // obsługa wejścia
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

Supla::Clock suplaClock;
RTC_DS3231* rtc = nullptr;
bool rtcSynchronized = false;
int32_t upTime = millis();

void setup() {
  Serial.begin(115200);
  uart_internal_loopback(0, 3);   // copy all logs from UART TX to RX
  Serial.onReceive(TelnetLog::OnSerialReceive);  // set callback for all RX notifications
  delay(500);
  SUPLA_LOG_DEBUG("Setup START");
  Wire.begin(I2C_SDA, I2C_SCL);
  delay(500);
  // Konfiguracja początkowa ekspandera przekaźników (ustawienie wszystkich na OFF - stan WYSOKI)
  Wire.beginTransmission(PCF_RELAYS_ADDR);
  Wire.write(0xFF); 
  Wire.endTransmission();

  rtc = new RTC_DS3231();
  if (!rtc->begin(&Wire)) {
    rtc = nullptr;
    SUPLA_LOG_DEBUG("Nie znaleziono modułu RTC");
  } else 
    SUPLA_LOG_DEBUG("Znaleziono moduł RTC DS3231");

  if (rtc && !rtc->lostPower()) {
    time_t rtcUnixTime = rtc->now().unixtime();
    struct timeval tv = { .tv_sec = rtcUnixTime, .tv_usec = 0 };
    settimeofday(&tv, NULL);
    SUPLA_LOG_DEBUG("Sukces! Zaktualizowany czas lokalny z RTC");
  }
  display.Initialize();
  display.drawSuplaLogo();
  sensors.Initialize();
  delay(500);
  config.initialize();
  sensors.setupSensorCloudData();
  SprinklerRelay::RegisterRelays();
  SuplaDevice.setName(MY_DEVICE_NAME);
  SuplaDevice.setCustomHostnamePrefix(MY_WIFI_NAME);
  //SuplaDevice.setInitialMode(Supla::InitialMode::StartInNotConfiguredMode);
  SuplaDevice.setManufacturerId(0);
  SuplaDevice.setProductId(0);
  SUPLA_LOG_DEBUG("setup zakończony\n");
  SuplaDevice.begin() ;
  SUPLA_LOG_DEBUG("begin wykonany\n");

  SprinklerRelay::InitalizeRelays();
  messenger.initialize();
  
}


void loop() {
  SuplaDevice.iterate();    
  SprinklerRelay::TicTacTimer();
  display.updateView();
  TelnetLog::Iterate();
  // zmień na wifi jeśli w trybie konfiguracyjnym nie złapało sieci LAN
  if (millis()-upTime>20000 && SuplaDevice.getDeviceMode() == Supla::DEVICE_MODE_CONFIG && config.getIntfType() == Supla::Network::IntfType::Ethernet && config.getIp() == INADDR_NONE)
      config.setupNetwork(Supla::Network::IntfType::WiFi);

  // synchronizuj zegar ponownie o 3:30 w nocy
  if (rtc && suplaClock.isReady() && suplaClock.getHour() == 3 && suplaClock.getMin() == 30 && suplaClock.getSec() == 0)
    rtcSynchronized = false; 
  if (rtc && !rtcSynchronized && suplaClock.isReady()) {
    rtc->adjust(DateTime(suplaClock.getYear(), suplaClock.getMonth(), suplaClock.getDay(), suplaClock.getHour(), suplaClock.getMin(), suplaClock.getSec()));
    rtcSynchronized = true;
    SUPLA_LOG_DEBUG("Sukces! Czas RTC zsynchronizowany z SUPLA Cloud");
    delay(1000);  // tylko jedna synchronizacja
  }
}

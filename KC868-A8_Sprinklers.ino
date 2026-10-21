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
#include <Supla/clock/clock.h>
#include "Definitions.h"
#include "SprinklerRelay.h"
#include "SprinklerDisplay.h"
#include "SprinklerConfig.h"
#include "SprinklerMessenger.h"
#include "SprinklerSensors.h"
#include "TelnetLog.h"

SprinklerConfig config;
SprinklerSensors sensors;
SprinklerMessenger messenger;
SprinklerDisplay display;

Supla::Clock suplaClock;
RTC_DS3231* rtc = nullptr;
bool rtcSynchronized = false;
int32_t upTime = millis();

void setup() {
  Serial.begin(115200);
  TelnetLog::Initialize();
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

  if (config.getTelnetDebug() && config.isNetReady())
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

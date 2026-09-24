#include <SuplaDevice.h>
#include <supla/network/esp_web_server.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <PCF8574.h>
#include <ETH.h>
#include "Definitions.h"
#include <Network.h>
#include <DallasTemperature.h>

#include "SprinklerRelay.h"
#include "SprinklerDisplay.h"
#include "SprinklerConfig.h"
#include "SprinklerMessenger.h"
#include "SprinklerSensors.h"

SprinklerDisplay display;
SprinklerConfig config;
SprinklerSensors sensors;
SprinklerMessenger messenger;

int32_t upTime = millis();

void setup() {
  Serial.begin(115200);
  delay(500);
   // Inicjalizacja magistrali I2C dla ekspanderów PCF
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
  config.Initialize();
  sensors.setupSensorCloudData();
  SprinklerRelay::registerRelays();
  SuplaDevice.setName(MY_DEVICE_NAME);
  //SuplaDevice.setInitialMode(Supla::InitialMode::StartInNotConfiguredMode);
  SuplaDevice.setProductId(0);
  SUPLA_LOG_DEBUG(F("setup zakończony\n"));
  SuplaDevice.begin() ;//GUID, SERVER, EMAIL, AUTHKEY);
  SUPLA_LOG_DEBUG(F("begin wykonany\n"));
  SprinklerRelay::initalizeRelays();
  messenger.Initialize();
}


void loop() {
  SuplaDevice.iterate();    
  SprinklerRelay::ticTacTimer();
  messenger.pushMessages();
  display.updateView();
  // zmień na wifi jeśli w trybie konfiguracyjnym nie złapało sieci LAN
  if (millis()-upTime>20000 && SuplaDevice.getDeviceMode() == Supla::DEVICE_MODE_CONFIG && config.getIntfType() == Supla::Network::IntfType::Ethernet && config.getIp() == INADDR_NONE)
      config.setupNetwork(Supla::Network::IntfType::WiFi);
}

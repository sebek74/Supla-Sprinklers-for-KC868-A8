#include <sstream>
#include <string>

// SPDX-FileCopyrightText: AC SOFTWARE SP. Z O.O.
// SPDX-License-Identifier: GPL-2.0-or-later
//#define STATUS_LED_GPIO 2
#define BUTTON_CFG_RELAY_GPIO 0

#include <SuplaDevice.h>
#include <Update.h>
#include <WebServer.h>
#include <supla/control/relay.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include <supla/device/status_led.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/events.h>

// Includes for HTML elements
#include <supla/network/html/device_info.h>
#include <supla/network/html/protocol_parameters.h>
#include <supla/network/html/wifi_parameters.h>
#include <supla/network/html/ethernet_parameters.h>
#include <supla/network/html/button_update.h>
#include <supla/network/html/button_refresh.h>
#include <supla/network/html/custom_text_parameter.h>
#include <supla/network/html/custom_parameter.h>
#include <supla/network/html/select_input_parameter.h>
#include "Definitions.h"
#include "SprinklerConfig.h"

// Choose where Supla should store roller shutter data in persistent memory
// We recommend to use external FRAM memory
// #define FRAM_CS_PIN TBD // choose a free GPIO and replace TBD
// #define STORAGE_OFFSET 100
// #include <supla/storage/fram_spi.h>
// Supla::FramSpi fram(FRAM_CS_PIN, STORAGE_OFFSET);
#include <supla/storage/eeprom.h>
Supla::Eeprom eeprom;

// This class provides web server to handle config mode
Supla::EspWebServer suplaServer;
//HTTPUpdateServer httpUpdater;

extern SprinklerConfig config;


//Supla::Device::StatusLed statusLed(STATUS_LED_GPIO, true); // inverted state
#include <DallasTemperature.h>
#include "SprinklerSensors.h"
extern SprinklerSensors sensors;

class ButtonReset : public Supla::HtmlElement {
public:
  // Generuje kod HTML z przyciskiem restartu
  void send(Supla::WebSender* sender) override {
    sender->send("</div><input type=\"hidden\" name=\"just_reboot\" value=\"1\">"
                 "<button type=\"submit\">"
                 "RESTART"
                 "</button><div>");
  }

  // Przechwytuje kliknięcie przycisku
  bool handleResponse(const char* key, const char* value) override {
    if (strcmp(key, "just_reboot") == 0 && strcmp(value, "1") == 0) {
      Serial.println("Otrzymano żądanie restartu urządzenia...");
      
      // Bezpieczny restart urządzenia dostarczany przez bibliotekę Supla
      SuplaDevice.scheduleSoftRestart(500); 
      return true;
    }
    return false;
  }
};

void SprinklerSection::send(Supla::WebSender* sender) {
  uint8_t scheduleHour = 4;
  //Supla::Storage::ConfigInstance()->getUInt8("SCHEDULE_TIME", &scheduleHour);
  //sender->send("<script>document.currentScript.parentElement.style.display = 'none';</script></div><div class=\"box\"><h3>Parametry nawodnienia</h3>");
  sender->labeledField("SCHEDULE_TIME", "Czas startu nawodnienia", [&]() {
    Supla::HtmlTag selectTag = sender->selectTag("SCHEDULE_TIME", "SCHEDULE_TIME");
    selectTag.body([&]() {
      for (int i=0;i<24;i++) {
        char hour[6];
        sprintf(hour, "%d:00", i);
        sender->selectOption(i, hour, i==scheduleHour);
      }
    });
  });

  sender->labeledField("SPR_SHORT", "Zraszacz - czas krótki", [&]() {
    sender->numberInput("SPR_SHORT",
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(30, 0),
                            .value = Supla::fixed(10, 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField("SPR_LONG", "Zraszacz - czas długi", [&]() {
    sender->numberInput("SPR_LONG",
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(60, 0),
                            .value = Supla::fixed(20, 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField("DROPS_SHORT", "Kropelkowe - czas krótki", [&]() {
    sender->numberInput("DROPS_SHORT",
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(60, 0),
                            .value = Supla::fixed(30, 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField("DROPS_LONG", "Kropelkowe - czas długi", [&]() {
    sender->numberInput("DROPS_LONG",
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(100, 0),
                            .value = Supla::fixed(60, 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
  //sender->send("</div><div class=\"box\"><h3>Parametry przekaźników</h3>");
  sender->labeledField("LIGHT_TIME", "Czas włączenia światła przed szopą", [&]() {
    sender->numberInput("LIGHT_TIME",
                        {
                            .min = Supla::fixed(30, 0),
                            .max = Supla::fixed(600, 0),
                            .value = Supla::fixed(120, 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
  //sender->send("</div><div class=\"box\"><h3>Parametry czujników</h3>");
  for (int i=0; i<sensors.getSensorsAmount(); i++) {
    char param[17], defaultName[17], address[64] = "Czujnik temperatury ";
    snprintf(param, 16, "DALLAS%d", i);
    sensors.getDeviceAdress(i, address+strlen(address), 64-strlen(address));

    sender->labeledField(param, address, [&]() {
      sender->textInput(address,
                 param, 
                 config.getSensorName(i), 
                 16
      );
    });
  }
    
  //httpUpdater.setup(suplaServer.getServerPtr(), "/update");
  //suplaUpdate.send(sender);
}


void SprinklerConfig::Initialize() {
/*
  auto r1 = new Supla::Control::Relay(RELAY_GPIO);
  // CH 1 - Action trigger
    auto at1 = new Supla::Control::ActionTrigger();*/

  auto storage = Supla::Storage::ConfigInstance();
  // Buttons configuration
  this->init();

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::EthernetParameters;
  new Supla::Html::WifiParameters();
  new Supla::Html::ProtocolParameters();

  //new SprinklerSection();
  
  //new Supla::Html::StatusLedParameters;

  auto time = new Supla::Html::SelectInputParameter("SCHEDULE_TIME", "Czas startu nawodnienia");
  for (int i=0;i<24;i++) {
    char hour[6];
    snprintf(hour, 6, "%d:00", i);
    time->registerValue(hour, i);
  }
  
  new Supla::Html::CustomParameter("SPR_SHORT", "Zraszacz - czas krótki", 10, 1, 30);
  new Supla::Html::CustomParameter("SPR_LONG", "Zraszacz - czas długi", 20, 1, 60);
  new Supla::Html::CustomParameter("DROPS_SHORT", "Kropelkowe - czas krótki", 30, 1, 60);
  new Supla::Html::CustomParameter("DROPS_LONG", "Kropelkowe - czas długi", 60, 1, 120);
  new Supla::Html::CustomParameter("LIGHT_TIME", "Czas włączenia światła przed szopą", 120, 30, 600);

  for (int i=0; i<sensors.getSensorsAmount(); i++) {
    char param[16], address[64] = "Czujnik temperatury ";
    sprintf(param, "DALLAS%d", i);
    sensors.getDeviceAdress(i, address+strlen(address), 64-strlen(address));
    Serial.print("info z konfiga: ");
    Serial.println(address);
    new Supla::Html::CustomTextParameter(param, address , 64);
  }

  new Supla::Html::ButtonRefresh();
  new ButtonReset();
  new Supla::Html::ButtonUpdate(&suplaServer);

     // Text based command input
  // PARAM_CMD1 - Html field input name
  // "Relay control" - label that is displayed on config page
  //auto textCmd = new Supla::Html::TextCmdInput(PARAM_CMD1, "Relay control");
  // First we register allowed text input values and we assign which event
  // should be generated:
  /*textCmd->registerCmd("ON", Supla::ON_EVENT_1);
  textCmd->registerCmd("OFF", Supla::ON_EVENT_2);
  textCmd->registerCmd("TOGGLE", Supla::ON_EVENT_3);*/
  // Then we link events with actions on a relay.
  // Last "true" parameter will make sure that those actions won't be disabled
  // when we enter config mode. By default actions are disabled when device
  // enters config mode, but those text commands can be used only in config
  // mode, so we want to have them enabled.
/*  textCmd->addAction(Supla::TURN_ON, r1, Supla::ON_EVENT_1, true);
  textCmd->addAction(Supla::TURN_OFF, r1, Supla::ON_EVENT_2, true);
  textCmd->addAction(Supla::TOGGLE, r1, Supla::ON_EVENT_3, true);*/

  // Select based command input - exactly the same configuration as for text
  // field
  // PARAM_CMD2 - Html field input name
  // "Relay control" - label that is displayed on config page
 /* auto selectCmd =
    new Supla::Html::SelectCmdInputParameter(PARAM_CMD1, "Relay control");
  // First we register allowed text input values and we assign which event
  // should be generated:
  selectCmd->registerCmd("ON", Supla::ON_EVENT_1);
  selectCmd->registerCmd("OFF", Supla::ON_EVENT_2);
  selectCmd->registerCmd("TOGGLE", Supla::ON_EVENT_3);*/
  // Then we link events with actions on a relay.
  // Last "true" parameter will make sure that those actions won't be disabled
  // when we enter config mode. By default actions are disabled when device
  // enters config mode, but those text commands can be used only in config
  // mode, so we want to have them enabled.
  //selectCmd->addAction(Supla::TURN_ON, r1, Supla::ON_EVENT_1, true);
  //selectCmd->addAction(Supla::TURN_OFF, r1, Supla::ON_EVENT_2, true);
  //selectCmd->addAction(Supla::TOGGLE, r1, Supla::ON_EVENT_3, true);

    SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
    SuplaDevice.setLeaveCfgModeAfterInactivityMin(5);
    //suplaUpdate.handleResponse("updbeta", updBeta);
  // Start!
    LoadStorage();
    Serial.println(F("Zakonczone ustawianie konfiguracji."));
  }

   void SprinklerConfig::LoadStorage() {    
    /*Supla::KeyValueElement* element = first;
    while (element) {
      uint8_t raw[1024];
      element->serialize(raw, 1023);
      Serial.printf("mam taki element: >%s< typ>%d<\n", raw, static_cast<uint8_t>(*(raw+SUPLA_STORAGE_KEY_SIZE)));
      if (element->isKeyEqual("SPR_SHORT")) {
        int32_t result = 77;
       element->getInt32(&result);
       Serial.printf("znaleziony! >%d<\n", result);
      }
      element = element->getNext();
    }
    
*/
    if (!this->getInt32("SPR_SHORT", &sprShort)) {
     /* Serial.println("nieudane pobranie sprShort");
      auto size = this->getBlobSize("SPR_SHORT");
      char value[128];
      this->getBlob("SPR_SHORT", value, 127);
      Serial.printf("blob: %d >%s<\n", size, value);
    }
    else {
      Serial.printf("udane pobranie sprShort: %d\n", sprShort);*/
    }
    this->getInt32("SPR_LONG", &sprLong);
    this->getInt32("DROPS_SHORT", &dropShort);
    this->getInt32("DROPS_LONG", &dropLong);
    this->getInt32("SCHEDULE_TIME", &scheduleHour);
    this->getInt32("LIGHT_TIME", &lightActivationTimeS);
    uint8_t ethDisabled = 1;
    this->getUInt8(Supla::EthDisableTag, &ethDisabled);
    
    for (int i=0; i<sensors.getSensorsAmount(); i++) {
      char param[17];
      snprintf(param, 16, "DALLAS%d", i);
      this->getString(param, sensorNames[i], 16);
    }
    Serial.printf("RESTORED CONFIG VALUES: %d %d %d %d %d %d %d\n", sprShort, sprLong, dropShort, dropLong, scheduleHour, lightActivationTimeS, ethDisabled);
    if (!ethDisabled) 
      setupNetwork(Supla::Network::IntfType::Ethernet);
    else
      setupNetwork(Supla::Network::IntfType::WiFi);
  
   }

  void SprinklerConfig::setupNetwork(Supla::Network::IntfType type) {
    if (network!=nullptr) {
      Serial.println("Uruchamianie awaryjnego połączenia bezprzewodowego WI-FI w trybie konfiguracji...");
      //network = new Supla::ESPWifi();
      //network->SetSetupNeeded();
      Supla::Storage::ConfigInstance()->setUInt8(Supla::EthDisableTag, 1);
      Serial.println("Miękki restart. Przełączam na WI-FI...");
      SuplaDevice.softRestart();
      return;
    }
    if (type == Supla::Network::IntfType::Ethernet) {
      Serial.println("Uruchamianie połączenia kablowego ETHERNET...");
      network = new Supla::ESPETH(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
    } else if (type == Supla::Network::IntfType::WiFi){
        Serial.println("Uruchamianie połączenia bezprzewodowego WI-FI...");
      network = new Supla::ESPWifi();
      //network->SetSetupNeeded();
    }
  }
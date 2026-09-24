
#include <supla/network/esp_wifi.h>
#include <WiFi.h>
#include <supla/network/esp32eth.h>
#include <Supla/network/html_element.h>
#include <Supla/network/web_sender.h>
#include <Supla/storage/LittleFS_config.h>
#include "Definitions.h"

class SprinklerSection : public Supla::HtmlElement {
 public:
  SprinklerSection() : HtmlElement(Supla::HtmlSection::HTML_SECTION_FORM) {};
  void send(Supla::WebSender* sender) override;
};

// This class provides configuration storage (like SSID, password, all
// user defined parameters, etc. We use LittleFS.

class SprinklerConfig  : public Supla::LittleFsConfig {
  private:
    int32_t sprShort, sprLong, dropShort, dropLong, scheduleHour, messageHour;
    int32_t lightActivationTimeS;
    char sensorNames[8][17];
    Supla::Network *network;

  public:
    SprinklerConfig() : Supla::LittleFsConfig(2048)  {
      sprShort = 10;
      sprLong = 20;
      dropShort = 30;
      dropLong = 60;
      lightActivationTimeS = 120;
      //strcpy(scheduleTime, "4:00");
      scheduleHour = 4; // 4:00
      messageHour = 22;
      network = nullptr;
      //wifiNetwork = std::nullptr_t;
      //ethNetwork = std::nullptr_t;
      char defaultName[17];
      for (int i=0; i<8; i++) {
        snprintf(defaultName, 16, "Temperatura %d", i+1);
        strcpy (sensorNames[i], defaultName);
      }
    };

    int32_t getSprShort() { return sprShort;};
    int32_t getSprLong() { return sprLong;};
    int32_t getSprBoth() { return sprShort+sprLong;};
    int32_t getDropShort() { return dropShort;};
    int32_t getDropLong() { return dropLong;};
    int32_t getDropBoth() { return dropShort+dropLong;};
    int32_t getScheduleHour() { return scheduleHour;};
    void incScheduleHour() {
      scheduleHour=(scheduleHour+1)%24;
      setInt32("SCHEDULE_TIME", scheduleHour);
    }
    int32_t getMessageHour() { return messageHour; };
    int32_t getLightActivationTimeS() { return lightActivationTimeS; };
    char* getSensorName(int32_t id) { return sensorNames[id]; };
    
    time_t makeScheduleTimeToday() {
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
#ifndef DEBUG // ustaw bieżący czas w trybie DEBUG, włączy się w następnej minucie
      timeinfo->tm_hour = scheduleHour;
      timeinfo->tm_min  = 0;
#endif
      timeinfo->tm_sec  = 0;
      return mktime(timeinfo);
    }

    time_t makeMessageTimeToday() {
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
#ifndef DEBUG // ustaw bieżący czas w trybie DEBUG, włączy się w następnej minucie
      timeinfo->tm_hour = messageHour;
      timeinfo->tm_min  = 0;
#endif
      timeinfo->tm_sec  = 0;
      return mktime(timeinfo);
    } 

    Supla::Network::IntfType getIntfType() { 
      if (network==nullptr) return Supla::Network::IntfType::Unknown;
      return network->getIntfType(); 
    };

    bool isNetReady() {
      if (network==nullptr) return false;
      return network->isReady();
    };

    IPAddress getIp() {
      if (network==nullptr) return false;
      if (network->getIntfType() == Supla::Network::IntfType::Ethernet)
        return ETH.localIP(); 
      if (network->getIntfType() == Supla::Network::IntfType::WiFi) {
        return WiFi.softAPIP();
      }
      return INADDR_NONE;
    };

    String getSSID() {
      return WiFi.softAPSSID();
    }
    
    void setupNetwork(Supla::Network::IntfType type);
    void Initialize();
    void LoadStorage();
};
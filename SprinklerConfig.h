
#include <supla/network/esp_wifi.h>
#include <WiFi.h>
#include <supla/network/esp32eth.h>
#include <Supla/network/html_element.h>
#include <Supla/network/web_sender.h>
#include <Supla/storage/LittleFS_config.h>
#include "Definitions.h"

class SprinklerSection : public Supla::HtmlElement {
  private:
    const char* h4title;
  public:
    SprinklerSection(const char* title) : HtmlElement(Supla::HtmlSection::HTML_SECTION_FORM), h4title(title) {};
    void send(Supla::WebSender* sender) override;
};

// This class provides configuration storage (like SSID, password, all
// user defined parameters, etc. We use LittleFS.

class SprinklerConfig  : public Supla::LittleFsConfig {
  private:
    int32_t sprShort, sprLong, dropShort, dropLong, scheduleHour, scheduleMessageHour;
    int32_t lightTimeSMessage, pumpTimeSMessage, pumpAdvOffTimeS;
    int32_t doorMessageHourStart, doorMessageHourEnd;
    int32_t lightActivationTimeS;
    uint8_t telnetDebug;
    char sensorNames[8][17];
    Supla::Network *network;

  public:
    SprinklerConfig() : Supla::LittleFsConfig(2048)  {
      sprShort = 10;
      sprLong = 20;
      dropShort = 30;
      dropLong = 60;
      lightActivationTimeS = 120;
      lightTimeSMessage = 30;
      pumpTimeSMessage = 60;
      pumpAdvOffTimeS = 5;
      //strcpy(scheduleTime, "4:00");
      scheduleHour = 4; // 4:00
      scheduleMessageHour = 22;
      doorMessageHourStart = 22;
      doorMessageHourEnd = 6;
      telnetDebug = 0;
      network = nullptr;
      //wifiNetwork = std::nullptr_t;
      //ethNetwork = std::nullptr_t;
      char defaultName[17];
      for (int i=0; i<8; i++) {
        snprintf(defaultName, sizeof(defaultName), "Temperatura %d", i+1);
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
    int32_t getScheduleMessageHour() { return scheduleMessageHour; };
    int32_t getDoorMessageHourStart() { return doorMessageHourStart; };
    int32_t getDoorMessageHourEnd() { return doorMessageHourEnd; };
    bool isDoorMessageHourActive(int hour);
    int32_t getLightActivationTimeS() { return lightActivationTimeS; };
    int32_t getLightTimeSMessage() { return lightTimeSMessage; };
    int32_t getPumpTimeSMessage() { return pumpTimeSMessage; };
    int32_t getPumpAdvOffTimeS() { return pumpAdvOffTimeS; };
    bool getTelnetDebug() { return telnetDebug>0; };
    
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
      timeinfo->tm_hour = scheduleMessageHour;
      timeinfo->tm_min  = 0;
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
    void initialize();
    void loadStorage();
    void loadNetwork();
    void applyCert();
    void setupDownloadConfig();
};
#include <sstream>
#include <string>

#include <SuplaDevice.h>
#include <Update.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <supla/control/relay.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include <supla/storage/littlefs_config.h>
#include <supla/network/esp_web_server.h>
#include <supla/device/supla_ca_cert.h>
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
#include <supla/network/html/custom_checkbox_parameter.h>
#include "Definitions.h"
#include "SprinklerConfig.h"
#include <supla/storage/eeprom.h>
Supla::Eeprom eeprom;

Supla::EspWebServer suplaServer;

extern SprinklerConfig config;
#include <DallasTemperature.h>
#include "SprinklerSensors.h"
extern SprinklerSensors sensors;

#define SPRINKLER_TIME_SHORT "SPR_SHORT"
#define SPRINKLER_TIME_LONG "SPR_LONG"
#define DROPS_TIME_SHORT "DROPS_SHORT"
#define DROPS_TIME_LONG "DROPS_LONG"
#define SCHEDULE_START_TIME "SCHEDULE_TIME"
#define SCHEDULE_MESSAGE_HOUR "MESSAGE_TIME"
#define LIGHT_ON_TIME "LIGHT_TIME"
#define LIGHT_MESSAGE_TIME "LIGHT_MSG_T"
#define PUMP_MESSAGE_TIME "PUMP_MSG_T"
#define PUMP_ADVANCE_OFF_TIME "PUMP_AO_T"
#define TEMP_PARAM_HEADER "DALLAS"
#define DOOR_MESSAGE_HOUR_START "DOOR_T1"
#define DOOR_MESSAGE_HOUR_END "DOOR_T2"
#define TELNET_DEBUG "TELNET_DBG"


class ButtonReset : public Supla::HtmlElement {
public:
  ButtonReset() : HtmlElement(Supla::HtmlSection::HTML_SECTION_BUTTON_AFTER) {};
  void send(Supla::WebSender* sender) override {
    sender->send("<input type=\"hidden\" name=\"just_reboot\" id=\"just_reboot\" value=\"0\">"
                 "<button type=\"button\" onclick=\"var e=document.getElementById('just_reboot'); e.value='1'; var f=document.getElementById('cfgform'); f.submit();\">"
                 "RESTART"
                 "</button>");
  }

  // Przechwytuje kliknięcie przycisku
  bool handleResponse(const char* key, const char* value) override {
    if (strcmp(key, "just_reboot") == 0 && strcmp(value, "1") == 0) {
      SUPLA_LOG_DEBUG("Otrzymano żądanie restartu urządzenia...");    
      // Bezpieczny restart urządzenia dostarczany przez bibliotekę Supla
      SuplaDevice.softRestart();
      return true;
    }
    return false;
  }
};


class ButtonDownloadConfig : public Supla::HtmlElement {
public:
  ButtonDownloadConfig() : HtmlElement(Supla::HtmlSection::HTML_SECTION_BUTTON_AFTER) {};
  void send(Supla::WebSender* sender) override {
    sender->send(
        "<a href=\"/download-config\" style=\"text-decoration:none;\">"
        "<button type=\"button\">Pobierz plik konfiguracyjny (Backup)</button>"
        "</a>");
  }
};

class ButtonUploadConfig : public Supla::HtmlElement {
  public:
    ButtonUploadConfig() : HtmlElement(Supla::HtmlSection::HTML_SECTION_BUTTON_AFTER) {}
    void send(Supla::WebSender* sender) override {
        sender->send(
            "<div class=\"w\" style=\"margin-top:15px; border:1px dashed #ccc; padding:15px; border-radius:4px;\">"
            "<b style=\"display:block; margin-bottom:8px;\">⚙️ Przywróć konfigurację z pliku:</b>"
            "<form method=\"POST\" action=\"/upload-config\" enctype=\"multipart/form-data\">"
            "<input type=\"file\" name=\"update\" style=\"margin-bottom:10px; width:100%;\"><br>"
            "<button type=\"submit\" style=\"background-color:#007bff; color:white; border:none; padding:8px 15px; border-radius:4px; cursor:pointer; width:100%; font-size:14px;\">"
            "📤 Wgraj i przywróć konfigurację"
            "</button>"
            "</form>"
            "</div>"
        );
    }
};

void SprinklerSection::send(Supla::WebSender* sender) {
  sender->send("<h4>");
  sender->send(h4title);
  sender->send("</h4>");
}
  /*
  config.init();
  config.loadStorage();   // załaduj ponownie po odświeżeniu strony i przed ponownym jej generowaniem 
  sender->labeledField(SCHEDULE_START_TIME, "Czas startu nawodnienia", [&]() {
    Supla::HtmlTag selectTag = sender->selectTag(SCHEDULE_START_TIME, SCHEDULE_START_TIME);
    selectTag.body([&]() {
      for (int i=0;i<24;i++) {
        char hour[6];
        sprintf(hour, "%d:00", i);
        sender->selectOption(i, hour, i==config.getScheduleHour());
      }
    });
  });

  sender->labeledField(SPRINKLER_TIME_SHORT, "Zraszacz - czas krótki", [&]() {
    sender->numberInput(SPRINKLER_TIME_SHORT,
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(30, 0),
                            .value = Supla::fixed(config.getSprShort(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField(SPRINKLER_TIME_LONG, "Zraszacz - czas długi", [&]() {
    sender->numberInput(SPRINKLER_TIME_LONG,
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(60, 0),
                            .value = Supla::fixed(config.getSprLong(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField(DROPS_TIME_SHORT, "Kropelkowe - czas krótki", [&]() {
    sender->numberInput(DROPS_TIME_SHORT,
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(60, 0),
                            .value = Supla::fixed(config.getDropShort(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });
  sender->labeledField(DROPS_TIME_LONG, "Kropelkowe - czas długi", [&]() {
    sender->numberInput(DROPS_TIME_LONG,
                        {
                            .min = Supla::fixed(1, 0),
                            .max = Supla::fixed(100, 0),
                            .value = Supla::fixed(config.getDropLong(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
  sender->send("</div><div class=\"box\"><h3>Parametry przekaźników</h3>");
  sender->labeledField(LIGHT_ON_TIME, "Czas włączenia światła przed szopą", [&]() {
    sender->numberInput(LIGHT_ON_TIME,
                        {
                            .min = Supla::fixed(30, 0),
                            .max = Supla::fixed(600, 0),
                            .value = Supla::fixed(config.getLightActivationTimeS(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
  sender->send("</div><div class=\"box\"><h3>Parametry czujników</h3>");
  for (int i=0; i<sensors.getSensorsAmount(); i++) {
    char param[17], defaultName[17], address[64] = "Czujnik temperatury ";
    snprintf(param, sizeof(param), "%s%d", TEMP_PARAM_HEADER, i);
    sensors.getDeviceAdress(i, address+strlen(address), 64-strlen(address));

    sender->labeledField(param, address, [&]() {
      sender->textInput(address,
                 param, 
                 config.getSensorName(i), 
                 16
      );
    });
  }
  sender->send("</div><div class=\"box\"><h3>Parametry powiadomień</h3>");
  sender->labeledField(SCHEDULE_MESSAGE_HOUR, "Godzina komunikatu o programie nawodnienia", [&]() {
    Supla::HtmlTag selectTag = sender->selectTag(SCHEDULE_MESSAGE_HOUR, SCHEDULE_MESSAGE_HOUR);
    selectTag.body([&]() {
      sender->selectOption(-1, "Komunikat wyłączony", false);
      for (int i=0;i<24;i++) {
        char hour[6];
        sprintf(hour, "%d:00", i);
        sender->selectOption(i, hour, i==config.getMessageHour());
      }
    });
  });
  sender->labeledField(LIGHT_MESSAGE_TIME, "Czas powiadomienia o włączonym świetle przed szopą", [&]() {
    sender->numberInput(LIGHT_MESSAGE_TIME,
                        {
                            .min = Supla::fixed(15, 0),
                            .max = Supla::fixed(60, 0),
                            .value = Supla::fixed(config.getLightTimeSMessage(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
  sender->labeledField(PUMP_MESSAGE_TIME, "Czas powiadomienia o włączonej pompie", [&]() {
    sender->numberInput(PUMP_MESSAGE_TIME,
                        {
                            .min = Supla::fixed(30, 0),
                            .max = Supla::fixed(120, 0),
                            .value = Supla::fixed(config.getPumpTimeSMessage(), 0),
                            .step = Supla::fixed(1, 0),
                        });
  });  
}
*/
void SprinklerConfig::initialize() {
  auto storage = Supla::Storage::ConfigInstance();
  this->init();

  new Supla::Html::DeviceInfo(&SuplaDevice);
  new Supla::Html::EthernetParameters();
  new Supla::Html::WifiParameters();
  new Supla::Html::ProtocolParameters();
  //new SprinklerSection();

  new SprinklerSection("Czasy nawodnienia");
  new Supla::Html::CustomParameter(SPRINKLER_TIME_SHORT, "Zraszacz - czas krótki [min]", sprShort, 1, 30);
  new Supla::Html::CustomParameter(SPRINKLER_TIME_LONG, "Zraszacz - czas długi [min]", sprLong, 1, 60);
  new Supla::Html::CustomParameter(DROPS_TIME_SHORT, "Kropelkowe - czas krótki [min]", dropShort, 1, 60);
  new Supla::Html::CustomParameter(DROPS_TIME_LONG, "Kropelkowe - czas długi [min]", dropLong, 1, 120);
  new Supla::Html::CustomParameter(PUMP_ADVANCE_OFF_TIME, "Czas wyprzedzenia wyłączenia pompy [s]", pumpAdvOffTimeS, 3, 60);

  new SprinklerSection("Harmonogram i powiadomienia z systemu nawodnienia");
  auto time = new Supla::Html::SelectInputParameter(SCHEDULE_START_TIME, "Godzina startu nawodnienia");
  for (int i=0;i<24;i++) {
    char hour[6];
    snprintf(hour, 6, "%d:00", i);
    time->registerValue(hour, i);
  }
  time = new Supla::Html::SelectInputParameter(SCHEDULE_MESSAGE_HOUR, "Godzina komunikatu o programie nawodnienia");
  time->registerValue("Komunikat wyłączony", -1);
  for (int i=0;i<24;i++) {
    char hour[6];
    snprintf(hour, 6, "%d:00", i);
    time->registerValue(hour, i);
  }
  new Supla::Html::CustomParameter(PUMP_MESSAGE_TIME, "Czas powiadomienia o włączonej pompie [min]", pumpTimeSMessage, 20, 120);
  
  time = new Supla::Html::SelectInputParameter(DOOR_MESSAGE_HOUR_START, "Godzina startu komunikatu o otwarciu drzwi");
  time->registerValue("Komunikat wyłączony", -1);
  for (int i=0;i<24;i++) {
    char hour[6];
    snprintf(hour, 6, "%d:00", i);
    time->registerValue(hour, i);
  }

  time = new Supla::Html::SelectInputParameter(DOOR_MESSAGE_HOUR_END, "Godzina końca komunikatu o otwarciu drzwi");
  for (int i=0;i<24;i++) {
    char hour[6];
    snprintf(hour, 6, "%d:00", i);
    time->registerValue(hour, i);
  }

  new SprinklerSection("Parametry światła przed szopą");
  new Supla::Html::CustomParameter(LIGHT_ON_TIME, "Czas automatycznego wyłączenia światła przed szopą [s]", lightActivationTimeS, 30, 600);
  new Supla::Html::CustomParameter(LIGHT_MESSAGE_TIME, "Czas powiadomienia o włączonym świetle przed szopą [min]", lightTimeSMessage, 10, 120);
  

  new SprinklerSection("Nazwy czujników temperatury");

  for (int i=0; i<sensors.getSensorsAmount(); i++) {
    char param[16], address[64] = "Czujnik temperatury ";
    sprintf(param, "%s%d", TEMP_PARAM_HEADER, i);
    sensors.getDeviceAdress(i, address+strlen(address), 64-strlen(address));
    SUPLA_LOG_DEBUG("info z konfiga: ");
    SUPLA_LOG_DEBUG(address);
    new Supla::Html::CustomTextParameter(param, address , 64);
  }

  new SprinklerSection("Debug");

  new Supla::Html::CustomCheckboxParameter(TELNET_DEBUG, "Usługa Telnet dla wyjścia Serial", telnetDebug);

  new Supla::Html::ButtonRefresh();
  new Supla::Html::ButtonUpdate(&suplaServer);
  new ButtonReset();
  //new ButtonDownloadConfig();
  //new ButtonUploadConfig();

  SuplaDevice.setInitialMode(Supla::InitialMode::StartInCfgMode);
  SuplaDevice.setLeaveCfgModeAfterInactivityMin(5);
  loadStorage();
  applyCert();
  loadNetwork();
  setupDownloadConfig();
  SUPLA_LOG_DEBUG("Zakończone ustawianie konfiguracji.");
}

void SprinklerConfig::loadStorage() {    
  SUPLA_LOG_DEBUG("ładuję storage loadStorage()");
  if (this->getInt32(SPRINKLER_TIME_SHORT, &sprShort))
    SUPLA_LOG_DEBUG("wartość SPR_SHORT %d", sprShort);
  else
    SUPLA_LOG_DEBUG("wartość SPR_SHORT nie została prawidłowo odczytana");
  this->getInt32(SPRINKLER_TIME_LONG, &sprLong);
  this->getInt32(DROPS_TIME_SHORT, &dropShort);
  this->getInt32(DROPS_TIME_LONG, &dropLong);
  this->getInt32(SCHEDULE_START_TIME, &scheduleHour);
  this->getInt32(SCHEDULE_MESSAGE_HOUR, &scheduleMessageHour);
  this->getInt32(DOOR_MESSAGE_HOUR_START, &doorMessageHourStart);
  this->getInt32(DOOR_MESSAGE_HOUR_END, &doorMessageHourEnd);
  this->getInt32(LIGHT_ON_TIME, &lightActivationTimeS);
  this->getInt32(LIGHT_MESSAGE_TIME, &lightTimeSMessage);
  this->getInt32(PUMP_MESSAGE_TIME, &pumpTimeSMessage);
  this->getInt32(PUMP_ADVANCE_OFF_TIME, &pumpAdvOffTimeS);
  this->getUInt8(TELNET_DEBUG, &telnetDebug);
   
  for (int i=0; i<sensors.getSensorsAmount(); i++) {
    char param[17];
    snprintf(param, 16, "%s%d", TEMP_PARAM_HEADER, i);
    this->getString(param, sensorNames[i], 16);
  }
}

void SprinklerConfig::loadNetwork() {
  uint8_t ethDisabled = 1;
  this->getUInt8(Supla::EthDisableTag, &ethDisabled);
 
  SUPLA_LOG_DEBUG("RESTORED CONFIG VALUES: %d %d %d %d %d %d %d", sprShort, sprLong, dropShort, dropLong, scheduleHour, lightActivationTimeS, ethDisabled);
  if (!ethDisabled) 
    setupNetwork(Supla::Network::IntfType::Ethernet);
  else
    setupNetwork(Supla::Network::IntfType::WiFi);
}

void SprinklerConfig::applyCert() {
  int32_t sec = 2;
  char custom_cert[3000]="";
  this->getInt32("sec", &sec);
  switch (sec) {
    case 0:
      SuplaDevice.setSuplaCACert(suplaCACert);
      SUPLA_LOG_DEBUG("Aplikuję standardowy certyfikat");
       //SuplaDevice.setSupla3rdPartyCACert(supla3rdCACert);
      break;
    case 1:
      if (this->getString("custom_ca", custom_cert, sizeof(custom_cert))) {
        SuplaDevice.setSuplaCACert(custom_cert);
        SUPLA_LOG_DEBUG("Aplikuję własny certyfikat (długość %d)", strlen(custom_cert));
      }
      break;
    default:
      break;  // bez certyfikatu
  }
}

void SprinklerConfig::setupNetwork(Supla::Network::IntfType type) {
  if (network!=nullptr) {
    SUPLA_LOG_DEBUG("Uruchamianie awaryjnego połączenia bezprzewodowego WI-FI w trybie konfiguracji...");
    //network = new Supla::ESPWifi();
    //network->SetSetupNeeded();
    Supla::Storage::ConfigInstance()->setUInt8(Supla::EthDisableTag, 1);
    SUPLA_LOG_DEBUG("Miękki restart. Przełączam na WI-FI...");
    SuplaDevice.softRestart();
    return;
  }
  if (type == Supla::Network::IntfType::Ethernet) {
    SUPLA_LOG_DEBUG("Uruchamianie połączenia kablowego ETHERNET...");
    network = new Supla::ESPETH(ETH_TYPE, ETH_ADDR, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_POWER_PIN, ETH_CLK_MODE);
  } else if (type == Supla::Network::IntfType::WiFi){
    SUPLA_LOG_DEBUG("Uruchamianie połączenia bezprzewodowego WI-FI...");
    network = new Supla::ESPWifi();
    //network->SetSetupNeeded();
  }

}
File uploadFile;
void SprinklerConfig::setupDownloadConfig() {
  auto serverPtr = suplaServer.getServerPtr();
  if (serverPtr != nullptr) {
      serverPtr->on("/download-config", HTTP_GET, []() {
          String filename = "/supla-dev.cfg"; 

          if (LittleFS.exists(filename)) {
              File file = LittleFS.open(filename, "r");
              if (file) {
                  // Wysyłamy plik jako załącznik (wymusi to pobranie pliku przez przeglądarkę)
                  suplaServer.getServerPtr()->sendHeader("Content-Disposition", "attachment; filename=supla-dev.cfg");
                  suplaServer.getServerPtr()->streamFile(file, "application/json");
                  file.close();
                  return;
              }
          }
          
          // Komunikat błędu, jeśli plik nie istnieje lub jest zablokowany
          suplaServer.getServerPtr()->send(404, "text/plain", "Blad: Plik konfiguracyjny nie istnieje lub nie udalo sie go otworzyc.");
      });
      serverPtr->on("/upload-config", HTTP_POST, []() {
            suplaServer.getServerPtr()->send(200, "text/html", 
                "<h3>Plik zostal wgrany pomyslnie!</h3><p>Urządzenie uruchamia sie ponownie z nowa konfiguracja...</p>");
            delay(1000);
            ESP.restart(); // Restart urządzenia w celu wczytania nowych ustawień
        }, 
        // Druga funkcja przetwarza napływające porcje (bity) pliku w tle (w trybie uploader)
        []() {
            HTTPUpload& upload = suplaServer.getServerPtr()->upload();
            
            if (upload.status == UPLOAD_FILE_START) {
                Serial.printf("Rozpoczęto wgrywanie pliku: %s\n", upload.filename.c_str());
                // Otwieramy plik konfiguracyjny w trybie zapisu (nadpisywanie)
                uploadFile = LittleFS.open("/config.json", "w");
            } 
            else if (upload.status == UPLOAD_FILE_WRITE) {
                if (uploadFile) {
                    // Zapisujemy odebrany bufor danych do pamięci Flash
                    uploadFile.write(upload.buf, upload.currentSize);
                }
            } 
            else if (upload.status == UPLOAD_FILE_END) {
                if (uploadFile) {
                    uploadFile.close();
                    Serial.printf("Pomyślnie zapisano plik. Rozmiar: %d bajtów\n", upload.totalSize);
                } else {
                    Serial.println("Błąd zapisu pliku na partycji LittleFS.");
                }
            }
        });
  }
}

bool SprinklerConfig::isDoorMessageHourActive(int hour) {
  if (doorMessageHourStart==-1) 
    return false;
  if (doorMessageHourStart<=doorMessageHourEnd)
     return (doorMessageHourStart<=hour && hour<doorMessageHourEnd);
  else  
    return (!(doorMessageHourEnd<=hour && hour<doorMessageHourStart));    
}
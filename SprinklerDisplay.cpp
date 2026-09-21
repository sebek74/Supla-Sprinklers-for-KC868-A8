#include <SuplaDevice.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <supla/control/virtual_relay.h>
#include <math.h>
#include <DallasTemperature.h>
#include "Definitions.h"
#include "SprinklerSensors.h"
#include "Definitions.h"
#include "SprinklerRelay.h"
#include "SprinklerConfig.h"
#include "SprinklerProgramRelay.h"
#include "SprinklerDisplay.h"
#include "Bitmaps.h"
#include "ArialMT10pt7b.h"
#include "ArialMT7pt7b.h"
#include "ArialMT5pt7b.h" 
/*
 & running man
 ' trybik
 ( termometr
 ) dzwoneczek
 * zegarek
 [ ą
 \ ć
 ] ę
 ^ ł
 _ ń
 ` ó 
 { ś
 | ż
 } ź
*/

extern SprinklerSensors sensors;
extern SprinklerConfig config;
extern const char* relayNames[];

bool SprinklerDisplay::Initialize() {

#ifdef USE_SSD1306
 #define WHITE_COLOR SSD1306_WHITE
 #define INVERSE_COLOR SSD1306_INVERSE
 #define BLACK_COLOR SSD1306_INVERSE
 if (!begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, true)) { 
    Serial.println(F("Nie znaleziono wyświetlacza SSD1306"));
    return false;
  } else {
    Serial.println(F("Wyświetlacz SSD1306 znaleziony"));
    initialized = true;
    return true;
  }
#endif
#ifdef USE_SH1106G
  #define WHITE_COLOR SH110X_WHITE
  #define INVERSE_COLOR SH110X_INVERSE
  #define BLACK_COLOR SH110X_BLACK
  if (!begin(SCREEN_ADDRESS, true)) { 
    Serial.println(F("Nie znaleziono wyświetlacza SH1106"));
    return false;
  } else {
    Serial.println(F("Wyświetlacz SSH1106 znaleziony"));
    initialized = true;
    return true;
  }
#endif 
}

int32_t frameId = 0;
int pumpPhases[] = {0,1,2,1};

void SprinklerDisplay::updateView() {
  if (!initialized) return;
  if (millis() - lastDisplayUpdate < DISPLAY_INTERVAL) return;
  lastDisplayUpdate = millis();
  frameId++;    
  if (SuplaDevice.getDeviceMode() == Supla::DeviceMode::DEVICE_MODE_CONFIG) { 
    displayConfig();
    return;
  } 
  if (screenOff) return;
  clearDisplay();
  dimContrast();
  clearDisplay();
  setTextColor(WHITE_COLOR); // Draw white text
    
  // piktogramy zaworów
  setFont(&ArialMT5pt7b);
  setTextColor(WHITE_COLOR);

  for (int i=RelayId::_FirstValve; i<=RelayId::_LastValve; i++) { 
    if (frameId%3<2 || !SprinklerRelay::getRelayById(i)->isOn()) {
      if (i==this->editMode) fillRect(17*(i-1), 0, 14, 14, WHITE_COLOR);  //negatyw
      drawBitmap(17*(i-1)+1, 1, relay_bits[i-1], 12, 12, (i!=this->editMode)?WHITE_COLOR:BLACK_COLOR);
    }
    uint32_t scheduledTime = SprinklerRelay::getScheduledProgramTime((RelayId)i);
    if (scheduledTime==0) continue;
    char number[4];
    snprintf(number, 4, "%d", scheduledTime);
    int16_t x1, y1, offset;
    uint16_t w, h;
    getTextBounds(number, 0, 0, &x1, &y1, &w, &h);
    setCursor(5+17*(i-1)-w/2, TOP_ROW);
    print(scheduledTime);
  }

  drawSchedulerIcon();
  drawNetworkIcon();

  if (!drawEditMode()) {
    // pompa
    if (SprinklerRelay::getRelayById(RelayId::Pump)->isOn()) {
      if (frameId%3<2) drawBitmap(111, 39, pump_bits[pumpPhases[frameId%4]], 9, 21, WHITE_COLOR);
    }
    else 
      drawBitmap(111, 39, pump_bits[1], 9, 21, WHITE_COLOR);
    drawTank();
    if (!drawProgramTimers())
      drawSensor();
  }
  display();
}

void SprinklerDisplay::drawSchedulerIcon() {
  // dzwonek schedulera
  if (SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->isOn()) {
    if (SprinklerRelay::getProgramRelay()->getProgramInProgress() && SprinklerRelay::getProgramRelay()->isRunScheduled())
      switch ((frameId/2)%3) {
        case 0: drawBitmap(84, 0, bell1Bitmap24x16, 24, 16, WHITE_COLOR); break;
        case 1: drawBitmap(84, 0, bell2Bitmap24x16, 24, 16, WHITE_COLOR); break;
        case 2: drawBitmap(84, 0, bell3Bitmap24x16, 24, 16, WHITE_COLOR); break;
      }
      else {
        drawBitmap(84, 0, bell1Bitmap24x16, 24, 16, WHITE_COLOR);
        int16_t x1, y1, offset;
        uint16_t w, h;
        char sensorData[9];
        time_t time = config.makeScheduleTimeToday();
        struct tm* timeinfo = localtime(&time);
        snprintf(sensorData, 9, "%d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        setFont(&ArialMT5pt7b);
        setTextColor(WHITE_COLOR);
        getTextBounds(sensorData, 0, 0, &x1, &y1, &w, &h);
        setCursor(SCREEN_WIDTH-32 - w/2, TOP_ROW);
        print(sensorData); 
      }
  }
};

bool SprinklerDisplay::drawProgramTimers() {
  bool result = false;
  setTextColor(WHITE_COLOR); // Draw white text
  RelayId activeValveId = SprinklerRelay::getActiveValveId();
  uint32_t relayTime = 0;
  if (activeValveId>0) 
    SprinklerRelay::getValveById(activeValveId)->getRemainingCountdownTimerSec(&relayTime);
  uint32_t programTime = SprinklerRelay::getProgramRelay()->getCountdownTimerRemainingTimeSec();

  setCursor(25, 43);
  if (relayTime>0) {
    if (relayTime/3600>0) {
      setFont(&ArialMT10pt7b);
      printf("%0d:%02d", relayTime/3600, (relayTime/60)%60);
      setFont(&ArialMT7pt7b);
      printf(":%02d", relayTime%60);
    } else {
      setFont(&ArialMT10pt7b);
      printf("%02d", relayTime/60);
      setFont(&ArialMT7pt7b);
      printf(":%02d", relayTime%60);
    }
    result = true;
    RelayId valveId = SprinklerRelay::getActiveValveId();
    if (valveId>0) 
      drawBitmap(6, 31, relay_bits[valveId-1], 12, 12, WHITE_COLOR); 
  } 
  
  if (SprinklerRelay::getProgramRelay()->getProgramInProgress()) {
    setCursor(25, 63);
    if (programTime/3600>0) {
      setFont(&ArialMT10pt7b);
      printf("%0d:%02d", programTime/3600, (programTime/60)%60);
      setFont(&ArialMT7pt7b);
      printf(":%02d", programTime%60);
    } else {
      setFont(&ArialMT10pt7b);
      printf("%02d", programTime/60);
      setFont(&ArialMT7pt7b);
      printf(":%02d", programTime%60);
    }
    result = true;   
    drawBitmap(5, 49, clock_bits[frameId%4], 15, 15, WHITE_COLOR);
  }
  return result;
}

void SprinklerDisplay::drawSensor() {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  int32_t pagesAmount = sensors.getSensorsAmount();
  if (timeinfo->tm_year>120) pagesAmount++;
  if (SprinklerRelay::isScheduleCycleEnabled()) pagesAmount++;

  if (enterPageTime==0) enterPageTime = millis();
  if (millis()-enterPageTime>SENSOR_PAGE_TIME) {
    sensorPage++;
    enterPageTime = millis();
  }
  if (sensorPage>=pagesAmount) {
    sensorPage = 0;
    enterPageTime = millis();
  }

  char description[17];
  char sensorData1[12] = "", sensorData2[9] = "", sensorData3[9] = "";
 
  int16_t x1, y1;
  uint16_t w=0, w1=0, h, w2=0, w3=0;

  // czas
  if (sensorPage==sensors.getSensorsAmount() && timeinfo->tm_year>120) {
    strcpy(description, "*Zegar");
    snprintf(sensorData1, 9, "%d%s%02d", timeinfo->tm_hour, (timeinfo->tm_sec%2)?":":"~", timeinfo->tm_min);
    snprintf(sensorData2, 9, "%s%02d", (timeinfo->tm_sec%2)?":":"~", timeinfo->tm_sec);
  } // godzina startu
  else if (SprinklerRelay::isScheduleCycleEnabled() && 
          ((sensorPage==sensors.getSensorsAmount() && timeinfo->tm_year<=120) || 
              (sensorPage==sensors.getSensorsAmount()+1 && timeinfo->tm_year>120))) {
    strcpy(description, ")Godzina startu");
    time_t time = config.makeScheduleTimeToday();
    struct tm* timeinfo = localtime(&time);
    snprintf(sensorData1, 9, "%d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  } // status drzwi
  else {
    //drawBitmap(-2, 30, termometr_bits, 32, 33, WHITE_COLOR);
    description[0] = '(';
    strcpy(description+1, sensors.getDeviceDescription(sensorPage));
    double calkowita;
    double ulamkowa = modf(sensors.getTemp(sensorPage), &calkowita);
    snprintf(sensorData1, 9, "%.0f", calkowita);
    snprintf(sensorData2, 9, ".%1.0f", fabs(ulamkowa*10));
    snprintf(sensorData3, 9, "'C");
  }

  setFont(&ArialMT5pt7b);
  setTextColor(WHITE_COLOR);
  getTextBounds(P(description), 0, 30, &x1, &y1, &w, &h); 
  setCursor((SCREEN_WIDTH - w) / 2 -9, MIDDLE_ROW);
  print(P(description));

  if (strlen(sensorData1)>0) {
    setFont(&ArialMT10pt7b);
    getTextBounds(String(sensorData1), 0, 63, &x1, &y1, &w1, &h);   
  }
  if (strlen(sensorData2)>0) {
    setFont(&ArialMT7pt7b);
    getTextBounds(String(sensorData2), 0, 63, &x1, &y1, &w2, &h);
  }
  if (strlen(sensorData3)>0) {
    setFont(&ArialMT10pt7b);
    getTextBounds(String(sensorData3), 0, 63, &x1, &y1, &w3, &h);
  }

  setCursor(((SCREEN_WIDTH - w1 - w2 - w3) / 2) -9, BOTTOM_ROW);
  if (strlen(sensorData1)>0) {
    setFont(&ArialMT10pt7b);
    print(sensorData1);
  }
  if (strlen(sensorData2)>0) {
    setFont(&ArialMT7pt7b);
    print(sensorData2);
  }
  if (strlen(sensorData3)>0) {
    setFont(&ArialMT10pt7b);
    print(sensorData3);
  }

  if (SprinklerRelay::isDoorOpen())
    drawBitmap(0, MIDDLE_ROW-8, door_open_bits, 8, 14, WHITE_COLOR);
  else
    drawBitmap(0, MIDDLE_ROW-6, door_closed_bits, 8, 12, WHITE_COLOR);
  if (SprinklerRelay::isLightOn()) {
    uint32_t remainingSec;
    SprinklerRelay::getRelayById(RelayId::FixedLightSwitch)->getRemainingCountdownTimerSec(&remainingSec);
    if (remainingSec==0 || frameId%3<2)
      drawBitmap(0, MIDDLE_ROW+9, bulb_on_bits, 8, 14, WHITE_COLOR);
  }
  else
    drawBitmap(0, MIDDLE_ROW+10, bulb_off_bits, 8, 12, WHITE_COLOR);
  
  // wyświetl całą linię lub kawałek jeśli pokazuje strzałkę opróżniania lub dolewania
  drawLine(0, TOP_ROW+4, 
    (SprinklerRelay::getRelayById(RelayId::EmptyTank)->isOn() || 
      SprinklerRelay::getRelayById(RelayId::RefillTank)->isOn())?101 : (SCREEN_WIDTH-1), 
      TOP_ROW+4, WHITE_COLOR);
}

// Funkcja rysująca odpowiednią ikonę w prawym górnym rogu ekranu (pozycja X:110, Y:0)
void SprinklerDisplay::drawNetworkIcon() {
  if (!initialized) return;
  if (config.getIntfType() == Supla::Network::IntfType::Ethernet) {
    int status = SuplaDevice.getCurrentStatus();
    
    if (status == STATUS_REGISTERED_AND_READY || ((status == STATUS_INITIALIZED || status == STATUS_REGISTER_IN_PROGRESS) && frameId%2))
      drawBitmap(110, 0, lan_bits, 16, 16, WHITE_COLOR);
    else {
       setFont(&ArialMT5pt7b);
        setTextColor(WHITE_COLOR); // Draw white text
        setCursor(SCREEN_WIDTH-18, 8);
        printf("%02d", status);
        setCursor(SCREEN_WIDTH-14, 16);
        print("x");
    }
  } else if (config.getIntfType() == Supla::Network::IntfType::WiFi) {
    int percent = WiFi.RSSI();
    int status = SuplaDevice.getCurrentStatus();
    if ((status == STATUS_INITIALIZED || status == STATUS_REGISTER_IN_PROGRESS) && frameId%2)
        drawBitmap(110, 0, wifi_bits[0], 16, 16, WHITE_COLOR);
    else if (status != STATUS_REGISTERED_AND_READY) {
        setFont(&ArialMT5pt7b);
        setTextColor(WHITE_COLOR); // Draw white text
        setCursor(SCREEN_WIDTH-18, 8);
        printf("%02d", status);
        setCursor(SCREEN_WIDTH-14, 16);
        print("x");
        return;
    }
    if (percent >= -50) {
      drawBitmap(SCREEN_WIDTH-18, 0, wifi_bits[0], 16, 16, WHITE_COLOR);
    } else if (percent >= -60) {
      drawBitmap(SCREEN_WIDTH-18, 0, wifi_bits[1], 16, 16, WHITE_COLOR);
    } else if (percent >= -70) {
      drawBitmap(SCREEN_WIDTH-18, 0, wifi_bits[2], 16, 16, WHITE_COLOR);
    } else {
      drawBitmap(SCREEN_WIDTH-18, 0, wifi_bits[3], 16, 16, WHITE_COLOR);
    }
  }
}

void SprinklerDisplay::drawTank() {
  drawBitmap(103, 35, tank_bits[0], 25, 29, WHITE_COLOR); // tank
   switch (SprinklerRelay::getTankStatus()) {
    case ActionId::TankRefill:
      if (frameId%2) drawBitmap(103, 35, tank_bits[2], 25, 29, WHITE_COLOR);
      break;
    case ActionId::TankEmpty:
      if (frameId%2) drawBitmap(103, 35, tank_bits[1], 25, 29, WHITE_COLOR);
      break;  
    case ActionId::TankDoNothing:
      drawBitmap(103, 35, tank_bits[3], 25, 29, WHITE_COLOR);
      break;
   }
   if (SprinklerRelay::getRelayById(RelayId::EmptyTank)->isOn() && frameId%2)
       drawBitmap(111, 20, image_arrow_up_bits, 9, 17, WHITE_COLOR);
   if (SprinklerRelay::getRelayById(RelayId::RefillTank)->isOn() && !(frameId%2))
       drawBitmap(111, 20, image_arrow_down_bits, 9, 17, WHITE_COLOR);
}

void SprinklerDisplay::displayConfig() {
  if (!initialized) return;
  restoreFullContrast();
  int16_t x1, y1;
  uint16_t w, h;
  const char* info[4];
  clearDisplay();
  setTextWrap(false);
  setTextColor(WHITE_COLOR);
  String sh(config.getSSID());
  if (config.getIntfType()==Supla::Network::IntfType::Ethernet) {
    info[0] = "Tryb konfiguracyjny";
    info[1] = "Po^[cz się]z sieci[";
    info[2] = "lokaln[ i wybierz adres";
    info[3] = config.getIp().toString().c_str();
    if (strlen(info[3])==0)
      info[3] = "<nieustalony>";
  }
  if (config.getIntfType()==Supla::Network::IntfType::WiFi) {
    info[0] = "Tryb konfiguracyjny";
    info[1] = "Po^[cz si] z sieci[ WiFi";
    info[2] = sh.c_str();
    info[3] = "192.168.4.1"; //config.getIp().toString().c_str();
  }
  int32_t y=-4;
  for (int i=0 ; i<4 ; i++) {
    setFont((i<3)? &ArialMT5pt7b : &ArialMT7pt7b);
    getTextBounds(info[i], 0, 0, &x1, &y1, &w, &h);
    y+=h+5;
    if ((frameId%3==0) && i==0) continue;
    setCursor(max((SCREEN_WIDTH - w) / 2 - x1, 0), y);
    print(info[i]);
  }
  display();
}

void SprinklerDisplay::drawSuplaLogo() {
  if (!initialized) return;
  for (int i = -SCREEN_WIDTH; i<=0; i+=8) {
    clearDisplay();
    drawBitmap(i, 0, supla_logo_bits, 128, 64, WHITE_COLOR);
    setFont(&ArialMT5pt7b);
    setTextColor(WHITE_COLOR);
    char* text = "Powered by";
    int16_t x1, y1;
    uint16_t w, h;
    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    setCursor(SCREEN_WIDTH - w - 8, 8);
    print(text); 
    text = "SuplaDevice";
    getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    setCursor(SCREEN_WIDTH - w - 8, BOTTOM_ROW-2);
    print(text); 
    display();
    delay(15);
  }
}

bool SprinklerDisplay::drawEditMode() {
  int16_t x1, y1;
  uint16_t w, h;
  char info[32];
  setTextWrap(false);
  
  if (millis()-enterEditMode > EDIT_MODE_EXIT_TIME) {
    exitEditMode();
    return false;
  }
  if (editMode!=-1) {
    drawLine(0, TOP_ROW+3, SCREEN_WIDTH-1, TOP_ROW+3, WHITE_COLOR);
    setFont(&ArialMT5pt7b);
    setTextColor(WHITE_COLOR);
    switch (editMode) {
      case Pump:
      case RunCycleNow:
      case RefillTank:
      case EmptyTank:
        info[0]='&';
        break;
      default:
         info[0]='\'';
    }
    strcpy(info+1, (editMode==_EditScheduleTime)? "Czas startu cyklu" : P(relayNames[editMode]).c_str());
    getTextBounds(info, 0, 30, &x1, &y1, &w, &h);
    setCursor((SCREEN_WIDTH - w) / 2, MIDDLE_ROW);
    print(info);
    if (frameId%4<3) {
      setFont(&ArialMT10pt7b);
      setTextColor(WHITE_COLOR);
      if (editMode==RelayId::Pump || editMode==RelayId::EmptyTank || editMode==RefillTank || editMode==RunCycleNow || editMode==ScheduleCycle)
        strcpy(info, SprinklerRelay::getRelayById(editMode)->isOn()?"W\\[CZONY":"WY\\[CZONY");
      else if (editMode==RelayId::_EditScheduleTime)
        sprintf(info, "%0d:00", config.getScheduleHour());
      else
        SprinklerRelay::getRelayTimeText(editMode, info, 16);
      getTextBounds(info, 0, 63, &x1, &y1, &w, &h);
      setCursor((SCREEN_WIDTH - w) / 2, BOTTOM_ROW-3);
      print(info);
    }
    return true;
  }
  else 
    return false;
}

bool SprinklerDisplay::restoreFullContrast() { 
  bool result = screenOff;
  if (screenOff)  {
    oled_command(OLED_ON);
    screenOff = false;
  }
  if (currentContrast<255) {
    setContrast(255);
    currentContrast = 255;
  }
  lastAction = millis();
  return result;
};

void SprinklerDisplay::dimContrast() { 
  if ((millis()-lastAction>DIM_TIME) && (currentContrast>1)) {
    setContrast(1);
    currentContrast = 1;
    //Serial.println("dimming display");
  }
  if ((millis()-lastAction>OFF_TIME) && !screenOff)
  {
    oled_command(OLED_OFF);
    screenOff = true;
  }
  
};

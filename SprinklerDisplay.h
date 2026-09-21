#ifndef OLED_H
#define OLED_H

#include <Adafruit_GFX.h>
#include <Wire.h>
#include "Definitions.h"

#define TOP_ROW 24
#define MIDDLE_ROW 40
#define BOTTOM_ROW 63

#ifdef USE_SH1106G
#include <Adafruit_SH110X.h>
class SprinklerDisplay : public Adafruit_SH1106G {
  public:
    SprinklerDisplay() : Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
#endif

#ifdef USE_SSD1306
#include <Adafruit_SSD1306.h>
class SprinklerDisplay : public Adafruit_SSD1306 {
  public:
    SprinklerDisplay() : Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
#endif
      lastAction = millis();
      enterEditMode = millis();
      enterPageTime = 0;
      currentContrast = 255;
      screenOff = false;
      editMode = -1;
      sensorPage = 0;
    };
  private:
    bool initialized = false;
    int32_t lastAction;
    int32_t lastDisplayUpdate;
    int32_t currentContrast;
    bool screenOff;
    int32_t enterEditMode;
    int32_t editMode;
    int8_t sensorPage;
    int32_t enterPageTime;
  protected:
    void drawNetworkIcon();
    void drawSchedulerIcon();
    void drawTank();
    void displayConfig();
    void drawDisplay();
    void drawSensor();
    bool drawProgramTimers();
    bool getInitialized() { return initialized; };
  public:
    bool Initialize();
    void updateView();
    void drawSuplaLogo();
    bool restoreFullContrast();
    void dimContrast();
    bool drawEditMode();
    void nextEditMode() { 
      editMode=(editMode==RelayId::ScheduleCycle)? -1 :  editMode+1;
      enterEditMode = millis();
    }
    void exitEditMode() { editMode=-1; }
    void pingEditMode() { enterEditMode = millis(); }
    int32_t getEditMode() { return editMode; };

    String P(const char* txt) {
      String temp(txt==nullptr?"":txt);
      temp.replace("Ą", "A"); temp.replace("ą", "[");
      temp.replace("Ć", "C"); temp.replace("ć", "\\");
      temp.replace("Ę", "E"); temp.replace("ę", "]");
      temp.replace("Ł", "L"); temp.replace("ł", "^");
      temp.replace("Ń", "N"); temp.replace("ń", "_");
      temp.replace("Ó", "O"); temp.replace("ó", "`");
      temp.replace("Ś", "S"); temp.replace("ś", "{");
      temp.replace("Ź", "Z"); temp.replace("ź", "|");
      temp.replace("Ż", "Z"); temp.replace("ż", "}");
      return temp;
    } 
};

#endif //OLED_H
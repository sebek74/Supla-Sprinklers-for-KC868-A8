#include <stdint.h>
#include <SuplaDevice.h>

#define MSG_PUMP_ON  "Pompa włączona"
#define MSG_PUMP_OFF  "Pompa wyłączona"
#define MSG_SCHEDULE_ON  "Harmonogram nawodnienia włączony na godzinę %d:00"
#define MSG_SCHEDULE_OFF  "Harmonogram nawodnienia wyłączony"
#define MSG_PROGRAM_ON  "Program nawodnienia rozpoczęty (%d minut)"
#define MSG_PROGRAM_OFF  "Program nawodnienia zakończony"
#define MSG_ALL_OFF   "Wszystkie zawory i pompa wyłączone."
#define MSG_TANK_EMPTY_ON  "Rozpoczęcie awaryjnego opróżnienia."
#define MSG_TANK_EMPTY_OFF  "Zakończenie awaryjnego opróżnienia." 
#define MSG_TANK_REFILL_ON  "Rozpoczęcie dolewania wody."
#define MSG_TANK_REFILL_OFF  "Zakończenie dolewania wody."
#define MSG_SCHEDULE_FAILED  "Harmonogram nawodnienia pominięty. Nie wskazano pracy do wykonania."
#define MSG_DAILY_MESSAGE_ENABLED  "Harmonogram podlewania jest ustawiony (%d minut)."
#define MSG_DAILY_MESSAGE_DISABLED  "Harmonogram podlewania nie jest ustawiony."
#define MSG_DAILY_MESSAGE_NO_WORK  "Harmonogram podlewania jest pusty!"
#define MSG_PUMP_ON_X_MIN   "Pompa włączona od %d minut"
#define MSG_LIGHT_ON_X_MIN   "Światło przed szopą włączone od %d minut"
#define MSG_DOOR_OPEN  "Drzwi do szopy otwarte o %d:%02d"

#define MSG_TITLE "Sterownik nawodnienia"

class SprinklerMessenger {
public: 
  void initialize();
  void sendMessage(const char* code, uint32_t param1=0,  uint32_t param2=0);
  void sendMessage(int16_t context, const char* code, uint32_t param1=0,  uint32_t param2=0);
};
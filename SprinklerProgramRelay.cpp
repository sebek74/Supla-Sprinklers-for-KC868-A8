#include <supla/control/virtual_relay.h>
#include <SuplaDevice.h>
#include <supla/actions.h>
#include <Wire.h>
#include <supla/control/relay.h>
#include <supla/events.h>
#include <supla/control/button.h>
#include <supla/sensor/binary.h>
#include <supla/control/action_trigger.h>
#include "SprinklerProgramRelay.h"
#include "SprinklerRelay.h"
#include "SprinklerDisplay.h"
#include "SprinklerMessenger.h"

extern SprinklerMessenger messenger;
extern SprinklerDisplay display;
bool overridetime = true;
void SprinklerProgramRelay::updateRemainingTime(_supla_int_t addDuration) {
  //mojCzasDzialaniaMs += addDuration;
  //this->durationMs = mojCzasDzialaniaMs - momentWlaczeniaMs;
  //this->getChannel()->setNewValue(this->isOn(), mojCzasDzialaniaMs-momentWlaczeniaMs);
  overridetime = false;
  turnOff();
  turnOn(mojCzasDzialaniaMs-(millis()-momentWlaczeniaMs)+addDuration);  // Resetujemy punkt odniesienia do teraz
  overridetime = true;
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay updateRemainingTime!!! %d/%d\n", addDuration, this->durationMs);
}

void SprinklerProgramRelay::turnOn(_supla_int_t duration) {
  if (overridetime && SprinklerRelay::calculateProgramTimeMs()==0) {  // żaden z kroków programy nie został ustawiony 
      overridetime = false;
      turnOff();
      overridetime = true;
      return;
  }
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay turnOn step 1!!! %d, %d\n", overridetime?1:0, duration);
  if(overridetime) {
    mojCzasDzialaniaMs = SprinklerRelay::calculateProgramTimeMs();
    messenger.sendMessage(Msg::PROGRAM_ON, mojCzasDzialaniaMs/MS_IN_MIN);
    display.exitEditMode();
  }
  else {
    mojCzasDzialaniaMs = duration;
  }
  duration = mojCzasDzialaniaMs;       
  momentWlaczeniaMs = millis();
  czyOdlicza = true;
  VirtualRelay::turnOn(mojCzasDzialaniaMs);
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay turnOn step 2!!! %d/%d\n", duration, this->durationMs);
  this->durationMs = mojCzasDzialaniaMs;
  programInProgress = true;
} 

void SprinklerProgramRelay::turnOff(_supla_int_t duration) {
    czyOdlicza = false;
    VirtualRelay::turnOff(duration);
    SUPLA_LOG_DEBUG("handle SprinklerProgramRelay turnOff!!! %d, %d\n", overridetime?1:0, duration);
    if (overridetime) SprinklerRelay::completeProgram();
}

// 4. KLUCZ: Nadpisujemy pętlę logiczną, aby kontrolować stan odliczania
void SprinklerProgramRelay::iterateAlways() {
    Supla::Control::Relay::iterateAlways();

    // Jeśli czas minął, upewniamy się, że flaga odliczania zgasła
    if (czyOdlicza && (millis() - momentWlaczeniaMs >= mojCzasDzialaniaMs)) {
        czyOdlicza = false;
        programInProgress = false;
        if (!SprinklerRelay::isPumpRequired())
          SprinklerRelay::getRelayById(RelayId::Pump)->turnOff();
        SprinklerRelay::disableScheduleCycle(DISABLE_MESSAGE);
    }
}

// 5. TAJNA METODA: To z niej aplikacja pobiera informację o zegarku na ekranie!
// Nadpisujemy ją, zwracając nasz własny wyliczony czas w sekundach
int32_t SprinklerProgramRelay::handleNewValueFromServer(TSD_SuplaChannelNewValue *newValue) {
    // Wywołujemy oryginalną metodę, aby SUPLA obsłużyła stan pinu
    int32_t result = VirtualRelay::handleNewValueFromServer(newValue);
   
    // Jeśli przekaźnik się właśnie włączył z aplikacji, resetujemy nasz licznik
    if (this->isOn()) {
        if (!czyOdlicza) {
            momentWlaczeniaMs = millis();
            czyOdlicza = true;
        }
    }
    uint32_t  val = getCountdownTimerRemainingTimeSec();
    if (val>0)  newValue->DurationMS = val;
    SUPLA_LOG_DEBUG("handle SprinklerProgramRelay::handleNewValueFromServer!!! %d\n", val);
    return result;
}

// Jeśli Twoja wersja biblioteki posiada metodę zwracającą pozostały czas do aplikacji,
// to to mapowanie upewni się, że aplikacja dostanie właściwą liczbę sekund:
uint32_t SprinklerProgramRelay::getCountdownTimerRemainingTimeSec() {
    if (!czyOdlicza || !this->isOn()) {
        return 0;
    }
    unsigned long minelo = millis() - momentWlaczeniaMs;
    if (minelo >= mojCzasDzialaniaMs) {
        return 0;
    }
    SUPLA_LOG_DEBUG("programRelay momentWlaczeniaMs=%d, mojCzasDzialaniaMs=%d, minelo=%d\n", momentWlaczeniaMs, mojCzasDzialaniaMs, minelo);
    return 1+(mojCzasDzialaniaMs - minelo) / 1000;
}

  void SprinklerProgramRelay::startCycleNow(bool scheduled) { 
      turnOn(); 
      runScheduled = scheduled; 
      programInProgress = true;
  }

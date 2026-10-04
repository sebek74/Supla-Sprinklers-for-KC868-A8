#include <supla/control/virtual_relay.h>
#include <SuplaDevice.h>
#include <supla/actions.h>
#include <Wire.h>
#include <supla/control/relay.h>
#include <supla/events.h>
#include <supla/control/button.h>
#include <supla/sensor/binary.h>
#include <supla/control/action_trigger.h>
#include "Definitions.h"
#include "SprinklerProgramRelay.h"
#include "SprinklerRelay.h"
#include "SprinklerDisplay.h"
#include "SprinklerMessenger.h"

extern SprinklerMessenger messenger;
extern SprinklerDisplay display;

void SprinklerProgramRelay::startCycleNow(bool scheduled) { 
  startTimeMs = millis();
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay startCycleNow");
  calculatedTimeMs = SprinklerRelay::calculateProgramTimeMs();
  messenger.sendMessage(MSG_PROGRAM_ON, calculatedTimeMs/MS_IN_MIN);
  turnOn(); 
  runScheduled = scheduled; 
}

void SprinklerProgramRelay::updateRemainingTime(_supla_int_t addDurationMs) {
  calculatedTimeMs+=addDurationMs;
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay updateRemainingTime %d/%d", addDurationMs, this->durationMs);
  turnOn(startTimeMs-millis()+calculatedTimeMs);  // Resetujemy punkt odniesienia do teraz
}

void SprinklerProgramRelay::recalculateRemainingTime() {
  if (!getProgramInProgress()) return;
  _supla_int_t newDurationMs = SprinklerRelay::calculateProgramTimeMs();
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay recalculateRemainingTime %d", newDurationMs);
  if (newDurationMs>0) {
    startTimeMs = millis();
    calculatedTimeMs = newDurationMs;
    turnOn(newDurationMs);
  }
  else
    turnOff(0);
}

void SprinklerProgramRelay::turnOn(_supla_int_t newDurationMs) {
  display.exitEditMode();
  if (inProgress && newDurationMs==0)
    return; // zapobiegnij skasowaniu odliczania
  if (!inProgress) {  // przełączenie bezpośrednio w aplikacji lub chmurze
      newDurationMs = SprinklerRelay::calculateProgramTimeMs();
      if (newDurationMs==0) {   // żaden z kroków programu nie został ustawiony
        turnOff();
        startTimeMs = 0;
        return;
      }
      calculatedTimeMs = newDurationMs;
      messenger.sendMessage(MSG_PROGRAM_ON, newDurationMs/MS_IN_MIN);
      startTimeMs = millis();
  } 
  //else  
    //messenger.sendMessage(MSG_PROGRAM_ON, newDurationMs/MS_IN_MIN);
  SUPLA_LOG_DEBUG("handle SprinklerProgramRelay turnOn(%d)", newDurationMs);
  VirtualRelay::turnOn(newDurationMs);
  VirtualRelay::durationMs = newDurationMs;
  inProgress = true;
} 

void SprinklerProgramRelay::turnOff(_supla_int_t duration) {
    VirtualRelay::turnOff(duration);
    SUPLA_LOG_DEBUG("handle SprinklerProgramRelay turnOff");
    SprinklerRelay::completeProgram(runScheduled);
    inProgress = false;
}

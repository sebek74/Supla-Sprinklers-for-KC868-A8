#include <SuplaDevice.h>
#include <Wire.h>
#include <supla/control/button.h>
#include <supla/control/relay.h>
#include <supla/control/virtual_relay.h>
#include <supla/sensor/binary.h>
#include <supla/events.h>
#include <supla/control/button.h>
#include <supla/control/action_trigger.h>
#include "Definitions.h"
#include "SprinklerProgramRelay.h"
#include "SprinklerRelay.h"
#include "SprinklerConfig.h"
#include "SprinklerDisplay.h"
#include "SprinklerMessenger.h"

extern SprinklerConfig config;
extern SprinklerDisplay display;
extern SprinklerMessenger messenger;


const char* relayNames [] = {
  "Pompa", 
  "Zraszacze front", 
  "Zraszacze ogród",
  "Zraszacze przy garażu",
  "Kwiaty",
  "Warzywniak",
  "Dolewanie do studni",
  "Opróżnianie studni",
  "Stan wody w zbiorniku",
  "Uruchom cykl teraz",
  "Zaplanuj cykl",
  "Zraszacze front",
  "Zraszacze front",
  "Zraszacze ogród",
  "Zraszacze ogród",
  "Zraszacze przy garażu",
  "Zraszacze przy garażu",
  "Kwiaty",
  "Kwiaty",
  "Warzywniak",
  "Warzywniak",
  "Wyłącz całe nawodnienie",
  "Światło przed szopą",
  "Czasowe światło przed szopą",
  "Włącznik zasilania"
};

char* sensorNames [] = {
  "",
  "",
  "",
  "Drzwi szopa",
  "Światło przed szopą",
  "Światło przed kotłownią",
  "Zbiornik prawie pusty",
  "Zbiornik przepełniony",
  ""
};

class SprinklerStopAll : public Supla::Control::VirtualRelay {
  public:
    void turnOn(_supla_int_t duration = 0) override {
      if (SprinklerRelay::isPumpOn()) {
        SprinklerRelay::getPumpRelay()->turnOff();
        VirtualRelay::turnOn(config.getPumpAdvOffTimeS()*1000);
        this->durationMs = config.getPumpAdvOffTimeS()*1000;
      } else {
        Supla::Control::VirtualRelay::turnOn(duration);
      }
    }

    void turnOff(_supla_int_t duration = 0) override {
      SprinklerRelay::turnOffAll();
      Supla::Control::VirtualRelay::turnOff(duration);
    }
};

bool SprinklerRelay::enableNextMessage = true;

class SprinklerActionHandler : public Supla::ActionHandler {
  public:
    void handleAction(int event, int action) override {
      SUPLA_LOG_DEBUG("action event: %d %d", event, action);
      display.restoreFullContrast();
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      Supla::Control::Relay* pump;
      
      switch (action) {
        case ActionId::RestoreDisplay:
          break;
        case ActionId::DoorOpen:
          if (config.isDoorMessageHourActive(timeinfo->tm_hour))
            messenger.sendMessage(MSG_DOOR_OPEN, timeinfo->tm_hour, timeinfo->tm_min);
          break;
        case ActionId::TankRefilling: 
          SprinklerRelay::getRelayById(RelayId::RefillTank)->turnOn();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action); 
          break;
        case ActionId::TankEmptying:
          SprinklerRelay::getRelayById(RelayId::EmptyTank)->turnOn();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action); 
          break;
        case ActionId::TankDoNothing:
          if (SprinklerRelay::getRelayById(RelayId::RefillTank)->isOn())
            SprinklerRelay::getRelayById(RelayId::RefillTank)->turnOff();
          if (SprinklerRelay::getRelayById(RelayId::EmptyTank)->isOn())
            SprinklerRelay::getRelayById(RelayId::EmptyTank)->turnOff();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action);
          break;
        case ActionId::EmptyRelayOn:
          if (!SprinklerRelay::getPumpRelay()->isOn())
            SprinklerRelay::getPumpRelay()->turnOn(); 
          messenger.sendMessage(MSG_TANK_EMPTY_ON);
          break;
        case ActionId::EmptyRelayOff:
          if (!SprinklerRelay::isPumpRequired() && SprinklerRelay::getPumpRelay()->isOn())
            SprinklerRelay::getPumpRelay()->turnOff();
          messenger.sendMessage(MSG_TANK_EMPTY_OFF);
          break;
        case ActionId::RefillRelayOn:
          messenger.sendMessage(MSG_TANK_REFILL_ON);
          break;
        case ActionId::RefillRelayOff:
          messenger.sendMessage(MSG_TANK_REFILL_OFF);
          break;
        case ActionId::ActionStopAll:
          SprinklerRelay::getRelayById(RelayId::StopAll)->turnOn();
          break;
        case ActionId::ActionTogglePump:
          if (SprinklerRelay::getPumpRelay()->isOn()) {
            SprinklerRelay::pumpStartMs = millis();
            if (!SprinklerRelay::isPumpRequired())
              messenger.sendMessage(MSG_PUMP_ON);
          }
          else {
            SprinklerRelay::pumpStartMs = 0;
            if (!SprinklerRelay::getProgramRelay()->isOn())
              messenger.sendMessage(MSG_PUMP_OFF);
          }
          break;
        case ActionId::ActionRunCycle:
          SprinklerRelay::getProgramRelay()->startCycleNow(false);
          break;
        case ActionId::ActionToggleScheduleCycle:
          SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->toggle();
          break;
        case ActionId::ActionMessageScheduleCycle:
          if (SprinklerRelay::enableNextMessage)
            if (SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->isOn())
              messenger.sendMessage(MSG_SCHEDULE_ON, config.getScheduleHour());
            else
              messenger.sendMessage(MSG_SCHEDULE_OFF);
            SprinklerRelay::enableNextMessage = true;
          break;
        case ActionId::AddProgramTimeSprShort:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(config.getSprShort()*MS_IN_MIN);
          break;
        case ActionId::SubProgramTimeSprShort:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(-config.getSprShort()*MS_IN_MIN);
          break;
         case ActionId::AddProgramTimeSprLong:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(config.getSprLong()*MS_IN_MIN);
          break;
        case ActionId::SubProgramTimeSprLong:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(-config.getSprLong()*MS_IN_MIN);
          break;
        case ActionId::AddProgramTimeDropShort:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(config.getDropShort()*MS_IN_MIN);
          break;
        case ActionId::SubProgramTimeDropShort:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(-config.getDropShort()*MS_IN_MIN);
          break;
        case ActionId::AddProgramTimeDropLong:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(config.getDropLong()*MS_IN_MIN);
          break;
        case ActionId::SubProgramTimeDropLong:
          if (SprinklerRelay::getProgramRelay()->isOn())
            SprinklerRelay::getProgramRelay()->updateRemainingTime(-config.getDropLong()*MS_IN_MIN);
          break;
        case ActionId::NextEditMode:
          if (display.restoreFullContrast()) break;
          display.nextEditMode();
          break;
        case ActionId::NextEditValue:
          if (display.restoreFullContrast()) break;
          SprinklerRelay::nextEditValue();
          break;
        case FixedLightOn:
          SprinklerRelay::fixedLightStartMs = millis();
          break;
        case FixedLightOff:
          SprinklerRelay::fixedLightStartMs = 0;
          break;
      } 
    }
};

SprinklerActionHandler sprinklerActionHandler;
int32_t SprinklerRelay::fixedLightStartMs = 0;
int32_t SprinklerRelay::pumpStartMs = 0;


void SprinklerRelay::registerRelays() {

  auto button = new Supla::Control::Button(BUTTON_CFG_RELAY_GPIO, true, true);
  button->configureAsConfigButton(&SuplaDevice); 
  button->addAction(Supla::ENTER_CONFIG_MODE, SuplaDevice, Supla::ON_HOLD, true);
  button->addAction(Supla::LEAVE_CONFIG_MODE_AND_RESET, SuplaDevice, Supla::ON_CLICK_1, true);

  relay[RelayId::Pump] = new Supla::Control::Relay(outPcf, 7, false);//, SUPLA_BIT_FUNC_PUMPSWITCH);
  relay[RelayId::Pump]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::FrontSprinklers] = new SprinklerRelay(6, true);
  relay[RelayId::BackSprinklers] = new SprinklerRelay(5, true);
  relay[RelayId::SideSprinklers] = new SprinklerRelay(4, true);
  relay[RelayId::Flowers] = new SprinklerRelay(3, false);
  relay[RelayId::Vegetables] = new SprinklerRelay(2, false);
  relay[RelayId::EmptyTank] = new Relay(outPcf, 1, false);
  relay[RelayId::EmptyTank]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::RefillTank] = new Relay(outPcf, 0, false);
  relay[RelayId::RefillTank]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::VirtualTankLevel] = new Supla::Control::VirtualRelay();
  relay[RelayId::VirtualTankLevel]->getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_WATER_TANK);
  relay[RelayId::RunCycleNow] = programRelay = new SprinklerProgramRelay();

  for (int i = RelayId::_FirstProgram; i<=RelayId::_LastProgram; i++) {
    relay[i] = new Supla::Control::VirtualRelay();
    relay[i]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
    relay[i]->setDefaultStateRestore();
  }

  relay[RelayId::StopAll] = new SprinklerStopAll();
  relay[RelayId::StopAll]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);

  relay[RelayId::FixedLight] = new Supla::Control::Relay(32, true);
  relay[RelayId::FixedLight]->setDefaultStateOff();
  relay[RelayId::FixedLight]->getChannel()->setDefault(SUPLA_CHANNELFNC_LIGHTSWITCH);
  relay[RelayId::TimedLight] = new Supla::Control::VirtualRelay();    // włącz aby uruchomić czasówkę na FixedLight (wyłącza się sam po chwili od uruchomienia)
  relay[RelayId::TimedLight]->setDefaultStateOff();
  relay[RelayId::TimedLight]->getChannel()->setDefault(SUPLA_CHANNELFNC_LIGHTSWITCH);

  // drugi przekaźnik, na S4, bez powiązania z przyciskiem
  relay[RelayId::Extra] = new Supla::Control::Relay(33, true);
  relay[RelayId::Extra]->setDefaultStateOff();
  relay[RelayId::Extra]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);

  // fizyczne przyciski
  #define BUTTON_SETUP(relayId) \
    button = new Supla::Control::Button(inPcf, relayId, true, true); \
    button->setButtonType(Supla::Control::Button::ButtonType::MONOSTABLE); \
    button->setHoldTime(1000); \
    button->setMulticlickTime(350); \
    button->disableActionsInConfigMode();  

  BUTTON_SETUP(RelayId::Pump);
  button->addAction(Supla::TOGGLE, relay[RelayId::Pump], Supla::ON_CLICK_1);
  button->addAction(ActionId::ActionStopAll, sprinklerActionHandler, Supla::ON_HOLD);
 
  BUTTON_SETUP(RelayId::FrontSprinklers);
  button->addAction(ActionId::NextEditMode, sprinklerActionHandler, Supla::ON_CLICK_1);
  button->addAction(ActionId::ActionRunCycle, sprinklerActionHandler, Supla::ON_HOLD);
 
  BUTTON_SETUP(RelayId::BackSprinklers);
  button->addAction(ActionId::NextEditValue, sprinklerActionHandler, Supla::ON_CLICK_1);
  button->addAction(ActionId::ActionToggleScheduleCycle, sprinklerActionHandler, Supla::ON_HOLD);

  // przekaźnik światła na złączu S3 z przyciskiem do przekaźnika kropelkowego ogród
  BUTTON_SETUP(RelayId::_ShedLightSwitchButton);
  button->setOnLoadConfigType(Supla::Control::Button::OnLoadConfigType::LOAD_BUTTON_SETUP_ONLY);
  button->addAction(Supla::TOGGLE, relay[RelayId::TimedLight], Supla::ON_CLICK_1);
  button->addAction(Supla::TOGGLE, relay[RelayId::FixedLight], Supla::ON_HOLD);
  auto at = new Supla::Control::ActionTrigger();
  at->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_ShedLightSwitchButton);
  at->setRelatedChannel(relay[RelayId::FixedLight]);
  at->attach(button);

  // action trigger na 5tym wejsciu PCF (do przypisania w chmurze do uruchamiania światła przed kotłownią)
  BUTTON_SETUP(RelayId::_BackHouseSwitchButton);
  button->setInitialCaption(sensorNames[RelayId::_BackHouseSwitchButton]);
  at = new Supla::Control::ActionTrigger();
  at->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_BackHouseSwitchButton);
  at->setInitialCaption(sensorNames[RelayId::_BackHouseSwitchButton]);
  at->attach(button);

  SUPLA_LOG_DEBUG("dodaję sensory w studni");

  // sensory poziomu wody na 6 i 7mym wejściu PCF
  lowWaterSensor = new Supla::Sensor::Binary(inPcf, RelayId::RefillTank, true, true);
  lowWaterSensor->setInitialCaption(sensorNames[RelayId::RefillTank]);
  lowWaterSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_CONTAINER_LEVEL_SENSOR);
  lowWaterSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::RefillTank);
  lowWaterSensor->addAction(ActionId::TankRefilling, sprinklerActionHandler, Supla::ON_TURN_ON);
  lowWaterSensor->addAction(ActionId::TankDoNothing, sprinklerActionHandler, Supla::ON_TURN_OFF);
  lowWaterSensor->setFilteringTimeMs(1000, true);
  lowWaterSensor->disableActionsInConfigMode();

  highWaterSensor = new Supla::Sensor::Binary(inPcf, RelayId::EmptyTank, true, true);
  highWaterSensor->setInitialCaption(sensorNames[RelayId::EmptyTank]);
  highWaterSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_CONTAINER_LEVEL_SENSOR);
  highWaterSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::EmptyTank);
  highWaterSensor->addAction(ActionId::TankEmptying, sprinklerActionHandler, Supla::ON_TURN_ON);
  highWaterSensor->addAction(ActionId::TankDoNothing, sprinklerActionHandler, Supla::ON_TURN_OFF);
  highWaterSensor->setFilteringTimeMs(1000, true);
  highWaterSensor->disableActionsInConfigMode();

  // kontaktron szopa na wejściu 3 PCF
  doorSensor = new Supla::Sensor::Binary(inPcf, RelayId::_DoorSensor, true, true);
  doorSensor->setInitialCaption(sensorNames[RelayId::_DoorSensor]);
  doorSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_OPENINGSENSOR_DOOR);
  doorSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_DoorSensor);
  doorSensor->addAction(ActionId::DoorOpen, sprinklerActionHandler, Supla::ON_TURN_OFF);
  doorSensor->setFilteringTimeMs(1000, true);
  doorSensor->disableActionsInConfigMode();

  SUPLA_LOG_DEBUG("ustawiam akcje dla przekaznikow");
  
  // domyślne nazwy elementów kontrolnych i akcje
  
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOff, sprinklerActionHandler, Supla::ON_TURN_OFF);
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOn, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::RefillTank]->addAction(ActionId::RefillRelayOff, sprinklerActionHandler, Supla::ON_TURN_OFF);
  relay[RelayId::RefillTank]->addAction(ActionId::RefillRelayOn, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::Pump]->addAction(ActionId::ActionTogglePump, sprinklerActionHandler, Supla::ON_CHANGE);
  //relay[RelayId::StopAll]->setDefaultImpulseDurationMs(2000);
  //relay[RelayId::StopAll]->addAction(ActionId::ActionStopAll, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::FixedLight]->addAction(ActionId::FixedLightOn, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::FixedLight]->addAction(ActionId::FixedLightOff, sprinklerActionHandler, Supla::ON_TURN_OFF);
  relay[RelayId::ScheduleCycle]->addAction(ActionId::ActionMessageScheduleCycle, sprinklerActionHandler, Supla::ON_CHANGE);

  char relayName[64];
  for (int id=0; id<=RelayId::_LastRelay ;id++) {
    if (id!=RelayId::RunCycleNow) {
      relay[id]->addAction(ActionId::RestoreDisplay, sprinklerActionHandler, Supla::ON_TURN_ON);
      relay[id]->addAction(ActionId::RestoreDisplay, sprinklerActionHandler, Supla::ON_TURN_OFF);
    }
    switch (id) {
      case RelayId::FrontTimeShort:
      case RelayId::BackTimeShort:
      case RelayId::SideTimeShort:
        relay[id]->addAction(ActionId::AddProgramTimeSprShort, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeSprShort, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, sizeof(relayName), "%s %dm", relayNames[id], config.getSprShort());
        break;
      case RelayId::FrontTimeLong:
      case RelayId::BackTimeLong:
      case RelayId::SideTimeLong:
        relay[id]->addAction(ActionId::AddProgramTimeSprLong, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeSprLong, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, sizeof(relayName), "%s %dm", relayNames[id], config.getSprLong());
        break;
      case RelayId::FlowersTimeShort:
      case RelayId::VegetablesTimeShort:
        relay[id]->addAction(ActionId::AddProgramTimeDropShort, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeDropShort, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, sizeof(relayName), "%s %dm", relayNames[id], config.getDropShort());
        break;
      case RelayId::FlowersTimeLong:
      case RelayId::VegetablesTimeLong:
        relay[id]->addAction(ActionId::AddProgramTimeDropLong, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeDropLong, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, sizeof(relayName), "%s %dm", relayNames[id], config.getDropLong());
        break;
      default:
        strncpy(relayName, relayNames[id], sizeof(relayName));
    }
    relay[id]->setInitialCaption(relayName);
    relay[id]->getChannel()->setChannelNumber(id);
  }

  SUPLA_LOG_DEBUG("przekazniki ustawione");
}

ActionId SprinklerRelay::getTankStatus() {
  if (lowWaterSensor->getValue()) return ActionId::TankRefilling;
  if (highWaterSensor->getValue()) return ActionId::TankEmptying;
  return ActionId::TankDoNothing;
}

void SprinklerRelay::initalizeRelays() {
  if (relaysInitialized) return;
   
  relay[RelayId::VirtualTankLevel]->getChannel()->setContainerFillValue(ActionId::TankDoNothing);

  #define VALVE_SETUP(id) \
    relay[id]->enableCountdownTimerFunction(); \
    relay[id]->setStoredTurnOnDurationMs(5000); //konieczne, bo inaczej przekaznik sie wyłącza od razu niezaleznie od ustawiwnie wartosci w turnOn
//relay[id]->setDefaultStaircaseDurationMs(0); \
  VALVE_SETUP(RelayId::FrontSprinklers);
  VALVE_SETUP(RelayId::BackSprinklers);
  VALVE_SETUP(RelayId::SideSprinklers);
  VALVE_SETUP(RelayId::Vegetables);
  VALVE_SETUP(RelayId::Flowers);
  VALVE_SETUP(RelayId::RunCycleNow);
  relaysInitialized = true;
}

void SprinklerRelay::completeProgram(bool runScheduled) {
  for (int id = RelayId::_FirstValve; id<=RelayId::_LastValve; id++) {
    uint32_t time=0;
    relay[id]->getRemainingCountdownTimerSec(&time);
    if (relay[id]->isOn() && time>0)  // nie wyłączaj uruchomionych ręcznie na stałe 
      relay[id]->turnOff();
  }
  if (!SprinklerRelay::isPumpRequired() && getPumpRelay()->isOn())
      getPumpRelay()->turnOff();
  programRelay->completeCycleNow();
  messenger.sendMessage(MSG_PROGRAM_OFF);
  if (runScheduled)
    disableScheduleCycle(false);
}

void SprinklerRelay::turnOffAll() { // pompa już jest wyłączona
  for (int id = _FirstValve; id<=RelayId::_LastPsychicalRelay; id++) {
    if (SprinklerRelay::getRelayById(id)->isOn())
      SprinklerRelay::getRelayById(id)->turnOff();
  }
  /*if (SprinklerRelay::getProgramRelay()->isOn())
    SprinklerRelay::getProgramRelay()->turnOff();*/
  if (SprinklerRelay::getProgramRelay()->isOn()) 
    SprinklerRelay::getProgramRelay()->turnOff();
  //programRelay->completeCycleNow();
  messenger.sendMessage(MSG_ALL_OFF);
}

// overrides

// włącz zawór zawsze zgodnie z długością ustawionego programu
void SprinklerRelay::turnOn(_supla_int_t duration) {
  _supla_int_t overrideDuration = getScheduledProgramTimeS(this->myRelayId)*MS_IN_MIN;
  Relay::turnOn(overrideDuration);
  Relay::durationMs = overrideDuration;
  if (overrideDuration==0 && this->getProgramRelay()->isOn())
    this->getProgramRelay()->turnOff();
} 

void SprinklerRelay::turnOff(_supla_int_t duration) {\
  getProgramRelay()->recalculateRemainingTime();  // zaktualizuj zegar programu w momencie wyłączenia zaworu (także ręcznego)
  Relay::turnOff(duration);
}

_supla_int_t SprinklerRelay::calculateProgramTimeMs() {
  _supla_int_t time = 0;
  if (relay[RelayId::FrontTimeShort]->isOn()) time+=config.getSprShort();
  if (relay[RelayId::FrontTimeLong]->isOn()) time+=config.getSprLong();
  if (relay[RelayId::BackTimeShort]->isOn()) time+=config.getSprShort();
  if (relay[RelayId::BackTimeLong]->isOn()) time+=config.getSprLong();
  if (relay[RelayId::SideTimeShort]->isOn()) time+=config.getSprShort();
  if (relay[RelayId::SideTimeLong]->isOn()) time+=config.getSprLong();
  if (relay[RelayId::FlowersTimeShort]->isOn()) time+=config.getDropShort();
  if (relay[RelayId::FlowersTimeLong]->isOn()) time+=config.getDropLong();
  if (relay[RelayId::VegetablesTimeShort]->isOn()) time+=config.getDropShort();
  if (relay[RelayId::VegetablesTimeLong]->isOn()) time+=config.getDropLong();
  SUPLA_LOG_DEBUG("CALCULATED TIME %d, IN MS: %d", time, time*MS_IN_MIN);
  return time*MS_IN_MIN;
}

RelayId SprinklerRelay::getActiveValveId() {
  if (SprinklerRelay::getRelayById(RelayId::FrontSprinklers)->isOn()) return RelayId::FrontSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::BackSprinklers)->isOn()) return RelayId::BackSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::SideSprinklers)->isOn()) return RelayId::SideSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::Flowers)->isOn()) return RelayId::Flowers;
  if (SprinklerRelay::getRelayById(RelayId::Vegetables)->isOn()) return RelayId::Vegetables;
  return RelayId::Pump;
}

void SprinklerRelay::getRelayTimeText(int32_t relayId, char* result, int32_t len) {
  uint32_t time = 0;
  switch (relayId) {
    case RelayId::FrontSprinklers: 
      if (relay[RelayId::FrontTimeShort]->isOn()) time+=config.getSprShort();
      if (relay[RelayId::FrontTimeLong]->isOn()) time+=config.getSprLong();
      break;
    case RelayId::BackSprinklers: 
      if (relay[RelayId::BackTimeShort]->isOn()) time+=config.getSprShort();
      if (relay[RelayId::BackTimeLong]->isOn()) time+=config.getSprLong();
      break; 
    case RelayId::SideSprinklers: 
      if (relay[RelayId::SideTimeShort]->isOn()) time+=config.getSprShort();
      if (relay[RelayId::SideTimeLong]->isOn()) time+=config.getSprLong();
      break; 
     case RelayId::Flowers: 
      if (relay[RelayId::FlowersTimeShort]->isOn()) time+=config.getDropShort();
      if (relay[RelayId::FlowersTimeLong]->isOn()) time+=config.getDropLong();
      break; 
    case RelayId::Vegetables: 
      if (relay[RelayId::VegetablesTimeShort]->isOn()) time+=config.getDropShort();
      if (relay[RelayId::VegetablesTimeLong]->isOn()) time+=config.getDropLong();
      break;
  }
  snprintf(result, len, "%d:%02d", time/60, time%60);
}

void SprinklerRelay::nextEditValue() {
  
  #define NEXT_EDIT_VALUE(valveShortTime, valveLongTime) \
    if (!relay[valveShortTime]->isOn() && !relay[valveLongTime]->isOn())  \
        relay[valveShortTime]->turnOn();  \
      else if (relay[valveShortTime]->isOn() && relay[valveLongTime]->isOn()) { \
        relay[valveShortTime]->turnOff(); \
        relay[valveLongTime]->turnOff();  \
      } else if (relay[valveShortTime]->isOn()) { \
        relay[valveShortTime]->turnOff(); \
        relay[valveLongTime]->turnOn(); \
      } else if (relay[valveLongTime]->isOn())  \
        relay[valveShortTime]->turnOn();
  
  
  switch (display.getEditMode()) {
    case FrontSprinklers: 
      NEXT_EDIT_VALUE(FrontTimeShort, FrontTimeLong)
      break;
    case BackSprinklers:
      NEXT_EDIT_VALUE(BackTimeShort, BackTimeLong)
      break;
    case SideSprinklers:
      NEXT_EDIT_VALUE(SideTimeShort, SideTimeLong)
      break;
    case Vegetables:
      NEXT_EDIT_VALUE(VegetablesTimeShort, VegetablesTimeLong)
      break;
    case Flowers:
      NEXT_EDIT_VALUE(FlowersTimeShort, FlowersTimeLong)
      break;
    case Pump:
    case RefillTank:
    case EmptyTank:
    case RunCycleNow:
    case ScheduleCycle:
        relay[display.getEditMode()]->toggle();
        break;
    case _EditScheduleTime:
        config.incScheduleHour();
        break;
  }
  display.pingEditMode();   // licz od nowa czas autoamtycznego wyjścia z trybu edycji
  SUPLA_LOG_DEBUG("nextEditValue %d", display.getEditMode());
}

uint32_t SprinklerRelay::getScheduledProgramTimeS(int relayId) {
  uint32_t result = 0;
  switch (relayId) {
    case RelayId::FrontSprinklers:
      if (relay[RelayId::FrontTimeShort]->isOn()) result+=config.getSprShort(); 
      if (relay[RelayId::FrontTimeLong]->isOn()) result+=config.getSprLong();
      break;
    case RelayId::BackSprinklers:
      if (relay[RelayId::BackTimeShort]->isOn()) result+=config.getSprShort(); 
      if (relay[RelayId::BackTimeLong]->isOn()) result+=config.getSprLong();
      break;  
    case RelayId::SideSprinklers:
      if (relay[RelayId::SideTimeShort]->isOn()) result+=config.getSprShort(); 
      if (relay[RelayId::SideTimeLong]->isOn()) result+=config.getSprLong();
      break;
    case RelayId::Flowers:
      if (relay[RelayId::FlowersTimeShort]->isOn()) result+=config.getDropShort(); 
      if (relay[RelayId::FlowersTimeLong]->isOn()) result+=config.getDropLong();
      break;
    case RelayId::Vegetables:
      if (relay[RelayId::VegetablesTimeShort]->isOn()) result+=config.getDropShort(); 
      if (relay[RelayId::VegetablesTimeLong]->isOn()) result+=config.getDropLong();
      break;
  }
  return result;
}

bool scheduleBlockExecuted = false;
bool messageBlockExecuted = false;

void SprinklerRelay::ticTacTimer() {

  // uruchomienie programu o danej godzinie
  time_t scheduletime = config.makeScheduleTimeToday(); 
  time_t now = time(nullptr);
  if (scheduletime==now && isScheduleCycleEnabled() && !getProgramRelay()->getProgramInProgress()) {
    if (!scheduleBlockExecuted) {
      if (calculateProgramTimeMs()>0) {
        getProgramRelay()->startCycleNow(true);
      } else {// nic do zrobienia
        //disableScheduleCycle();
#ifndef DEBUG  // unikaj komunikatów co minutę
        messenger.sendMessage(MSG_SCHEDULE_FAILED);
#endif
      }
      scheduleBlockExecuted = true;
    }
  } else
    scheduleBlockExecuted = false;
  
  // wysłanie komunikatu o wskazanej godzinie
  if (config.getScheduleMessageHour()>-1) {
    time_t messageTime = config.makeMessageTimeToday(); 
    time_t now = time(nullptr);
    
    if (messageTime==now) {
        if (!messageBlockExecuted) {
          if (isScheduleCycleEnabled()) {
              uint32_t scheduledTime = calculateProgramTimeMs() / MS_IN_MIN;
              messenger.sendMessage((scheduledTime>0) ? MSG_DAILY_MESSAGE_ENABLED: MSG_DAILY_MESSAGE_NO_WORK, scheduledTime);
          }
          else 
            messenger.sendMessage(MSG_DAILY_MESSAGE_DISABLED);
        }
        messageBlockExecuted = true;
    } else
      messageBlockExecuted = false;
  }

  // wysłanie komunikatu o włączonym świetle
  if (fixedLightStartMs>0 && fixedLightStartMs+config.getLightTimeSMessage()*60*1000<millis()) {
    messenger.sendMessage(MSG_LIGHT_ON_X_MIN, config.getLightTimeSMessage());
    fixedLightStartMs=0;
  }

  // wysłanie komunikatu o włączonej pompie
  if (pumpStartMs>0 && pumpStartMs+config.getPumpTimeSMessage()*60*1000<millis() && !programRelay->getProgramInProgress()) {
    messenger.sendMessage(MSG_PUMP_ON_X_MIN, config.getPumpTimeSMessage());
    pumpStartMs=0;
  }


  // synchronizacja relay dla światła
  if (relay[RelayId::TimedLight]->isOn()) {
    if (!relay[RelayId::FixedLight]->isOn()) {
      relay[RelayId::FixedLight]->turnOn(config.getLightActivationTimeS()*1000);
    }
    else 
      relay[RelayId::FixedLight]->turnOff();
    relay[RelayId::TimedLight]->turnOff();
  }
    
  uint32_t remaininingTime, setTime;
  
  bool isNextValveRequiresPump = relay[RelayId::EmptyTank]->isOn();
  if (!isNextValveRequiresPump) // jeśli opóżnianie jest włączone to nie możemy planować wyłączenia pompy już teraz
    for (int valveId = RelayId::_FirstValve; valveId<=_LastValve; valveId++) {
      if (getScheduledProgramTimeS(valveId)>0) {   //znajdz następny krok programu w kolejności
        isNextValveRequiresPump = getValveById(valveId)->getRequiresPump(); // czy następny program wymaga pompy
        break;
      }
    }

  RelayId activeRelayId = SprinklerRelay::getActiveValveId();
  uint32_t remainingSeconds = 0;
  if (activeRelayId>0)
    SprinklerRelay::getValveById(activeRelayId)->getRemainingCountdownTimerSec(&remainingSeconds);

  // wyłącz pompę kilka sekund przed zamknięciem zaworu jeśli następny krok nie wymaga pompy i nie jest włączone opróżnianie
  if (!isNextValveRequiresPump && remainingSeconds>0 && remainingSeconds<=config.getPumpAdvOffTimeS() && SprinklerRelay::getPumpRelay()->isOn())
    SprinklerRelay::getPumpRelay()->turnOff();
  
  if (activeRelayId>0/*remainingSeconds>0 */|| !programRelay->isOn()) // żadne akcje dalej niepotrzebne gdy program nie działa lub nie wymaga jeszcze kolejnego kroku
    return;

  // szukam kolejnego kroku programu i go uruchamiam
  // uwaga: wywołanie relay[valveShort/LongTime]->turnOff() powoduje wywołanie obsługi updateRemainingTime, trzeba ten czas przywrócić
  #define NEXT_STEP(valve, valveShortTime, valveLongTime) \
   setTime = getScheduledProgramTimeS(valve); \
    if (setTime>0) { \
      relay[valve]->turnOn(setTime*MS_IN_MIN); \
      if (relay[valveShortTime]->isOn()) relay[valveShortTime]->turnOff(); \
      if (relay[valveLongTime]->isOn()) relay[valveLongTime]->turnOff(); \
      if (getValveById(valve)->getRequiresPump() && !getPumpRelay()->isOn()) \
        getPumpRelay()->turnOn(); \
      programRelay->updateRemainingTime(setTime*MS_IN_MIN); \
      return;\
    }  
      
  NEXT_STEP(RelayId::FrontSprinklers, RelayId::FrontTimeShort, RelayId::FrontTimeLong)
  NEXT_STEP(RelayId::BackSprinklers, RelayId::BackTimeShort, RelayId::BackTimeLong)
  NEXT_STEP(RelayId::SideSprinklers, RelayId::SideTimeShort, RelayId::SideTimeLong)
  NEXT_STEP(RelayId::Flowers, RelayId::FlowersTimeShort, RelayId::FlowersTimeLong)
  NEXT_STEP(RelayId::Vegetables, RelayId::VegetablesTimeShort, RelayId::VegetablesTimeLong)
}
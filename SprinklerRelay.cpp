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
  "Wyłącz wszystko",
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

bool SprinklerRelay::enableNextMessage = true;

class SprinklerActionHandler : public Supla::ActionHandler {
  public:
    void handleAction(int event, int action) override {
      SUPLA_LOG_DEBUG("action event: %d %d", event, action);
      display.restoreFullContrast();
      Supla::Control::Relay* pump;
      
      switch (action) {
        case ActionId::RestoreDisplay:
          display.restoreFullContrast();
          break;
        case ActionId::DoorOpen:
          display.restoreFullContrast();
          messenger.sendMessage(Msg::DOOR_OPEN);
          break;
        case ActionId::TankRefill: 
          SprinklerRelay::getRelayById(RelayId::RefillTank)->turnOn();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action); 
          messenger.sendMessage(Msg::TANK_EMPTY);
          break;
        case ActionId::TankEmpty:
          SprinklerRelay::getRelayById(RelayId::EmptyTank)->turnOn();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action); 
          messenger.sendMessage(Msg::TANK_FULL);
          break;
        case ActionId::TankDoNothing:
          if (SprinklerRelay::getRelayById(RelayId::RefillTank)->isOn())
            SprinklerRelay::getRelayById(RelayId::RefillTank)->turnOff();
          if (SprinklerRelay::getRelayById(RelayId::EmptyTank)->isOn())
            SprinklerRelay::getRelayById(RelayId::EmptyTank)->turnOff();
          SprinklerRelay::getRelayById(RelayId::VirtualTankLevel)->getChannel()->setContainerFillValue(action);
          messenger.sendMessage(Msg::TANK_NOMINAL);
          break;
        case ActionId::EmptyRelayOn:
          if (!SprinklerRelay::getPumpRelay()->isOn())
            SprinklerRelay::getPumpRelay()->turnOn(); 
            break;
        case ActionId::EmptyRelayOff:
          if (!SprinklerRelay::isPumpRequired() && SprinklerRelay::getPumpRelay()->isOn())
            SprinklerRelay::getPumpRelay()->turnOff(); 
            break;
        case ActionId::ActionStopAll:
          SprinklerRelay::turnOffAll();
          break;
        case ActionId::ActionTogglePump:
          pump = SprinklerRelay::getRelayById(RelayId::Pump);
          if (SprinklerRelay::getPumpRelay()->isOn() && !SprinklerRelay::isPumpRequired())
            messenger.sendMessage(Msg::PUMP_ON);
          if (SprinklerRelay::getPumpRelay()->isOn())
            SprinklerRelay::pumpStartMs = millis();
          else
            SprinklerRelay::pumpStartMs = 0;
          break;
        case ActionId::ActionRunCycle:
          //relay[RelayId::RunCycleNow]->enableCountdownTimerFunction();
          //relay[RelayId::RunCycleNow]->setDefaultStaircaseDurationMs(5000);
          //relay[RelayId::RunCycleNow]->setDefaultImpulseDurationMs(6000);
          //relay[RelayId::RunCycleNow]->applyDuration(4000, true);
          //SprinklerRelay::getRelayById(RelayId::RunCycleNow)->setStoredTurnOnDurationMs(6000);
          SprinklerRelay::getProgramRelay()->startCycleNow(false);
          break;
        case ActionId::ActionToggleScheduleCycle:
          SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->toggle();
          break;
        case ActionId::ActionMessageScheduleCycle:
          if (SprinklerRelay::enableNextMessage)
            if (SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->isOn())
              messenger.sendMessage(Msg::SCHEDULE_ON, config.getScheduleHour());
            else
              messenger.sendMessage(Msg::SCHEDULE_OFF);
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

  relay[RelayId::Pump] = new Supla::Control::Relay(outPcf, 0, false);//, SUPLA_BIT_FUNC_PUMPSWITCH);
  relay[RelayId::Pump]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::FrontSprinklers] = new SprinklerRelay(1, true);
  relay[RelayId::BackSprinklers] = new SprinklerRelay(2, true);
  relay[RelayId::SideSprinklers] = new SprinklerRelay(3, true);
  relay[RelayId::Flowers] = new SprinklerRelay(4, false);
  relay[RelayId::Vegetables] = new SprinklerRelay(5, false);
  relay[RelayId::EmptyTank] = new Relay(outPcf, 6, false);
  relay[RelayId::EmptyTank]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::RefillTank] = new Relay(outPcf, 7, false);
  relay[RelayId::RefillTank]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
  relay[RelayId::VirtualTankLevel] = new Supla::Control::VirtualRelay();
  relay[RelayId::VirtualTankLevel]->getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_WATER_TANK);
  relay[RelayId::RunCycleNow] = programRelay = new SprinklerProgramRelay();

  for (int i = RelayId::_FirstProgram; i<=RelayId::_LastProgram; i++) {
    relay[i] = new Supla::Control::VirtualRelay();
    relay[i]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);
    relay[i]->setDefaultStateRestore();
  }

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
  lowWaterSensor->addAction(ActionId::TankRefill, sprinklerActionHandler, Supla::ON_TURN_ON);
  lowWaterSensor->addAction(ActionId::TankDoNothing, sprinklerActionHandler, Supla::ON_TURN_OFF);
  lowWaterSensor->disableActionsInConfigMode();

  highWaterSensor = new Supla::Sensor::Binary(inPcf, RelayId::EmptyTank, true, true);
  highWaterSensor->setInitialCaption(sensorNames[RelayId::EmptyTank]);
  highWaterSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_CONTAINER_LEVEL_SENSOR);
  highWaterSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::EmptyTank);
  highWaterSensor->addAction(ActionId::TankEmpty, sprinklerActionHandler, Supla::ON_TURN_ON);
  highWaterSensor->addAction(ActionId::TankDoNothing, sprinklerActionHandler, Supla::ON_TURN_OFF);
  highWaterSensor->disableActionsInConfigMode();

  // kontaktron szopa na wejściu 3 PCF
  doorSensor = new Supla::Sensor::Binary(inPcf, RelayId::_DoorSensor, true, true);
  doorSensor->setInitialCaption(sensorNames[RelayId::_DoorSensor]);
  doorSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_OPENINGSENSOR_DOOR);
  doorSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_DoorSensor);
  doorSensor->addAction(ActionId::DoorOpen, sprinklerActionHandler, Supla::ON_TURN_OFF); 
  doorSensor->disableActionsInConfigMode();

  SUPLA_LOG_DEBUG("ustawiam akcje dla przekaznikow");
  
  // domyślne nazwy elementów kontrolnych i akcje
  
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOff, sprinklerActionHandler, Supla::ON_TURN_OFF);
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOn, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::Pump]->addAction(ActionId::ActionTogglePump, sprinklerActionHandler, Supla::ON_CHANGE);
  relay[RelayId::StopAll]->setDefaultImpulseDurationMs(2000);
  relay[RelayId::StopAll]->addAction(ActionId::ActionStopAll, sprinklerActionHandler, Supla::ON_TURN_ON);
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
  if (lowWaterSensor->getValue()) return ActionId::TankRefill;
  if (highWaterSensor->getValue()) return ActionId::TankEmpty;
  return ActionId::TankDoNothing;
}

void SprinklerRelay::initalizeRelays() {
  if (relaysInitialized) return;
   
  relay[RelayId::VirtualTankLevel]->getChannel()->setContainerFillValue(ActionId::TankDoNothing);

  #define VALVE_SETUP(id) \
    relay[id]->enableCountdownTimerFunction(); \
    relay[id]->setDefaultStaircaseDurationMs(10*MS_IN_MIN); \
    relay[id]->setStoredTurnOnDurationMs(10*MS_IN_MIN);

  VALVE_SETUP(RelayId::FrontSprinklers);
  VALVE_SETUP(RelayId::BackSprinklers);
  VALVE_SETUP(RelayId::SideSprinklers);
  VALVE_SETUP(RelayId::Vegetables);
  VALVE_SETUP(RelayId::Flowers);
  VALVE_SETUP(RelayId::RunCycleNow);
  relaysInitialized = true;
}

void SprinklerRelay::completeProgram() {
  for (int id = RelayId::_FirstValve; id<=RelayId::_LastValve; id++) 
    if (relay[id]->isOn()) relay[id]->turnOff();
  if (!SprinklerRelay::isPumpRequired() && getPumpRelay()->isOn())
      getPumpRelay()->turnOff();
  programRelay->completeCycleNow();
  messenger.sendMessage(Msg::PROGRAM_OFF);
}

void SprinklerRelay::turnOffAll() {
  for (int id = RelayId::Pump; id<=RelayId::_LastPsychicalRelay; id++) 
    if (relay[id]->isOn()) relay[id]->turnOff();
  if (relay[RelayId::RunCycleNow]->isOn()) relay[RelayId::RunCycleNow]->turnOff();
  if (relay[RelayId::StopAll]->isOn()) relay[RelayId::StopAll]->turnOff();
  programRelay->completeCycleNow();
  messenger.sendMessage(Msg::ALL_OFF);
}

uint32_t SprinklerRelay::calculateProgramTimeMs() {
  uint32_t time = 0;
  SUPLA_LOG_DEBUG("USE CONFIG VALUES: %d %d %d %d\n", config.getSprShort(), config.getSprLong(), config.getDropShort(), config.getDropLong());
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
  SUPLA_LOG_DEBUG("CALCULATED TIME %d, IN MS: %d\n", time, time*MS_IN_MIN);
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
  switch (display.getEditMode()) {
    case FrontSprinklers:
      if (!relay[FrontTimeShort]->isOn() && !relay[FrontTimeLong]->isOn())
        relay[FrontTimeShort]->turnOn();
      else if (relay[FrontTimeShort]->isOn() && relay[FrontTimeLong]->isOn()) {
        relay[FrontTimeShort]->turnOff();
        relay[FrontTimeLong]->turnOff();
      } else if (relay[FrontTimeShort]->isOn()) {
        relay[FrontTimeShort]->turnOff();
        relay[FrontTimeLong]->turnOn();
      } else if (relay[FrontTimeLong]->isOn())
        relay[FrontTimeShort]->turnOn();
      break;
    case BackSprinklers:
      if (!relay[BackTimeShort]->isOn() && !relay[BackTimeLong]->isOn())
        relay[BackTimeShort]->turnOn();
      else if (relay[BackTimeShort]->isOn() && relay[BackTimeLong]->isOn()) {
        relay[BackTimeShort]->turnOff();
        relay[BackTimeLong]->turnOff();
      } else if (relay[BackTimeShort]->isOn()) {
        relay[BackTimeShort]->turnOff();
        relay[BackTimeLong]->turnOn();
      } else if (relay[BackTimeLong]->isOn())
        relay[BackTimeShort]->turnOn();
      break;
    case SideSprinklers:
      if (!relay[SideTimeShort]->isOn() && !relay[SideTimeLong]->isOn())
        relay[SideTimeShort]->turnOn();
      else if (relay[SideTimeShort]->isOn() && relay[SideTimeLong]->isOn()) {
        relay[SideTimeShort]->turnOff();
        relay[SideTimeLong]->turnOff();
      } else if (relay[SideTimeShort]->isOn()) {
        relay[SideTimeShort]->turnOff();
        relay[SideTimeLong]->turnOn();
      } else if (relay[SideTimeLong]->isOn())
        relay[SideTimeShort]->turnOn();
      break;
    case Vegetables:
      if (!relay[VegetablesTimeShort]->isOn() && !relay[VegetablesTimeLong]->isOn())
        relay[VegetablesTimeShort]->turnOn();
      else if (relay[VegetablesTimeShort]->isOn() && relay[VegetablesTimeLong]->isOn()) {
        relay[VegetablesTimeShort]->turnOff();
        relay[VegetablesTimeLong]->turnOff();
      } else if (relay[VegetablesTimeShort]->isOn()) {
        relay[VegetablesTimeShort]->turnOff();
        relay[VegetablesTimeLong]->turnOn();
      } else if (relay[VegetablesTimeLong]->isOn())
        relay[VegetablesTimeShort]->turnOn();
      break;
    case Flowers:
      if (!relay[FlowersTimeShort]->isOn() && !relay[FlowersTimeLong]->isOn())
        relay[FlowersTimeShort]->turnOn();
      else if (relay[FlowersTimeShort]->isOn() && relay[FlowersTimeLong]->isOn()) {
        relay[FlowersTimeShort]->turnOff();
        relay[FlowersTimeLong]->turnOff();
      } else if (relay[FlowersTimeShort]->isOn()) {
        relay[FlowersTimeShort]->turnOff();
        relay[FlowersTimeLong]->turnOn();
      } else if (relay[FlowersTimeLong]->isOn())
        relay[FlowersTimeShort]->turnOn();
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
  SUPLA_LOG_DEBUG("nextEditValue %d\n", display.getEditMode());
}

uint32_t SprinklerRelay::getScheduledProgramTime(int relayId) {
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
        messenger.sendMessage(Msg::SCHEDULE_FAILED);
      }
      scheduleBlockExecuted = true;
    }
  } else
    scheduleBlockExecuted = false;
  
  // wysłanie komunikatu o wskazanej godzinie
  #ifndef DEBUG // unikaj komunikatów co minutę
  if (config.getMessageHour()>-1) {
    time_t messageTime = config.makeMessageTimeToday(); 
    time_t now = time(nullptr);
    
    if (messageTime==now) {
        if (!messageBlockExecuted) {
          if (isScheduleCycleEnabled()) {
              uint32_t scheduledTime = calculateProgramTimeMs() / MS_IN_MIN;
              messenger.sendMessage((scheduledTime>0) ? Msg::DAILY_MESSAGE_ENABLED: Msg::DAILY_MESSAGE_NO_WORK, scheduledTime);
          }
          else
            messenger.sendMessage(Msg::DAILY_MESSAGE_DISABLED);
        }
        messageBlockExecuted = true;
    } else
      messageBlockExecuted = false;
  }
  #endif

  // wysłanie komunikatu o włączonym świetle
  if (fixedLightStartMs>0 && fixedLightStartMs+30*60*1000<millis()) {
    messenger.sendMessage(Msg::LIGHT_ON_30MIN);
    fixedLightStartMs=0;
  }

  // wysłanie komunikatu o włączonej pompie
  if (pumpStartMs>0 && pumpStartMs+60*60*1000<millis() && !programRelay->getProgramInProgress()) {
    messenger.sendMessage(Msg::PUMP_ON_60MIN);
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
      if (getScheduledProgramTime(valveId)>0) {   //znajdz następny krok programu w kolejności
        isNextValveRequiresPump = getValveById(valveId)->getRequiresPump(); // czy następny program wymaga pompy
        break;
      }
    }

  RelayId activeRelayId = SprinklerRelay::getActiveValveId();
  uint32_t remainingSeconds = 0;
  if (activeRelayId>0)
    SprinklerRelay::getValveById(activeRelayId)->getRemainingCountdownTimerSec(&remainingSeconds);

  // wyłącz pompę kilka sekund przed zamknięciem zaworu jeśli następny krok nie wymaga pompy i nie jest włączone opróżnianie
  if (!isNextValveRequiresPump && remainingSeconds>0 && remainingSeconds<=PUMP_ADVANCE_OFF_TIME_MS/1000 && SprinklerRelay::getPumpRelay()->isOn())
    SprinklerRelay::getPumpRelay()->turnOff();
  
  if (remainingSeconds>0 || !programRelay->isOn()) // żadne akcje dalej niepotrzebne gdy program nie działa lub nie wymaga jeszcze kolejnego kroku
    return;

  // szukam kolejnego kroku programu i go uruchamiam
  #define NEXT_STEP(valve, valveShortTime, valveLongTime) \
   setTime = getScheduledProgramTime(valve); \
    if (setTime>0) { \
      relay[valve]->turnOn(setTime*MS_IN_MIN); \
      relay[valveShortTime]->turnOff(); \
      relay[valveLongTime]->turnOff(); \
      if (getValveById(valve)->getRequiresPump() && !getPumpRelay()->isOn()) \
        getPumpRelay()->turnOn(); \
      programRelay->updateRemainingTime(setTime*MS_IN_MIN); \
      return; \
    }  

  NEXT_STEP(RelayId::FrontSprinklers, RelayId::FrontTimeShort, RelayId::FrontTimeLong)
  NEXT_STEP(RelayId::BackSprinklers, RelayId::BackTimeShort, RelayId::BackTimeLong)
  NEXT_STEP(RelayId::SideSprinklers, RelayId::SideTimeShort, RelayId::SideTimeLong)
  NEXT_STEP(RelayId::Flowers, RelayId::FlowersTimeShort, RelayId::FlowersTimeLong)
  NEXT_STEP(RelayId::Vegetables, RelayId::VegetablesTimeShort, RelayId::VegetablesTimeLong)
}

// overrides

// włącz zawór zawsze zgodnie z długością ustawionego programu
void SprinklerRelay::turnOn(_supla_int_t duration) {
  uint32_t overrideDuration = getScheduledProgramTime(this->myRelayId)*MS_IN_MIN;
  Relay::turnOn(overrideDuration);
  this->durationMs = overrideDuration;
} 
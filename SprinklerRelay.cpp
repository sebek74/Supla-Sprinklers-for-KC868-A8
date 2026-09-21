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

class SprinklerActionHandler : public Supla::ActionHandler {
  public:
    void handleAction(int event, int action) override {
      char txt[128];
      sprintf(txt, "action event: %d %d", event, action);
      Serial.println(txt);
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
          messenger.sendMessage(Msg::ALL_OFF);
          break;
        case ActionId::ActionTogglePump:
          pump = SprinklerRelay::getRelayById(RelayId::Pump);
          if (SprinklerRelay::getPumpRelay()->isOn() && !SprinklerRelay::isPumpRequired())
            messenger.sendMessage(Msg::PUMP_ON);
          break;
        case ActionId::ActionRunCycle:
          //relay[RelayId::RunCycleNow]->enableCountdownTimerFunction();
          //relay[RelayId::RunCycleNow]->setDefaultStaircaseDurationMs(5000);
          //relay[RelayId::RunCycleNow]->setDefaultImpulseDurationMs(6000);
          //relay[RelayId::RunCycleNow]->applyDuration(4000, true);
          //SprinklerRelay::getRelayById(RelayId::RunCycleNow)->setStoredTurnOnDurationMs(6000);
          SprinklerRelay::getProgramRelay()->startCycleNow(false);
          break;
        case ActionId::ActionScheduleCycle:
          SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->toggle();
          if (SprinklerRelay::getRelayById(RelayId::ScheduleCycle)->isOn())
              messenger.sendMessage(Msg::SCHEDULE_ON);
            else
              messenger.sendMessage(Msg::SCHEDULE_OFF);
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
      } 
    }
};

SprinklerActionHandler sprinklerActionHandler;

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

  relay[RelayId::FixedLightSwitch] = new Supla::Control::Relay(32, true);
  relay[RelayId::FixedLightSwitch]->setDefaultStateOff();
  relay[RelayId::FixedLightSwitch]->getChannel()->setDefault(SUPLA_CHANNELFNC_LIGHTSWITCH);
  relay[RelayId::TimedLightSwitch] = new Supla::Control::VirtualRelay();    // włącz aby uruchomić czasówkę na fixedlightswitch (wyłącza się sam po chwili od uruchomienia)
  relay[RelayId::TimedLightSwitch]->setDefaultStateOff();
  relay[RelayId::TimedLightSwitch]->getChannel()->setDefault(SUPLA_CHANNELFNC_LIGHTSWITCH);

  // drugi przekaźnik, na S4, bez powiązania z przyciskiem
  relay[RelayId::ExtraSwitch] = new Supla::Control::Relay(33, true);
  relay[RelayId::ExtraSwitch]->setDefaultStateOff();
  relay[RelayId::ExtraSwitch]->getChannel()->setDefault(SUPLA_CHANNELFNC_POWERSWITCH);


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
  button->addAction(ActionId::ActionScheduleCycle, sprinklerActionHandler, Supla::ON_HOLD);

  // przekaźnik światła na złączu S3 z przyciskiem do przekaźnika kropelkowego ogród
  
  BUTTON_SETUP(RelayId::_ShedLightSwitchButton);
  button->setOnLoadConfigType(Supla::Control::Button::OnLoadConfigType::LOAD_BUTTON_SETUP_ONLY);
  button->addAction(Supla::TOGGLE, relay[RelayId::TimedLightSwitch], Supla::ON_CLICK_1);
  button->addAction(Supla::TOGGLE, relay[RelayId::FixedLightSwitch], Supla::ON_HOLD);
  auto at = new Supla::Control::ActionTrigger();
  at->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::FixedLightSwitch);
  at->setRelatedChannel(relay[RelayId::FixedLightSwitch]);
  at->attach(button);

  // przycisk warzywniaka jako wolny niezwiązany z przekaźnikiem (do przypisania w chmurze do uruchamiania światła przed kotłownią)
  BUTTON_SETUP(RelayId::_BackHouseSwitchButton);
  button->setInitialCaption(sensorNames[RelayId::_BackHouseSwitchButton]);
  at = new Supla::Control::ActionTrigger();
  at->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_BackHouseSwitchButton);
  at->setInitialCaption(sensorNames[RelayId::_BackHouseSwitchButton]);
  at->attach(button);

  Serial.println(F("dodaję sensory w studni"));

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

  // kontaktron szopa na wejściu 3 (boczne zraszacze)
  doorSensor = new Supla::Sensor::Binary(inPcf, RelayId::_DoorSensor, true, true);
  doorSensor->setInitialCaption(sensorNames[RelayId::_DoorSensor]);
  doorSensor->getChannel()->setDefault(SUPLA_CHANNELFNC_OPENINGSENSOR_DOOR);
  doorSensor->getChannel()->setChannelNumber(RelayId::_LastRelay+RelayId::_DoorSensor);
  doorSensor->addAction(ActionId::DoorOpen, sprinklerActionHandler, Supla::ON_TURN_OFF); 
  doorSensor->disableActionsInConfigMode();

  Serial.println(F("ustawiam akcje dla przekaznikow"));
  
  // domyślne nazwy elementów kontrolnych i akcje
  
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOff, sprinklerActionHandler, Supla::ON_TURN_OFF);
  relay[RelayId::EmptyTank]->addAction(ActionId::EmptyRelayOn, sprinklerActionHandler, Supla::ON_TURN_ON);
  relay[RelayId::Pump]->addAction(ActionId::ActionTogglePump, sprinklerActionHandler, Supla::ON_CHANGE);
  relay[RelayId::StopAll]->setDefaultImpulseDurationMs(2000);
  relay[RelayId::StopAll]->addAction(ActionId::ActionStopAll, sprinklerActionHandler, Supla::ON_TURN_ON);

  char relayName[65];
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
        snprintf(relayName, 64, "%s %dm", relayNames[id], config.getSprShort());
        break;
      case RelayId::FrontTimeLong:
      case RelayId::BackTimeLong:
      case RelayId::SideTimeLong:
        relay[id]->addAction(ActionId::AddProgramTimeSprLong, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeSprLong, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, 64, "%s %dm", relayNames[id], config.getSprLong());
        break;
      case RelayId::FlowersTimeShort:
      case RelayId::VegetablesTimeShort:
        relay[id]->addAction(ActionId::AddProgramTimeDropShort, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeDropShort, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, 64, "%s %dm", relayNames[id], config.getDropShort());
        break;
      case RelayId::FlowersTimeLong:
      case RelayId::VegetablesTimeLong:
        relay[id]->addAction(ActionId::AddProgramTimeDropLong, sprinklerActionHandler, Supla::ON_TURN_ON);
        relay[id]->addAction(ActionId::SubProgramTimeDropLong, sprinklerActionHandler, Supla::ON_TURN_OFF);
        snprintf(relayName, 64, "%s %dm", relayNames[id], config.getDropLong());
        break;
      default:
        strncpy(relayName, relayNames[id], 64);
    }
    relay[id]->setInitialCaption(relayName);
    relay[id]->getChannel()->setChannelNumber(id);
  }

  Serial.println(F("przekazniki ustawione"));
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
}

uint32_t SprinklerRelay::calculateProgramTimeMs() {
  uint32_t time = 0;
  Serial.printf("USE CONFIG VALUES: %d %d %d %d\n", config.getSprShort(), config.getSprLong(), config.getDropShort(), config.getDropLong());
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
  Serial.printf("CALCULATED TIME %d, IN MS: %d\n", time, time*MS_IN_MIN);
  return time*MS_IN_MIN;
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
  display.pingEditMode();   // avoid leaving edit mode
  Serial.printf("nextEditValue %d\n", display.getEditMode());
    /*
  BackSprinklers = 2,
  SideSprinklers = 3,
  Flowers = 4,
  Vegetables = 5,
    case */
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

void SprinklerRelay::ticTacTimer() {
  time_t scheduletime = config.makeScheduleTimeToday(); 
  time_t now = time(nullptr);
  if (scheduletime==now && isScheduleCycleEnabled() && !getProgramRelay()->getProgramInProgress()) {
    if (calculateProgramTimeMs()>0) {
      getProgramRelay()->startCycleNow(true);
    } else {// nic do zrobienia
      disableScheduleCycle();
      messenger.sendMessage(Msg::SCHEDULE_FAILED);
    }
  }

  // synchronizacja relay dla światła
  if (relay[RelayId::TimedLightSwitch]->isOn()) {
    if (!relay[RelayId::FixedLightSwitch]->isOn()) {
      relay[RelayId::FixedLightSwitch]->turnOn(config.getLightActivationTimeS()*1000);
    }
    else 
      relay[RelayId::FixedLightSwitch]->turnOff();
    relay[RelayId::TimedLightSwitch]->turnOff();
  }
    
  uint32_t remaininingTime, setTime;
  
  bool isNextValveRequiresPump = relay[RelayId::EmptyTank]->isOn();
  if (!isNextValveRequiresPump)
    for (int valveId = RelayId::_FirstValve; valveId<=_LastValve; valveId++) {
      if (getScheduledProgramTime(valveId)>0) {   //znajdz następny krok programu w kolejności
        isNextValveRequiresPump = getValveById(valveId)->getRequiresPump();
        break;
      }
    }

  RelayId activeRelayId = SprinklerRelay::getActiveValveId();
  uint32_t remainingSeconds = 0;
  if (activeRelayId>0)
    SprinklerRelay::getValveById(activeRelayId)->getRemainingCountdownTimerSec(&remainingSeconds);

  if (!isNextValveRequiresPump && remainingSeconds>0 && remainingSeconds<=PUMP_ADVANCE_OFF_TIME_MS/1000 && SprinklerRelay::getPumpRelay()->isOn())
    SprinklerRelay::getPumpRelay()->turnOff();
  
  if (remainingSeconds>0 || !programRelay->isOn()) // żadne akcje dalej niepotrzebne gdy program nie działa lub nie wymaga jeszcze kolejnego kroku
    return;

  /*
  if (relay[RelayId::FrontSprinklers]->getRemainingCountdownTimerSec(&remaininingTime) &&
    remaininingTime>0 && 
    relay[RelayId::FrontSprinklers]->isOn())
    return;
  if (relay[RelayId::BackSprinklers]->getRemainingCountdownTimerSec(&remaininingTime) &&
    remaininingTime>0 && 
    relay[RelayId::BackSprinklers]->isOn())
    return;
  if (relay[RelayId::SideSprinklers]->getRemainingCountdownTimerSec(&remaininingTime) &&
    remaininingTime>0 && 
    relay[RelayId::SideSprinklers]->isOn())
    return;
  if (relay[RelayId::Flowers]->getRemainingCountdownTimerSec(&remaininingTime) &&
    remaininingTime>0 && 
    relay[RelayId::Flowers]->isOn())
    return;
  if (relay[RelayId::Vegetables]->getRemainingCountdownTimerSec(&remaininingTime) &&
    remaininingTime>0 && 
    relay[RelayId::Vegetables]->isOn())
    return;
*/

  // szukam kolejnego kroku programu
  setTime = getScheduledProgramTime(RelayId::FrontSprinklers);
  if (setTime>0) {
    relay[RelayId::FrontSprinklers]->turnOn(setTime*MS_IN_MIN);
    relay[RelayId::FrontTimeShort]->turnOff();
    relay[RelayId::FrontTimeLong]->turnOff();
    if (!relay[RelayId::Pump]->isOn()) relay[RelayId::Pump]->turnOn();
    programRelay->updateRemainingTime(setTime*MS_IN_MIN);
    return;
  } 

  setTime = getScheduledProgramTime(RelayId::BackSprinklers);
  if (setTime>0) {
    relay[RelayId::BackSprinklers]->turnOn(setTime*MS_IN_MIN);
    relay[RelayId::BackTimeShort]->turnOff();
    relay[RelayId::BackTimeLong]->turnOff();
    if (!relay[RelayId::Pump]->isOn()) relay[RelayId::Pump]->turnOn();
    programRelay->updateRemainingTime(setTime*MS_IN_MIN);
    return;
  }
   
  setTime = getScheduledProgramTime(RelayId::SideSprinklers);
  if (setTime>0) {
    relay[RelayId::SideSprinklers]->turnOn(setTime*MS_IN_MIN);
    relay[RelayId::SideTimeShort]->turnOff();
    relay[RelayId::SideTimeLong]->turnOff();
    if (!relay[RelayId::Pump]->isOn()) relay[RelayId::Pump]->turnOn();
    programRelay->updateRemainingTime(setTime*MS_IN_MIN);
    return;
  } 
 
  setTime = getScheduledProgramTime(RelayId::Flowers);
  if (setTime>0) {
    relay[RelayId::Flowers]->turnOn(setTime*MS_IN_MIN);
    relay[RelayId::FlowersTimeShort]->turnOff();
    relay[RelayId::FlowersTimeLong]->turnOff();
    if (!SprinklerRelay::isPumpRequired() && relay[RelayId::Pump]->isOn())
      relay[RelayId::Pump]->turnOff();
    programRelay->updateRemainingTime(setTime*MS_IN_MIN);
    return;
  } 

  setTime = getScheduledProgramTime(RelayId::Vegetables);
  if (setTime>0) {
    relay[RelayId::Vegetables]->turnOn(setTime*MS_IN_MIN);
    relay[RelayId::VegetablesTimeShort]->turnOff();
    relay[RelayId::VegetablesTimeLong]->turnOff();
    if (!SprinklerRelay::isPumpRequired() && relay[RelayId::Pump]->isOn())
      relay[RelayId::Pump]->turnOff();
    programRelay->updateRemainingTime(setTime*MS_IN_MIN);
    return;
  } 
}

// overrides


void SprinklerRelay::turnOn(_supla_int_t duration) {
  mojCzasDzialaniaMs = /*duration;*/getScheduledProgramTime(this->myRelayId)*MS_IN_MIN;
  //duration = mojCzasDzialaniaMs;       
  momentWlaczeniaMs = millis();
  czyOdlicza = true;
  Relay::turnOn(mojCzasDzialaniaMs);
  //Serial.printf("handle turnOn!!! %d/%d", duration, this->durationMs);
  this->durationMs = mojCzasDzialaniaMs;
  //relay[RelayId::Pump]->turnOn();
} 

void SprinklerRelay::turnOff(_supla_int_t duration) {
    czyOdlicza = false;
    Relay::turnOff(duration);
    //relay[RelayId::Pump]->turnOff();
}

// 4. KLUCZ: Nadpisujemy pętlę logiczną, aby kontrolować stan odliczania
void SprinklerRelay::iterateAlways() {
    Supla::Control::Relay::iterateAlways();

    // Jeśli czas minął, upewniamy się, że flaga odliczania zgasła
    if (czyOdlicza && (millis() - momentWlaczeniaMs >= mojCzasDzialaniaMs)) {
        czyOdlicza = false;
    }
}

// 5. TAJNA METODA: To z niej aplikacja pobiera informację o zegarku na ekranie!
// Nadpisujemy ją, zwracając nasz własny wyliczony czas w sekundach
int32_t SprinklerRelay::handleNewValueFromServer(TSD_SuplaChannelNewValue *newValue) {
    // Wywołujemy oryginalną metodę, aby SUPLA obsłużyła stan pinu
    int32_t result = Relay::handleNewValueFromServer(newValue);
   
    // Jeśli przekaźnik się właśnie włączył z aplikacji, resetujemy nasz licznik
    if (this->isOn()) {
        if (!czyOdlicza) {
            momentWlaczeniaMs = millis();
            czyOdlicza = true;
        }
    }
    uint32_t  val = getTimerRemainingTimeSec();
    if (val>0)  newValue->DurationMS = val;
    return result;
}

// Jeśli Twoja wersja biblioteki posiada metodę zwracającą pozostały czas do aplikacji,
// to to mapowanie upewni się, że aplikacja dostanie właściwą liczbę sekund:
uint32_t SprinklerRelay::getTimerRemainingTimeSec() {
    if (!czyOdlicza || !this->isOn()) {
        return 0;
    }
    unsigned long minelo = millis() - momentWlaczeniaMs;
    if (minelo >= mojCzasDzialaniaMs) {
        return 0;
    }
    return 1+(mojCzasDzialaniaMs - minelo) / 1000;
}

/*
uint32_t SprinklerRelay::getRemainingTimeSec() {
  uint32_t remaininingTime = ((SprinklerRelay*)relay[RelayId::FrontSprinklers])->getTimerRemainingTimeSec();
  if (remaininingTime>0) return remaininingTime;
  remaininingTime = ((SprinklerRelay*)relay[RelayId::BackSprinklers])->getTimerRemainingTimeSec();
  if (remaininingTime>0) return remaininingTime;
  remaininingTime = ((SprinklerRelay*)relay[RelayId::SideSprinklers])->getTimerRemainingTimeSec();
  if (remaininingTime>0) return remaininingTime;
  remaininingTime = ((SprinklerRelay*)relay[RelayId::Flowers])->getTimerRemainingTimeSec();
  if (remaininingTime>0) return remaininingTime;
  remaininingTime = ((SprinklerRelay*)relay[RelayId::Vegetables])->getTimerRemainingTimeSec();
  if (remaininingTime>0) return remaininingTime;
  
  int32_t remainingSeconds = 0;
  getRemainingCountdownTimerSec(&remainingSeconds);
  return remainingSeconds;
}*/

RelayId SprinklerRelay::getActiveValveId() {
  if (SprinklerRelay::getRelayById(RelayId::FrontSprinklers)->isOn()) return RelayId::FrontSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::BackSprinklers)->isOn()) return RelayId::BackSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::SideSprinklers)->isOn()) return RelayId::SideSprinklers;
  if (SprinklerRelay::getRelayById(RelayId::Flowers)->isOn()) return RelayId::Flowers;
  if (SprinklerRelay::getRelayById(RelayId::Vegetables)->isOn()) return RelayId::Vegetables;
  return RelayId::Pump;
}
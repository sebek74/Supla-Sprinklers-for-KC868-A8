#ifndef SPRINKLERRELAY_H
#define SPRINKLERRELAY_H

#include <PCF8574.h>
#include <supla/io/PCF8574.h>
#include <supla/sensor/container.h>
#include <supla/sensor/binary.h>
#include <supla/control/relay.h>
#include "Definitions.h"



enum RelayId {
  Pump = 0,
    _FirstValve = 1,
  FrontSprinklers = 1,
  BackSprinklers = 2,
  SideSprinklers = 3,
    _DoorSensor = 3,
  Flowers = 4,
    _ShedLightSwitchButton = 4,
  Vegetables = 5,
    _BackHouseSwitchButton = 5,
    _LastValve = 5,
  RefillTank = 6,
  EmptyTank = 7,
    _LastPsychicalRelay = 7,
  RunCycleNow = 8,
    _WaterContainerSensor = 8,
  ScheduleCycle = 9,
    _FirstProgram = 9,
  FrontTimeShort = 10,
    _EditScheduleTime = 10,
    _LastEditMode = 10,
  FrontTimeLong = 11,
  BackTimeShort = 12,
  BackTimeLong = 13,
  SideTimeShort = 14,
  SideTimeLong = 15,
  FlowersTimeShort = 16,
  FlowersTimeLong = 17, 
  VegetablesTimeShort = 18,
  VegetablesTimeLong = 19,
   _LastProgram = 19,
  StopAll = 20,
  FixedLight = 21,
  TimedLight = 22,
  Extra = 23,
    _LastRelay = 23
};


enum ActionId {
  RestoreDisplay,
  TankRefilling = 10,
  TankDoNothing = 50,
  TankEmptying = 100,
  EmptyRelayOn,
  EmptyRelayOff,
  RefillRelayOn,
  RefillRelayOff,
  ActionTogglePump, 
  ActionStopAll,
  ActionRunCycle,
  ActionToggleScheduleCycle,
  ActionMessageScheduleCycle,
  AddProgramTimeSprShort,
  AddProgramTimeSprLong,
  AddProgramTimeSprBoth,
  AddProgramTimeDropShort,
  AddProgramTimeDropLong,
  AddProgramTimeDropBoth,
  SubProgramTimeSprShort,
  SubProgramTimeSprLong,
  SubProgramTimeSprBoth,
  SubProgramTimeDropShort,
  SubProgramTimeDropLong,
  SubProgramTimeDropBoth,
  NextEditMode,
  NextEditValue,
  DoorOpen,
  FixedLightOn,
  FixedLightOff
};

#define REQUIRES_PUMP true

class SprinklerProgramRelay;
class SprinklerActionHandler;
class SprinklerRelay;

class SprinklerRelay : public Supla::Control::Relay {
  
private:
  friend class SprinklerActionHandler;
  bool requiresPump;
  int myRelayId;
  inline static bool relaysInitialized = false;

protected:
  inline static Supla::Io::PCF8574 *inPcf = new Supla::Io::PCF8574(PCF_INPUTS_ADDR);
  inline static Supla::Io::PCF8574 *outPcf = new Supla::Io::PCF8574(PCF_RELAYS_ADDR);
  inline static Supla::Control::Relay *relay[RelayId::_LastRelay+1] = {};
  inline static Supla::Sensor::Container* waterContainerSensor = nullptr;
  inline static Supla::Sensor::Binary* lowWaterSensor = nullptr;
  inline static Supla::Sensor::Binary* highWaterSensor= nullptr;
  inline static Supla::Sensor::Binary* doorSensor= nullptr;    
  inline static SprinklerProgramRelay* programRelay =nullptr;
  inline static int32_t fixedLightStartMs = 0;
  inline static int32_t pumpStartMs = 0;

public:
  static void registerRelays(); 
  static void initalizeRelays();
  static void initializeConfigButton();
  static void ticTacTimer();
  static _supla_int_t calculateProgramTimeMs();

  explicit SprinklerRelay(int pin, int relayId, bool pumpRequired) : Supla::Control::Relay(outPcf, pin, false) {
      getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_STAIRCASETIMER);
      getChannel()->setDefaultIcon(1);
      requiresPump = pumpRequired;
      myRelayId = relayId;
  };
  static Supla::Control::Relay* getRelayById(int relayId) {return relay[relayId]; };
  static SprinklerRelay* getValveById(int relayId) { 
    if (relayId>=RelayId::_FirstValve && relayId<=RelayId::_LastValve) 
      return static_cast<SprinklerRelay*>(relay[relayId]);
    else
      return nullptr;
  }
  static Supla::Sensor::Container* getWaterContainerSensor() { return waterContainerSensor; };
  static SprinklerProgramRelay* getProgramRelay() { return programRelay; };
  static void turnOffAll();
  static void completeProgram(bool runScheduled);
  //static void updatePumpRelay();
  static RelayId getActiveValveId();
  //static uint32_t getActiveValveRemainingTimeSec();
  static bool isPumpRequired() {
    for (int id=RelayId::_FirstValve; id<=RelayId::_LastValve; id++) {
      SprinklerRelay* valve = getValveById(id);
      if (valve->getRequiresPump() && valve->isOn()) return true;
    }
    return relay[RelayId::EmptyTank]->isOn();
  }
  static Supla::Control::Relay* getPumpRelay() { return relay[RelayId::Pump]; }
  static bool isPumpOn() { return relay[RelayId::Pump]->isOn(); }
  static bool isScheduleCycleEnabled() { return relay[RelayId::ScheduleCycle]->isOn(); }
  static bool enableNextMessage;
  static void disableScheduleCycle(bool enableMessage) { 
    enableNextMessage = enableMessage;
    relay[RelayId::ScheduleCycle]->turnOff(); 
  }

  static ActionId getTankStatus();
  static void nextEditValue();
  static bool isDoorOpen() { return !doorSensor->getValue(); }
  static bool isLightOn() { return relay[RelayId::FixedLight]->isOn(); }
  static uint32_t getScheduledProgramTimeS(int relayId);
  static void getRelayTimeText(int32_t relayId, char* result, int32_t len);

  bool getRequiresPump() { return requiresPump; }
  void turnOn(_supla_int_t duration = 0) override;
  void turnOff(_supla_int_t duration = 0) override;
};

#endif //SPRINKLERRELAY_H
#ifndef SPRINKLERRELAY_H
#define SPRINKLERRELAY_H

#include <PCF8574.h>
#include <supla/io/PCF8574.h>
#include <supla/sensor/container.h>
#include <supla/sensor/binary.h>
#include <supla/control/relay.h>
#include <supla/control/button.h>
#include "Definitions.h"



enum RelayId {
  Pump = 0,
    _CoverButton1 = 0,
  FrontSprinklers = 1,
    _FirstValve = 1,
    _CoverButton2 = 1,
  BackSprinklers = 2,
    _CoverButton3= 2,
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
  TogglePump,
  ActionTogglePumpEvent, 
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
  //inline static Supla::Control::Button* coverButton[3];
  inline static SprinklerProgramRelay* programRelay =nullptr;
  inline static int32_t fixedLightStartMs = 0;
  inline static int32_t pumpStartMs = 0;

public:
  static void RegisterRelays(); 
  static void InitalizeRelays();
  static void InitializeConfigButton();
  static void TicTacTimer();
  static _supla_int_t CalculateProgramTimeMs();

  explicit SprinklerRelay(int pin, int relayId, bool pumpRequired) : Supla::Control::Relay(outPcf, pin, false) {
      getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_STAIRCASETIMER);
      getChannel()->setDefaultIcon(1);
      requiresPump = pumpRequired;
      myRelayId = relayId;
  };
  static Supla::Control::Relay* GetRelayById(int relayId) {return relay[relayId]; };
  static SprinklerRelay* GetValveById(int relayId) { 
    if (relayId>=RelayId::_FirstValve && relayId<=RelayId::_LastValve) 
      return static_cast<SprinklerRelay*>(relay[relayId]);
    else
      return nullptr;
  }
  static Supla::Sensor::Container* GetWaterContainerSensor() { return waterContainerSensor; };
  static SprinklerProgramRelay* GetProgramRelay() { return programRelay; };
  static void TurnOffAll();
  static void CompleteProgram(bool runScheduled);
  //static void updatePumpRelay();
  static RelayId GetActiveValveId();
  //static uint32_t getActiveValveRemainingTimeSec();
  static bool IsPumpRequired() {
    for (int id=RelayId::_FirstValve; id<=RelayId::_LastValve; id++) {
      SprinklerRelay* valve = GetValveById(id);
      if (valve->getRequiresPump() && valve->isOn()) return true;
    }
    return relay[RelayId::EmptyTank]->isOn();
  }
  static Supla::Control::Relay* GetPumpRelay() { return relay[RelayId::Pump]; }
  static bool IsPumpOn() { return relay[RelayId::Pump]->isOn(); }
  static bool IsScheduleCycleEnabled() { return relay[RelayId::ScheduleCycle]->isOn(); }
  static bool enableNextMessage;
  static void DisableScheduleCycle(bool enableMessage) { 
    enableNextMessage = enableMessage;
    relay[RelayId::ScheduleCycle]->turnOff(); 
  }

  static ActionId GetTankStatus();
  static void NextEditValue();
  static bool IsDoorOpen() { return !doorSensor->getValue(); }
  static bool IsLightOn() { return relay[RelayId::FixedLight]->isOn(); }
  static uint32_t GetScheduledProgramTimeS(int relayId);
  static void GetRelayTimeText(int32_t relayId, char* result, int32_t len);

  bool getRequiresPump() { return requiresPump; }
  void turnOn(_supla_int_t duration = 0) override;
  void turnOff(_supla_int_t duration = 0) override;
};

#endif //SPRINKLERRELAY_H
#ifndef SPLINKERRELAY_H
#define SPLINKERRELAY_H

#define PCF_RELAYS_ADDR 0x20  // Ekspander wyjściowy (Przekaźniki)
#define PCF_INPUTS_ADDR 0x22  // Ekspander wejściowy (Wejścia cyfrowe)
#define MAX_CHANNELS 8

#include <PCF8574.h>
#include <supla/io/PCF8574.h>


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
  VirtualTankLevel = 8,
    _EditScheduleTime = 8,
  RunCycleNow = 9,
    _FirstProgram = 10,
  ScheduleCycle = 10,
  FrontTimeShort = 11,
  FrontTimeLong = 12,
  BackTimeShort = 13,
  BackTimeLong = 14,
  SideTimeShort = 15,
  SideTimeLong = 16,
  FlowersTimeShort = 17,
  FlowersTimeLong = 18, 
  VegetablesTimeShort = 19,
  VegetablesTimeLong = 20,
  StopAll = 21,
    _LastProgram = 21,
  FixedLight = 22,
  TimedLight = 23,
  Extra = 24,
    _LastRelay = 24
};


enum ActionId {
  RestoreDisplay,
  TankRefill = 10,
  TankDoNothing = 50,
  TankEmpty = 100,
  EmptyRelayOn,
  EmptyRelayOff,
  ActionTogglePump, 
  ActionStopAll,
  ActionRunCycle,
  ActionScheduleCycle,
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


class SprinklerProgramRelay;
class SprinklerActionHandler;

class SprinklerRelay : public Supla::Control::Relay {
  
private:
  friend class SprinklerActionHandler;
  uint32_t mojCzasDzialaniaMs = 4000;
  unsigned long momentWlaczeniaMs = 0;
  bool czyOdlicza = false;
  bool requiresPump;
  int myRelayId;
  inline static bool relaysInitialized = false;

protected:
  inline static Supla::Io::PCF8574 *inPcf = new Supla::Io::PCF8574(0x22);
  inline static Supla::Io::PCF8574 *outPcf = new Supla::Io::PCF8574(0x20);
  inline static Supla::Control::Relay *relay[RelayId::_LastRelay+1] = {};
  //inline static Supla::Control::Button *button[RelayId::_LastRelay+1] = {};
  //inline static Supla::Control::ActionTrigger *at[RelayId::_LastRelat+1] = {};
  inline static Supla::Sensor::Binary* lowWaterSensor = nullptr;
  inline static Supla::Sensor::Binary* highWaterSensor= nullptr;
  inline static Supla::Sensor::Binary* doorSensor= nullptr;    
  inline static SprinklerProgramRelay* programRelay =nullptr;
  static int32_t fixedLightStartMs;
  static int32_t pumpStartMs;

public:
  static void registerRelays(); 
  static void initalizeRelays();
  static void initializeConfigButton();
  static void ticTacTimer();
  static uint32_t calculateProgramTimeMs();
  static uint32_t getCountdownTimerRemainingTimeSec();

    explicit SprinklerRelay(int pin, bool pumpRequired) : Supla::Control::Relay(outPcf, pin, false) {
        getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_STAIRCASETIMER);
        getChannel()->setDefaultIcon(1);
        requiresPump = pumpRequired;
        myRelayId = pin;
    };
    static Supla::Control::Relay* getRelayById(int relayId) {return relay[relayId]; };
    static SprinklerRelay* getValveById(int relayId) { 
      if (relayId>=RelayId::_FirstValve && relayId<=RelayId::_LastValve) 
        return (SprinklerRelay*)relay[relayId];
      else
        return nullptr;
    }
    static SprinklerProgramRelay* getProgramRelay() { return programRelay; };
    static void turnOffAll();
    static void completeProgram();
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
    static void disableScheduleCycle() { relay[RelayId::ScheduleCycle]->turnOff(); }

    static ActionId getTankStatus();
    static void nextEditValue();
    static bool isDoorOpen() { return !doorSensor->getValue(); }
    static bool isLightOn() { return relay[RelayId::FixedLight]->isOn(); }

    uint32_t getTimerRemainingTimeSec();
    bool getRequiresPump() { return requiresPump; }
    void turnOn(_supla_int_t duration = 0) override;
    void turnOff(_supla_int_t duration = 0) override;
    void iterateAlways() override;
    int32_t handleNewValueFromServer(TSD_SuplaChannelNewValue *newValue) override;
    
    static uint32_t getScheduledProgramTime(int relayId);

    virtual uint32_t getProgramTimeMs() { return mojCzasDzialaniaMs; }
    virtual void setProgramTimeMs(uint32_t time) { mojCzasDzialaniaMs = time; }
    static void getRelayTimeText(int32_t relayId, char* result, int32_t len);
};

#endif //SPLINKERRELAY_H
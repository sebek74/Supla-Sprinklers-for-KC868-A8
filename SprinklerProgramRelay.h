#ifndef SPLINKERPROGRAMRELAY_H
#define SPLINKERPROGRAMRELAY_H

#include <supla/control/virtual_relay.h>

class SprinklerProgramRelay : public Supla::Control::VirtualRelay {
private:
    uint32_t mojCzasDzialaniaMs = 10000; 
    unsigned long momentWlaczeniaMs = 0;
    bool czyOdlicza = false;
    bool programInProgress = false;
    bool runScheduled = false;
public:
    SprinklerProgramRelay() {
        this->getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_STAIRCASETIMER);
        this->getChannel()->setDefaultIcon(1);
    };
    void startCycleNow(bool scheduled);
    void completeCycleNow() { runScheduled=false; }
    bool isRunScheduled() { return runScheduled; };
    void updateRemainingTime(_supla_int_t addDuration);
    //void handleAction(int event, int action) override;
    void turnOn(_supla_int_t duration = 0) override;
    void turnOff(_supla_int_t duration = 0) override;
    void iterateAlways() override;
    int32_t handleNewValueFromServer(TSD_SuplaChannelNewValue *newValue) override;
    uint32_t getCountdownTimerRemainingTimeSec();
    virtual uint32_t getProgramTime() { return mojCzasDzialaniaMs; }
    bool getProgramInProgress() { return czyOdlicza && programInProgress; }
    //void handleAction(int event, int action) override;
    //virtual void setProgramTime(uint32_t time) { mojCzasDzialaniaMs = time; }
};

#endif //SPLINKERPROGRAMRELAY_H 
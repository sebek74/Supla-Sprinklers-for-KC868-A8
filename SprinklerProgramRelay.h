#ifndef SPRINKLERPROGRAMRELAY_H
#define SPRINKLERPROGRAMRELAY_H

#include <supla/control/virtual_relay.h>

class SprinklerProgramRelay : public Supla::Control::VirtualRelay {
private:
    unsigned long startTimeMs = 0;
    unsigned long calculatedTimeMs = 0;
    bool runScheduled = false;
    bool inProgress = false;
public:
    SprinklerProgramRelay() {
        this->getChannel()->setDefaultFunction(SUPLA_CHANNELFNC_STAIRCASETIMER);
        this->getChannel()->setDefaultIcon(1);
    };
    void startCycleNow(bool scheduled);
    void completeCycleNow() { runScheduled=false; }
    bool isRunScheduled() { return runScheduled; };
    void updateRemainingTime(_supla_int_t addDuration);
    void turnOn(_supla_int_t duration = 0) override;
    void turnOff(_supla_int_t duration = 0) override;
    bool getProgramInProgress() { return this->isOn(); }
    void recalculateRemainingTime();
};

#endif //SPRINKLERPROGRAMRELAY_H 


#ifndef SPRINKLERSENSORS_H
#define SPRINKLERSENSORS_H

#include "supla/sensor/thermometer.h"
#include <string>

#define DALLAS_GPIO 14

class SprinklerSensors {

  public:
    void Initialize();
    double getTemp(int32_t index);
    int32_t getSensorsAmount();
    bool getDeviceAdress(int32_t index, DeviceAddress& address);
    bool getDeviceAdress(int32_t index, char* result, int32_t len);
    const char* getDeviceDescription(int32_t index);
    void setupSensorCloudData();
};
#endif //SPRINKLESENSORS
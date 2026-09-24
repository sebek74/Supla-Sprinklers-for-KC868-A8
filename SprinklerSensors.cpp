#include <supla/sensor/DS18B20.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <supla/storage/storage.h>
#include "SprinklerSensors.h"
#include "SprinklerConfig.h"
#include "SprinklerRelay.h"

extern SprinklerConfig config;
DallasTemperature dallas;
Supla::Sensor::DS18B20* dbSensors[16];

void SprinklerSensors::Initialize() {
  dbSensors[0] = new Supla::Sensor::DS18B20(DALLAS_GPIO);  // get 1st sensor, initialize pin
  dallas = dbSensors[0]->getHwSensors();
  DeviceAddress address;
  dallas.setWaitForConversion(true);
  dallas.requestTemperatures();
  dallas.setWaitForConversion(false);
  for (int i = 0; i < dallas.getDeviceCount(); i++) {
    dallas.getAddress(address, i);
    if (i>0)
      dbSensors[i] = new Supla::Sensor::DS18B20(DALLAS_GPIO, address);
    dbSensors[i]->setInitialCaption(config.getSensorName(i));
    dbSensors[i]->getChannel()->setChannelNumber(RelayId::_LastRelay*3+i);
    SUPLA_LOG_DEBUG("Sensor %d - address {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}", i, \
        address[0], address[1], address[2], address[3], address[4], address[5], address[6], address[7]);
  }
}

int32_t SprinklerSensors::getSensorsAmount() {
  return dallas.getDeviceCount();
};

bool SprinklerSensors::getDeviceAdress(int32_t index, DeviceAddress& address) {
  return dallas.getAddress(address, index);
};

bool SprinklerSensors::getDeviceAdress(int32_t index, char* result, int32_t len) {
  DeviceAddress address;
  if (!dallas.getAddress(address, index)) return false;
  snprintf(
      result, len,
      "{0x%02X%02X%02X%02X%02X%02X%02X%02X}",
      address[0],
      address[1],
      address[2],
      address[3],
      address[4],
      address[5],
      address[6],
      address[7]);
  return true;   
}
const char* SprinklerSensors::getDeviceDescription(int32_t index) {
  if (index >= dallas.getDeviceCount())
      return "";
  return config.getSensorName(index);
}

double SprinklerSensors::getTemp(int32_t index) {
  if (index >= dallas.getDeviceCount())
      return -99;
  return dallas.getTempCByIndex(index);
};

void SprinklerSensors::setupSensorCloudData() {
  for (int i=0; i<getSensorsAmount(); i++) {
    char key[16], value[64];
    snprintf(key, 16, "DALLAS%d", i);
    if (!Supla::Storage::ConfigInstance()->getString(key, value, 64))
      sprintf(value, "Termometr %d", i+1);
    dbSensors[i]->setInitialCaption(value);
  }
}
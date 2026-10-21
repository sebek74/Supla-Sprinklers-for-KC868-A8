#include <supla/sensor/DS18B20.h>
#include <supla/control/relay.h>
#include <supla/sensor/binary.h>
#include <supla/storage/storage.h>
#include <supla/sensor/thermometer.h>
#include <RTClib.h>
#include "SprinklerSensors.h"
#include "SprinklerConfig.h"
#include "SprinklerRelay.h"

#define TEMP_PARAM_HEADER "DALLAS"
extern SprinklerConfig config;
extern RTC_DS3231* rtc;

class RTCClock : public Supla::Sensor::Thermometer {
  private:
    double getValue() override {
      if (rtc) return rtc->getTemperature();
      return TEMPERATURE_NOT_AVAILABLE;
    }
  public:
    double getTemperature() {
      return getValue();
    }
};

RTCClock* rtcClock = nullptr;
DallasTemperature dallas;

void SprinklerSensors::Initialize() {
  Supla::Sensor::DS18B20* zeroSensor = new Supla::Sensor::DS18B20(ONE_WIRE_GPIO);  // get 1st sensor, initialize pin
  dallas = zeroSensor->getHwSensors();
  if (zeroSensor->getValue()==TEMPERATURE_NOT_AVAILABLE)  return;
  DeviceAddress address;
  dallas.setWaitForConversion(true);
  dallas.requestTemperatures();
  dallas.setWaitForConversion(false);
  for (int i = 0; i < dallas.getDeviceCount(); i++) {
    if (i==0) 
      sensors[0] = zeroSensor;
    else {
      dallas.getAddress(address, i);
      sensors[i] = new Supla::Sensor::DS18B20(ONE_WIRE_GPIO, address);
    }
    sensors[i]->setInitialCaption(config.getSensorName(i));
    sensors[i]->getChannel()->setChannelNumber(RelayId::_LastRelay * 3 + i);
    sensorAmount++;
    SUPLA_LOG_DEBUG("Temp sensor %d (%02X%02X%02X%02X%02X%02X%02X%02X) added", i,
                    address[0], address[1], address[2], address[3], address[4], address[5], address[6], address[7]);
  }
  if (rtc) {
    rtcClock = new RTCClock;
    sensors[sensorAmount] = rtcClock;
    rtcClock->setInitialCaption(config.getSensorName(sensorAmount));
    rtcClock->getChannel()->setChannelNumber(RelayId::_LastRelay * 3 + sensorAmount);
    SUPLA_LOG_DEBUG("Temp sensor %d (RTC) added", sensorAmount);
    sensorAmount++;
  }
}

bool SprinklerSensors::getDeviceAdress(int32_t index, DeviceAddress& address) {
  if (index < dallas.getDeviceCount())
    return dallas.getAddress(address, index);
  return false;
};

bool SprinklerSensors::getDeviceAdress(int32_t index, char* result, int32_t len) {
  DeviceAddress address;
  if (index < dallas.getDeviceCount()) {
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
  } else if (sensors[index] == rtcClock) {
    snprintf(result, len, "(RTC)");
    return true;
  }
  return false;
}

const char* SprinklerSensors::getDeviceDescription(int32_t index) {
  if (index >= sensorAmount)
    return "";
  return config.getSensorName(index);
}

double SprinklerSensors::getTemp(int32_t index) {
  if (index < dallas.getDeviceCount())
    return dallas.getTempCByIndex(index);
  if (sensors[index] == rtcClock)
    return rtcClock->getTemperature();
  return TEMPERATURE_NOT_AVAILABLE;
};

void SprinklerSensors::setupSensorCloudData() {
  for (int i = 0; i < sensorAmount; i++) {
    char key[16], value[64];
    snprintf(key, 16, "%s%d", TEMP_PARAM_HEADER, i);
    if (!Supla::Storage::ConfigInstance()->getString(key, value, 64))
      sprintf(value, "Termometr %d", i + 1);
    sensors[i]->setInitialCaption(value);
  }
}
#include <stdint.h>
#include <SuplaDevice.h>
#include <supla/control/relay.h>
#include <supla/control/virtual_relay.h>
#include <supla/sensor/binary.h>
#include <supla/log_wrapper.h>
#include "SprinklerMessenger.h"
#include <supla/device/notifications.h>
#include "SprinklerRelay.h"

void SprinklerMessenger::initialize() {
  Supla::Notification::RegisterNotification(-1); 
}

void SprinklerMessenger::sendMessage(const char* code, uint32_t param1,  uint32_t param2) {
  sendMessage(-1, code, param1, param2);
}

void SprinklerMessenger::sendMessage(int16_t context, const char* code, uint32_t param1,  uint32_t param2) {
  char text[64];
  snprintf(text, sizeof(text), code, param1, param2);
  Supla::Notification::Send(context, MSG_TITLE, text);
  SUPLA_LOG_DEBUG("Messenger: %s\n", text);
}


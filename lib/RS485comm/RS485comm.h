#include "Arduino.h"
#include <hw_config.h>
#include <freertos/FreeRTOS.h>

namespace RS485comm {

static const uint8_t enablePin = COMM_EN_PIN;
static const char HEAD = '#';

}
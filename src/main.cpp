#include <Arduino.h>
#include "../lib/util.h"

void setup() {
  Serial.begin(115200);
  Wire.begin();

  initI2CUtilities();
}

void loop() {

}
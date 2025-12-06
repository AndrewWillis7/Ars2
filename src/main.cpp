#include <Arduino.h>
#include <I2CUtils.h>

void setup() {
  Serial.begin(115200);
  Wire.begin();
  I2CUtils::begin();
}

void loop() {

}
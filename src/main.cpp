#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>

// Sensor Includes
#include "../lib/sensors/color_sensor.h"

// Sensor Objects
ColorSensor color1("Color1", HW_SC_CS1);
ColorSensor color2("Color2", HW_SC_CS2);

bool setupComplete = false;
void setup() {
  Serial.begin(115200);
  delay(100);
  I2CUtils::begin();

  while (!Serial.available()) delay(10);
  Serial.read();

  I2CUtils::scanI2C();

  while (!Serial.available()) delay(10);
  Serial.read();

  color1.setup();
  color2.setup();
  Serial.print("\nSetup Complete!");

  while (!Serial.available()) delay(10);
  Serial.read();

  color1.startTask(50, 1);
  color2.startTask(50, 1);

  setupComplete = true;
}

void loop() {
  if (!setupComplete) {
    return;
  }

  static uint32_t last = 0;
  if (millis() - last > 500) {
    color1.debugPrint();
    color2.debugPrint();
    last = millis();
    //Serial.print(I2CUtils::getCurrentChannel());
    Serial.println();
  }
}
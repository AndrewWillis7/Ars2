#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>

// Sensor Includes
#include "../lib/sensors/color_sensor.h"
#include "../lib/sensors/optical_sensor.h"
#include "../lib/sensors/encoder_sensor.h"

// Sensor Objects
ColorSensor color1("Color1", HW_SC_CS1);
ColorSensor color2("Color2", HW_SC_CS2);

OpticalSensor opt1("Optical1", HW_SC_OP1);
OpticalSensor opt2("Optical2", HW_SC_OP2);

EncoderSensor enc1("Encoder1", HW_SC_EN1);
EncoderSensor enc2("Encoder2", HW_SC_EN2);
EncoderSensor enc3("Encoder3", HW_SC_EN3);

bool setupComplete = false;
void setup() {
  Serial.begin(115200);
  delay(100);
  I2CUtils::begin();

  while (!Serial.available()) delay(10);
  Serial.read();

  I2CUtils::scanI2C();

  // Setups
  while (!Serial.available()) delay(10);
  Serial.read();

  color1.setup();
  color2.setup();

  opt1.setup();
  opt2.setup();

  enc1.setup();
  enc2.setup();
  enc3.setup();

  Serial.print("\nSetup Complete!");

  // Task Starts
  while (!Serial.available()) delay(10);
  Serial.read();

  color1.startTask(50, 1);
  color2.startTask(50, 1);

  opt1.startTask(50, 1);
  opt2.startTask(50, 1);

  enc1.startTask(50, 1, 1);
  enc2.startTask(50, 1, 1);
  enc3.startTask(50, 1, 1);

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

    opt1.debugPrint();
    opt2.debugPrint();

    enc1.debugPrint();
    enc2.debugPrint();
    enc3.debugPrint();

    last = millis();
    //Serial.print(I2CUtils::getCurrentChannel());
    Serial.println();
  }
}
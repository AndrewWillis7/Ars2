#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>
#include <RS485comm.h>
#include <TelemetryBus.h>

// Sensor Includes
#include "../lib/sensors/color_sensor.h"
#include "../lib/sensors/optical_sensor.h"

// Sensor Objects
ColorSensor color1("Color1", HW_SC_CS1);
ColorSensor color2("Color2", HW_SC_CS2);

OpticalSensor opt1("Optical1", HW_SC_OP1, OFF_1_X, OFF_1_Y, OFF_1_H);
OpticalSensor opt2("Optical2", HW_SC_OP2, OFF_2_X, OFF_2_Y, OFF_2_H);

// Process Includes
#include "../lib/Telemetry/RS485Transciever.h"

// Process Objects
// Odometry odoCalc(&enc1, &enc2, &enc3);
RS485Transceiver rs485trx;

bool is_setup = false;

//
bool debugModeSensor = true;
char buf[256];
//

void single_startup() {
    Serial.begin(baudrate);
    RS485comm::begin(Serial1, baudrate);
    delay(100);
    I2CUtils::begin();
    //rs485rx.setup();

    color1.setup();
    color2.setup();

    opt1.setup();
    opt2.setup();

    delay(100);

    color1.startTask(10, 1);
    color2.startTask(10, 1);

    opt1.startTask(10, 1);
    opt2.startTask(10, 1);

    is_setup = true;
}

void h_duplex_startup() {
  Serial.begin(baudrate);
  RS485comm::begin(Serial1, baudrate);
  delay(100);

  I2CUtils::begin();
  TelemetryBus::begin(32);

  color1.setup();
  color2.setup();
  opt1.setup();
  opt2.setup();

  color1.startTask(10, 1);
  color2.startTask(10, 1);
  opt1.startTask(10, 1);
  opt2.startTask(10, 1);

  Serial.println("Posted and Waiting...");

  RS485comm::enableRX();

  rs485trx.setup();

  is_setup = true;
}

void comm_loop() {
  Serial.begin(baudrate);
  RS485comm::begin(Serial1, baudrate);
  delay(100);

  RS485comm::enableRX();

  rs485trx.setup(); 
}

void scan() {
    Serial.begin(baudrate);
    delay(100);
    I2CUtils::begin();
    while (!Serial.available()) {
        delay(10);  // be polite to the CPU
    }
    while (Serial.available()) {
        Serial.read();  // flush the input buffer
    }
    I2CUtils::scanI2C(); // run scan model
}

void setup() {
  //default_startup();
  //scan();
  //comm_loop();
  h_duplex_startup();
}



void loop() {
  if (!is_setup) return;
  delay(1);
}

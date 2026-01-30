#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>
#include <RS485comm.h>

// Sensor Includes
#include "../lib/sensors/color_sensor.h"
#include "../lib/sensors/optical_sensor.h"

// Sensor Objects
ColorSensor color1("Color1", HW_SC_CS1);
ColorSensor color2("Color2", HW_SC_CS2);

OpticalSensor opt1("Optical1", HW_SC_OP1, OFF_1_X, OFF_1_Y, OFF_1_H);
OpticalSensor opt2("Optical2", HW_SC_OP2, OFF_2_X, OFF_2_Y, OFF_2_H);

// Process Includes
#include "../lib/processes/odometry.h"
#include "../lib/processes/RS485Reciever.h"

// Process Objects
// Odometry odoCalc(&enc1, &enc2, &enc3);
RS485Reciever rs485rx;

bool is_setup = false;

//
bool debugModeSensor = true;
char buf[256];
//

void default_startup() {
    Serial.begin(9600);
    RS485comm::begin(Serial1, 9600);
    delay(100);
    I2CUtils::begin();
    //rs485rx.setup();

    color1.setup();
    color2.setup();

    opt1.setup();
    opt2.setup();

    delay(100);

    color1.startTask(50, 1);
    color2.startTask(50, 1);

    opt1.startTask(50, 1);
    opt2.startTask(50, 1);

    is_setup = true;
}

void comm_test() {
  Serial.begin(9600);
  RS485comm::begin(Serial1, 9600);
  delay(100);

  is_setup = true;
}

void scan() {
    Serial.begin(9600);
    delay(100);
    I2CUtils::begin();
    while (!Serial.available()) {
        delay(10);  // be polite to the CPU
    }
    while (Serial.available()) {
        Serial.read();  // flush the input buffer
    }
    I2CUtils::scanI2C();
}

void setup() {
  //comm_test();
  //default_startup();
  scan();
}

void loop() {
  if (!is_setup) {
    return;
  }

  static uint32_t last = 0;
  if (millis() - last > 1) {
    
    snprintf(buf, sizeof(buf),
      "P%+03.0f,%+03.0f,%+03.0fCSA%d,%d,%dCSB%d,%d,%d",
      (opt1.pos.x + opt2.pos.x)/2 * 100.0f,
      (opt1.pos.y + opt2.pos.y)/2 * 100.0f,
      (opt1.pos.h + opt2.pos.h)/2 * 100.0f,
      color1.red, color1.green, color1.blue,
      color2.red, color2.green, color2.blue
    );

    //color1.debugPrint();
    //color2.debugPrint();

    //opt1.debugPrint();
    //opt2.debugPrint();

    //Serial.print(opt1.pos.x - opt2.pos.x);
    //Serial.println();
    //Serial.print(opt1.pos.y - opt2.pos.y);
    //Serial.println();
    //Serial.print(opt1.pos.h - opt2.pos.h);
    //Serial.println();

    last = millis();
    RS485comm::sendPacket(buf);
    //RS485comm::printStats();
    Serial.println(buf);
  }
}

#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>
#include <RS485comm.h>

// Sensor Includes
#include "../lib/sensors/color_sensor.h"
#include "../lib/sensors/optical_sensor.h"
#include "../lib/sensors/encoder_sensor.h"

// Sensor Objects
ColorSensor color1("Color1", HW_SC_CS1);
ColorSensor color2("Color2", HW_SC_CS2);

OpticalSensor opt1("Optical1", HW_SC_OP1, OFF_1_X, OFF_1_Y, OFF_1_H);
OpticalSensor opt2("Optical2", HW_SC_OP2, OFF_2_X, OFF_2_Y, OFF_2_H);

EncoderSensor enc1("Encoder1", HW_SC_EN1);
EncoderSensor enc2("Encoder2", HW_SC_EN2);
EncoderSensor enc3("Encoder3", HW_SC_EN3);

// Process Includes
#include "../lib/processes/odometry.h"
#include "../lib/processes/RS485Reciever.h"

// Process Objects
Odometry odoCalc(&enc1, &enc2, &enc3);
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

    //color1.setup();
    //color2.setup();

    opt1.setup();
    opt2.setup();

    enc1.setup();
    enc2.setup();
    enc3.setup();

    delay(100);

    //color1.startTask(50, 1);
    //color2.startTask(50, 1);

    opt1.startTask(50, 1);
    opt2.startTask(50, 1);

    enc1.startTask(50, 1, 1);
    enc2.startTask(50, 1, 1);
    enc3.startTask(50, 1, 1);

    odoCalc.setup();

    is_setup = true;
}

void comm_test() {
  Serial.begin(9600);
  RS485comm::begin(Serial1, 9600);
  delay(100);

  is_setup = true;
}

void setup() {
  //comm_test();
  default_startup();
}

void loop() {
  if (!is_setup) {
    return;
  }

  static uint32_t last = 0;
  if (millis() - last > 1) {
    
    snprintf(buf, sizeof(buf),
      "P%+03.0f,%+03.0f,%+03.0fO%+03.0f,%+03.0f,%+03.0f",
      (opt1.pos.x + opt2.pos.x)/2 * 100.0f,
      (opt1.pos.y + opt2.pos.y)/2 * 100.0f,
      (opt1.pos.h + opt2.pos.h)/2 * 100.0f,
      odoCalc.x  * 100.0f,
      odoCalc.y  * 100.0f,
      odoCalc.h  * 100.0f
    );

    // color1.debugPrint();
    // color2.debugPrint();

    //opt1.debugPrint();
    // opt2.debugPrint();

    // enc1.debugPrint();
    // enc2.debugPrint();
    // enc3.debugPrint();

    last = millis();
    RS485comm::sendPacket(buf);
    //RS485comm::printStats();
    Serial.println(buf);
  }
}
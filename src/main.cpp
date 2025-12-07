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

OpticalSensor opt1("Optical1", HW_SC_OP1);
OpticalSensor opt2("Optical2", HW_SC_OP2);

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
bool debugModeSensor = false;
//

// -- Start-Ups --
void debug_startup() {
    Serial.begin(115200);
    RS485comm::begin(Serial1, 115200);
    delay(100);
    I2CUtils::begin();
    rs485rx.setup();

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

    Serial.print("\nSetup Complete! Attempting Comm Ping...");

    RS485comm::sendRaw("PING FROM ESP!");
    delay(1000); // Wait for response

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

    // Post-Process Starts
    odoCalc.setup();

    is_setup = true;
}

void default_startup() {
    Serial.begin(115200);
    RS485comm::begin(Serial1, 115200);
    delay(100);
    I2CUtils::begin();
    rs485rx.setup();

    color1.setup();
    color2.setup();

    opt1.setup();
    opt2.setup();

    enc1.setup();
    enc2.setup();
    enc3.setup();

    delay(100);

    color1.startTask(50, 1);
    color2.startTask(50, 1);

    opt1.startTask(50, 1);
    opt2.startTask(50, 1);

    enc1.startTask(50, 1, 1);
    enc2.startTask(50, 1, 1);
    enc3.startTask(50, 1, 1);

    odoCalc.setup();

    is_setup = true;
}

void setup() {
  debug_startup();
  //default_startup();
}

void loop() {
  if (!is_setup) {
    return;
  }

  static uint32_t last = 0;
  if (millis() - last > 500 && debugModeSensor) {
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
    
  } else { // When we are not in Debug, we send packets
    // Packet = #Oxyh#Pxyh#Fxyh#Crgb
  }
}
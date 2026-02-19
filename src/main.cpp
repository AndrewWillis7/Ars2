#include <Arduino.h>
#include <I2CUtils.h>
#include <hw_config.h>
#include <RS485comm.h>
#include <TelemetryBus.h>
#include "../lib/globals.h"

// Sensor Includes
#include "../lib/sensors/color_sensor.h"
#include "../lib/sensors/optical_sensor.h"

// Processes
#include "../lib/Telemetry/RS485Transciever.h"

// statics and vars
static uint32_t HEARTBEAT_INTERVAL_MS = 5000;
uint32_t lastHeartbeat = 0;
static std::vector<SensorBase*> activeSensors;

// ---- Process Objects ----
static RS485Transceiver rs485trx;

static bool g_ready = false;

static void bringUpCore() {
  Serial.begin(baudrate);
  delay(50);

  RS485comm::begin(Serial1, baudrate);
  RS485comm::enableRX();

  I2CUtils::begin();
  TelemetryBus::begin(256);

  globals::reserveSensors(16);
  Serial.println("Core Build. Awaiting INIT");
}

static void bringUpSensors() {
  // Instantiate Sensors
  using namespace globals;

  // repeat wait until globals::state == globals::SystemState::Running;
  while (state != SystemState::RUNNING) {
    Serial.println("Waiting for INIT");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  Serial.println("Configuration Locked. Bring up Sensors");

  // Loop through the gained sensors and instantiate the objects
  for (auto &cfg : sensors) {
    SensorBase* s = nullptr;

    if (cfg.type == "COLOR") {
      s = new ColorSensor(cfg.name.c_str(), cfg.port);
    }
    else if (cfg.type == "OPTICAL") {
      if (cfg.name == "OPTL") {
        s = new OpticalSensor(cfg.name.c_str(), cfg.port, offsets[0], offsets[1], offsets[2]);
      } else if (cfg.name == "OPTR") {
        s = new OpticalSensor(cfg.name.c_str(), cfg.port, offsets[3], offsets[4], offsets[5]);
      }
    }
    else {
      Serial.printf("Unknown sensor type: %s\n", cfg.type.c_str());
      continue;
    }

    s->setup();
    s->startTask(10, 1);
    activeSensors.push_back(s);

    Serial.printf("Started sensor: %s on port %u\n", cfg.name.c_str(), cfg.port);
  }

  Serial.println("All Sensors started!");
}

static void bringUpComms() {
  rs485trx.setup();
}

void setup() {
  bringUpCore();
  bringUpComms();

  Serial.println("System Online.");

  bringUpSensors();

  Serial.println("Sensors Initialized");
  RS485comm::enableRX();
  g_ready = true;
}

void loop() {
  uint32_t now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL_MS) {
    lastHeartbeat = now;
    Serial.println("[HEARTBEAT] system posted!");
  }
}

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
#include "../lib/wifi_forward.h"

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
  using namespace globals;

  while (state != SystemState::RUNNING) {
    Serial.println("Waiting for INIT");
    vTaskDelay(pdMS_TO_TICKS(250));
  }

  Serial.println("Configuration Locked. Bring up Sensors");

  // --- Debug: confirm we actually parsed all sensors from INIT ---
  Serial.printf("[SENSORS] Config entries = %u\n", (unsigned)sensors.size());
  for (auto &cfg : sensors) {
    Serial.printf("[SENSORS] cfg name=%s type=%s port=%u\n",
                  cfg.name.c_str(), cfg.type.c_str(), (unsigned)cfg.port);
  }

  // --- Make offsets safe (prevents UB / heap corruption) ---
  if (offsets.size() < 6) {
    offsets.resize(6, 0.0f);
    Serial.println("[SENSORS] offsets not provided; defaulting to 0s");
  }

  activeSensors.clear();

  for (auto &cfg : sensors) {
    SensorBase* s = nullptr;

    if (cfg.type == "COLOR") {
      s = new ColorSensor(cfg.name.c_str(), cfg.port);
    }
    else if (cfg.type == "OPTICAL") {
      float ox = 0, oy = 0, oh = 0;

      if (cfg.name == "OPTL") {
        ox = offsets[0]; oy = offsets[1]; oh = offsets[2];
      } else if (cfg.name == "OPTR") {
        ox = offsets[3]; oy = offsets[4]; oh = offsets[5];
      } else {
        Serial.printf("[%s] OPTICAL name not recognized (expected OPTL/OPTR)\n", cfg.name.c_str());
        continue;
      }

      s = new OpticalSensor(cfg.name.c_str(), cfg.port, ox, oy, oh);
    }
    else {
      Serial.printf("[%s] Unknown sensor type: %s\n", cfg.name.c_str(), cfg.type.c_str());
      continue;
    }

    if (!s) {
      Serial.printf("[%s] Failed to allocate sensor object\n", cfg.name.c_str());
      continue;
    }

    // Setup (should set internal ok flag)
    s->setup();

    // Only start task + keep it if it actually came up
    if (!s->isAlive()) {
      Serial.printf("[%s] FAILED bring-up on port %u (not started)\n",
                    cfg.name.c_str(), (unsigned)cfg.port);
      delete s;
      continue;
    }

    s->startTask(10, 1);
    activeSensors.push_back(s);

    Serial.printf("Started sensor: %s on port %u\n",
                  cfg.name.c_str(), (unsigned)cfg.port);
  }

  Serial.printf("[SENSORS] Active = %u / %u\n",
                (unsigned)activeSensors.size(), (unsigned)sensors.size());
}

const uint8_t RS485Transceiver::NOW_PEER[6] = { 0x98, 0xA3, 0x16, 0xF7, 0x5F, 0xF4 };
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

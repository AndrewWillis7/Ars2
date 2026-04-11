#include <Arduino.h>

#include <AppController.h>

static AppController app;

void setup() {
  app.setup();
}

void loop() {
  app.loop();
}

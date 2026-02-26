#pragma once
#include <Arduino.h>
#include <cstring>

extern "C" {
  #include <esp_now.h>
  #include <esp_wifi.h>
  #include <esp_netif.h>
  #include <esp_event.h>
  #include <nvs_flash.h>
}

namespace wifi_forward {

static constexpr size_t MAX_PAYLOAD = 240;

static bool g_up = false;
static bool g_wifi_ready = false;
static uint8_t g_peer[6] = {0};

static void printErr(const char* what, esp_err_t err) {
  Serial.printf("[ESPNOW] %s failed: %d (0x%04X)\n", what, (int)err, (unsigned)err);
}

inline bool ensureWifiReady() {
  if (g_wifi_ready) return true;

  // 1) NVS (required for WiFi)
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // Fix-up for corrupted/old NVS partitions
    err = nvs_flash_erase();
    if (err != ESP_OK) { printErr("nvs_flash_erase", err); return false; }
    err = nvs_flash_init();
  }
  if (err != ESP_OK) { printErr("nvs_flash_init", err); return false; }

  // 2) Netif + default event loop
  err = esp_netif_init();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printErr("esp_netif_init", err);
    return false;
  }

  err = esp_event_loop_create_default();
  if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
    printErr("event_loop_create_default", err);
    return false;
  }

  // 3) Default STA netif (safe even if created already; may return nullptr)
  esp_netif_t* sta = esp_netif_create_default_wifi_sta();
  (void)sta;

  // 4) WiFi driver init
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  err = esp_wifi_init(&cfg);
  if (err != ESP_OK && err != ESP_ERR_WIFI_INIT_STATE) {
    printErr("esp_wifi_init", err);
    return false;
  }

  // 5) Put WiFi in STA mode + start
  err = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  if (err != ESP_OK) { printErr("esp_wifi_set_storage", err); return false; }

  err = esp_wifi_set_mode(WIFI_MODE_STA);
  if (err != ESP_OK) { printErr("esp_wifi_set_mode", err); return false; }

  err = esp_wifi_start();
  if (err != ESP_OK && err != ESP_ERR_WIFI_NOT_INIT) {
    // If it says NOT_INIT here, something above didn't really init.
    printErr("esp_wifi_start", err);
    return false;
  }

  g_wifi_ready = true;
  Serial.println("[ESPNOW] WiFi driver ready (STA)");
  return true;
}

inline bool begin(const uint8_t peerMac[6]) {
  memcpy(g_peer, peerMac, 6);

  if (!ensureWifiReady()) return false;

  // ESPNOW init
  esp_err_t err = esp_now_init();
  if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
    // Try clean re-init once
    esp_now_deinit();
    err = esp_now_init();
    if (err != ESP_OK && err != ESP_ERR_ESPNOW_EXIST) {
      printErr("esp_now_init", err);
      g_up = false;
      return false;
    }
  }

  // Remove then add peer (idempotent)
  esp_now_del_peer(g_peer);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, g_peer, 6);
  peer.channel = 0;
  peer.encrypt = false;

  err = esp_now_add_peer(&peer);
  if (err != ESP_OK) {
    printErr("esp_now_add_peer", err);
    esp_now_deinit();
    g_up = false;
    return false;
  }

  g_up = true;
  Serial.printf("[ESPNOW] up -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                g_peer[0], g_peer[1], g_peer[2], g_peer[3], g_peer[4], g_peer[5]);
  return true;
}

inline void end() {
  if (!g_up) return;
  esp_now_deinit();
  g_up = false;
  Serial.println("[ESPNOW] down");
}

inline bool isUp() { return g_up; }

inline void sendBytes(const uint8_t* data, size_t len) {
  if (!g_up || !data || len == 0) return;
  if (len > MAX_PAYLOAD) len = MAX_PAYLOAD;
  esp_now_send(g_peer, data, (int)len);
}

} // namespace wifi_forward
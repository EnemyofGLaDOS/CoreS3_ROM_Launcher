#include <Arduino.h>
#include <esp_wifi.h>
#include <esp_now.h>

// ── Packet layout ─────────────────────────────────────────────────────────────
// Matches StickS3 exactly: uint32_t, active LOW
//   bit 0 = up,  bit 1 = down,  bit 2 = left,  bit 3 = right
//   bit 4 = select,  bit 5 = start,  bit 6 = A,  bit 7 = B
// Upper bytes unused — reserved for future expansion
// ─────────────────────────────────────────────────────────────────────────────

// All buttons released (active LOW — all bits HIGH)
#define INPUT_NEUTRAL 0xFFFFFFFF

// If no packet arrives within this many ms, treat controller as disconnected
// and hold neutral so Mario doesn't run off a cliff
#define CONTROLLER_TIMEOUT_MS 500

static volatile uint32_t espnow_input      = INPUT_NEUTRAL;
static volatile uint32_t last_recv_time_ms = 0;
static volatile bool     controller_connected = false;

// Guard against double-init: esp_wifi_init() crashes if called twice
static bool controller_initialised = false;

static void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len == sizeof(uint32_t)) {
        espnow_input      = *(uint32_t *)data;
        last_recv_time_ms = (uint32_t)millis();
        if (!controller_connected) {
            controller_connected = true;
            Serial.println("controller_recv: StickS3 connected!");
        }
    }
}

extern "C" void controller_init()
{
    // Safe to call multiple times — osd_init() will call this again internally.
    // esp_wifi_init() hard-crashes on a second call, so we guard with a flag.
    if (controller_initialised) {
        Serial.println("controller_init: already initialised, skipping");
        return;
    }
    controller_initialised = true;

    Serial.println("controller_init: starting ESP-NOW receiver");

    // WiFi must be initialised before ESP-NOW.
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);  // must match StickS3

    if (esp_now_init() != ESP_OK) {
        Serial.println("controller_init: ESP-NOW init FAILED");
        return;
    }

    esp_now_register_recv_cb(onDataRecv);
    Serial.println("controller_init: ESP-NOW receiver ready — waiting for StickS3");
}

extern "C" uint32_t controller_read_input()
{
    // Timeout guard — if StickS3 goes silent, release all buttons
    uint32_t now = (uint32_t)millis();
    if (controller_connected && (now - last_recv_time_ms) > CONTROLLER_TIMEOUT_MS) {
        controller_connected = false;
        espnow_input = INPUT_NEUTRAL;
        Serial.println("controller_recv: StickS3 timed out — holding neutral");
    }

    return espnow_input;
}

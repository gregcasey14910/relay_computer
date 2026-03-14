/*******************************************************************************
 * GENERIC REGISTER MODULE - Configurable for A, B, C, or D
 *
 * CONFIGURATION: Set which register this module implements
 * Change the #define below to create Reg_A, Reg_B, Reg_C, or Reg_D
 ******************************************************************************/

// =====================================================================================================
// CONFIGURATION - CHANGE THIS TO SELECT WHICH REGISTER
// =====================================================================================================
#define MY_REGISTER 'C'    // Valid values: 'A', 'B', 'C', 'D'

// =====================================================================================================
// AUTO-CONFIGURATION - Do not modify below this line
// =====================================================================================================
#if MY_REGISTER == 'A'
  #define MY_DEVICE_IDX    IDX_REG_A
  #define MY_SELECT_SIGNAL RSA
  #define MY_LOAD_SIGNAL   RLA
  #define MY_REG_NAME      "A"
#elif MY_REGISTER == 'B'
  #define MY_DEVICE_IDX    IDX_REG_B
  #define MY_SELECT_SIGNAL RSB
  #define MY_LOAD_SIGNAL   RLB
  #define MY_REG_NAME      "B"
#elif MY_REGISTER == 'C'
  #define MY_DEVICE_IDX    IDX_REG_C
  #define MY_SELECT_SIGNAL RSC
  #define MY_LOAD_SIGNAL   RLC
  #define MY_REG_NAME      "C"
#elif MY_REGISTER == 'D'
  #define MY_DEVICE_IDX    IDX_REG_D
  #define MY_SELECT_SIGNAL RSD
  #define MY_LOAD_SIGNAL   RLD
  #define MY_REG_NAME      "D"
#else
  #error "MY_REGISTER must be 'A', 'B', 'C', or 'D'"
#endif

/*******************************************************************************
 * Working version with SSD1306 display
 * Uses ESPNOW_Common.h for centralized MAC address management
 * Updated for P2P ACK scheme with dynamic peer registration
 *
 * Hardware:
 *   ESP32-C3-SuperMini
 *   SSD1306 OLED Display (128x64, I2C address 0x3C)
 *     - SDA: GPIO0
 *     - SCL: GPIO1
 *
 * Display Format:
 *   REG X           [--]   (header + control indicator)
 *    0x42                  (BIG register value)
 *   DBUS: 0x42             (data bus value)
 *
 * Date: February 7, 2026
 ******************************************************************************/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../common/P2P_Signals.h"
#include "../common/mac_globals.cpp"  // MAC addresses only - all modules need this

// =====================================================================================================
// DISPLAY CONFIGURATION
// =====================================================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================================================================================================
// STATE TRACKING
// =====================================================================================================
uint8_t registerValue = 0x00;          // Register value
bool reg_initialized = false;          // Has register received first value?
uint8_t last_dbus = 0x00;             // Last DBUS value

// Control indicator timing
#define CONTROL_DISPLAY_MS 500         // Show control indicator for 500ms
unsigned long select_time = 0;         // Last SELECT timestamp
unsigned long load_time = 0;           // Last LOAD timestamp
unsigned long last_display_update = 0;
volatile bool display_dirty = false;   // Set by callbacks, cleared by loop()

// Statistics
unsigned long select_count = 0;
unsigned long load_count = 0;

// =====================================================================================================
// FUNCTION: Update Display
// =====================================================================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("REG ");
  display.print(MY_REG_NAME);

  display.setCursor(104, 0);

  bool select_active = (millis() - select_time) < CONTROL_DISPLAY_MS;
  bool load_active = (millis() - load_time) < CONTROL_DISPLAY_MS;

  if (select_active && load_active) {
    display.print("[R2]");
  } else if (select_active) {
    display.print("[RS]");
  } else if (load_active) {
    display.print("[RL]");
  } else {
    display.print("[--]");
  }

  display.setTextSize(4);
  display.setCursor(0, 18);

  if (!reg_initialized) {
    display.print(" 0xXX");
  } else {
    display.print(" 0x");
    if (registerValue < 0x10) display.print("0");
    display.print(registerValue, HEX);
  }

  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("DBUS: ");
  if (!reg_initialized) {
    display.print("--");
  } else {
    display.print("0x");
    if (last_dbus < 0x10) display.print("0");
    display.print(last_dbus, HEX);
  }

  display.display();
}

// =====================================================================================================
// FUNCTION: Handle SELECT (RS)
// =====================================================================================================
void handleSelect(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.print("*** RS");
  Serial.print(MY_REG_NAME);
  Serial.println(" - Outputting register to DBUS");
  last_dbus = registerValue;
  select_time = millis();
  select_count++;
  display_dirty = true;

  if (!esp_now_is_peer_exist(sender_mac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, sender_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("  ✗ Failed to add sender as peer");
      return;
    }
    Serial.println("  ✓ Added sender as peer dynamically");
  }

  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = MY_SELECT_SIGNAL;
  response.value = registerValue;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;

  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  if (result == ESP_OK) {
    Serial.print("  → Response sent, value=0x");
    Serial.println(registerValue, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// FUNCTION: Handle LOAD (RL)
// =====================================================================================================
void handleLoad(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.print("*** RL");
  Serial.print(MY_REG_NAME);
  Serial.print(" - Loading: 0x");
  Serial.println(request->value, HEX);

  registerValue = request->value;
  last_dbus = request->value;
  reg_initialized = true;
  load_time = millis();
  load_count++;
  display_dirty = true;

  if (!esp_now_is_peer_exist(sender_mac)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, sender_mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.println("  ✗ Failed to add sender as peer");
      return;
    }
    Serial.println("  ✓ Added sender as peer dynamically");
  }

  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = MY_LOAD_SIGNAL;
  response.value = registerValue;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;

  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  if (result == ESP_OK) {
    Serial.print("  → Response sent, value=0x");
    Serial.println(registerValue, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// ESP-NOW: Data Received Callback
// =====================================================================================================
void onDataRecv(const uint8_t *sender_mac, const uint8_t *data, int len) {
  if (len != sizeof(P2PMessage)) return;

  P2PMessage *msg = (P2PMessage *)data;

  if (msg->msg_type == MSG_BROADCAST) return;
  if (msg->dst_idx != 0xFF && msg->dst_idx != MY_DEVICE_IDX) return;

  const char* signal_name = "UNKNOWN";
  if (msg->signal == MY_SELECT_SIGNAL) signal_name = "SELECT";
  else if (msg->signal == MY_LOAD_SIGNAL) signal_name = "LOAD";

  Serial.print("Received: signal=");
  Serial.print(signal_name);
  Serial.print(", value=0x");
  Serial.println(msg->value, HEX);

  switch (msg->signal) {
    case MY_SELECT_SIGNAL: handleSelect(msg, sender_mac); break;
    case MY_LOAD_SIGNAL:   handleLoad(msg, sender_mac);   break;
    default: break;
  }
}

// =====================================================================================================
// ESP-NOW: Data Sent Callback
// =====================================================================================================
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("✗ Response send failed in callback");
  }
}

// =====================================================================================================
// SETUP
// =====================================================================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n========================================");
  Serial.print("REG_");
  Serial.print(MY_REG_NAME);
  Serial.println(" INITIALIZATION");
  Serial.println("========================================");

  Wire.begin(0, 1);
  delay(100);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✗ SSD1306 allocation failed");
    while(1);
  }
  Serial.println("✓ SSD1306 display initialized");
  updateDisplay();

  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.println("\n========== NETWORK SETUP ==========");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("===================================\n");

  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW init failed");
    return;
  }
  Serial.println("✓ ESP-NOW initialized");

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, device_MACs[IDX_MASTER], 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ Failed to add Master as peer");
  } else {
    Serial.print("✓ Master registered: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", device_MACs[IDX_MASTER][i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  Serial.print("✓ Reg_");
  Serial.print(MY_REG_NAME);
  Serial.println(" ready");
  Serial.println("========================================\n");
}

// =====================================================================================================
// LOOP
// =====================================================================================================
void loop() {
  bool time_to_refresh = (millis() - last_display_update >= 100);
  if (display_dirty || time_to_refresh) {
    display_dirty = false;
    updateDisplay();
    last_display_update = millis();
  }
  delay(10);
}

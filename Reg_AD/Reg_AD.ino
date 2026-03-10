/*******************************************************************************
 * REG_AD MODULE - Handles BOTH Register A and Register D
 * 
 * Responds to 4 signals:
 *   RSA (55) - Select Register A → output regA on DBUS
 *   RSD (52) - Select Register D → output regD on DBUS
 *   RLA (59) - Load Register A from request value
 *   RLD (56) - Load Register D from request value
 * 
 * Hardware:
 *   ESP32-C3-SuperMini
 *   SSD1306 OLED Display (128x64, I2C address 0x3C)
 *     - SDA: GPIO0
 *     - SCL: GPIO1
 * 
 * Display Format (3 lines):
 *   Line 1: REG A  [--]     REG D [--]   (headers + status indicators)
 *   Line 2: 0xXX  0xYY                   (RegA and RegD values in HEX)
 *   Line 3: DBUS: 0xZZ                   (last DBUS value)
 * 
 * Status Indicators:
 *   [--] = Idle
 *   [S-] = Select active (RS asserted)
 *   [-L] = Load active (RL asserted)
 *   [SL] = Both active
 * 
 * Date: February 9, 2026
 ******************************************************************************/

#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "../common/P2P_Signals.h"
#include "../common/mac_globals.cpp"  // MAC addresses

// =====================================================================================================
// CONFIGURATION
// =====================================================================================================
#define MY_DEVICE_IDX  IDX_REG_A  // Register as Reg_A in the network

// =====================================================================================================
// DISPLAY CONFIGURATION
// =====================================================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================================================================================================
// REGISTER STATE
// =====================================================================================================
uint8_t regA_value = 0x00;        // Register A value
uint8_t regD_value = 0x00;        // Register D value
bool regA_initialized = false;    // Has A received first value?
bool regD_initialized = false;    // Has D received first value?
uint8_t last_dbus = 0x00;        // Last DBUS value

// Control indicator timing
#define CONTROL_DISPLAY_MS 500    // Show control indicator for 500ms
unsigned long regA_select_time = 0;
unsigned long regA_load_time = 0;
unsigned long regD_select_time = 0;
unsigned long regD_load_time = 0;
unsigned long last_display_update = 0;

// Statistics
unsigned long regA_select_count = 0;
unsigned long regA_load_count = 0;
unsigned long regD_select_count = 0;
unsigned long regD_load_count = 0;

// =====================================================================================================
// FUNCTION: Update Display
// =====================================================================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Line 1: Headers + status indicators (small font)
  display.setTextSize(1);
  
  // Register A header and status (left side)
  display.setCursor(0, 0);
  display.print("REG A ");  // One space before status
  
  // Register A status indicator
  bool regA_sel = (millis() - regA_select_time) < CONTROL_DISPLAY_MS;
  bool regA_ld = (millis() - regA_load_time) < CONTROL_DISPLAY_MS;
  
  if (regA_sel && regA_ld) {
    display.print("[SL]");
  } else if (regA_sel) {
    display.print("[S-]");
  } else if (regA_ld) {
    display.print("[-L]");
  } else {
    display.print("[--]");
  }
  
  // Register D header and status (right justified)
  // "REG D [--]" = 11 characters at font size 1 (6 pixels wide) = 66 pixels
  // Screen width = 128 pixels, so start at 128 - 66 = 62
  display.setCursor(62, 0);
  display.print("REG D ");  // One space before status
  
  bool regD_sel = (millis() - regD_select_time) < CONTROL_DISPLAY_MS;
  bool regD_ld = (millis() - regD_load_time) < CONTROL_DISPLAY_MS;
  
  if (regD_sel && regD_ld) {
    display.print("[SL]");
  } else if (regD_sel) {
    display.print("[S-]");
  } else if (regD_ld) {
    display.print("[-L]");
  } else {
    display.print("[--]");
  }
  
  // Line 2: Register values (BIGGER font - size 3)
  display.setTextSize(3);
  display.setCursor(0, 16);
  
  // Register A value
  if (!regA_initialized) {
    display.print("x??");
  } else {
    display.print("x");
    if (regA_value < 0x10) display.print("0");
    display.print(regA_value, HEX);
  }
  
  // Register D value
  display.setCursor(68, 16);
  if (!regD_initialized) {
    display.print("x??");
  } else {
    display.print("x");
    if (regD_value < 0x10) display.print("0");
    display.print(regD_value, HEX);
  }
  
  // Line 3: DBUS value (bottom, small font)
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("DBUS: ");
  if (!regA_initialized && !regD_initialized) {
    display.print("--");
  } else {
    display.print("0x");
    if (last_dbus < 0x10) display.print("0");
    display.print(last_dbus, HEX);
  }
  
  display.display();
}

// =====================================================================================================
// FUNCTION: Handle SELECT Register A (RSA)
// =====================================================================================================
void handleSelectA(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.println("*** RSA - Outputting Register A to DBUS");
  last_dbus = regA_value;
  regA_select_time = millis();
  regA_select_count++;
  updateDisplay();
  
  // Check if sender is registered as peer, add if not
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
  
  // Send response with Register A value
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = RSA;
  response.value = regA_value;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;
  
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent, RegA=0x");
    Serial.println(regA_value, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// FUNCTION: Handle LOAD Register A (RLA)
// =====================================================================================================
void handleLoadA(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.print("*** RLA - Loading Register A: 0x");
  Serial.println(request->value, HEX);
  
  regA_value = request->value;
  last_dbus = request->value;
  regA_initialized = true;
  regA_load_time = millis();
  regA_load_count++;
  updateDisplay();
  
  // Check if sender is registered as peer, add if not
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
  
  // Send response with loaded value
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = RLA;
  response.value = regA_value;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;
  
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent, RegA=0x");
    Serial.println(regA_value, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// FUNCTION: Handle SELECT Register D (RSD)
// =====================================================================================================
void handleSelectD(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.println("*** RSD - Outputting Register D to DBUS");
  last_dbus = regD_value;
  regD_select_time = millis();
  regD_select_count++;
  updateDisplay();
  
  // Check if sender is registered as peer, add if not
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
  
  // Send response with Register D value
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = RSD;
  response.value = regD_value;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;
  
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent, RegD=0x");
    Serial.println(regD_value, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// FUNCTION: Handle LOAD Register D (RLD)
// =====================================================================================================
void handleLoadD(P2PMessage *request, const uint8_t *sender_mac) {
  Serial.print("*** RLD - Loading Register D: 0x");
  Serial.println(request->value, HEX);
  
  regD_value = request->value;
  last_dbus = request->value;
  regD_initialized = true;
  regD_load_time = millis();
  regD_load_count++;
  updateDisplay();
  
  // Check if sender is registered as peer, add if not
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
  
  // Send response with loaded value
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = RLD;
  response.value = regD_value;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;
  
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent, RegD=0x");
    Serial.println(regD_value, HEX);
  } else {
    Serial.print("  ✗ Response send failed: ");
    Serial.println(result);
  }
}

// =====================================================================================================
// ESP-NOW: Data Received Callback
// =====================================================================================================
void onDataRecv(const esp_now_recv_info *recv_info, const uint8_t *data, int len) {
  if (len != sizeof(P2PMessage)) {
    Serial.print("✗ Invalid message size: ");
    Serial.print(len);
    Serial.print(" bytes (expected ");
    Serial.print(sizeof(P2PMessage));
    Serial.println(")");
    return;
  }
  
  P2PMessage *msg = (P2PMessage *)data;
  
  // Ignore broadcasts - for CYD Monitor only
  if (msg->msg_type == MSG_BROADCAST) {
    return;
  }
  
  // Only respond to messages addressed to us
  if (msg->dst_idx != 0xFF && msg->dst_idx != MY_DEVICE_IDX) {
    return;
  }
  
  Serial.print("Received: type=");
  Serial.print(msg->msg_type);
  Serial.print(", signal=");
  Serial.print(getSignalName(msg->signal));
  Serial.print("(");
  Serial.print(msg->signal);
  Serial.print("), value=0x");
  Serial.print(msg->value, HEX);
  Serial.print(", seq=");
  Serial.println(msg->seq_num);
  
  // Handle the request
  switch (msg->signal) {
    case RSA:
      handleSelectA(msg, recv_info->src_addr);
      break;
      
    case RLA:
      handleLoadA(msg, recv_info->src_addr);
      break;
      
    case RSD:
      handleSelectD(msg, recv_info->src_addr);
      break;
      
    case RLD:
      handleLoadD(msg, recv_info->src_addr);
      break;
      
    default:
      // Ignore other signals (not for this module)
      break;
  }
}

// =====================================================================================================
// ESP-NOW: Data Sent Callback
// =====================================================================================================
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
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
  Serial.println("REG_AD MODULE INITIALIZATION");
  Serial.println("(Handles both Register A and Register D)");
  Serial.println("========================================");
  
  // Initialize I2C for display
  Wire.begin(0, 1);  // SDA=GPIO0, SCL=GPIO1
  delay(100);
  
  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✗ SSD1306 allocation failed");
    while(1);
  }
  Serial.println("✓ SSD1306 display initialized");
  
  // Show initial display
  updateDisplay();
  
  // Initialize WiFi for ESP-NOW
  WiFi.mode(WIFI_STA);
  delay(100);
  
  Serial.println("\n========== NETWORK SETUP ==========");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("===================================\n");
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW init failed");
    return;
  }
  Serial.println("✓ ESP-NOW initialized");
  
  // Register Master as peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, device_MACs[IDX_MASTER], 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("✗ Failed to add Master as peer");
  } else {
    Serial.print("✓ Master registered as peer: ");
    for (int i = 0; i < 6; i++) {
      Serial.printf("%02X", device_MACs[IDX_MASTER][i]);
      if (i < 5) Serial.print(":");
    }
    Serial.println();
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  
  Serial.println("✓ Reg_AD ready (handles A and D)");
  Serial.println("========================================\n");
}

// =====================================================================================================
// LOOP
// =====================================================================================================
void loop() {
  // Refresh display every 100ms to update control indicators
  if (millis() - last_display_update >= 100) {
    updateDisplay();
    last_display_update = millis();
  }
  
  delay(10);
}

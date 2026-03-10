/*******************************************************************************
 * GENERIC REGISTER MODULE - Configurable for A, B, C, or D
 * 
 * CONFIGURATION: Set which register this module implements
 * Change the #define below to create Reg_A, Reg_B, Reg_C, or Reg_D
 ******************************************************************************/

// =====================================================================================================
// CONFIGURATION - CHANGE THIS TO SELECT WHICH REGISTER
// =====================================================================================================
#define MY_REGISTER 'A'    // Valid values: 'A', 'B', 'C', 'D'

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

// Statistics
unsigned long select_count = 0;
unsigned long load_count = 0;

// =====================================================================================================
// FUNCTION: Update Display
// =====================================================================================================
void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Line 1: Header + control indicator (small font)
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("REG ");
  display.print(MY_REG_NAME);  // Uses configured register name
  
  // Control line indicator on same line, far right
  display.setCursor(104, 0);
  
  // Check which controls are active (within 500ms)
  bool select_active = (millis() - select_time) < CONTROL_DISPLAY_MS;
  bool load_active = (millis() - load_time) < CONTROL_DISPLAY_MS;
  
  if (select_active && load_active) {
    display.print("[R2]");  // BOTH active
  } else if (select_active) {
    display.print("[RS]");  // Only SELECT active
  } else if (load_active) {
    display.print("[RL]");  // Only LOAD active
  } else {
    display.print("[--]");  // Neither active
  }
  
  // Line 2: Big register value
  display.setTextSize(4);
  display.setCursor(0, 18);
  
  if (!reg_initialized) {
    display.print(" 0xXX");  // Not initialized yet
  } else {
    display.print(" 0x");
    if (registerValue < 0x10) display.print("0");  // Leading zero
    display.print(registerValue, HEX);
  }
  
  // Line 3: DBUS value (bottom)
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
  select_time = millis();  // Mark when SELECT occurred
  select_count++;
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
  
  // Send response directly back to sender
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = MY_SELECT_SIGNAL;  // Echo back the SELECT signal (RSA/RSB/RSC/RSD)
  response.value = registerValue;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;  // Reply to sender
  
  // Send directly to sender's MAC address
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent to sender, value=0x");
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
  Serial.print(" - Loading value into ");
  Serial.print(MY_REG_NAME);
  Serial.print(": 0x");
  Serial.println(request->value, HEX);
  
  registerValue = request->value;
  last_dbus = request->value;
  reg_initialized = true;  // Mark as initialized on first load
  load_time = millis();    // Mark when LOAD occurred
  load_count++;
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
  
  // Send response directly back to sender
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = request->seq_num;
  response.signal = MY_LOAD_SIGNAL;  // Echo back the LOAD signal (RLA/RLB/RLC/RLD)
  response.value = registerValue;
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = request->src_idx;  // Reply to sender
  
  // Send directly to sender's MAC address
  esp_err_t result = esp_now_send(sender_mac, (uint8_t *) &response, sizeof(response));
  
  if (result == ESP_OK) {
    Serial.print("  → Response sent to sender, value=0x");
    Serial.println(registerValue, HEX);
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
  
  // Ignore broadcasts (type=3) - these are for CYD Monitor only
  if (msg->msg_type == MSG_BROADCAST) {
    return;  // Silent ignore
  }
  
  // Only respond to messages addressed to us
  if (msg->dst_idx != 0xFF && msg->dst_idx != MY_DEVICE_IDX) {
    return;  // Not for us, ignore
  }
  
  // Identify signal for debug
  const char* signal_name = "UNKNOWN";
  if (msg->signal == MY_SELECT_SIGNAL) signal_name = "SELECT";
  else if (msg->signal == MY_LOAD_SIGNAL) signal_name = "LOAD";
  
  Serial.print("Received: type=");
  Serial.print(msg->msg_type);
  Serial.print(", signal=");
  Serial.print(signal_name);
  Serial.print("(0x");
  Serial.print(msg->signal, HEX);
  Serial.print("), value=0x");
  Serial.print(msg->value, HEX);
  Serial.print(", seq=");
  Serial.println(msg->seq_num);
  
  // Handle the request - pass sender's MAC address
  switch (msg->signal) {
    case MY_SELECT_SIGNAL:
      handleSelect(msg, recv_info->src_addr);
      break;
      
    case MY_LOAD_SIGNAL:
      handleLoad(msg, recv_info->src_addr);
      break;
      
    default:
      // Ignore other signals (not for this register)
      break;
  }
}

// =====================================================================================================
// ESP-NOW: Data Sent Callback
// =====================================================================================================
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  // Now showing send status for debugging
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
  
  // Initialize I2C for display
  Wire.begin(0, 1);  // SDA=GPIO0, SCL=GPIO1
  delay(100);
  
  // Initialize display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("✗ SSD1306 allocation failed");
    while(1);  // Halt if display fails
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
  
  // Register Master as peer so we can send responses back
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, device_MACs[IDX_MASTER], 6);  // Get Master MAC from common header
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
  
  Serial.print("✓ Reg_");
  Serial.print(MY_REG_NAME);
  Serial.println(" ready");
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

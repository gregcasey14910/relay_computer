// espnow_comms.cpp - ESP-NOW Communication Implementation

#include "espnow_comms.h"
#include "definitions.h"

// =====================================================================================================
// INITIALIZATION
// =====================================================================================================
void initESPNow() {
  WiFi.mode(WIFI_STA);
  delay(100);
  Serial.println("\n=========================");
  Serial.print("MASTER MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Role: MASTER (Peer-to-Peer)");
  Serial.println("=========================");
    
  // Initialize ESP-NOW with Peer-to-Peer registration
  if (esp_now_init() != ESP_OK) {
      Serial.println("✗ ESP-NOW init failed");
      return;
  }
  Serial.println("✓ ESP-NOW initialized");
  
  // Register callbacks
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);
  
  // Register peer devices for peer-to-peer communication
  Serial.println("\nRegistering peers...");
  
  // Register Reg_A (peer-to-peer)
  esp_now_peer_info_t regA_peer;
  memset(&regA_peer, 0, sizeof(regA_peer));
  memcpy(regA_peer.peer_addr, regA_MAC, 6);
  regA_peer.channel = 0;
  regA_peer.encrypt = false;
  if (esp_now_add_peer(&regA_peer) == ESP_OK) {
    Serial.print("✓ Reg_A registered: ");
    printMAC(regA_MAC);
    Serial.println();
  }
  
// Register Reg_B (peer-to-peer) - Reg_BC module handles both B and C
  esp_now_peer_info_t regB_peer;
  memset(&regB_peer, 0, sizeof(regB_peer));
  memcpy(regB_peer.peer_addr, regB_MAC, 6);
  regB_peer.channel = 0;
  regB_peer.encrypt = false;
  if (esp_now_add_peer(&regB_peer) == ESP_OK) {
    Serial.print("✓ Reg_B registered: ");
    printMAC(regB_MAC);
    Serial.println();
  }
  
  // Register Reg_C (peer-to-peer) - Same MAC as Reg_B (same physical module)
  // Note: regC_MAC points to same address as regB_MAC, but we register both indices
  esp_now_peer_info_t regC_peer;
  memset(&regC_peer, 0, sizeof(regC_peer));
  memcpy(regC_peer.peer_addr, regC_MAC, 6);
  regC_peer.channel = 0;
  regC_peer.encrypt = false;
  if (esp_now_add_peer(&regC_peer) == ESP_OK) {
    Serial.print("✓ Reg_C registered: ");
    printMAC(regC_MAC);
    Serial.println();
  }


  // Register Reg_D (peer-to-peer)
  esp_now_peer_info_t regD_peer;
  memset(&regD_peer, 0, sizeof(regD_peer));
  memcpy(regD_peer.peer_addr, regD_MAC, 6);
  regD_peer.channel = 0;
  regD_peer.encrypt = false;
  if (esp_now_add_peer(&regD_peer) == ESP_OK) {
    Serial.print("✓ Reg_D registered: ");
    printMAC(regD_MAC);
    Serial.println();
  }

  // Register ALU (peer-to-peer)
  esp_now_peer_info_t alu_peer;
  memset(&alu_peer, 0, sizeof(alu_peer));
  memcpy(alu_peer.peer_addr, alu_MAC, 6);
  alu_peer.channel = 0;
  alu_peer.encrypt = false;
  if (esp_now_add_peer(&alu_peer) == ESP_OK) {
    Serial.print("✓ ALU registered: ");
    printMAC(alu_MAC);
    Serial.println();
  }
  
  // Register CYD Monitor (broadcast for monitoring)
  esp_now_peer_info_t cyd_peer;
  memset(&cyd_peer, 0, sizeof(cyd_peer));
  memcpy(cyd_peer.peer_addr, cyd_MAC, 6);
  cyd_peer.channel = 0;
  cyd_peer.encrypt = false;
  if (esp_now_add_peer(&cyd_peer) == ESP_OK) {
    Serial.print("✓ CYD Monitor registered: ");
    printMAC(cyd_MAC);
    Serial.println();
  }
  
  // Register broadcast address
  esp_now_peer_info_t broadcast_peer;
  memset(&broadcast_peer, 0, sizeof(broadcast_peer));
  memcpy(broadcast_peer.peer_addr, broadcast_MAC, 6);
  broadcast_peer.channel = 0;
  broadcast_peer.encrypt = false;
  if (esp_now_add_peer(&broadcast_peer) == ESP_OK) {
    Serial.println("✓ Broadcast registered");
  }
  
  Serial.println("✓ ESP-NOW Peer-to-Peer ready!\n");
}

// =====================================================================================================
// SEND CALLBACK
// =====================================================================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
#else
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
  // Optionally log send status for debugging
}

// =====================================================================================================
// RECEIVE CALLBACK
// =====================================================================================================
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
#else
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
#endif
  if (len != sizeof(P2PMessage)) {
    Serial.print("⚠ Wrong message size: ");
    Serial.print(len);
    Serial.print(" expected ");
    Serial.println(sizeof(P2PMessage));
    return;
  }
  
  P2PMessage msg;
  memcpy(&msg, data, sizeof(msg));
  
  // Handle RESPONSE messages (ACKs from Reg_A or ALU)
  if (msg.msg_type == MSG_RESPONSE && msg.seq_num == expected_seq) {
    memcpy((void*)&pending_response, &msg, sizeof(P2PMessage));
    response_received = true;   
    
    // Extract data if this was a SELECT operation
    if (msg.signal == RSA || msg.signal == RSD || msg.signal == RSB || msg.signal == RSC) {
      varValues[Da_B] = msg.value;  // Put on data bus
      
      // Update the register variable so display shows it!
      if (msg.signal == RSA) varValues[A_Rg] = msg.value;
      if (msg.signal == RSD) varValues[D_Rg] = msg.value;
      if (msg.signal == RSB) varValues[B_Rg] = msg.value;
      if (msg.signal == RSC) varValues[C_Rg] = msg.value;
    }
    
    // ALSO update register on LOAD responses
    if (msg.signal == RLA) varValues[A_Rg] = msg.value;
    if (msg.signal == RLD) varValues[D_Rg] = msg.value;
    if (msg.signal == RLB) varValues[B_Rg] = msg.value;
    if (msg.signal == RLC) varValues[C_Rg] = msg.value;
  }
  
  // Handle BROADCAST messages (from ALU sending results, or from other modules for monitoring)
  if (msg.msg_type == MSG_BROADCAST) {
    if (msg.signal == ALU_OUT || msg.signal == DBUS) {
      varValues[Da_B] = msg.value;
      Serial.print("📡 Broadcast: ");
      Serial.print(getSignalName(msg.signal));
      Serial.print(" = 0x");
      Serial.println(msg.value, HEX);
    }
  }
}

// =====================================================================================================
// PEER-TO-PEER REQUEST FUNCTION
// =====================================================================================================
bool sendRequest(uint8_t* peer_mac, uint8_t signal, uint8_t value, uint16_t timeout_ms) {
  P2PMessage req;
  req.msg_type = MSG_REQUEST;
  req.seq_num = next_seq_num++;
  req.signal = signal;
  req.value = value;
  req.data16 = 0;
  req.src_idx = MY_DEVICE_IDX;
  req.dst_idx = getMACIndex(peer_mac);
  
  // Reset response tracking
  response_received = false;
  expected_seq = req.seq_num;
  
  // Send request
  esp_err_t result = esp_now_send(peer_mac, (uint8_t*)&req, sizeof(req));
  if (result != ESP_OK) {
    Serial.print("✗ Send failed to ");
    printMAC(peer_mac);
    Serial.println();
    return false;
  }
  
  // Wait for response with timeout
  unsigned long start = millis();
  while (!response_received && (millis() - start < timeout_ms)) {
    delay(1);
    yield();  // Allow WiFi stack to process incoming response
  }
  
  if (response_received) {
    return true;
  }
  
  // Timeout
  Serial.print("✗ TIMEOUT waiting for ");
  Serial.print(getDeviceName(req.dst_idx));
  Serial.print(" signal=");
  Serial.println(getSignalName(signal));
  return false;
}

// =====================================================================================================
// BROADCAST FOR MONITORING (8-bit values)
// =====================================================================================================
void broadcastForMonitoring(uint8_t signal, uint8_t value) {
  P2PMessage msg;
  msg.msg_type = MSG_BROADCAST;
  msg.seq_num = 0;
  msg.signal = signal;
  msg.value = value;
  msg.data16 = 0;
  msg.src_idx = MY_DEVICE_IDX;
  msg.dst_idx = 0xFF;
  
  esp_now_send(broadcast_MAC, (uint8_t*)&msg, sizeof(msg));
}

// =====================================================================================================
// BROADCAST FOR MONITORING (16-bit values) - NEW
// =====================================================================================================
void broadcastForMonitoring(uint8_t signal, uint16_t value16) {
  P2PMessage msg;
  msg.msg_type = MSG_BROADCAST;
  msg.seq_num = 0;
  msg.signal = signal;
  msg.value = value16 & 0xFF;        // Lower 8 bits in value field
  msg.data16 = value16;              // Full 16 bits in data16 field
  msg.src_idx = MY_DEVICE_IDX;
  msg.dst_idx = 0xFF;
  
  esp_now_send(broadcast_MAC, (uint8_t*)&msg, sizeof(msg));
}

// =====================================================================================================
// LEGACY sendBroadcast - 8-bit version
// =====================================================================================================
void sendBroadcast(uint8_t signal, uint8_t value) {
  broadcastForMonitoring(signal, value);
}

// =====================================================================================================
// LEGACY sendBroadcast - 16-bit version (NEW)
// =====================================================================================================
void sendBroadcast(uint8_t signal, uint16_t value16) {
  broadcastForMonitoring(signal, value16);
}

// =====================================================================================================
// Helper: Only broadcast when signal state changes (for REMOTE devices only)
// =====================================================================================================
void broadcastOnChange(uint8_t signal, uint8_t currentState, uint8_t* prevState) {
  if (currentState != *prevState) {
    sendBroadcast(signal, currentState);
    *prevState = currentState;    
  }
}

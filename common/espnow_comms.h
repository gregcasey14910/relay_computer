// espnow_comms.h - ESP-NOW Peer-to-Peer Communication Functions
// Handles all wireless communication between Master, Reg_A, ALU, and Monitor

#ifndef ESPNOW_COMMS_H
#define ESPNOW_COMMS_H

#include "P2P_Signals.h"  // ADD THIS LINE - defines REQUEST_TIMEOUT_MS
#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

// Forward declarations
void initESPNow();
void broadcastOnChange(uint8_t signal, uint8_t currentState, uint8_t* prevState);
bool sendRequest(uint8_t* peer_mac, uint8_t signal, uint8_t value, uint16_t timeout_ms = REQUEST_TIMEOUT_MS);

// Broadcast for monitoring - 8-bit version
void broadcastForMonitoring(uint8_t signal, uint8_t value);

// Broadcast for monitoring - 16-bit version (NEW)
void broadcastForMonitoring(uint8_t signal, uint16_t value16);

// Legacy broadcast function - 8-bit version
void sendBroadcast(uint8_t signal, uint8_t value);

// Legacy broadcast function - 16-bit version (NEW)
void sendBroadcast(uint8_t signal, uint16_t value16);

// Callbacks
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status);
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len);

#endif // ESPNOW_COMMS_H
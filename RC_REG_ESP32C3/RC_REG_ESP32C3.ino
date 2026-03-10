#include <esp_now.h>
#include <WiFi.h>

int register1 = 0xf0;
int register2 = 0xce;

// LED_BUILTIN is GPIO8 on ESP32-C3 DevKit (defined by board variant)

// Structure to receive two values
typedef struct struct_message {
  int cmd;
  int bus_value;
} struct_message;

// Received values
struct_message receivedData;
struct_message response;

// Callback when data is received
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int len) {
  digitalWrite(LED_BUILTIN, LOW);  // LED on (active LOW)
  if (len == sizeof(struct_message)) {
    memcpy(&receivedData, data, sizeof(struct_message));
    
    // Print struct fields
    // Serial.print("cmd: ");
    // Serial.print(receivedData.cmd);
    // Serial.print(", bus_value: ");
    // Serial.println(receivedData.bus_value, HEX);

    // CREATE RESPONSE
    response.cmd = 200;  // Default response command indicator
    
    // Determine response based on received cmd value
    if (receivedData.cmd == 0) {                          // Ping 
      Serial.println("Ping");
      response.bus_value = 100;                           // Respond with 100
      
    } else if (receivedData.cmd == 1) {                   // Sel Register 1  
      // Serial.print("SELECT Register 1 = ");
      // Serial.println(register1, HEX);
      response.bus_value = register1;                     // reply with Register 1                        
      
    } else if (receivedData.cmd == 2) {                   // Sel Register 2 
      // Serial.print("SELECT Register 2 = ");
      // Serial.println(register2, HEX);
      response.bus_value = register2;                     // reply with Register 2   
      
    } else if (receivedData.cmd == 3) {                   // Sel Register 1_2  
      Serial.println("SELECT both registers");
      response.bus_value = (register1 << 8) + register2;  // reply with Register 1_2
      
    } else if (receivedData.cmd == 11) {                  // Clock Register 1  
      register1 = receivedData.bus_value;
      response.cmd = 200;                                 // ACK code
      response.bus_value = register1;                     // Send back register1 value as confirmation
      
    } else if (receivedData.cmd == 12) {                  // Clock Register 2 
      register2 = receivedData.bus_value;
      response.cmd = 201;                                 // ACK code
      response.bus_value = register2;                     // Send back register2 value as confirmation
      
    } else if (receivedData.cmd == 13) {                  // Clock both registers
      register1 = receivedData.bus_value >> 8;
      register2 = receivedData.bus_value & 0xFF;     
      response.cmd = 202;                                 // ACK code
      response.bus_value = (register1 << 8) + register2;  // Send back combined value as confirmation
      
    } else {
      Serial.println("Invalid command received, no response sent");
      digitalWrite(LED_BUILTIN, HIGH);   // LED off on invalid command
      return;
    }
    
    // Check if sender is already a peer, if not add it
    if (!esp_now_is_peer_exist(mac_addr)) {
      esp_now_peer_info_t peerInfo = {};
      memcpy(peerInfo.peer_addr, mac_addr, 6);
      peerInfo.channel = 0;  
      peerInfo.encrypt = false;
      
      if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        digitalWrite(LED_BUILTIN, HIGH);   // LED off on error
        return;
      }
      Serial.println("Peer added successfully");
    }
    
    // Send response back to sender
    esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&response, sizeof(struct_message));
    // LED will turn off in onDataSent callback after transmission completes
    if (result == ESP_OK) {
      // Serial.println("Response queued for sending");
    } else {
      Serial.println("Error sending response");
      digitalWrite(LED_BUILTIN, HIGH);   // LED off on send error
    }
  }
}

// Callback when data is sent - called AFTER send completes
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  digitalWrite(LED_BUILTIN, HIGH);   // LED off AFTER transmission completes
  Serial.printf("%02X %02X\n", register1, register2);
  // Serial.print("Send ");
  // Serial.print(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
  // Serial.print(" | cmd=");
  // Serial.print(response.cmd);
  // Serial.print(" | value=0x");
  // Serial.println(response.bus_value, HEX);
  // Serial.print("Current registers: Reg1=0x");
  // Serial.print(register1, HEX);
  // Serial.print(", Reg2=0x");
  // Serial.println(register2, HEX);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);  // LED off initially

  Serial.println("Type 1 Register PCB Powered UP");
  WiFi.mode(WIFI_STA);
  delay(500);
  
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    while(1);
  }
  
  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
  
  Serial.println("Waiting for messages...");
  Serial.print("Initial Reg1=0x");
  Serial.print(register1, HEX);
  Serial.print(", Reg2=0x");
  Serial.println(register2, HEX);
}

void loop() {
  delay(10);
}
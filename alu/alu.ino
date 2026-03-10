/*
 * ALU - Peer-to-Peer Responder with SSD1306 Display
 * 
 * Remote ALU Module with 74HCT181 Emulation
 * Handles peer-to-peer requests from Master with acknowledge protocol
 * Displays ALU status on SSD1306 OLED
 * 
 * Hardware:
 * - ESP32-S3 Super Mini
 * - SSD1306 128x64 OLED (I2C: SDA=GPIO8, SCL=GPIO9)
 * - WS2812 NeoPixel on GPIO48
 */

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "../common/P2P_Signals.h"
#include "../common/mac_globals.cpp"  // MAC addresses

// Which device am I?
#define MY_DEVICE_IDX  IDX_ALU

// Hardware pin definitions
#define LED_PIN     48
#define NUM_PIXELS  1
#define SDA_PIN     8
#define SCL_PIN     9
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

// Hardware objects
Adafruit_NeoPixel pixel(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// =====================================================================================================
// ALU STATE
// =====================================================================================================

uint8_t FCT = 0;      // Function code (0-7)
uint8_t B_REG = 0;    // B operand (A input to 74HCT181)
uint8_t C_REG = 0;    // C operand (B input to 74HCT181)
uint8_t RESULT = 0;   // ALU result
uint8_t M_bit = 0;    // 74HCT181 M bit (0=arithmetic, 1=logic)
uint8_t S_bits = 0;   // 74HCT181 S bits (0-15)
bool sign_flag = 0;
bool carry_flag = 0;
bool zero_flag = 0;

// Input tracking
bool have_FCT = false;
bool have_B = false;
bool have_C = false;

// =====================================================================================================
// I2C SCANNER
// =====================================================================================================

void scanI2C() {
  byte error, address;
  int deviceCount = 0;
  
  Serial.println("\n=============================");
  Serial.println("Scanning I2C Bus...");
  Serial.println("=============================");
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      deviceCount++;
    }
    else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }    
  }
  
  if (deviceCount == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.print("\nFound ");
    Serial.print(deviceCount);
    Serial.println(" device(s)\n");
  }
}

void displayI2CMap() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  display.println("I2C Device Scan");
  display.println("---------------");
  
  byte error, address;
  int deviceCount = 0;
  int line = 2;
  
  for(address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      display.setCursor(0, line * 8);
      display.print("0x");
      if (address < 16) display.print("0");
      display.print(address, HEX);
      display.print(" Found");
      deviceCount++;
      line++;
      
      if (line > 7) break;
    }
  }
  
  if (deviceCount == 0) {
    display.setCursor(0, 16);
    display.println("No devices found");
  } else {
    display.setCursor(0, 56);
    display.print("Total: ");
    display.print(deviceCount);
  }
  
  display.display();
}

// =====================================================================================================
// OLED DISPLAY
// =====================================================================================================

void displayALUStatus() {
  display.clearDisplay();
  
  // Draw trapezoid - wider at top (ALU chip shape)
  display.drawLine(21, 10, 107, 10, SSD1306_WHITE);   // Top
  display.drawLine(42, 42, 86, 42, SSD1306_WHITE);    // Bottom
  display.drawLine(21, 10, 42, 42, SSD1306_WHITE);    // Left side
  display.drawLine(107, 10, 86, 42, SSD1306_WHITE);   // Right side
  
  // "ALU" centered in trapezoid
  display.setTextSize(2);
  display.setCursor(52, 20);
  display.print("ALU");
  
  // B_REG above left side
  display.setTextSize(1);
  display.setCursor(17, 2);
  display.print("0x");
  if (B_REG < 16) display.print("0");
  display.print(B_REG, HEX);
  
  // C_REG above right side
  display.setCursor(83, 2);
  display.print("0x");
  if (C_REG < 16) display.print("0");
  display.print(C_REG, HEX);
  
  // ALU Result centered below
  display.setTextSize(1);
  display.setCursor(52, 46);
  display.print("0x");
  if (RESULT < 16) display.print("0");
  display.print(RESULT, HEX);
  
  // LEFT SIDE - Function code
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.print("RC");
  
  display.setCursor(0, 28);
  display.print("F=");
  display.print((FCT & 0x04) ? '1' : '0');
  display.print((FCT & 0x02) ? '1' : '0');
  display.print((FCT & 0x01) ? '1' : '0');
  
  // Function name below F code
  display.setCursor(0, 36);
  const char* fct_names[] = {"CLR", "ADD", "INC", "AND", "ORR", "XOR", "NOT", "SHL"};
  display.print(fct_names[FCT & 0x07]);
  
  // RIGHT SIDE - 74HCT181 codes
  display.setCursor(110, 20);
  display.print("181");
  
  display.setCursor(98, 28);
  display.print("S");
  display.print((S_bits & 0x08) ? '1' : '0');
  display.print((S_bits & 0x04) ? '1' : '0');
  display.print((S_bits & 0x02) ? '1' : '0');
  display.print((S_bits & 0x01) ? '1' : '0');
  
  display.setCursor(110, 36);
  display.print("M=");
  display.print(M_bit);
  
  // Status flags at bottom
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("Sign=");
  display.print(sign_flag ? "1" : "0");
  
  display.setCursor(44, 56);
  display.print("Carry=");
  display.print(carry_flag ? "1" : "0");
  
  display.setCursor(90, 56);
  display.print("Zero=");
  display.print(zero_flag ? "1" : "0");
  
  display.display();
}

// =====================================================================================================
// NEOPIXEL
// =====================================================================================================

void flashRandomColor() {
  // Generate random RGB values (0-255 for each channel)
  uint8_t r = random(0, 256);
  uint8_t g = random(0, 256);
  uint8_t b = random(0, 256);
  
  // Ensure at least some brightness (not pure black)
  if (r < 50 && g < 50 && b < 50) {
    // If all channels too dim, boost one randomly
    switch(random(0, 3)) {
      case 0: r = random(128, 256); break;
      case 1: g = random(128, 256); break;
      case 2: b = random(128, 256); break;
    }
  }
  
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
  delay(100);  // Flash duration
  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
}

// =====================================================================================================
// ESP-NOW SETUP
// =====================================================================================================

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("  ALU - Peer-to-Peer Responder");
  Serial.println("  74HCT181 Emulation");
  Serial.println("  with SSD1306 Display");
  Serial.println("========================================\n");
  
  // Initialize I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  Serial.print("I2C initialized on SDA=GPIO");
  Serial.print(SDA_PIN);
  Serial.print(", SCL=GPIO");
  Serial.println(SCL_PIN);
  
  // Scan I2C bus
  scanI2C();
  
  // Initialize SSD1306 display
  Serial.println("Initializing SSD1306 display...");
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("✗ SSD1306 allocation failed!");
    Serial.println("  Check I2C connections and address");
  } else {
    Serial.print("✓ SSD1306 found at address 0x");
    Serial.println(SCREEN_ADDRESS, HEX);
    
    // Show I2C scan results on display
    displayI2CMap();
    Serial.println("  I2C map displayed on OLED");
    delay(3000);
    
    // Initial ALU display
    displayALUStatus();
    Serial.println("  ALU status display active");
  }
  
  // Initialize NeoPixel
  pixel.begin();
  pixel.setBrightness(50);
  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
  Serial.println("✓ NeoPixel initialized on GPIO 48");
  
  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ Error initializing ESP-NOW");
    pixel.setPixelColor(0, pixel.Color(255, 0, 0));
    pixel.show();
    return;
  }
  
  Serial.println("✓ ESP-NOW initialized");
  
  esp_now_register_recv_cb(onDataRecv);
  
  // Register Master as peer for direct ACKs
  esp_now_peer_info_t master_peer;
  memset(&master_peer, 0, sizeof(master_peer));
  memcpy(master_peer.peer_addr, device_MACs[IDX_MASTER], 6);
  master_peer.channel = 0;
  master_peer.encrypt = false;
  
  if (esp_now_add_peer(&master_peer) == ESP_OK) {
    Serial.print("✓ Master registered: ");
    printMAC(device_MACs[IDX_MASTER]);
    Serial.println();
  } else {
    Serial.println("✗ Failed to register Master");
  }
  
  // Register broadcast address for monitoring messages
  esp_now_peer_info_t broadcast_peer;
  memset(&broadcast_peer, 0, sizeof(broadcast_peer));
  memcpy(broadcast_peer.peer_addr, broadcast_MAC, 6);
  broadcast_peer.channel = 0;
  broadcast_peer.encrypt = false;
  
  if (esp_now_add_peer(&broadcast_peer) == ESP_OK) {
    Serial.println("✓ Broadcast address registered");
  } else {
    Serial.println("✗ Failed to register broadcast");
  }
  
  Serial.println("\n✓ Ready for ALU requests!");
  Serial.println("Waiting for FCT_ALU, B_2_ALU, C_2_ALU...\n");
  
  // Flash green to show ready
  pixel.setPixelColor(0, pixel.Color(0, 255, 0));
  pixel.show();
  delay(500);
  pixel.setPixelColor(0, pixel.Color(0, 0, 0));
  pixel.show();
}

// =====================================================================================================
// 74HCT181 HELPER FUNCTIONS
// =====================================================================================================

uint8_t F2MS(uint8_t fcode) {
  fcode = fcode & 0x07;
  uint8_t m_bit;
  uint8_t s_code;
  
  switch(fcode) {
    case 0:  // CLR (000)
      m_bit = 1;
      s_code = 0b0011;  // Logic: 0
      break;   
    case 1:  // ADD (001)
      m_bit = 0;
      s_code = 0b1001;  // Arithmetic: B + C (Cn=0)
      break;
    case 2:  // INC (010)
      m_bit = 0;
      s_code = 0b0000;  // Arithmetic: B + 1 (Cn=1)
      break;
    case 3:  // AND (011)
      m_bit = 1;
      s_code = 0b1011;  // Logic: B AND C
      break;
    case 4:  // ORR (100)
      m_bit = 1;
      s_code = 0b1110;  // Logic: B OR C
      break;
    case 5:  // XOR (101)
      m_bit = 1;
      s_code = 0b0110;  // Logic: B XOR C
      break;
    case 6:  // NOT (110)
      m_bit = 1;
      s_code = 0b0000;  // Logic: NOT B
      break;
    case 7:  // SHL (111)
      m_bit = 0;
      s_code = 0b1001;  // Arithmetic: B + B (shift left via addition)
      break;
    default:
      m_bit = 0;
      s_code = 0;
      break;
  }
  
  return (m_bit << 4) | s_code;
}

/*
 * Emulate single 74HCT181 4-bit ALU
 * Inputs: A, B (4-bit values), S (4-bit select), M (mode bit), Cn (carry in, active low)
 * Outputs: F (4-bit result), Cn4 (carry out, active low)
 */
void emulate181(uint8_t a, uint8_t b, uint8_t s, uint8_t m, bool cn, uint8_t &f, bool &cn4) {
  a = a & 0x0F;  // Ensure 4-bit
  b = b & 0x0F;
  s = s & 0x0F;
  
  if (m == 1) {
    // LOGIC MODE - no carry
    switch(s) {
      case 0x0: f = ~a; break;                    // NOT A
      case 0x1: f = ~(a | b); break;              // NOR
      case 0x2: f = (~a) & b; break;              // (NOT A) AND B
      case 0x3: f = 0; break;                     // 0
      case 0x4: f = ~(a & b); break;              // NAND
      case 0x5: f = ~b; break;                    // NOT B
      case 0x6: f = a ^ b; break;                 // XOR
      case 0x7: f = (~a) | b; break;              // (NOT A) OR B
      case 0x8: f = a & (~b); break;              // A AND (NOT B)
      case 0x9: f = ~(a ^ b); break;              // XNOR
      case 0xA: f = b; break;                     // B
      case 0xB: f = a & b; break;                 // AND
      case 0xC: f = 0x0F; break;                  // 1
      case 0xD: f = a | (~b); break;              // A OR (NOT B)
      case 0xE: f = a | b; break;                 // OR
      case 0xF: f = a; break;                     // A
      default: f = 0; break;
    }
    f = f & 0x0F;
    cn4 = true;  // No carry in logic mode
    
  } else {
    // ARITHMETIC MODE - with carry
    uint8_t result;
    uint8_t cin = cn ? 0 : 1;  // Convert active-low to normal
    
    switch(s) {
      case 0x0: result = a + cin; break;                          // A
      case 0x1: result = (a | b) + cin; break;                    // A OR B
      case 0x2: result = (a | (~b & 0x0F)) + cin; break;          // A OR NOT B
      case 0x3: result = 0x0F + cin; break;                       // -1 (all ones)
      case 0x4: result = a + (a & (~b & 0x0F)) + cin; break;      // A + (A AND NOT B)
      case 0x5: result = (a | b) + (a & (~b & 0x0F)) + cin; break;// (A OR B) + (A AND NOT B)
      case 0x6: result = a - b - 1 + cin; break;                  // A - B - 1
      case 0x7: result = (a & (~b & 0x0F)) - 1 + cin; break;      // (A AND NOT B) - 1
      case 0x8: result = a + (a & b) + cin; break;                // A + (A AND B)
      case 0x9: result = a + b + cin; break;                      // A + B
      case 0xA: result = (a | (~b & 0x0F)) + (a & b) + cin; break;// (A OR NOT B) + (A AND B)
      case 0xB: result = (a & b) - 1 + cin; break;                // (A AND B) - 1
      case 0xC: result = a + a + cin; break;                      // A + A (shift left)
      case 0xD: result = (a | b) + a + cin; break;                // (A OR B) + A
      case 0xE: result = (a | (~b & 0x0F)) + a + cin; break;      // (A OR NOT B) + A
      case 0xF: result = a - 1 + cin; break;                      // A - 1
      default: result = 0; break;
    }
    
    f = result & 0x0F;
    cn4 = ((result & 0x10) == 0);  // Carry out (active low) if bit 4 is 0
  }
}

/*
 * Emulate two cascaded 74HCT181 chips for 8-bit operations
 * A is B_REG, B is C_REG in our system
 */
uint8_t emulate8BitALU(uint8_t a_input, uint8_t b_input, uint8_t s_code, uint8_t m_bit, 
                        bool &carry_out, bool &sign_out, bool &zero_out) {
  // For SHL operation (FCT=7), both inputs to 74HCT181 should be B_REG
  if (FCT == 7) {
    b_input = a_input;  // SHL: B + B
  }
  
  // Split into nibbles
  uint8_t a_low = a_input & 0x0F;
  uint8_t a_high = (a_input >> 4) & 0x0F;
  uint8_t b_low = b_input & 0x0F;
  uint8_t b_high = (b_input >> 4) & 0x0F;
  
  // Set carry in based on operation:
  // - INC (FCT=2): Cn=1 (active low, false = carry in = 1)
  // - ADD (FCT=1): Cn=0 (active low, true = no carry in)
  // - All others: Cn=1 (active low, true = no carry in)
  bool cn_low;
  if (FCT == 2) {
    cn_low = false;  // INC: force carry in (Cn=1)
  } else if (FCT == 1) {
    cn_low = true;   // ADD: no carry in (Cn=0)
  } else {
    cn_low = true;   // Default: no carry in
  }
  
  // Process lower 4 bits
  uint8_t f_low;
  bool cn4_low;
  emulate181(a_low, b_low, s_code, m_bit, cn_low, f_low, cn4_low);
  
  // Process upper 4 bits (carry from lower chip)
  uint8_t f_high;
  bool cn4_high;
  emulate181(a_high, b_high, s_code, m_bit, cn4_low, f_high, cn4_high);
  
  // Combine results
  uint8_t result = (f_high << 4) | f_low;
  
  // Set flags
  carry_out = !cn4_high;  // Convert active-low to active-high
  sign_out = (result & 0x80) != 0;  // MSB = sign bit
  zero_out = (result == 0);
  
  return result;
}

// =====================================================================================================
// 74HCT181 ALU EMULATION
// =====================================================================================================

void computeALU() {
  // Only compute when we have all three inputs
  if (!have_FCT || !have_B || !have_C) {
    return;
  }
  
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║     Computing ALU Operation          ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.printf("  B_REG (A) = 0x%02X (%d)\n", B_REG, B_REG);
  Serial.printf("  C_REG (B) = 0x%02X (%d)\n", C_REG, C_REG);
  Serial.printf("  FCT       = %d\n", FCT);
  
  // Get M and S bits from function code
  uint8_t ms_value = F2MS(FCT);
  M_bit = (ms_value >> 4) & 0x01;
  S_bits = ms_value & 0x0F;
  
  // Compute ALU result using 74HCT181 emulation
  RESULT = emulate8BitALU(B_REG, C_REG, S_bits, M_bit, carry_flag, sign_flag, zero_flag);
  
  // Operation names matching truth table
  const char* op_names[] = {"CLR", "ADD", "INC", "AND", "ORR", "XOR", "NOT", "SHL"};
  
  Serial.printf("\n  Operation: %s\n", op_names[FCT & 0x07]);
  Serial.printf("  RESULT    = 0x%02X (%d)\n", RESULT, RESULT);
  Serial.printf("  74HCT181  = M:%d S:0x%X\n", M_bit, S_bits);
  Serial.printf("  Flags     = S:%d C:%d Z:%d\n\n", sign_flag, carry_flag, zero_flag);
  
  // Update display
  displayALUStatus();
  
  // Send results back via broadcast
  sendResults();
  
  // Reset for next operation
  have_FCT = false;
  have_B = false;
  have_C = false;
  
  Serial.println("✓ ALU computation complete, ready for next operation\n");
}

// =====================================================================================================
// SEND RESULTS
// =====================================================================================================

void sendResults() {
  Serial.println("Broadcasting ALU results:");
  
  // Broadcast ALU result
  broadcastForMonitoring(ALU_OUT, RESULT);
  Serial.printf("  • ALU_OUT = 0x%02X\n", RESULT);
  
  // Broadcast 74HCT181 M and S bits
  uint8_t hct_ms = (M_bit << 4) | (S_bits & 0x0F);
  broadcastForMonitoring(HCT_MS, hct_ms);
  Serial.printf("  • HCT_MS  = 0x%02X (M=%d S=0x%X)\n", hct_ms, M_bit, S_bits);
  
  // Broadcast flags (Sign, Carry, Zero)
  uint8_t scz = (sign_flag << 2) | (carry_flag << 1) | zero_flag;
  broadcastForMonitoring(ALU_SCZ, scz);
  Serial.printf("  • ALU_SCZ = 0x%02X (S=%d C=%d Z=%d)\n", scz, sign_flag, carry_flag, zero_flag);
  
  Serial.println();
}

// =====================================================================================================
// RECEIVE CALLBACK
// =====================================================================================================

void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // Validate message size
  if (len != sizeof(P2PMessage)) {
    Serial.printf("⚠ Invalid message size: %d (expected %d)\n", len, sizeof(P2PMessage));
    return;
  }
  
  P2PMessage msg;
  memcpy(&msg, data, sizeof(msg));
  
  // Only process REQUEST messages
  if (msg.msg_type != MSG_REQUEST) {
    return;
  }
  
  // Dynamic peer registration - add sender if not already registered
  if (!esp_now_is_peer_exist(info->src_addr)) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, info->src_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) == ESP_OK) {
      Serial.print("│ ✓ Added sender as peer dynamically: ");
      printMAC(info->src_addr);
      Serial.println();
    }
  }
  
  // Flash LED on any received request
  flashRandomColor();
  
  // Log incoming request
  Serial.println("┌─────────────────────────────────────");
  Serial.print("│ 📨 REQUEST: ");
  Serial.print(getSignalName(msg.signal));
  Serial.printf(" = 0x%02X", msg.value);
  if (msg.signal == FCT_ALU) {
    Serial.printf(" (FCT=%d)", msg.value & 0x07);
  }
  Serial.println();
  Serial.printf("│    seq=%d src=%d→dst=%d\n", msg.seq_num, msg.src_idx, msg.dst_idx);
  
  // Prepare ACK response
  P2PMessage response;
  response.msg_type = MSG_RESPONSE;
  response.seq_num = msg.seq_num;
  response.signal = msg.signal;
  response.value = 0;  // ACK (0 = success)
  response.data16 = 0;
  response.src_idx = MY_DEVICE_IDX;
  response.dst_idx = msg.src_idx;
  
  bool valid_signal = true;
  
  // Process based on signal type
  switch (msg.signal) {
    case FCT_ALU:
      FCT = msg.value & 0x07;  // Only lower 3 bits
      have_FCT = true;
      Serial.printf("│ ✓ Stored FCT = %d\n", FCT);
      break;
      
    case B_2_ALU:
      B_REG = msg.value;
      have_B = true;
      Serial.printf("│ ✓ Stored B_REG = 0x%02X (%d)\n", B_REG, B_REG);
      break;
      
    case C_2_ALU:
      C_REG = msg.value;
      have_C = true;
      Serial.printf("│ ✓ Stored C_REG = 0x%02X (%d)\n", C_REG, C_REG);
      break;
      
    default:
      Serial.println("│ ⚠ Unknown signal - rejecting");
      response.value = 0xFF;  // NACK
      valid_signal = false;
      break;
  }
  
  // Send ACK back to Master
  esp_err_t result = esp_now_send(info->src_addr, (uint8_t*)&response, sizeof(response));
  if (result == ESP_OK) {
    Serial.println("│ ✓ ACK sent to Master");
  } else {
    Serial.printf("│ ✗ ACK failed: %d\n", result);
  }
  
  // Broadcast for monitoring (only if valid signal)
  if (valid_signal) {
    broadcastForMonitoring(msg.signal, msg.value);
    Serial.println("│ ✓ Broadcast for monitoring");
  }
  
  // Show status
  Serial.printf("│ Status: FCT=%s B=%s C=%s\n", 
                have_FCT ? "✓" : "○",
                have_B ? "✓" : "○", 
                have_C ? "✓" : "○");
  Serial.println("└─────────────────────────────────────");
  
  // Update display with new values
  displayALUStatus();
  
  // Try to compute if we have all inputs
  if (valid_signal) {
    computeALU();
  }
}

// =====================================================================================================
// BROADCAST FOR MONITORING
// =====================================================================================================

void broadcastForMonitoring(uint8_t signal, uint8_t value) {
  P2PMessage msg;
  msg.msg_type = MSG_BROADCAST;
  msg.seq_num = 0;
  msg.signal = signal;
  msg.value = value;
  msg.data16 = 0;
  msg.src_idx = MY_DEVICE_IDX;
  msg.dst_idx = 0xFF;  // Broadcast
  
  esp_now_send(broadcast_MAC, (uint8_t*)&msg, sizeof(msg));
}

// =====================================================================================================
// MAIN LOOP
// =====================================================================================================

void loop() {
  // Optional: Print status periodically
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 30000) {  // Every 30 seconds
    lastStatus = millis();
    
    Serial.println("───────────────────────────────────────");
    Serial.println("ALU Status Report:");
    Serial.printf("  B_REG   = 0x%02X (%d)\n", B_REG, B_REG);
    Serial.printf("  C_REG   = 0x%02X (%d)\n", C_REG, C_REG);
    Serial.printf("  FCT     = %d\n", FCT);
    Serial.printf("  RESULT  = 0x%02X (%d)\n", RESULT, RESULT);
    Serial.printf("  Flags   = S:%d C:%d Z:%d\n", sign_flag, carry_flag, zero_flag);
    Serial.printf("  Inputs  = FCT:%s B:%s C:%s\n",
                  have_FCT ? "✓" : "○",
                  have_B ? "✓" : "○",
                  have_C ? "✓" : "○");
    Serial.println("───────────────────────────────────────\n");
  }
  
  delay(10);
}

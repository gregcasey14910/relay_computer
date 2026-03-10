#include <Wire.h>

#define MCP_ADDR 0x21

// MCP23017 registers
#define IODIRA  0x00
#define IODIRB  0x01
#define GPPUA   0x0C
#define GPPUB   0x0D
#define GPIOA   0x12
#define GPIOB   0x13

void mcpWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delay(10);   // *** delay included after every write ***
}

uint8_t mcpRead(uint8_t reg) {
  Wire.beginTransmission(MCP_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  delay(10);   // *** delay included before read ***

  Wire.requestFrom(MCP_ADDR, (uint8_t)1);
  if (Wire.available()) return Wire.read();
  return 0xFF;
}

void setup() {
  Serial.begin(115200);
  delay(500);            // allow USB + pins to stabilize

  Wire.begin(0,1);       // SDA=0, SCL=1
  delay(500);            // allow MCP23017 to power‑up

  Serial.println("MCP23017 Test Starting (0x21)...");

  // Configure MCP23017
  mcpWrite(IODIRB, 0x00);   // Port B = outputs
  mcpWrite(IODIRA, 0xFF);   // Port A = inputs
  mcpWrite(GPPUA, 0x00);    // No pullups A
  mcpWrite(GPPUB, 0x00);    // No pullups B
}

void loop() {

  // ------------------------
  // Write 0x0F → Port B
  // ------------------------
  mcpWrite(GPIOB, 0x0F);
  uint8_t read1 = mcpRead(GPIOA);

  // Serial.print("WROTE 0x0F | READ 0x");
  // Serial.println(read1, HEX);

  // ------------------------
  // Write 0x00 → Port B
  // ------------------------
  mcpWrite(GPIOB, 0x00);
  uint8_t read2 = mcpRead(GPIOA);

  // Serial.print("WROTE 0x00 | READ 0x");
  // Serial.println(read2, HEX);

  delay(500);
}
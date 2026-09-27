#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR    0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const byte U2_ADDR = 0x20;
const byte U3_ADDR = 0x21;
const byte U7_ADDR = 0x22;
const byte U4_ADDR = 0x23;

Adafruit_MCP23X17 mcp_U2;
Adafruit_MCP23X17 mcp_U3;
Adafruit_MCP23X17 mcp_U7;
Adafruit_MCP23X17 mcp_U4;

// D[7:0] all on U2 (0x20) GPB0-7
#define D0   8   // U2 GPB0
#define D1   9   // U2 GPB1
#define D2  10   // U2 GPB2
#define D3  11   // U2 GPB3
#define D4  12   // U2 GPB4
#define D5  13   // U2 GPB5
#define D6  14   // U2 GPB6
#define D7  15   // U2 GPB7
// A[7:0] all on U3 (0x21) GPB0-7
#define A0   8   // U3 GPB0
#define A1   9   // U3 GPB1
#define A2  10   // U3 GPB2
#define A3  11   // U3 GPB3
#define A4  12   // U3 GPB4
#define A5  13   // U3 GPB5
#define A6  14   // U3 GPB6
#define A7  15   // U3 GPB7
// A[15:8] all on U4 (0x23) GPB0-7
#define A8   8   // U4 GPB0
#define A9   9   // U4 GPB1
#define A10 10   // U4 GPB2
#define A11 11   // U4 GPB3
#define A12 12   // U4 GPB4
#define A13 13   // U4 GPB5
#define A14 14   // U4 GPB6
#define A15 15   // U4 GPB7
#define A_2_R  13  // U7 GPB5 — Abus to R register
#define D_2_RL  9  // U7 GPB1 — Dbus to R register Lower bits 7:0
#define D_2_RH 11  // U7 GPB3 — Dbus to R register Higher bits 15:8
#define LDHL 12  // U7 GPB4
#define LDH  10  // U7 GPB2
#define LDL   8  // U7 GPB0

void pulse(Adafruit_MCP23X17 &mcp, uint8_t pin, int count) {
  for (int i = 0; i < count; i++) {
    mcp.digitalWrite(pin, HIGH);
    delay(80);
    mcp.digitalWrite(pin, LOW);
    delay(80);
  }
}

void pulseStrobe(int count, uint8_t strobe) {
  mcp_U4.writeGPIOB(0x00);  // A[15:8] held LOW
  mcp_U2.writeGPIOB(0x00);  // DBUS held LOW
  for (int i = 0; i < count; i++) {
    mcp_U3.writeGPIOB((i & 1) ? 0x00 : 0xFF);
    mcp_U7.digitalWrite(strobe, HIGH);
    delay(400);
    mcp_U7.digitalWrite(strobe, LOW);
    delay(400);
  }
  mcp_U2.writeGPIOB(0x00);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
}

// Schematic: D_DBUS[n] on GPB[n], W_DBUS[n] on GPA[7-n] — intentional bit reversal
static uint8_t reverseBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

void loopbackTest(Adafruit_MCP23X17 &mcp, const char *label) {
  static const uint8_t patterns[] = { 0xFF, 0x00, 0x55, 0xAA, 0xF0, 0x0F };
  bool pass = true;
  Serial.printf("--- %s GPB->GPA loopback ---\n", label);
  for (uint8_t pat : patterns) {
    mcp.writeGPIOB(pat);
    delay(5);
    uint8_t gpb = mcp.readGPIOB();   // verify write took
    uint8_t raw = mcp.readGPIOA();
    uint8_t got = reverseBits(raw);
    bool ok = (got == pat);
    if (!ok) pass = false;
    Serial.printf("  write 0x%02X  rdGPB 0x%02X  rawGPA 0x%02X  rev 0x%02X  %s\n",
                  pat, gpb, raw, got, ok ? "OK" : "FAIL");
  }
  mcp.writeGPIOB(0x00);
  Serial.printf("%s: %s\n", label, pass ? "LOOPBACK PASS" : "LOOPBACK FAIL");
  display.printf("%s: %s\n", label, pass ? "PASS" : "FAIL");
  display.display();
}

// A0 pulse with A_2_R high in the middle two quarters
void pulseA0(int count) {
  for (int i = 0; i < count; i++) {
    mcp_U3.digitalWrite(A0, HIGH);
    delay(16);                        // Q1: A_2_R low
    mcp_U7.digitalWrite(A_2_R, HIGH);  // Q2 start
    delay(48);                        // Q2 + Q3
    mcp_U7.digitalWrite(A_2_R, LOW);   // Q4 start
    delay(16);                        // Q4: A_2_R low
    mcp_U3.digitalWrite(A0, LOW);
    delay(80);
  }
}

void setup() {
  Serial.begin(115200);
  delay(3000);  // wait for USB CDC to re-enumerate and monitor to reconnect

  Wire.begin(0, 1);
  Wire.setClock(50000);
  Wire.setTimeOut(200);

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("RC_Test ready");
  display.display();

  Serial.println("\n=== RC_Test ===");

  if (!mcp_U2.begin_I2C(U2_ADDR)) {
    Serial.println("U2 not found!");
    display.println("U2 not found!");
    display.display();
    return;
  }
  if (!mcp_U3.begin_I2C(U3_ADDR)) {
    Serial.println("U3 not found!");
    display.println("U3 not found!");
    display.display();
    return;
  }
  if (!mcp_U7.begin_I2C(U7_ADDR)) {
    Serial.println("U7 not found!");
    display.println("U7 not found!");
    display.display();
    return;
  }
  if (!mcp_U4.begin_I2C(U4_ADDR)) {
    Serial.println("U4 not found!");
    display.println("U4 not found!");
    display.display();
    return;
  }

  // U2 GPA0-7: inputs (loopback from GPB via D bus)
  for (int p = 0; p <= 7; p++) { mcp_U2.pinMode(p, INPUT); }
  // U2 GPB0-7: D[7:0] all outputs
  for (int p = 8; p <= 15; p++) { mcp_U2.pinMode(p, OUTPUT); mcp_U2.digitalWrite(p, LOW); }
  // U3 GPA0-7: inputs (loopback from GPB via A bus)
  for (int p = 0; p <= 7; p++) { mcp_U3.pinMode(p, INPUT); }
  // U3 GPB0-7: A[7:0] all outputs
  for (int p = 8; p <= 15; p++) { mcp_U3.pinMode(p, OUTPUT); mcp_U3.digitalWrite(p, LOW); }
  // U4 GPA0-7: inputs (loopback from GPB via A bus high)
  for (int p = 0; p <= 7; p++) { mcp_U4.pinMode(p, INPUT); }
  // U4 GPB0-7: A[15:8] all outputs
  for (int p = 8; p <= 15; p++) { mcp_U4.pinMode(p, OUTPUT); mcp_U4.digitalWrite(p, LOW); }
  mcp_U7.pinMode(D_2_RL, OUTPUT);  mcp_U7.digitalWrite(D_2_RL, LOW);
mcp_U7.pinMode(A_2_R,  OUTPUT);  mcp_U7.digitalWrite(A_2_R,  LOW);
  mcp_U7.pinMode(LDHL,   OUTPUT);  mcp_U7.digitalWrite(LDHL,   LOW);
  mcp_U7.pinMode(LDH,    OUTPUT);  mcp_U7.digitalWrite(LDH,    LOW);
  mcp_U7.pinMode(LDL,    OUTPUT);  mcp_U7.digitalWrite(LDL,    LOW);

  loopbackTest(mcp_U2, "U2 DBUS");
  loopbackTest(mcp_U3, "U3 ABUS_LO");
  loopbackTest(mcp_U4, "U4 ABUS_HI");

  Serial.println("Running pulses A_2_R...");
  display.println("Running...");
  display.display();

  Serial.println(">> set 1 (A_2_R)");
  pulseStrobe(3, A_2_R);
  delay(100);
  Serial.println(">> set 2 (A_2_R)");
  pulseStrobe(3, A_2_R);

  // mcp_U7.digitalWrite(A_2_R, LOW);

  Serial.println("Done.");
  display.println("Done.");
  display.display();
}

void loop() {
  // nothing
}

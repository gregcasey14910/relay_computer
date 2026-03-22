// Relay Computer Test Suite
// Uses GPIO0 (SDA) and GPIO1 (SCL) for I2C

#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int LED_PINS[] = {5, 6, 7, 8, 9, 10};  // bit0 to bit5

// MCP23017 I2C Addresses (verified from netlist)
const byte U2_ADDR = 0x20;  // A2=0, A1=0, A0=0
const byte U3_ADDR = 0x21;  // A2=0, A1=0, A0=1
const byte U7_ADDR = 0x22;  // A2=0, A1=1, A0=0
const byte U4_ADDR = 0x23;  // A2=0, A1=1, A0=1

// Create MCP23017 objects
Adafruit_MCP23X17 mcp_U2;
Adafruit_MCP23X17 mcp_U3;
Adafruit_MCP23X17 mcp_U7;
Adafruit_MCP23X17 mcp_U4;

void runTest0_LEDCount() {
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  for (int b = 0; b < 6; b++) pinMode(LED_PINS[b], OUTPUT);

  // Bits 6,7,8,9: U7 MCP23017 (0x22)
  // bit6=GPA0(pin0), bit7=GPB7(pin15), bit8=GPB6(pin14), bit9=GPA1(pin1)
  bool u7_ok = mcp_U7.begin_I2C(U7_ADDR, &Wire);
  if (u7_ok) {
    mcp_U7.pinMode(0,  OUTPUT);  // GPA0 - bit6
    mcp_U7.pinMode(15, OUTPUT);  // GPB7 - bit7
    mcp_U7.pinMode(14, OUTPUT);  // GPB6 - bit8
    mcp_U7.pinMode(1,  OUTPUT);  // GPA1 - bit9
    Serial.println("U7 MCP23017 initialized for bits 6-9");
  } else {
    Serial.println("WARNING: U7 MCP23017 not found, bits 6-9 disabled");
  }

  Serial.println("Test 0: LED count 0-1023 on GPIO5-10 + U7 (bits 6-9)...");
  for (int i = 0; i <= 1023; i++) {
    for (int b = 0; b < 6; b++) digitalWrite(LED_PINS[b], (i >> b) & 1);
    if (u7_ok) {
      mcp_U7.digitalWrite(0,  (i >> 6) & 1);
      mcp_U7.digitalWrite(15, (i >> 7) & 1);
      mcp_U7.digitalWrite(14, (i >> 8) & 1);
      mcp_U7.digitalWrite(1,  (i >> 9) & 1);
    }
    if (i % 64 == 0) {
      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println("Test 0: LED Count");
      display.setTextSize(3);
      display.setCursor(0, 20);
      display.println(i);
      display.setTextSize(1);
      display.setCursor(0, 56);
      display.print("0-1023");
      display.display();
    }
    delay(1);
  }
  for (int b = 0; b < 6; b++) digitalWrite(LED_PINS[b], LOW);
  if (u7_ok) {
    mcp_U7.digitalWrite(0,  LOW);
    mcp_U7.digitalWrite(15, LOW);
    mcp_U7.digitalWrite(14, LOW);
    mcp_U7.digitalWrite(1,  LOW);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(0, 20);
  display.println("Test 0");
  display.println("  Done");
  display.display();

  Serial.println("Test 0 complete.\n");
}

void setup() {
  Serial.begin(115200);
  delay(500);  // Wait for USB CDC to settle

  Serial.println("Serial started!");

  // Initialize I2C on GPIO0 (SDA) and GPIO1 (SCL)
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  delay(100);
  Wire.begin(0, 1);
  Wire.setClock(50000);  // 50 kHz for reliability
  Wire.setTimeOut(5000);

  Serial.println("\n=== Relay Computer Test Suite ===");
  Serial.println("Enter test number to run:");
  Serial.println("0 - LED Count Test (GPIO5 LSB)");
  Serial.println("1 - I2C Full Scan");
  Serial.println("2 - U3 ABUS_L[7:0]  Loopback + SSD1306 (0-255)");
  Serial.println("3 - U4 ABUS_H[15:8] Loopback + SSD1306 (0-255)");
  Serial.println("4 - U2 DBUS[7:0]       Loopback + SSD1306 (0-255)");
  Serial.println("5 - U7 Misc_Ctrl[5:0]  Loopback + SSD1306 (0-63)");
  Serial.println("6 - SSD1306 OLED Test");
  Serial.println("7 - Run All Tests (loop, any key to stop)");
  Serial.println("8 - Ctrl_FF: D_LDHL, D_LDH, D_LDL toggle 3Hz (12 cycles each)");
  Serial.println("9 - Reg: LDHL pattern 011111110, 10Hz, 12 cycles");
  Serial.println("A - Test A: Auto 32-cycle bus walk");
  Serial.println("B - Test B: Interactive - Dyy, Eyy, Axxxx, CL, Q");
  Serial.println("================================\n");
}

void loop() {
  if (Serial.available()) {
    char input = Serial.read();
    Serial.println(input);  // Echo it back

    if (input == '0') {
      runTest0_LEDCount();
    }
    else if (input == '1') {
      runTest1_ESP32Communication();
    }
    else if (input == '2') {
      runTest2_ABUSLoopback();
    }
    else if (input == '3') {
      runTest3_ABUSHLoopback();
    }
    else if (input == '4') {
      runTest4_DBUSLoopback();
    }
    else if (input == '5') {
      runTest5_MiscCtrlLoopback();
    }
    else if (input == '6') {
      runTest6_SSD1306();
    }
    else if (input == '7') {
      runTest7_AllTests();
    }
    else if (input == '8') {
      runTest8_RCControl();
    }
    else if (input == '9') {
      runTest9_Reg();
    }
    else if (input == 'A' || input == 'a') {
      runTest10_WriteR0();
    }
    else if (input == 'B' || input == 'b') {
      runTestB_Interactive();
    }


    // Clear any remaining characters
    while(Serial.available()) Serial.read();

    // Show prompt again
    Serial.println("\nEnter test (0-9, A-Auto, B-Interactive, C-Single): ");
  }
}

void runTest1_ESP32Communication() {
  Serial.println("\n--- Test 1: Full I2C Scan (GPIO0=SDA, GPIO1=SCL) ---");

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Test 1: I2C Scan");
  display.println("Scanning...");
  display.display();

  int deviceCount = 0;
  // track up to 8 found addresses for OLED
  byte found[8];

  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("  Found: 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      if (addr == U2_ADDR) Serial.print("  (U2 - MCP23017)");
      if (addr == U3_ADDR) Serial.print("  (U3 - MCP23017)");
      if (addr == U7_ADDR) Serial.print("  (U7 - MCP23017)");
      if (addr == U4_ADDR) Serial.print("  (U4 - MCP23017)");
      if (addr == OLED_ADDR) Serial.print("  (SSD1306 OLED)");
      Serial.println();
      if (deviceCount < 8) found[deviceCount] = addr;
      deviceCount++;
    }
  }

  if (deviceCount == 0) {
    Serial.println("  No I2C devices found!");
  } else {
    Serial.print("  Total: ");
    Serial.print(deviceCount);
    Serial.println(" device(s)");
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Test 1: I2C Scan");
  display.print("Found: "); display.println(deviceCount);
  int y = 16;
  for (int i = 0; i < min(deviceCount, 8); i++) {
    display.setCursor(0, y);
    display.print("0x");
    if (found[i] < 16) display.print("0");
    display.print(found[i], HEX);
    if (found[i] == U2_ADDR) display.print(" U2");
    else if (found[i] == U3_ADDR) display.print(" U3");
    else if (found[i] == U7_ADDR) display.print(" U7");
    else if (found[i] == U4_ADDR) display.print(" U4");
    else if (found[i] == OLED_ADDR) display.print(" OLED");
    y += 8;
  }
  display.display();

  Serial.println("--- Test 1 Complete ---");
}

void runTest2_ABUSLoopback() {
  Serial.println("\n--- Test 2: U3 ABUS[7:0] Loopback + SSD1306 (0-255) ---");
  Serial.println("D_ABUS[0:7]: U3 GPB[0:7] (lib pins 8-15) = OUTPUT");
  Serial.println("W_ABUS[0:7]: U3 GPA[7:0] (lib pins 7-0)  = INPUT (bit-reversed)");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 2 Failed ---");
    return;
  }
  Serial.println("SSD1306 initialized OK");

  if (!mcp_U3.begin_I2C(U3_ADDR, &Wire)) {
    Serial.println("ERROR: U3 MCP23017 not found at 0x21!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U3 NOT FOUND");
    display.display();
    Serial.println("--- Test 2 Failed ---");
    return;
  }
  Serial.println("U3 initialized OK");

  // D_ABUS[0:7] = GPB[0:7] = lib pins 8-15, all OUTPUT
  // W_ABUS[0:7] = GPA[7:0] = lib pins 7-0,  all INPUT (bit-reversed)
  for (int i = 0; i < 8; i++) {
    mcp_U3.pinMode(8 + i, OUTPUT);  // GPB0-7
    mcp_U3.pinMode(7 - i, INPUT);   // GPA7-0
  }

  int passCount = 0;
  int failCount = 0;

  for (int val = 0; val <= 255; val++) {
    // Write all 8 bits to GPB[0:7]
    for (int b = 0; b < 8; b++) {
      mcp_U3.digitalWrite(8 + b, (val >> b) & 1);
    }
    delay(10);

    // Read back from GPA[7:0] — W_ABUS0=GPA7, W_ABUS1=GPA6, ... W_ABUS7=GPA0
    uint8_t readBack = 0;
    for (int b = 0; b < 8; b++) {
      if (mcp_U3.digitalRead(7 - b)) readBack |= (1 << b);
    }

    bool pass = (readBack == (uint8_t)val);
    if (pass) passCount++; else failCount++;

    // OLED
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ABUS[7:0] Loopback U3");

    display.setCursor(0, 10);
    display.print("OUT:0x");
    if (val < 16) display.print("0");
    display.print(val, HEX);
    display.print("  ");
    display.print(val); display.print("/255");

    display.setCursor(0, 20);
    display.print(" IN:0x");
    if (readBack < 16) display.print("0");
    display.print(readBack, HEX);

    display.setTextSize(3);
    display.setCursor(0, 34);
    display.println(pass ? "PASS" : "FAIL");
    display.display();

    Serial.print("val=0x");
    if (val < 16) Serial.print("0");
    Serial.print(val, HEX);
    Serial.print(" in=0x");
    if (readBack < 16) Serial.print("0");
    Serial.print(readBack, HEX);
    Serial.println(pass ? "  PASS" : "  FAIL");

    delay(30);
  }

  // Summary
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ABUS[7:0] Done");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("PASS:"); display.println(passCount);
  display.setCursor(0, 36);
  display.print("FAIL:"); display.println(failCount);
  display.display();

  // Zero all outputs (not-drive state)
  for (int b = 0; b < 8; b++) mcp_U3.digitalWrite(8 + b, LOW);

  Serial.println("--- Test 2 Complete ---");
  Serial.print("Pass: "); Serial.println(passCount);
  Serial.print("Fail: "); Serial.println(failCount);
}

void runTest3_ABUSHLoopback() {
  Serial.println("\n--- Test 3: U4 ABUS_H[15:8] Loopback + SSD1306 (0-255) ---");
  Serial.println("D_ABUS[8:15]: U4 GPB[0:7] (lib pins 8-15) = OUTPUT");
  Serial.println("W_ABUS[8:15]: U4 GPA[7:0] (lib pins 7-0)  = INPUT (bit-reversed)");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 3 Failed ---");
    return;
  }
  Serial.println("SSD1306 initialized OK");

  if (!mcp_U4.begin_I2C(U4_ADDR, &Wire)) {
    Serial.println("ERROR: U4 MCP23017 not found at 0x23!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U4 NOT FOUND");
    display.display();
    Serial.println("--- Test 3 Failed ---");
    return;
  }
  Serial.println("U4 initialized OK");

  // D_ABUS[8:15] = GPB[0:7] = lib pins 8-15, all OUTPUT
  // W_ABUS[8:15] = GPA[7:0] = lib pins 7-0,  all INPUT (bit-reversed)
  for (int i = 0; i < 8; i++) {
    mcp_U4.pinMode(8 + i, OUTPUT);  // GPB0-7
    mcp_U4.pinMode(7 - i, INPUT);   // GPA7-0
  }

  int passCount = 0;
  int failCount = 0;

  for (int val = 0; val <= 255; val++) {
    for (int b = 0; b < 8; b++) {
      mcp_U4.digitalWrite(8 + b, (val >> b) & 1);
    }
    delay(10);

    uint8_t readBack = 0;
    for (int b = 0; b < 8; b++) {
      if (mcp_U4.digitalRead(7 - b)) readBack |= (1 << b);
    }

    bool pass = (readBack == (uint8_t)val);
    if (pass) passCount++; else failCount++;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("ABUS_H[15:8] Lpbk U4");

    display.setCursor(0, 10);
    display.print("OUT:0x");
    if (val < 16) display.print("0");
    display.print(val, HEX);
    display.print("  ");
    display.print(val); display.print("/255");

    display.setCursor(0, 20);
    display.print(" IN:0x");
    if (readBack < 16) display.print("0");
    display.print(readBack, HEX);

    display.setTextSize(3);
    display.setCursor(0, 34);
    display.println(pass ? "PASS" : "FAIL");
    display.display();

    Serial.print("val=0x");
    if (val < 16) Serial.print("0");
    Serial.print(val, HEX);
    Serial.print(" in=0x");
    if (readBack < 16) Serial.print("0");
    Serial.print(readBack, HEX);
    Serial.println(pass ? "  PASS" : "  FAIL");

    delay(30);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ABUS_H[15:8] Done");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("PASS:"); display.println(passCount);
  display.setCursor(0, 36);
  display.print("FAIL:"); display.println(failCount);
  display.display();

  // Zero all outputs (not-drive state)
  for (int b = 0; b < 8; b++) mcp_U4.digitalWrite(8 + b, LOW);

  Serial.println("--- Test 3 Complete ---");
  Serial.print("Pass: "); Serial.println(passCount);
  Serial.print("Fail: "); Serial.println(failCount);
}

void runTest4_DBUSLoopback() {
  Serial.println("\n--- Test 4: U2 DBUS[7:0] Loopback + SSD1306 (0-255) ---");
  Serial.println("D_DBUS[0:7]: U2 GPB[0:7] (lib pins 8-15) = OUTPUT");
  Serial.println("W_DBUS[0:7]: U2 GPA[7:0] (lib pins 7-0)  = INPUT (bit-reversed)");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 4 Failed ---");
    return;
  }
  Serial.println("SSD1306 initialized OK");

  if (!mcp_U2.begin_I2C(U2_ADDR, &Wire)) {
    Serial.println("ERROR: U2 MCP23017 not found at 0x20!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U2 NOT FOUND");
    display.display();
    Serial.println("--- Test 4 Failed ---");
    return;
  }
  Serial.println("U2 initialized OK");

  // D_DBUS[0:7] = GPB[0:7] = lib pins 8-15, all OUTPUT
  // W_DBUS[0:7] = GPA[7:0] = lib pins 7-0,  all INPUT (bit-reversed)
  for (int i = 0; i < 8; i++) {
    mcp_U2.pinMode(8 + i, OUTPUT);
    mcp_U2.pinMode(7 - i, INPUT);
  }

  int passCount = 0;
  int failCount = 0;

  for (int val = 0; val <= 255; val++) {
    for (int b = 0; b < 8; b++) {
      mcp_U2.digitalWrite(8 + b, (val >> b) & 1);
    }
    delay(10);

    uint8_t readBack = 0;
    for (int b = 0; b < 8; b++) {
      if (mcp_U2.digitalRead(7 - b)) readBack |= (1 << b);
    }

    bool pass = (readBack == (uint8_t)val);
    if (pass) passCount++; else failCount++;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("DBUS[7:0] Loopback U2");

    display.setCursor(0, 10);
    display.print("OUT:0x");
    if (val < 16) display.print("0");
    display.print(val, HEX);
    display.print("  ");
    display.print(val); display.print("/255");

    display.setCursor(0, 20);
    display.print(" IN:0x");
    if (readBack < 16) display.print("0");
    display.print(readBack, HEX);

    display.setTextSize(3);
    display.setCursor(0, 34);
    display.println(pass ? "PASS" : "FAIL");
    display.display();

    Serial.print("val=0x");
    if (val < 16) Serial.print("0");
    Serial.print(val, HEX);
    Serial.print(" in=0x");
    if (readBack < 16) Serial.print("0");
    Serial.print(readBack, HEX);
    Serial.println(pass ? "  PASS" : "  FAIL");

    delay(30);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("DBUS[7:0] Done");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("PASS:"); display.println(passCount);
  display.setCursor(0, 36);
  display.print("FAIL:"); display.println(failCount);
  display.display();

  // Zero all outputs (not-drive state)
  for (int b = 0; b < 8; b++) mcp_U2.digitalWrite(8 + b, LOW);

  Serial.println("--- Test 4 Complete ---");
  Serial.print("Pass: "); Serial.println(passCount);
  Serial.print("Fail: "); Serial.println(failCount);
}

void runTest5_MiscCtrlLoopback() {
  Serial.println("\n--- Test 5: U7 Misc_Ctrl[5:0] Loopback + SSD1306 (0-63) ---");
  Serial.println("OUT: U7 GPB[0:5] (lib pins 8-13)");
  Serial.println("IN:  U7 GPA[7:2] (lib pins 7-2) (bit-reversed)");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 5 Failed ---");
    return;
  }
  Serial.println("SSD1306 initialized OK");

  if (!mcp_U7.begin_I2C(U7_ADDR, &Wire)) {
    Serial.println("ERROR: U7 MCP23017 not found at 0x22!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U7 NOT FOUND");
    display.display();
    Serial.println("--- Test 5 Failed ---");
    return;
  }
  Serial.println("U7 initialized OK");

  // GPB[0:5] (lib pins 8-13) = output, bit 0-5
  // GPA[7:2] (lib pins 7-2)  = input,  bit 0-5 (reversed: GPA7=bit0, GPA2=bit5)
  for (int i = 0; i < 6; i++) {
    mcp_U7.pinMode(8 + i, OUTPUT);  // GPB0-5
    mcp_U7.pinMode(7 - i, INPUT);   // GPA7-2
  }

  int passCount = 0;
  int failCount = 0;

  for (int val = 0; val <= 63; val++) {
    for (int b = 0; b < 6; b++) {
      mcp_U7.digitalWrite(8 + b, (val >> b) & 1);
    }
    delay(10);

    uint8_t readBack = 0;
    for (int b = 0; b < 6; b++) {
      if (mcp_U7.digitalRead(7 - b)) readBack |= (1 << b);
    }

    bool pass = (readBack == (uint8_t)val);
    if (pass) passCount++; else failCount++;

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Misc_Ctrl[5:0] U7");

    display.setCursor(0, 10);
    display.print("OUT:0x");
    if (val < 16) display.print("0");
    display.print(val, HEX);
    display.print("  ");
    display.print(val); display.print("/63");

    display.setCursor(0, 20);
    display.print(" IN:0x");
    if (readBack < 16) display.print("0");
    display.print(readBack, HEX);

    display.setTextSize(3);
    display.setCursor(0, 34);
    display.println(pass ? "PASS" : "FAIL");
    display.display();

    Serial.print("val=0x");
    if (val < 16) Serial.print("0");
    Serial.print(val, HEX);
    Serial.print(" in=0x");
    if (readBack < 16) Serial.print("0");
    Serial.print(readBack, HEX);
    Serial.println(pass ? "  PASS" : "  FAIL");

    delay(30);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Misc_Ctrl[5:0] Done");
  display.setTextSize(2);
  display.setCursor(0, 16);
  display.print("PASS:"); display.println(passCount);
  display.setCursor(0, 36);
  display.print("FAIL:"); display.println(failCount);
  display.display();

  // Zero all outputs (not-drive state)
  for (int b = 0; b < 6; b++) mcp_U7.digitalWrite(8 + b, LOW);

  Serial.println("--- Test 5 Complete ---");
  Serial.print("Pass: "); Serial.println(passCount);
  Serial.print("Fail: "); Serial.println(failCount);
}

void runTest6_SSD1306() {
  Serial.println("\n--- Test 3: SSD1306 OLED Test ---");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 3 Failed ---");
    return;
  }
  Serial.println("SSD1306 initialized OK");

  // Test 1: Fill white
  display.clearDisplay();
  display.fillScreen(SSD1306_WHITE);
  display.display();
  Serial.println("All pixels ON");
  delay(1000);

  // Test 2: Clear
  display.clearDisplay();
  display.display();
  Serial.println("All pixels OFF");
  delay(500);

  // Test 3: Hello message
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("RC DIAG");
  display.setTextSize(1);
  display.println("SSD1306 OK");
  display.println("128x64 OLED");
  display.display();
  Serial.println("Text displayed");
  delay(2000);

  // Test 4: Scrolling counter
  Serial.println("Counting 0-15 on display...");
  for (int i = 0; i <= 15; i++) {
    display.clearDisplay();
    display.setTextSize(4);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(40, 16);
    display.println(i);
    display.display();
    Serial.print("  ");
    Serial.println(i);
    delay(200);
  }

  display.clearDisplay();
  display.display();
  Serial.println("--- Test 6 Complete ---");
}

void runTest7_AllTests() {
  Serial.println("\n--- Test 7: Run All Tests (looping, any key to stop) ---");
  int pass = 0, fail = 0, run = 0;

  while (!Serial.available()) {
    run++;
    Serial.print("\n=== Loop "); Serial.print(run); Serial.println(" ===");

    runTest0_LEDCount();        if (Serial.available()) break;
    runTest1_ESP32Communication(); if (Serial.available()) break;
    runTest2_ABUSLoopback();    if (Serial.available()) break;
    runTest3_ABUSHLoopback();   if (Serial.available()) break;
    runTest4_DBUSLoopback();    if (Serial.available()) break;
    runTest5_MiscCtrlLoopback(); if (Serial.available()) break;
    runTest6_SSD1306();          if (Serial.available()) break;
    runTest8_RCControl();        if (Serial.available()) break;
    runTest9_Reg();
  }

  while (Serial.available()) Serial.read();
  Serial.println("\n--- Test 7 Stopped ---");
  Serial.print("Loops completed: "); Serial.println(run);
}

void runTest8_RCControl() {
  Serial.println("\n--- Test 8: Ctrl_FF - D_LDHL, D_LDH, D_LDL 3Hz 12 cycles each ---");
  Serial.println("D_LDHL: U7 GPB4 (pin 12), D_LDH: U7 GPB2 (pin 10), D_LDL: U7 GPB0 (pin 8)");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found at 0x3C!");
    Serial.println("--- Test 8 Failed ---");
    return;
  }

  if (!mcp_U7.begin_I2C(U7_ADDR, &Wire)) {
    Serial.println("ERROR: U7 MCP23017 not found at 0x22!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U7 NOT FOUND");
    display.display();
    Serial.println("--- Test 8 Failed ---");
    return;
  }
  Serial.println("U7 initialized OK");

  // D_LDHL = GPB4 = lib pin 12
  mcp_U7.pinMode(12, OUTPUT);
  mcp_U7.digitalWrite(12, LOW);

  while (Serial.available()) Serial.read();

  bool state = false;
  // 3 Hz = 333ms period = 167ms per half-cycle, 12 cycles
  for (int cycle = 0; cycle < 12 && !Serial.available(); cycle++) {
    state = !state;
    mcp_U7.digitalWrite(12, state);
    Serial.print("D_LDHL = "); Serial.print(state ? "HIGH" : "LOW");
    Serial.print("  cycle "); Serial.print(cycle + 1); Serial.println("/12");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Ctrl_FF     Test 8");
    display.print("D_LDHL  U7 GPB4  ");
    display.print(cycle + 1); display.println("/12");
    display.setTextSize(3);
    display.setCursor(0, 28);
    display.println(state ? " HIGH" : " LOW ");
    display.display();

    delay(167);
  }

  mcp_U7.digitalWrite(12, LOW);
  Serial.println("D_LDHL = LOW (idle)");

  // D_LDH = GPB2 = lib pin 10
  mcp_U7.pinMode(10, OUTPUT);
  mcp_U7.digitalWrite(10, LOW);

  state = false;
  for (int cycle = 0; cycle < 12 && !Serial.available(); cycle++) {
    state = !state;
    mcp_U7.digitalWrite(10, state);
    Serial.print("D_LDH = "); Serial.print(state ? "HIGH" : "LOW");
    Serial.print("  cycle "); Serial.print(cycle + 1); Serial.println("/12");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Ctrl_FF     Test 8");
    display.print("D_LDH   U7 GPB2  ");
    display.print(cycle + 1); display.println("/12");
    display.setTextSize(3);
    display.setCursor(0, 28);
    display.println(state ? " HIGH" : " LOW ");
    display.display();

    delay(167);
  }

  mcp_U7.digitalWrite(10, LOW);
  Serial.println("D_LDH = LOW (idle)");

  // D_LDL = GPB0 = lib pin 8
  mcp_U7.pinMode(8, OUTPUT);
  mcp_U7.digitalWrite(8, LOW);

  state = false;
  for (int cycle = 0; cycle < 12 && !Serial.available(); cycle++) {
    state = !state;
    mcp_U7.digitalWrite(8, state);
    Serial.print("D_LDL = "); Serial.print(state ? "HIGH" : "LOW");
    Serial.print("  cycle "); Serial.print(cycle + 1); Serial.println("/12");

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Ctrl_FF     Test 8");
    display.print("D_LDL   U7 GPB0  ");
    display.print(cycle + 1); display.println("/12");
    display.setTextSize(3);
    display.setCursor(0, 28);
    display.println(state ? " HIGH" : " LOW ");
    display.display();

    delay(167);
  }

  mcp_U7.digitalWrite(8, LOW);
  while (Serial.available()) Serial.read();
  Serial.println("D_LDL = LOW (idle)");
  Serial.println("--- Test 8 Complete ---");
}

void runTest9_Reg() {
  Serial.println("\n--- Test 9: Reg - LDHL pattern 011111110, 10Hz, 12 cycles ---");
  Serial.println("LDHL: U7 GPB4 (pin 12)  step pattern: 0-11111110");

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  if (!mcp_U7.begin_I2C(U7_ADDR, &Wire)) {
    Serial.println("ERROR: U7 MCP23017 not found at 0x22!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("U7 NOT FOUND");
    display.display();
    Serial.println("--- Test 9 Failed ---");
    return;
  }

  mcp_U7.pinMode(12, OUTPUT);  // LDHL = GPB4
  mcp_U7.pinMode(10, OUTPUT);  // LDH  = GPB2
  mcp_U7.pinMode(9,  OUTPUT);  // D_ENL= GPB1
  mcp_U7.pinMode(8,  OUTPUT);  // LDL  = GPB0
  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(8,  LOW);

  // Init bus chips - GPB[0:7] as outputs on U2, U3, U4
  mcp_U3.begin_I2C(U3_ADDR, &Wire);
  mcp_U4.begin_I2C(U4_ADDR, &Wire);
  mcp_U2.begin_I2C(U2_ADDR, &Wire);
  for (int i = 0; i < 8; i++) {
    mcp_U3.pinMode(8 + i, OUTPUT);
    mcp_U4.pinMode(8 + i, OUTPUT);
    mcp_U2.pinMode(8 + i, OUTPUT);
  }
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
  mcp_U2.writeGPIOB(0x00);

  // LDHL  pattern for steps 1-8: 0,1,1,1,1,1,1,0
  const bool ldhl[8] = {0, 1, 1, 1, 1, 1, 1, 0};
  // LDH   pattern for steps 1-8: 0,0,1,1,1,1,0,0
  const bool ldh[8]  = {0, 0, 1, 1, 1, 1, 0, 0};
  // D_ENL pattern for steps 1-8: 0,0,0,1,1,0,1,0
  const bool enl[8]  = {0, 0, 0, 1, 1, 0, 1, 0};
  // LDL   pattern for steps 1-8: 0,0,0,1,1,0,0,0
  const bool ldl[8]  = {0, 0, 0, 1, 1, 0, 0, 0};
  // 5Hz over 8 steps = 200ms per cycle = 25ms per step
  const int stepMs = 25;

  while (Serial.available()) Serial.read();

  for (int cycle = 1; cycle <= 12 && !Serial.available(); cycle++) {
    // even index (cycle 1,3,5... = index 0,2,4...) = all zeros; odd index = all ones
    uint8_t busVal = ((cycle - 1) % 2 == 0) ? 0xFF : 0x00;
    mcp_U3.writeGPIOB(busVal);  // ABUS_L
    mcp_U4.writeGPIOB(busVal);  // ABUS_H
    mcp_U2.writeGPIOB(busVal);  // DBUS
    Serial.print("Cycle "); Serial.print(cycle);
    Serial.println(busVal ? " BUS=0xFF (all 1s)" : " BUS=0x00 (all 0s)");

    mcp_U7.digitalWrite(12, LOW);  // idle (step 0)
    mcp_U7.digitalWrite(10, LOW);
    mcp_U7.digitalWrite(9,  LOW);
    mcp_U7.digitalWrite(8,  LOW);

    for (int step = 0; step < 8 && !Serial.available(); step++) {
      mcp_U7.digitalWrite(12, ldhl[step]);
      mcp_U7.digitalWrite(10, ldh[step]);
      mcp_U7.digitalWrite(9,  enl[step]);
      mcp_U7.digitalWrite(8,  ldl[step]);

      Serial.print("Cycle "); Serial.print(cycle); Serial.print("/12");
      Serial.print("  Step "); Serial.print(step + 1); Serial.print("/8");
      Serial.print("  LDHL="); Serial.print(ldhl[step]);
      Serial.print("  LDH=");  Serial.print(ldh[step]);
      Serial.print("  ENL=");  Serial.print(enl[step]);
      Serial.print("  LDL=");  Serial.println(ldl[step]);

      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Reg  Test 9   ");
      display.print(cycle); display.print("/12");
      display.setCursor(0, 12);
      display.print("Step "); display.print(step + 1); display.print("/8");
      display.setCursor(0, 24);
      display.print("LDHL="); display.print(ldhl[step]);
      display.print(" LDH="); display.print(ldh[step]);
      display.setCursor(0, 36);
      display.print("ENL=");  display.print(enl[step]);
      display.print("  LDL="); display.print(ldl[step]);
      display.display();

      delay(stepMs);
    }
  }

  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(8,  LOW);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
  mcp_U2.writeGPIOB(0x00);
  while (Serial.available()) Serial.read();
  Serial.println("LDHL=LOW LDH=LOW ENL=LOW LDL=LOW (idle)");
  Serial.println("Bus U2/U3/U4 = 0x00 (not-drive)");
  Serial.println("--- Test 9 Complete ---");
}

void runTest10_WriteR0() {
  Serial.println("\n--- Test A: 32 cycles, 8 states, ~10Hz ---");
  Serial.println("C1-8:   Dx walking, ENL=00111100, LDL=00011000");
  Serial.println("C9-16:  Dx walking, ENH=00111100, LDH=00011000");
  Serial.println("C17-32: Ax walking, LDHL=00011000");

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("ERROR: SSD1306 not found!");
    return;
  }
  if (!mcp_U2.begin_I2C(U2_ADDR, &Wire)) {
    Serial.println("ERROR: U2 not found!");
    return;
  }
  if (!mcp_U3.begin_I2C(U3_ADDR, &Wire)) {
    Serial.println("ERROR: U3 not found!");
    return;
  }
  if (!mcp_U4.begin_I2C(U4_ADDR, &Wire)) {
    Serial.println("ERROR: U4 not found!");
    return;
  }
  if (!mcp_U7.begin_I2C(U7_ADDR, &Wire)) {
    Serial.println("ERROR: U7 not found!");
    return;
  }

  // DBUS[7:0] = U2 GPB[0:7], ABUS_L[7:0] = U3 GPB[0:7], ABUS_H[15:8] = U4 GPB[0:7]
  for (int i = 0; i < 8; i++) mcp_U2.pinMode(8 + i, OUTPUT);
  for (int i = 0; i < 8; i++) mcp_U3.pinMode(8 + i, OUTPUT);
  for (int i = 0; i < 8; i++) mcp_U4.pinMode(8 + i, OUTPUT);
  mcp_U2.writeGPIOB(0x00);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);

  // U7 outputs: LDL=pin8, ENL=pin9, LDH=pin10, ENH=pin11, LDHL=pin12, ENHL=pin13
  mcp_U7.pinMode(8,  OUTPUT);
  mcp_U7.pinMode(9,  OUTPUT);
  mcp_U7.pinMode(10, OUTPUT);
  mcp_U7.pinMode(11, OUTPUT);
  mcp_U7.pinMode(12, OUTPUT);
  mcp_U7.pinMode(13, OUTPUT);
  mcp_U7.digitalWrite(8,  LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(11, LOW);
  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(13, LOW);

  // Shared patterns
  const bool sig[8] = {0, 0, 1, 1, 1, 1, 0, 0};  // ENL/ENH = 00111100
  const bool ld [8] = {0, 0, 0, 1, 1, 0, 0, 0};  // LDL/LDH = 00011000
  const bool dx [8] = {0, 1, 1, 1, 1, 1, 1, 0};  // Dx       = 01111110
  const int stepMs = 12;

  uint16_t abusMask = 0x0000;
  uint8_t dbusMask = 0x00;
  uint8_t controlMask = 0x00;

  while (Serial.available()) Serial.read();

  // Cycles 1-8: ENL + LDL
  for (int cycle = 1; cycle <= 8 && !Serial.available(); cycle++) {
    uint8_t dBit = cycle - 1;
    for (int step = 0; step < 8 && !Serial.available(); step++) {
      mcp_U2.writeGPIOB(dx[step] ? (1 << dBit) : 0x00);
      mcp_U7.digitalWrite(9, sig[step]);  // ENL
      mcp_U7.digitalWrite(8, ld[step]);   // LDL

      Serial.print("C"); Serial.print(cycle);
      Serial.print(" S"); Serial.print(step + 1);
      Serial.print("  D"); Serial.print(dBit); Serial.print("="); Serial.print(dx[step]);
      Serial.print("  ENL="); Serial.print(sig[step]);
      Serial.print("  LDL="); Serial.println(ld[step]);

      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Test A C"); display.print(cycle); display.print("/16 ENL");
      display.setCursor(0, 12);
      display.print("Step "); display.print(step + 1); display.print("/8");
      display.setCursor(0, 28);
      display.print("D"); display.print(dBit); display.print("="); display.println(dx[step]);
      display.setCursor(0, 38);
      display.print("ENL="); display.println(sig[step]);
      display.setCursor(0, 48);
      display.print("LDL="); display.println(ld[step]);
      display.display();

      delay(stepMs);
    }
  }

  // Cycles 9-16: ENH + LDH
  for (int cycle = 9; cycle <= 16 && !Serial.available(); cycle++) {
    uint8_t dBit = cycle - 9;
    for (int step = 0; step < 8 && !Serial.available(); step++) {
      mcp_U2.writeGPIOB(dx[step] ? (1 << dBit) : 0x00);
      mcp_U7.digitalWrite(11, sig[step]);  // ENH
      mcp_U7.digitalWrite(10, ld[step]);   // LDH

      Serial.print("C"); Serial.print(cycle);
      Serial.print(" S"); Serial.print(step + 1);
      Serial.print("  D"); Serial.print(dBit); Serial.print("="); Serial.print(dx[step]);
      Serial.print("  ENH="); Serial.print(sig[step]);
      Serial.print("  LDH="); Serial.println(ld[step]);

      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Test A C"); display.print(cycle); display.print("/16 ENH");
      display.setCursor(0, 12);
      display.print("Step "); display.print(step + 1); display.print("/8");
      display.setCursor(0, 28);
      display.print("D"); display.print(dBit); display.print("="); display.println(dx[step]);
      display.setCursor(0, 38);
      display.print("ENH="); display.println(sig[step]);
      display.setCursor(0, 48);
      display.print("LDH="); display.println(ld[step]);
      display.display();

      delay(stepMs);
    }
  }

  // Cycles 17-32: walk A0-A15 with LDHL pattern
  for (int cycle = 17; cycle <= 32 && !Serial.available(); cycle++) {
    uint8_t aBit = cycle - 17;  // 0-15
    // A0-A7 on U3, A8-A15 on U4
    uint8_t u3Val = (aBit < 8)  ? (1 << aBit)       : 0x00;
    uint8_t u4Val = (aBit >= 8) ? (1 << (aBit - 8)) : 0x00;

    for (int step = 0; step < 8 && !Serial.available(); step++) {
      mcp_U3.writeGPIOB(dx[step] ? u3Val : 0x00);
      mcp_U4.writeGPIOB(dx[step] ? u4Val : 0x00);
      mcp_U7.digitalWrite(12, ld[step]);   // LDHL
      mcp_U7.digitalWrite(13, sig[step]);  // ENHL

      Serial.print("C"); Serial.print(cycle);
      Serial.print(" S"); Serial.print(step + 1);
      Serial.print("  A"); Serial.print(aBit); Serial.print("="); Serial.print(dx[step]);
      Serial.print("  LDHL="); Serial.print(ld[step]);
      Serial.print("  ENHL="); Serial.println(sig[step]);

      display.clearDisplay();
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.print("Test A C"); display.print(cycle); display.print("/32 LDHL");
      display.setCursor(0, 12);
      display.print("Step "); display.print(step + 1); display.print("/8");
      display.setCursor(0, 28);
      display.print("A"); display.print(aBit); display.print("="); display.println(dx[step]);
      display.setCursor(0, 38);
      display.print("LDHL="); display.print(ld[step]);
      display.print(" ENHL="); display.println(sig[step]);
      display.display();

      delay(stepMs);
    }
  }

  // Idle
  mcp_U2.writeGPIOB(0x00);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
  mcp_U7.digitalWrite(8,  LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(11, LOW);
  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(13, LOW);

  while (Serial.available()) Serial.read();
  Serial.println("--- Test A Complete ---");
}

void runTestB_Interactive() {
  Serial.println("\n--- Test B: Interactive signal driver ---");
  Serial.println("Dyy   = DBUS low  byte hex (ENL+LDL)    e.g. D3F");
  Serial.println("Eyy   = DBUS high byte hex (ENH+LDH)    e.g. E80");
  Serial.println("Axxxx = ABUS 16-bit hex   (ENHL+LDHL)   e.g. A00FF");
  Serial.println("CL = clock zeros   Q = quit");

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  mcp_U2.begin_I2C(U2_ADDR, &Wire);
  mcp_U3.begin_I2C(U3_ADDR, &Wire);
  mcp_U4.begin_I2C(U4_ADDR, &Wire);
  mcp_U7.begin_I2C(U7_ADDR, &Wire);

  for (int i = 0; i < 8; i++) {
    mcp_U2.pinMode(8 + i, OUTPUT);
    mcp_U3.pinMode(8 + i, OUTPUT);
    mcp_U4.pinMode(8 + i, OUTPUT);
  }
  mcp_U7.pinMode(8,  OUTPUT);  // LDL
  mcp_U7.pinMode(9,  OUTPUT);  // ENL
  mcp_U7.pinMode(10, OUTPUT);  // LDH
  mcp_U7.pinMode(11, OUTPUT);  // ENH
  mcp_U7.pinMode(12, OUTPUT);  // LDHL
  mcp_U7.pinMode(13, OUTPUT);  // ENHL

  mcp_U2.writeGPIOB(0x00);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
  mcp_U7.digitalWrite(8,  LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(11, LOW);
  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(13, LOW);

  const bool dx [8] = {0, 1, 1, 1, 1, 1, 1, 0};  // 01111110
  const bool enx[8] = {0, 0, 1, 1, 1, 1, 0, 0};  // 00111100
  const bool ldx[8] = {0, 0, 0, 1, 1, 0, 0, 0};  // 00011000
  const int stepMs = 12;

  while (Serial.available()) Serial.read();

  while (true) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);  display.println("Test B: Interactive");
    display.setCursor(0, 10); display.println("Dyy  ENL+LDL");
    display.setCursor(0, 20); display.println("Eyy  ENH+LDH");
    display.setCursor(0, 30); display.println("Axxxx ENHL+LDHL");
    display.setCursor(0, 42); display.println("CL=zeros  Q=quit");
    display.setCursor(0, 54); display.println("Waiting...");
    display.display();

    Serial.print("\nCmd> ");

    // Read a line until newline
    String input = "";
    while (true) {
      if (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') break;
        input += c;
      }
    }
    input.trim();
    input.toUpperCase();

    if (input.length() == 0) continue;
    if (input == "Q") break;

    // CL — clock zeros into all registers
    if (input == "CL") {
      Serial.println("CL: clocking zeros into D-low, D-high, ABUS...");

      // Phase 1: DBUS=0, ENL+LDL
      mcp_U2.writeGPIOB(0x00);
      for (int step = 0; step < 8; step++) {
        mcp_U7.digitalWrite(9, enx[step]);  // ENL
        mcp_U7.digitalWrite(8, ldx[step]);  // LDL
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0, 0);
        display.println("CL 1/3  D=0 ENL+LDL");
        display.setCursor(0,14); display.print("Step "); display.print(step+1); display.println("/8");
        display.setCursor(0,28); display.print("ENL="); display.print(enx[step]);
        display.print("  LDL="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(9, LOW); mcp_U7.digitalWrite(8, LOW);

      // Phase 2: DBUS=0, ENH+LDH
      mcp_U2.writeGPIOB(0x00);
      for (int step = 0; step < 8; step++) {
        mcp_U7.digitalWrite(11, enx[step]);  // ENH
        mcp_U7.digitalWrite(10, ldx[step]);  // LDH
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0, 0);
        display.println("CL 2/3  D=0 ENH+LDH");
        display.setCursor(0,14); display.print("Step "); display.print(step+1); display.println("/8");
        display.setCursor(0,28); display.print("ENH="); display.print(enx[step]);
        display.print("  LDH="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(11, LOW); mcp_U7.digitalWrite(10, LOW);

      // Phase 3: ABUS=0, ENHL+LDHL
      mcp_U3.writeGPIOB(0x00);
      mcp_U4.writeGPIOB(0x00);
      for (int step = 0; step < 8; step++) {
        mcp_U7.digitalWrite(13, enx[step]);  // ENHL
        mcp_U7.digitalWrite(12, ldx[step]);  // LDHL
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0, 0);
        display.println("CL 3/3  A=0 ENHL+LDHL");
        display.setCursor(0,14); display.print("Step "); display.print(step+1); display.println("/8");
        display.setCursor(0,28); display.print("ENHL="); display.print(enx[step]);
        display.print(" LDHL="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(13, LOW); mcp_U7.digitalWrite(12, LOW);

      Serial.println("CL done.");
      display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
      display.setTextSize(2); display.setCursor(0, 20); display.println("CL Done");
      display.display();
      continue;
    }

    char cmd = input.charAt(0);
    String hexStr = input.substring(1);

    if (cmd == 'D' && hexStr.length() == 2) {
      uint8_t val = (uint8_t)strtoul(hexStr.c_str(), NULL, 16);
      Serial.print("D=0x"); Serial.println(val, HEX);
      for (int step = 0; step < 8; step++) {
        uint8_t bv = dx[step] ? val : 0x00;
        mcp_U2.writeGPIOB(bv);
        mcp_U7.digitalWrite(9, enx[step]);  // ENL
        mcp_U7.digitalWrite(8, ldx[step]);  // LDL
        Serial.print("s"); Serial.print(step+1);
        Serial.print("  DBUS U2(0x20) GPB=0x"); Serial.print(bv, HEX);
        Serial.print("  ENL U7(0x22) GPB1="); Serial.print(enx[step]);
        Serial.print("  LDL U7(0x22) GPB0="); Serial.println(ldx[step]);
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0,0);
        display.print("Test B  s"); display.print(step+1); display.println("/8");
        display.setTextSize(2); display.setCursor(0,12);
        display.print("D=0x"); display.println(val, HEX);
        display.setTextSize(1); display.setCursor(0,36);
        display.print("GPB=0x"); display.print(bv, HEX);
        display.print(" EN="); display.print(enx[step]);
        display.print(" LD="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(9, LOW); mcp_U7.digitalWrite(8, LOW);
      Serial.println("Done.");

    } else if (cmd == 'E' && hexStr.length() == 2) {
      uint8_t val = (uint8_t)strtoul(hexStr.c_str(), NULL, 16);
      Serial.print("E=0x"); Serial.println(val, HEX);
      for (int step = 0; step < 8; step++) {
        uint8_t bv = dx[step] ? val : 0x00;
        mcp_U2.writeGPIOB(bv);
        mcp_U7.digitalWrite(11, enx[step]);  // ENH
        mcp_U7.digitalWrite(10, ldx[step]);  // LDH
        Serial.print("s"); Serial.print(step+1);
        Serial.print("  DBUS U2(0x20) GPB=0x"); Serial.print(bv, HEX);
        Serial.print("  ENH U7(0x22) GPB3="); Serial.print(enx[step]);
        Serial.print("  LDH U7(0x22) GPB2="); Serial.println(ldx[step]);
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0,0);
        display.print("Test B  s"); display.print(step+1); display.println("/8");
        display.setTextSize(2); display.setCursor(0,12);
        display.print("E=0x"); display.println(val, HEX);
        display.setTextSize(1); display.setCursor(0,36);
        display.print("GPB=0x"); display.print(bv, HEX);
        display.print(" EN="); display.print(enx[step]);
        display.print(" LD="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(11, LOW); mcp_U7.digitalWrite(10, LOW);
      Serial.println("Done.");

    } else if (cmd == 'A' && hexStr.length() == 4) {
      uint16_t val = (uint16_t)strtoul(hexStr.c_str(), NULL, 16);
      Serial.print("A=0x"); Serial.println(val, HEX);
      for (int step = 0; step < 8; step++) {
        uint8_t u3Val = dx[step] ? (uint8_t)(val & 0xFF)        : 0x00;
        uint8_t u4Val = dx[step] ? (uint8_t)((val >> 8) & 0xFF) : 0x00;
        mcp_U3.writeGPIOB(u3Val);
        mcp_U4.writeGPIOB(u4Val);
        mcp_U7.digitalWrite(13, enx[step]);  // ENHL
        mcp_U7.digitalWrite(12, ldx[step]);  // LDHL - clocks high byte
        mcp_U7.digitalWrite(8,  ldx[step]);  // LDL  - clocks low byte
        Serial.print("s"); Serial.print(step+1);
        Serial.print("  ABSL U3(0x21) GPB=0x"); Serial.print(u3Val, HEX);
        Serial.print("  ABSH U4(0x23) GPB=0x"); Serial.print(u4Val, HEX);
        Serial.print("  ENHL U7(0x22) GPB5="); Serial.print(enx[step]);
        Serial.print("  LDHL U7(0x22) GPB4="); Serial.print(ldx[step]);
        Serial.print("  LDL U7(0x22) GPB0="); Serial.println(ldx[step]);
        display.clearDisplay(); display.setTextColor(SSD1306_WHITE);
        display.setTextSize(1); display.setCursor(0,0);
        display.print("Test B  s"); display.print(step+1); display.println("/8");
        display.setTextSize(2); display.setCursor(0,12);
        display.print("A=0x"); display.println(val, HEX);
        display.setTextSize(1); display.setCursor(0,36);
        display.print("L=0x"); display.print(u3Val, HEX);
        display.print(" H=0x"); display.print(u4Val, HEX);
        display.print(" EN="); display.print(enx[step]);
        display.print(" LD="); display.println(ldx[step]);
        display.display();
        delay(stepMs);
      }
      mcp_U7.digitalWrite(13, LOW); mcp_U7.digitalWrite(12, LOW); mcp_U7.digitalWrite(8, LOW);
      Serial.println("Done.");

    } else {
      Serial.println("Unknown. Use Dyy, Eyy, Axxxx, CL, Q");
    }
  }

  // Idle all on exit
  mcp_U2.writeGPIOB(0x00);
  mcp_U3.writeGPIOB(0x00);
  mcp_U4.writeGPIOB(0x00);
  mcp_U7.digitalWrite(8,  LOW);
  mcp_U7.digitalWrite(9,  LOW);
  mcp_U7.digitalWrite(10, LOW);
  mcp_U7.digitalWrite(11, LOW);
  mcp_U7.digitalWrite(12, LOW);
  mcp_U7.digitalWrite(13, LOW);
  Serial.println("--- Test B Complete ---");
}

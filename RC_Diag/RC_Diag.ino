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
    delay(1);
  }
  for (int b = 0; b < 6; b++) digitalWrite(LED_PINS[b], LOW);
  if (u7_ok) {
    mcp_U7.digitalWrite(0,  LOW);
    mcp_U7.digitalWrite(15, LOW);
    mcp_U7.digitalWrite(14, LOW);
    mcp_U7.digitalWrite(1,  LOW);
  }
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

    // Clear any remaining characters
    while(Serial.available()) Serial.read();

    // Show prompt again
    Serial.println("\nEnter test (0-LED, 1-I2C, 2-ABUS_L, 3-ABUS_H, 4-DBUS, 5-Misc, 6-OLED, 7-ALL): ");
  }
}

void runTest1_ESP32Communication() {
  Serial.println("\n--- Test 1: Full I2C Scan (GPIO0=SDA, GPIO1=SCL) ---");
  int deviceCount = 0;

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
    runTest6_SSD1306();
  }

  while (Serial.available()) Serial.read();
  Serial.println("\n--- Test 7 Stopped ---");
  Serial.print("Loops completed: "); Serial.println(run);
}

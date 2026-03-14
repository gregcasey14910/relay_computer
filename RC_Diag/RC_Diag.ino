// Relay Computer Test Suite
// Test 2: U2 Loopback Test - D_DBUS0 (GPB0) to W_DBUS0 (GPA7)
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
    Serial.print("Count: ");
    Serial.println(i);
    delay(40);
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

  runTest0_LEDCount();

  Serial.println("\n=== Relay Computer Test Suite ===");
  Serial.println("Enter test number to run:");
  Serial.println("0 - LED Count Test (GPIO5 LSB)");
  Serial.println("1 - I2C Full Scan");
  Serial.println("2 - U2 Loopback Test (GPB0 -> GPA7)");
  Serial.println("3 - SSD1306 OLED Test");
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
      runTest2_ImmediateLoopback();
    }
    else if (input == '3') {
      runTest3_SSD1306();
    }

    // Clear any remaining characters
    while(Serial.available()) Serial.read();
    
    // Show prompt again
    Serial.println("\nEnter test number (0-LED, 1-I2C Scan, 2-Loopback, 3-OLED): ");
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
      // Annotate known MCP23017 addresses
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

void runTest2_ImmediateLoopback() {
  Serial.println("\n--- Test 2: U2 Loopback Test ---");
  Serial.println("Testing D_DBUS0 (U2 GPB0) -> W_DBUS0 (U2 GPA7)");
  Serial.println("GPB0 = Output (Pin 8), GPA7 = Input (Pin 7)");
  Serial.println("Press any key to stop...\n");
  
  // Clear any existing serial data BEFORE starting the loop
  while(Serial.available()) Serial.read();
  delay(100);
  
  // Initialize U2
  if (!mcp_U2.begin_I2C(U2_ADDR, &Wire)) {
    Serial.println("ERROR: Failed to initialize U2 at 0x20!");
    Serial.println("Retrying after delay...");
    delay(100);
    if (!mcp_U2.begin_I2C(U2_ADDR, &Wire)) {
      Serial.println("ERROR: Second attempt failed!");
      return;
    }
  }
  Serial.println("U2 initialized successfully");
  delay(100);
  
  // Configure GPB0 as OUTPUT (D_DBUS0) - pin 8 in library numbering
  Serial.println("Configuring GPB0 (D_DBUS0) as OUTPUT...");
  mcp_U2.pinMode(8, OUTPUT);
  delay(50);
  Serial.println("  GPB0 configured as OUTPUT");
  
  // Configure GPA7 as INPUT (W_DBUS0) - pin 7 in library numbering
  Serial.println("Configuring GPA7 (W_DBUS0) as INPUT...");
  mcp_U2.pinMode(7, INPUT);
  delay(50);
  Serial.println("  GPA7 configured as INPUT\n");
  
  int cycleCount = 0;
  int passCount = 0;
  int failCount = 0;
  
  // Loop forever until key pressed
  while (!Serial.available()) {
    cycleCount++;
    
    // Test 1: Write HIGH to D_DBUS0
    mcp_U2.digitalWrite(8, HIGH);
    delay(100);
    
    // Read W_DBUS0
    bool bit7_high = mcp_U2.digitalRead(7);
    
    // Test 2: Write LOW to D_DBUS0
    mcp_U2.digitalWrite(8, LOW);
    delay(100);
    
    // Read W_DBUS0
    bool bit7_low = mcp_U2.digitalRead(7);
    
    // Check results
    bool high_pass = bit7_high;
    bool low_pass = !bit7_low;
    
    if (high_pass && low_pass) {
      passCount++;
      Serial.print("Cycle ");
      Serial.print(cycleCount);
      Serial.println(": PASS ✓");
    } else {
      failCount++;
      Serial.print("Cycle ");
      Serial.print(cycleCount);
      Serial.print(": FAIL ✗ (HIGH=");
      Serial.print(bit7_high ? "1" : "0");
      Serial.print(", LOW=");
      Serial.print(bit7_low ? "1" : "0");
      Serial.println(")");
    }
  }
  
  // Clear the serial buffer
  while(Serial.available()) Serial.read();
  
  // Print summary
  Serial.println("\n--- Test 2 Stopped ---");
  Serial.print("Total Cycles: ");
  Serial.println(cycleCount);
  Serial.print("Passes: ");
  Serial.println(passCount);
  Serial.print("Fails: ");
  Serial.println(failCount);
  if (cycleCount > 0) {
    Serial.print("Success Rate: ");
    Serial.print((passCount * 100) / cycleCount);
    Serial.println("%");
  }
  Serial.println("--- Test 2 Complete ---");
}

void runTest3_SSD1306() {
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
  Serial.println("--- Test 3 Complete ---");
}
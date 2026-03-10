// Master.ino - Main Entry Point
// ESP32 Relay Computer Emulator - Master Module (Refactored)
// Clean multi-file structure for maintainability


#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MCP23X17.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

// Project headers
#include "definitions.h"
#include "../common/mac_globals.cpp" 
#include "registers.h"
#include "control.h"
#include "display.h"
#include "sequencer.h"

// =====================================================================================================
// SETUP
// =====================================================================================================
void setup() {
  Serial.begin(115200);
  delay(3000);  // Wait for USB serial monitor to connect
  Serial.println("=== MASTER BOOTING ===");

  Wire.begin(0, 1);  // SDA = GPIO0, SCL = GPIO1
  delay(100);

  // Initialize ESP-NOW
  initESPNow();

  // Pin configuration
  pinMode(8, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(21, INPUT);
  pinMode(sel_pc_pin, OUTPUT);
  pinMode(ld_instr_pin, OUTPUT);
  pinMode(clock_pin, OUTPUT);

  digitalWrite(sel_pc_pin, LOW);
  digitalWrite(clock_pin, LOW);

  Wire.begin(0, 1);
  delay(100);
  
  // Initialize MCP23017 at address 0x21
  if (!mcp.begin_I2C(0x21)) {
    Serial.println("MCP23017 @ 0x21 not found! (continuing)");
  }
  for (int i = 0; i <= 15; i++) {
    mcp.pinMode(i, OUTPUT);
  }

  // Initialize MCP23017 at address 0x20
  if (!mcp0.begin_I2C(0x20)) {
    Serial.println("MCP23017 @ 0x20 not found! (continuing)");
  }
  for (int i = 0; i <= 15; i++) {
    mcp0.pinMode(i, OUTPUT);
  }

  // Initialize OLED display
  Serial.println(F("Starting..."));
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setTextSize(1);      
  display.setFont();
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Starting...");
  display.display();
  delay(10);
  Serial.println("Display initialized");  
}

// =====================================================================================================
// LOOP
// =====================================================================================================
void loop() {

  clock_gen();
  // if (state == 0) {
  //   Serial.println("===================================================================================================================================================================================================================");
  // }
  // bus_trace();

  sequencer();

  // Phase 0: SELECT operations
  incr_inc(0);
  ctrl_pc_ir(0);
  MEMORY(0);
  reg_INST(0, 1);   // Type 1 AD
  reg_INST(0, 2);   // Type 2 BC
  reg_INST(0, 3);   // Type 3 M Reg
  reg_INST(0, 4);   // Type 4 XY Reg
  reg_INST(0, 5);   // Type 5 J Reg
  alu(0);           // Type 9 ALU
 
  // Phase 1: LOAD operations
  incr_inc(1);
  ctrl_pc_ir(1);
  MEMORY(1);
  reg_INST(1, 1);   // Type 1 AD
  reg_INST(1, 2);   // Type 2 BC
  reg_INST(1, 3);   // Type 3 M Reg
  reg_INST(1, 4);   // Type 4 XY Reg
  reg_INST(1, 5);   // Type 5 J Reg
  alu(1);           // Type 9 ALU

  // delay(100);
  show_screen();

  // TEST: Stop when INCR reaches 0x02
  // if (varValues[INCR] == 0x02) {
  //   Serial.println("*** INCR=0x02 REACHED - HALTING ***");
  //   while(1) {
  //     delay(1000);  // Infinite loop - halt execution
  //   }
  // }

}

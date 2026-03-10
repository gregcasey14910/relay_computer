// display.cpp - Display and Trace Implementation

#include "display.h"
#include "definitions.h"
#include "sequencer.h"

// =====================================================================================================
// OLED DISPLAY FUNCTIONS
// =====================================================================================================
void drawStateDot(uint8_t state) {
  if (state > 23) return;
  const uint8_t dotSize = 3;
  const uint8_t numSlots = 24;
  const uint8_t screenWidth = 128;
  const uint8_t yPos = 64 - dotSize;
  float slotWidth = (float)screenWidth / numSlots;
  int xCenter = round(state * slotWidth + slotWidth / 2);
  int xPos = xCenter - dotSize / 2;
  display.fillRect(xPos, yPos, dotSize, dotSize, SSD1306_WHITE);
}

void show_screen() {
  display.clearDisplay();
  display.setCursor(0, 0);

  char lineBuf[32];

  for (int row = 0; row < 6; row++) {
    int leftIdx  = row;
    int rightIdx = row + 6;

    const char* leftName  = (leftIdx  < VarName_Count) ? varNames[leftIdx]  : "----";
    const char* rightName = (rightIdx < VarName_Count) ? varNames[rightIdx] : "----";

    uint16_t leftVal  = (leftIdx  < VarName_Count) ? varValues[leftIdx]  : 0;
    uint16_t rightVal = (rightIdx < VarName_Count) ? varValues[rightIdx] : 0;

    // For the first 4 left-side registers (A,B,C,D), show only LSB
    if (leftIdx < 4) {
      snprintf(lineBuf, sizeof(lineBuf),
               "%-4s   %02X | %-4s %04X",
               leftName, (uint8_t)(leftVal & 0xFF),
               rightName, rightVal);
    } else {
      snprintf(lineBuf, sizeof(lineBuf),
               "%-4s %04X | %-4s %04X",
               leftName, leftVal,
               rightName, rightVal);
    }

    display.println(lineBuf);
  }

  // Final line: show Incr (index 12) alone
  if (INCR < VarName_Count) {
    snprintf(lineBuf, sizeof(lineBuf),
             "%-4s %04X",
             varNames[INCR], varValues[INCR]);
    display.println(lineBuf);
  }

  drawStateDot(state);
  display.display();
}

void show_leds(uint8_t state) {
  uint8_t b = state;
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4; // Swap nibbles
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2; // Swap bit pairs
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1; // Swap individual bits
  b = 0;  // CLEAR THEM FOR DEBUG
  mcp.writeGPIOAB(b); 
}

// =====================================================================================================
// SERIAL TRACE FUNCTIONS
// =====================================================================================================
void individual_bus(int load, int reg, int sel) {
  if (get_Tbits(load, state) == 1) {
    Serial.print(BitFieldNames[load]);
    Serial.print(">");
  } else {
    Serial.print("    ");
  }
  Serial.print(varNames[reg]);
  Serial.print("=");
  Serial.printf("%04X", varValues[reg]);
  if (get_Tbits(sel, state) == 1) {
    Serial.print(">");
    Serial.print(BitFieldNames[sel]);
  } else {
    Serial.print("    ");
  }
  Serial.print(" ");   
}

void individual_2bus(int load, int reg, int sel) {
  Serial.print(get_Tbits(load, state) ? BitFieldNames[load] : "   ");
  Serial.print(get_Tbits(load, state) ? ">" : " ");
  Serial.print(varNames[reg]);
  Serial.print("=");
  Serial.printf("%02X", varValues[reg] & 0xFF);
  Serial.print(get_Tbits(sel, state) ? ">" + String(BitFieldNames[sel]) : "    ");
  Serial.print(" ");
}

void individual_3bus(int load1, int load2, int load3, int reg, int sel1, int sel2, int sel3) {
  int blank = 0;
  int v = 0;
  if (get_Tbits(load1, state) == 1) { 
    v = load1;
    blank = blank + 1;
  }
  if (get_Tbits(load2, state) == 1) { 
    v = load2;
    blank = blank + 1;
  }
  if (get_Tbits(load3, state) == 1) { 
    v = load3;
    blank = blank + 1;
  }
  if (blank == 1) {
    Serial.print(BitFieldNames[v]);
    Serial.print(">");
  } 
  if (blank > 1) {
    Serial.print("????");
  }
  if (blank < 1) {
    Serial.print("    ");
  }
  if (reg == 13) {  // Special Memory Case
    Serial.print("Mem ");
    Serial.print("=");
    Serial.printf("%02X", memArray[varValues[Ad_B]]);
  }  
  else {
    Serial.print(varNames[reg]);
    Serial.print("=");
    Serial.printf("%02X", varValues[reg]);
  }
  blank = 0;
  v = 0;
  if (get_Tbits(sel1, state) == 1) { 
    v = sel1;
    blank = blank + 1;
  }
  if (get_Tbits(sel2, state) == 1) { 
    v = sel2;
    blank = blank + 1;
  }
  if (get_Tbits(sel3, state) == 1) { 
    v = sel3;
    blank = blank + 1;
  }
  if (blank == 1) {
    Serial.print(">");
    Serial.print(BitFieldNames[v]);
  } 
  if (blank > 1) {
    Serial.print("????");
  }
  if (blank < 1) {
    Serial.print("    ");
  }
}

void bus_trace() {
  Serial.print("ST=");
  Serial.printf("%02d ", state);
  Serial.print("ZCS=");
  Serial.print(if_z() ? '1' : '0');
  Serial.print(if_c() ? '1' : '0');
  Serial.print(if_s() ? '1' : '0');
  Serial.print(" ");
  if (is_asserted(INC)) Serial.print("2");
  else Serial.print(" ");
  if (is_asserted(IALU)) Serial.print("1");
  else Serial.print(" ");  
  
  individual_bus(LPC, PC_R, SPC);
  
  int idx = decode_opcode_index(varValues[INST] & 0xFF);
  Serial.print("Mn=");
  if (idx >= 0) Serial.printf("%-6s ", inst_mnemonic[idx]);
  else          Serial.print("UNKN  ");

  individual_bus(LIC, INCR, SIC);
  individual_2bus(LIN, INST, I2B);
  
  Serial.print("AB=");
  Serial.printf("%04X ", varValues[Ad_B]);
  
  Serial.print("DB=");
  Serial.printf("%02X ", (uint8_t)(varValues[Da_B] & 0xFF));

  individual_2bus(RLA, A_Rg, RSA);  // Changed from individual_8bus2
  individual_2bus(RLB, B_Rg, RSB);
  individual_2bus(RLC, C_Rg, RSC);
  individual_2bus(RLD, D_Rg, RSD);
  individual_3bus(LM1, LM2, -1, M_Rg, SM1, SM2, SEM);
  individual_3bus(LDX, LDY, LXY, XY_R, SEX, SEY, SXY);
  individual_3bus(LJ1, LJ2, -1, J_Rg, SEJ, -2, -3);
  individual_3bus(MEW, memArray[Ad_B], MER, 13, -1, -2, -3);

  Serial.println();
}

// =====================================================================================================
// HELPER FUNCTIONS
// =====================================================================================================
uint8_t who_is_drv_DBUS() {
  const char* drivers[] = { "RSA", "RSB", "RSC", "RSD", "SM1", "SM2", "SEX", "SEY", "I2B", "MER" };
  const uint8_t signals[] = { RSA, RSB, RSC, RSD, SM1, SM2, SEX, SEY, I2B, MER };
  const uint8_t numDrivers = sizeof(drivers) / sizeof(drivers[0]);
  uint8_t matchCount = 0;
  uint8_t matchIndex = 0;
  for (uint8_t i = 0; i < numDrivers; i++) {
    if (get_Tbits(signals[i], state) == 1) {
      matchCount++;
      matchIndex = i;
    }
  }
  if (matchCount == 1) {
    strcpy(rem_name, drivers[matchIndex]);
    return matchIndex + 1;
  }
  if (matchCount == 0) {
    strcpy(rem_name, "___");
    return 255;
  }
  strcpy(rem_name, "XXX");
  return 10;
}

uint8_t who_is_drv_ABUS() {
  const char* drivers[] = { "SIC", "SPC", "SEJ", "SXY", "SEM" };
  const uint8_t signals[] = { SIC, SPC, SEJ, SXY, SEM };
  const uint8_t numDrivers = sizeof(drivers) / sizeof(drivers[0]);
  uint8_t matchCount = 0;
  uint8_t matchIndex = 0;
  for (uint8_t i = 0; i < numDrivers; i++) {
    if (get_Tbits(signals[i], state) == 1) {
      matchCount++;
      matchIndex = i;
    }
  }
  if (matchCount == 1) {
    strcpy(rem_name, drivers[matchIndex]);
    return matchIndex + 1;
  }
  if (matchCount == 0) {
    strcpy(rem_name, "___");
    return 255;
  }
  strcpy(rem_name, "XXX");
  return 10;
}

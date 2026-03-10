// globals.cpp - Global Variable Definitions
// All extern declarations from definitions.h are defined here

#include "definitions.h"

// =====================================================================================================
// HARDWARE OBJECTS
// =====================================================================================================
Adafruit_MCP23X17 mcp;
Adafruit_MCP23X17 mcp0;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
unsigned long loopCount = 0;

// =====================================================================================================
// PIN ASSIGNMENTS
// =====================================================================================================
uint8_t sel_pc_pin = 4;
uint8_t ld_instr_pin = 3;
uint8_t state = -1;
uint8_t clock_pin = 2;
int clck = 1;
int rest_time = 2;
int state_time = 1;

// =====================================================================================================
// MODULE EMULATION FLAGS
// =====================================================================================================
uint16_t emulate_type = 0b0000001111110000;
//                             ba987654321
uint16_t remote_type  = 0b0000010000001111;
uint8_t HALT = 0;

// =====================================================================================================
// GLOBAL STATE
// =====================================================================================================
int counter = 0;
unsigned long waitStartTime = 0;
bool receivedFromStarter = false;
char rem_name[4];

// =====================================================================================================
// SIGNAL STATE TRACKING
// =====================================================================================================
SignalState prevSig = {0};

// =====================================================================================================
// PEER-TO-PEER STATE
// =====================================================================================================
volatile P2PMessage pending_response;
volatile bool response_received = false;
volatile uint8_t expected_seq = 0;
uint8_t next_seq_num = 0;

// =====================================================================================================
// REGISTER VALUES
// =====================================================================================================
uint16_t varValues[13] = {
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

// =====================================================================================================
// MEMORY ARRAY
// =====================================================================================================
uint8_t memArray[128] = {
  0x41, 0x08, 0x42, 0x10, 0x8B, 0x43, 0x44, 0x45,
  0x46, 0x47, 0x48, 0x49, 0x4A, 0x08, 0x4B, 0x10,
  0x89, 0x4C, 0x4D, 0x4E, 0x4F, 0x20, 0x29, 0x32, 0x3b, 0xe0, 0x12, 0x34, 0xE6, 0x00, 0x00
  // Remaining 104 elements default to 0
};

// LDI A, 1      ; Load 1 into A
// MOV A, B      ; Copy A to B (B = 1)
// LDI A, 2      ; Load 2 into A
// MOV B, A      ; Copy B to A (A = 1)
// ADD A         ; A = A + B = 1 + 1 = 2
// LDI A, 3      ; Load 3 into A
// LDI A, 4      ; Load 4 into A
// LDI A, 5      ; Load 5 into A
// LDI A, 6      ; Load 6 into A
// LDI A, 7      ; Load 7 into A
// LDI A, 8      ; Load 8 into A
// LDI A, 9      ; Load 9 into A
// LDI A, 10     ; Load 10 into A
// MOV A, B      ; Copy A to B (B = 10)
// LDI A, 11     ; Load 11 into A
// MOV B, A      ; Copy B to A (A = 10)
// ADD           ; A = A + B = 10 + 11 = 21
// LDI A, 12     ; Load 12 into A
// LDI A, 13     ; Load 13 into A
// LDI A, 14     ; Load 14 into A
// LDI A, 15     ; Load 15 into A
// mov m1, a
// mov m2, b
// mov x,c
// mov y,d
// ldi j,0x1234
// JMP 0x00        ; Jump back to address 0x00 (infinite loop)



// =====================================================================================================
// REGISTER DESCRIPTOR ARRAY
// =====================================================================================================
RegisterDescriptor reg8bit[] = {
  {RSA, RLA, A_Rg, em_AReg, regA_MAC, "A_Rg"},
  {RSB, RLB, B_Rg, em_BReg, regB_MAC, "B_Rg"},
  {RSC, RLC, C_Rg, em_CReg, regC_MAC, "C_Rg"},
  {RSD, RLD, D_Rg, em_DReg, regD_MAC, "D_Rg"}
};


const char* inst_category[] = {
  "MOV-8", "SETAB", "ALU", "ALU", "ALU", "ALU", "ALU", "ALU",
  "MOV-16", "MOV-16", "MOV-16", "LOAD", "STORE", "INC-XY",
  "GOTO", "GOTO", "GOTO", "GOTO", "GOTO"
};

const char* inst_mnemonic[] = {
  "mov", "ldi", "add", "and", "clr", "eor", "not", "rol",
  "hlt", "bx", "movd", "ldr", "str", "ixy",
  "lwi", "b", "bl", "bcc", "blcc", "rts", "inc", "orr"
};

const char* inst_pattern[] = {
  "00dddsss", "01diiiii", "10000001", "10000011", "1000d111", "10000100", "10000101", "10000110",
  "10101110", "10101010", "1010dss0", "100100dd", "100110ss", "10110000",
  "11000000", "11100110", "11100111", "111cccc0", "111cccc1"
};

const uint8_t inst_count = sizeof(inst_pattern) / sizeof(inst_pattern[0]);

const char* varNames[VarName_Count] = {
  "A Rg", "B Rg", "C Rg", "D Rg", "M Rg",
  "XY R", "J Rg", "PC_R", "INST", "Ad B",
  "Da B", "Flgs", "INCR"
};

uint8_t conn_loc[68] = {
    0b00000000, 0b00000001, 0b00000010, 0b00000011, 0b00000100, 0b00000101, 0b00000110, 0b00000111, 0b00001000,
    0b00100000, 0b00100001, 0b00100010, 0b00100011, 0b00100100, 0b00100101, 0b00100110, 0b00100111, 0b00101000,
    0b00101001, 0b00101010, 0b00101011, 0b00101100,
    0b01000000, 0b01000001, 0b01000010, 0b01000011, 0b01000100, 0b01000101, 0b01000110, 0b01000111, 0b01001000,
    0b01001001, 0b01001010, 0b01001011, 0b01001100, 0b01001101,
    0b01100000, 0b01100001, 0b01100010, 0b01100011, 0b01100100, 0b01100101, 0b01100110, 0b01100111, 0b01101000,
    0b01101001, 0b01101010, 0b01101011, 0b01101100, 0b01101101, 0b01101110, 0b01101111,
    0b10000000, 0b10000001, 0b10000010, 0b10000011, 0b10000100, 0b10000101, 0b10000110, 0b10000111, 0b10001000,
    0b10001001, 0b10001010, 0b10001011, 0b10001100, 0b10001101, 0b10001110, 0b10001111
};

uint8_t tbits[68][3] = {
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
    {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}
};

// =====================================================================================================
// UTILITY FUNCTIONS
// =====================================================================================================
bool is_t(int x) {
  int y = emulate_type;
  if(((y >> (x-1)) & 0x0001) == 1) {
    return(true);
  } else {
    return(false);
  }
}

int sigNameToIndex(const char* name) {
  for (int i = 0; i < sizeof(BitFieldNames)/sizeof(BitFieldNames[0]); i++) {
    if (strcmp(BitFieldNames[i], name) == 0) {
      return i;
    }
  }
  return -1;
}

void assignVarValue(uint8_t name, uint16_t value) {
  if (name < VarName_Count) {
    varValues[name] = value;
  }
}

bool is_asserted(uint8_t signal_name) {
  if (state < 0 || state >= 24) return false;
  return get_Tbits(signal_name, (uint8_t)state) == 1;
}

int get_Tbits(uint8_t op_code_index, uint8_t state) {
  if (op_code_index >= 68 || state >= 24) return -1;

  uint8_t column = state / 8;
  uint8_t bit_index = 7 - (state % 8);

  return (tbits[op_code_index][column] >> bit_index) & 0x01;
}

uint8_t chk_reg_type(int type) {
  int bit_index = type - 1;
  
  if (emulate_type & (1 << bit_index)) {
    return 0;
  }
  
  if (remote_type & (1 << bit_index)) {
    return 1;
  }
  
  return 2;
}

// Flag manipulation
void set_z(bool z) {
  if (z)
    varValues[Flgs] |= (1 << 2);
  else
    varValues[Flgs] &= ~(1 << 2);
}

bool if_z(void) {
  return (varValues[Flgs] & (1 << 2)) != 0;
}

void set_c(bool c) {
  if (c)
    varValues[Flgs] |= (1 << 1);
  else
    varValues[Flgs] &= ~(1 << 1);
}

bool if_c(void) {
  return (varValues[Flgs] & (1 << 1)) != 0;
}

void set_s(bool s) {
  if (s)
    varValues[Flgs] |= (1 << 0);
  else
    varValues[Flgs] &= ~(1 << 0);
}

bool if_s(void) {
  return (varValues[Flgs] & (1 << 0)) != 0;
}

void debug_print_reg(uint8_t reg_index) {
  if (reg_index < VarName_Count) {
    Serial.print(varNames[reg_index]);
    Serial.print(" = 0x");
    Serial.println(varValues[reg_index], HEX);
  }
}

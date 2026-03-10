// definitions.h - Constants, Enums, Structs, and Global Variables
// ESP32 Relay Computer Emulator - Master Module

#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_SSD1306.h>

// =====================================================================================================
// PEER-TO-PEER ESP-NOW COMMON DEFINITIONS
// =====================================================================================================
#include "../common/P2P_Signals.h"  // Provides BitField enum and MAC addresses for all modules

// Which device am I?
#define MY_DEVICE_IDX  IDX_MASTER

// =====================================================================================================
// CONFIGURATION
// =====================================================================================================
#define IS_STARTER true
#define LED_PIN 8
#define REMOTE_SELECT_DELAY 10

// =====================================================================================================
// DISPLAY CONFIGURATION
// =====================================================================================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// =====================================================================================================
// PIN DEFINITIONS
// =====================================================================================================
extern uint8_t sel_pc_pin;
extern uint8_t ld_instr_pin;
extern uint8_t state;
extern uint8_t clock_pin;
extern int clck;
extern int rest_time;
extern int state_time;

// =====================================================================================================
// MODULE TYPE EMULATION FLAGS
// =====================================================================================================
extern uint16_t emulate_type;
extern uint16_t remote_type;
extern uint8_t HALT;

// Helper function for emulation type checking
bool is_t(int x);

// =====================================================================================================
// ENUMS
// =====================================================================================================
enum EmulateType {
  em_AReg   = 0x1,   // A Reg   - Module_type 1 (AD Reg)
  em_BReg   = 0x2,   // B Reg   - Module_type 2 (BC Reg)
  em_CReg   = 0x3,   // C Reg   - Module_type 2 (BC Reg)
  em_DReg   = 0x4,   // D Reg   - Module_type 1 (AD Reg)
  em_MReg   = 0x5,   // M Reg   - Module_type 3 (M Reg)
  em_XYReg  = 0x6,   // XY Reg  - Module_type 4 (XY Reg)
  em_JReg   = 0x7,   // J Reg   - Module_type 5 (J Reg)
  em_CPCIR  = 0x8,   // CPCIR   - Module_type 6 (CPCIR Reg)
  em_Incr   = 0x9,   // Incr    - Module_type 7 (Incr)
  em_Mem    = 0xA,   // Mem     - Module_type 8 (Mem)
  em_ALU    = 0xB    // ALU     - Module_type 9 (ALU)
};

// BitField enum is defined in P2P_Signals.h and shared by all modules

enum VarName {
  A_Rg,   // 0
  B_Rg,   // 1
  C_Rg,   // 2
  D_Rg,   // 3
  M_Rg,   // 4
  XY_R,   // 5
  J_Rg,   // 6
  PC_R,   // 7
  INST,   // 8 
  Ad_B,   // 9
  Da_B,   // 10
  Flgs,   // 11
  INCR,   // 12
  VarName_Count
};

// =====================================================================================================
// STRUCTS
// =====================================================================================================
// Generic register descriptor
typedef struct {
  uint8_t select_signal;
  uint8_t load_signal;
  uint8_t reg_var_idx;
  uint8_t em_type;
  uint8_t* mac_addr;
  const char* reg_name;
} RegisterDescriptor;

// Signal state tracking
struct SignalState {
  uint8_t RSA, RLA, RSD, RLD;
  uint8_t RSB, RLB, RSC, RLC;
  uint8_t SM1, SM2, SEM, LM1, LM2;
  uint8_t SEX, SEY, SXY, LDX, LDY, LXY;
  uint8_t SEJ, LJ1, LJ2;
  uint8_t SPC, SIC, I2B, LPC;
  uint8_t IALU_prev, FCT, B2A, C2A;
};

// =====================================================================================================
// GLOBAL VARIABLE DECLARATIONS (defined in globals.cpp)
// =====================================================================================================
extern Adafruit_MCP23X17 mcp;
extern Adafruit_MCP23X17 mcp0;
extern Adafruit_SSD1306 display;
extern unsigned long loopCount;

extern int counter;
extern unsigned long waitStartTime;
extern bool receivedFromStarter;
extern char rem_name[4];

extern SignalState prevSig;

extern uint16_t varValues[13];
extern const char* varNames[VarName_Count];
extern uint8_t memArray[128];

extern RegisterDescriptor reg8bit[4];

// BitFieldNames array is defined in P2P_Signals.h

extern const char* inst_category[];
extern const char* inst_mnemonic[];
extern const char* inst_pattern[];
extern const uint8_t inst_count;

extern uint8_t tbits[68][3];
extern uint8_t conn_loc[68];

// =====================================================================================================
// UTILITY FUNCTIONS
// =====================================================================================================
int sigNameToIndex(const char* name);
int get_Tbits(uint8_t op_code_index, uint8_t state);
void assignVarValue(uint8_t name, uint16_t value);
bool is_asserted(uint8_t signal_name);
void broadcastOnChange(uint8_t signal, uint8_t currentState, uint8_t* prevState);

// Flag manipulation functions
void set_z(bool z);
bool if_z(void);
void set_c(bool c);
bool if_c(void);
void set_s(bool s);
bool if_s(void);

// ESP-NOW communication variables (needed by common/espnow_comms.cpp)
extern volatile P2PMessage pending_response;
extern volatile bool response_received;
extern volatile uint8_t expected_seq;
extern uint8_t next_seq_num;

// Include common espnow functions
#include "../common/espnow_comms.h"

#endif // DEFINITIONS_H

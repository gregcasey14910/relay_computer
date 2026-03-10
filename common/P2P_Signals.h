// P2P_Signals.h
// Common definitions for peer-to-peer ESP-NOW communication
// Shared between Master and all remote modules

#ifndef P2P_SIGNALS_H
#define P2P_SIGNALS_H

#include <Arduino.h>

// =====================================================================================================
// DEVICE INDICES
// =====================================================================================================
#define IDX_MASTER   0
#define IDX_REG_A    1
#define IDX_ALU      2
#define IDX_CYD_MON  3
#define IDX_REG_B    4   // Register B (when remote)
#define IDX_REG_C    5   // Register C (when remote)
#define IDX_REG_D    6   // Register D (when remote)

#define MAX_DEVICES  12

// =====================================================================================================
// MESSAGE TYPES
// =====================================================================================================
#define MSG_REQUEST   0x01
#define MSG_RESPONSE  0x02
#define MSG_BROADCAST 0x03

// =====================================================================================================
// TIMEOUT VALUES
// =====================================================================================================
#define REQUEST_TIMEOUT_MS  100   // Timeout for register requests
#define ALU_TIMEOUT_MS      200   // Timeout for ALU operations (may need more time)

// =====================================================================================================
// P2P MESSAGE STRUCTURE
// =====================================================================================================
typedef struct {
  uint8_t msg_type;    // MSG_REQUEST, MSG_RESPONSE, or MSG_BROADCAST
  uint8_t seq_num;     // Sequence number for matching requests/responses
  uint8_t signal;      // Which signal (RSA, RLA, ALU operation, etc.)
  uint8_t value;       // 8-bit data value
  uint16_t data16;     // 16-bit data (for address bus, etc.)
  uint8_t src_idx;     // Source device index
  uint8_t dst_idx;     // Destination device index (0xFF = broadcast)
} P2PMessage;

// =====================================================================================================
// MAC ADDRESSES
// =====================================================================================================
// MAC addresses for your actual hardware
extern uint8_t device_MACs[MAX_DEVICES][6];

// MAC address shortcuts for easy access
#define regA_MAC device_MACs[IDX_REG_A]
#define regB_MAC device_MACs[IDX_REG_B]
#define regC_MAC device_MACs[IDX_REG_C]
#define regD_MAC device_MACs[IDX_REG_D]
#define alu_MAC device_MACs[IDX_ALU]
#define cyd_MAC device_MACs[IDX_CYD_MON]

// Broadcast address
extern uint8_t broadcast_MAC[6];

// =====================================================================================================
// SIGNAL DEFINITIONS - BitField enum used by ALL modules
// =====================================================================================================
enum BitField {
    F0    = 0,
    F1    = 1,
    F2    = 2,
    CL    = 3,
    NZ    = 4,
    EZ    = 5,
    CY    = 6,
    SN    = 7,
    LXY   = 8,

    frz   = 9,
    Hlt   = 10,
    RSTb  = 11,
    LIN   = 12,
    LIC   = 13,
    LPC   = 14,
    CLK   = 15,
    CLS   = 16,
    RSTa  = 17,
    Reset = 18,
    I2B   = 19,
    SIC   = 20,
    SPC   = 21,

    AT14  = 22,
    AT12  = 23,
    AT10  = 24,
    AT08  = 25,
    RSET  = 26,
    IM16  = 27,
    IMSC  = 28,
    ILOD  = 29,
    ISTR  = 30,
    IALU  = 31,
    INCB  = 32,
    IMV8  = 33,
    ISET  = 34,
    IGTP  = 35,

    SEJ   = 36,
    SEY   = 37,
    SEX   = 38,
    SXY   = 39,
    SM2   = 40,
    SM1   = 41,
    SEM   = 42,
    MEW   = 43,
    MER   = 44,
    B2M   = 45,
    LJ2   = 46,
    LJ1   = 47,
    LDY   = 48,
    LDX   = 49,
    LM2   = 50,
    LM1   = 51,

    RSD   = 52,
    RSC   = 53,
    RSB   = 54,
    RSA   = 55,
    RLD   = 56,
    RLC   = 57,
    RLB   = 58,
    RLA   = 59,
    ICY   = 60,
    SHL   = 61,
    NOT   = 62,
    XOR   = 63,
    ORR   = 64,
    AND   = 65,
    INC   = 66,
    ADD   = 67
};

// Virtual bus signals - ALWAYS defined (not part of BitField enum)
#define DBUS      100
#define ABUS      101
#define ALU_OUT   102

// ALU communication signals - ALWAYS defined
#define FCT_ALU   110
#define B_2_ALU   111
#define C_2_ALU   112
#define HCT_MS    113  // 74HCT181 M and S bits
#define ALU_SCZ   114  // ALU Sign, Carry, Zero flags

// Register broadcast signals - ALWAYS defined (for CYD Monitor)
#define REG_PC    120  // Program Counter register value
#define REG_INST  121  // Instruction Register value (IR/IN)
#define REG_M     122  // M Register value
#define REG_XY    123  // XY Register value
#define REG_J     124  // J Register value
#define REG_INCR  125  // Incrementer register value (NEW)

// System commands - ALWAYS defined
#define SIGNAL    200
#define PING      201

// =====================================================================================================
// HELPER FUNCTIONS
// =====================================================================================================

// Get device index from MAC address
inline uint8_t getMACIndex(const uint8_t* mac) {
  for (int i = 0; i < MAX_DEVICES; i++) {
    if (memcmp(mac, device_MACs[i], 6) == 0) {
      return i;
    }
  }
  return 0xFF; // Not found
}

// Get device name from index
inline const char* getDeviceName(uint8_t idx) {
  switch(idx) {
    case IDX_MASTER:   return "Master";
    case IDX_REG_A:    return "Reg_A";
    case IDX_REG_B:    return "Reg_B";
    case IDX_REG_C:    return "Reg_C";
    case IDX_REG_D:    return "Reg_D";
    case IDX_ALU:      return "ALU";
    case IDX_CYD_MON:  return "CYD_Mon";
    default:           return "Unknown";
  }
}

// BitFieldNames array - signal names for display/debug
static const char* BitFieldNames[68] = {
    "F0",    // 0
    "F1",    // 1
    "F2",    // 2
    "CL",    // 3
    "NZ",    // 4
    "EZ",    // 5
    "CY",    // 6
    "SN",    // 7
    "LXY",   // 8

    "frz",   // 9
    "Hlt",   // 10
    "RSTb",  // 11
    "LIN",   // 12
    "LIC",   // 13
    "LPC",   // 14
    "CLK",   // 15
    "CLS",   // 16
    "RSTa",  // 17
    "Reset", // 18
    "I2B",   // 19
    "SIC",   // 20
    "SPC",   // 21

    "AT14",  // 22
    "AT12",  // 23
    "AT10",  // 24
    "AT08",  // 25
    "RSET",  // 26
    "IM16",  // 27
    "IMSC",  // 28
    "ILOD",  // 29
    "ISTR",  // 30
    "IALU",  // 31
    "INCB",  // 32
    "IMV8",  // 33
    "ISET",  // 34
    "IGTP",  // 35

    "SEJ",   // 36
    "SEY",   // 37
    "SEX",   // 38
    "SXY",   // 39
    "SM2",   // 40
    "SM1",   // 41
    "SEM",   // 42
    "MEW",   // 43
    "MER",   // 44
    "B2M",   // 45
    "LJ2",   // 46
    "LJ1",   // 47
    "LDY",   // 48
    "LDX",   // 49
    "LM2",   // 50
    "LM1",   // 51

    "RSD",   // 52
    "RSC",   // 53
    "RSB",   // 54
    "RSA",   // 55
    "RLD",   // 56
    "RLC",   // 57
    "RLB",   // 58
    "RLA",   // 59
    "ICY",   // 60
    "SHL",   // 61
    "NOT",   // 62
    "XOR",   // 63
    "ORR",   // 64
    "AND",   // 65
    "INC",   // 66
    "ADD"    // 67
};

// Get signal name from signal code
inline const char* getSignalName(uint8_t signal) {
  // BitField signals
  if (signal < 68) return BitFieldNames[signal];
  
  // Virtual signals
  switch(signal) {
    case DBUS:      return "DBUS";
    case ABUS:      return "ABUS";
    case ALU_OUT:   return "ALU_OUT";
    case FCT_ALU:   return "FCT_ALU";
    case B_2_ALU:   return "B_2_ALU";
    case C_2_ALU:   return "C_2_ALU";
    case HCT_MS:    return "HCT_MS";
    case ALU_SCZ:   return "ALU_SCZ";
    case REG_PC:    return "REG_PC";
    case REG_INST:  return "REG_INST";
    case REG_M:     return "REG_M";
    case REG_XY:    return "REG_XY";
    case REG_J:     return "REG_J";
    case REG_INCR:  return "REG_INCR";
    case SIGNAL:    return "SIGNAL";
    case PING:      return "PING";
    default:        return "UNKNOWN";
  }
}

// Print MAC address helper
inline void printMAC(const uint8_t* mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
}

#endif // P2P_SIGNALS_H

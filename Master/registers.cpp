// registers.cpp - Register Operations Implementation

#include "registers.h"
#include "definitions.h"
#include "../common/espnow_comms.h"

// =====================================================================================================
// HELPER: Detect SRC==DST equity
// =====================================================================================================
void dst_src_equity(int dst, int src) {      
  if(is_asserted(dst) & is_asserted(src)) {
    assignVarValue(Da_B, 0x00); 
    delay(10);
  }
}

// =====================================================================================================
// GENERIC REGISTER HANDLER
// Handles SELECT and LOAD operations for any 8-bit register (A, B, C, D)
// =====================================================================================================
void handle_register(uint8_t phase, RegisterDescriptor* desc) {
  if (phase == 0) {
    // ========== PHASE 0: SELECT OPERATION ==========
    uint8_t sel_state = is_asserted(desc->select_signal);
    
    if(sel_state) {
      if(is_t(desc->em_type)) {
        // LOCAL register - put value on bus
        assignVarValue(Da_B, varValues[desc->reg_var_idx] & 0xff);
        broadcastForMonitoring(DBUS, (uint8_t)(varValues[desc->reg_var_idx] & 0xff));
      } else {
        // REMOTE register - use peer-to-peer request/response
        if (sendRequest(desc->mac_addr, desc->select_signal, 1, REQUEST_TIMEOUT_MS)) {
          // Got ACK with register value in varValues[Da_B]
          broadcastForMonitoring(desc->select_signal, (uint8_t)1);
          broadcastForMonitoring(DBUS, (uint8_t)(varValues[Da_B] & 0xff));
        } else {
          // Timeout - register not responding
          Serial.print(" ✗ ERROR: ");
          Serial.print(desc->reg_name);
          Serial.println(" timeout!");
        }
      }
    }
    
  } else if (phase == 1) {
    // ========== PHASE 1: LOAD OPERATION ==========
    dst_src_equity(desc->load_signal, desc->select_signal);
    uint8_t load_state = is_asserted(desc->load_signal);
    
    if(load_state && is_t(desc->em_type)) {
      // LOCAL register - load from bus
      assignVarValue(desc->reg_var_idx, varValues[Da_B]);
      
    } else if(load_state) {
      // REMOTE register - use peer-to-peer
      if (sendRequest(desc->mac_addr, desc->load_signal, varValues[Da_B] & 0xff, REQUEST_TIMEOUT_MS)) {
        broadcastForMonitoring(desc->load_signal, (uint8_t)(varValues[Da_B] & 0xff));
      } else {
        Serial.print(" ✗ ERROR: ");
        Serial.print(desc->reg_name);
        Serial.println(" timeout!");
      }
    }
  }
}

// =====================================================================================================
// MAIN REGISTER HANDLER
// =====================================================================================================
void reg_INST(uint8_t phase, uint8_t type) {
  if (phase != 0 && phase != 1) return;

  if (phase == 0) {  // PHASE 0: SELECT operations
    // Type 1: A & D Registers - use generic handler
    if (type == 1) {
      handle_register(phase, &reg8bit[0]);  // A register
      handle_register(phase, &reg8bit[3]);  // D register
    }
    
    // Type 2: B & C Registers - use generic handler
    if (type == 2) {
      handle_register(phase, &reg8bit[1]);  // B register
      handle_register(phase, &reg8bit[2]);  // C register
    }
    
    // Type 3: M Registers - ALL LOCAL, NO broadcasts
    if (type == 3) {
      uint8_t sm1_state = is_asserted(SM1);
      if(sm1_state && is_t(em_MReg)) {
        assignVarValue(Da_B, (varValues[M_Rg] >> 8) & 0xff);
      }
      
      uint8_t sm2_state = is_asserted(SM2);
      if(sm2_state && is_t(em_MReg)) {
        assignVarValue(Da_B, varValues[M_Rg] & 0xff);
      }
      
      uint8_t sem_state = is_asserted(SEM);
      if(sem_state && is_t(em_MReg)) {
        assignVarValue(Ad_B, varValues[M_Rg]);
      }
    }
    
    // Type 4: XY Registers - ALL LOCAL, NO broadcasts
    if (type == 4) {
      uint8_t sex_state = is_asserted(SEX);
      if(sex_state && is_t(em_XYReg)) {
        assignVarValue(Da_B, (varValues[XY_R] >> 8) & 0xff);
      }
      
      uint8_t sey_state = is_asserted(SEY);
      if(sey_state && is_t(em_XYReg)) {
        assignVarValue(Da_B, varValues[XY_R] & 0xff);
      }
      
      uint8_t sxy_state = is_asserted(SXY);
      if(sxy_state && is_t(em_XYReg)) {
        varValues[Ad_B] = varValues[XY_R];
      }
    }
    
    // Type 5: J Registers - ALL LOCAL, NO broadcasts
    if (type == 5) {
      uint8_t sej_state = is_asserted(SEJ);
      if(sej_state && is_t(em_JReg)) {
        assignVarValue(Ad_B, varValues[J_Rg]);
      }
    }
  }

  if (phase == 1) {  // PHASE 1: LOAD/CLOCK operations
    // Type 1: A & D Registers - use generic handler
    if (type == 1) {
      handle_register(phase, &reg8bit[0]);  // A register
      handle_register(phase, &reg8bit[3]);  // D register
    }
    
    // Type 2: B & C Registers - use generic handler
    if (type == 2) {
      handle_register(phase, &reg8bit[1]);  // B register
      handle_register(phase, &reg8bit[2]);  // C register
    }
    
    // Type 3: M Registers - ALL LOCAL, NO broadcasts
    if (type == 3) {
      uint8_t lm1_state = is_asserted(LM1);
      if(lm1_state) {
        dst_src_equity(LM1, SM1);
        if(is_t(em_MReg)) {
          uint16_t mrg = varValues[M_Rg];
          uint16_t bus = varValues[Da_B] & 0xFF;
          assignVarValue(M_Rg, (bus << 8) | (mrg & 0x00FF));
          // Broadcast M register update to monitor
          broadcastForMonitoring(REG_M, varValues[M_Rg]);
        }
      }
      
      uint8_t lm2_state = is_asserted(LM2);
      if(lm2_state) {
        dst_src_equity(LM2, SM2);
        if(is_t(em_MReg)) {
          uint16_t mrg = varValues[M_Rg];
          uint16_t bus = varValues[Da_B];
          assignVarValue(M_Rg, (mrg & 0xff00) | (bus & 0x00ff));
          // Broadcast M register update to monitor
          broadcastForMonitoring(REG_M, varValues[M_Rg]);
        }
      }
    }
    
    // Type 4: XY Registers - ALL LOCAL, NO broadcasts
    if (type == 4) {
      uint8_t ldx_state = is_asserted(LDX);
      if(ldx_state) {
        dst_src_equity(LDX, SEX);
        if(is_t(em_XYReg)) {
          uint16_t mrg = varValues[XY_R];
          uint16_t bus = varValues[Da_B] & 0xFF;
          assignVarValue(XY_R, (bus << 8) | (mrg & 0x00FF));
          // Broadcast XY register update to monitor
          broadcastForMonitoring(REG_XY, varValues[XY_R]);
        }
      }
      
      uint8_t ldy_state = is_asserted(LDY);
      if(ldy_state) {
        dst_src_equity(LDY, SEY);
        if(is_t(em_XYReg)) {
          uint16_t mrg = varValues[XY_R];
          uint16_t bus = varValues[Da_B];
          assignVarValue(XY_R, (mrg & 0xff00) | (bus & 0x00ff));
          // Broadcast XY register update to monitor
          broadcastForMonitoring(REG_XY, varValues[XY_R]);
        }
      }
      
      uint8_t lxy_state = is_asserted(LXY);
      if(lxy_state && is_t(em_XYReg)) {
        varValues[XY_R] = varValues[Ad_B];
        // Broadcast XY register update to monitor
        broadcastForMonitoring(REG_XY, varValues[XY_R]);
      }
    }
    
    // Type 5: J Registers - ALL LOCAL, NO broadcasts
    if (type == 5) {
      uint8_t lj1_state = is_asserted(LJ1);
      if(lj1_state && is_t(em_JReg)) {
        uint16_t mrg = varValues[J_Rg];
        uint16_t bus = varValues[Da_B] & 0xFF;
        assignVarValue(J_Rg, (bus << 8) | (mrg & 0x00FF));
        // Broadcast J register update to monitor
        broadcastForMonitoring(REG_J, varValues[J_Rg]);
      }
      
      uint8_t lj2_state = is_asserted(LJ2);
      if(lj2_state && is_t(em_JReg)) {
        uint16_t mrg = varValues[J_Rg];
        uint16_t bus = varValues[Da_B];
        assignVarValue(J_Rg, (mrg & 0xff00) | (bus & 0x00ff));
        // Broadcast J register update to monitor
        broadcastForMonitoring(REG_J, varValues[J_Rg]);
      }
    }
  }
}

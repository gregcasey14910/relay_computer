// control.cpp - Control Units Implementation

#include "control.h"
#include "definitions.h"
#include "../common/espnow_comms.h"
#include "sequencer.h"

// =====================================================================================================
// INCREMENTER
// =====================================================================================================
void incr_inc(int phase) {
  if(is_t(em_Incr) && (HALT == 0)) {
    if (phase == 0) {
      uint8_t sic_state = is_asserted(SIC);
      broadcastOnChange(SIC, sic_state, &prevSig.SIC);
      
      if(sic_state) {
        varValues[Ad_B] = varValues[INCR];
        sendBroadcast(ABUS, varValues[INCR]);
      }
    }
    if (phase == 1) {
      if(is_asserted(LIC)) {
        varValues[INCR] = varValues[Ad_B] + 1;
        // Broadcast INCR register update to monitor
        broadcastForMonitoring(REG_INCR, varValues[INCR]);
      }
    }  
  }
}

// =====================================================================================================
// ALU
// =====================================================================================================
void alu(int phase) {
  if(is_t(em_ALU)) {  // Local ALU
    if (phase == 0) {
      if(is_asserted(ADD) & is_asserted(IALU)) {  
        varValues[Da_B] = ((varValues[B_Rg] & 0x00ff) + (varValues[C_Rg] & 0x00ff));
        set_s((varValues[Da_B] & 0xff) >= 0x80);
        set_c(varValues[Da_B] >= 0x0100);
        set_z(varValues[Da_B] == 0x0000);              
      }
      if(is_asserted(INC) & is_asserted(IALU)) { 
        varValues[Da_B] = ((varValues[B_Rg] & 0x00ff) + 0x0001);
        set_s((varValues[Da_B] & 0xff) >= 0x80);
        set_c(varValues[Da_B] >= 0x0100);
        set_z(varValues[Da_B] == 0x0000);              
      }
      if(is_asserted(AND) & is_asserted(IALU)) {  
        varValues[Da_B] = ((varValues[B_Rg] & 0x00ff) & (varValues[C_Rg] & 0x00ff));
        set_s((varValues[Da_B] & 0xff) >= 0x80);
        set_z(varValues[Da_B] == 0x0000);         
      } 
      if(is_asserted(ORR) & is_asserted(IALU)) {  
        varValues[Da_B] = ((varValues[B_Rg] & 0x00ff) | (varValues[C_Rg] & 0x00ff));
        set_s((varValues[Da_B] & 0xff) >= 0x80);
        set_z(varValues[Da_B] == 0x0000);         
      } 
    }
    if(is_asserted(XOR) & is_asserted(IALU)) {  
      varValues[Da_B] = ((varValues[B_Rg] & 0x00ff) ^ (varValues[C_Rg] & 0x00ff));
      set_s((varValues[Da_B] & 0xff) >= 0x80);
      set_z(varValues[Da_B] == 0x0000);         
    } 
    if(is_asserted(NOT) & is_asserted(IALU)) {  
      varValues[Da_B] = (~varValues[B_Rg]);
      set_s((varValues[Da_B] & 0xff) >= 0x80);
      set_z(varValues[Da_B] == 0x0000);              
    }
    if(is_asserted(SHL) & is_asserted(IALU)) {  
      varValues[Da_B] = (varValues[B_Rg] << 1) | (varValues[B_Rg] >> 7);
      set_s((varValues[Da_B] & 0xff) >= 0x80);
      set_z(varValues[Da_B] == 0x0000);              
    }
    if (phase == 1) {
    }  
  }
  else {  // Remote ALU
    if(is_asserted(IALU) && state == 8) {
      Serial.println("=== Sending to remote ALU via P2P ===");
      
      uint8_t fct = varValues[INST] & 0x07;
      uint8_t b_val = varValues[B_Rg] & 0xff;
      uint8_t c_val = varValues[C_Rg] & 0xff;
      
      Serial.printf("FCT=0x%X B=0x%02X C=0x%02X\n", fct, b_val, c_val);
      
      // Send FCT
      Serial.print("TX FCT_ALU...");
      if (sendRequest(alu_MAC, FCT_ALU, fct, ALU_TIMEOUT_MS)) {
        broadcastForMonitoring(FCT_ALU, fct);
        
        // Send B
        Serial.print("TX B_2_ALU...");
        if (sendRequest(alu_MAC, B_2_ALU, b_val, ALU_TIMEOUT_MS)) {
          broadcastForMonitoring(B_2_ALU, b_val);
          
          // Send C
          Serial.print("TX C_2_ALU...");
          if (sendRequest(alu_MAC, C_2_ALU, c_val, ALU_TIMEOUT_MS)) {
            broadcastForMonitoring(C_2_ALU, c_val);
            Serial.println("✓ ALU inputs sent successfully");
            
            // Wait for ALU_OUT broadcast
            Serial.println("⏳ Waiting for ALU_OUT...");
            unsigned long wait_start = millis();
            bool got_result = false;
            
            while (!got_result && (millis() - wait_start < 500)) {
              delay(10);
              yield();
              
              if (varValues[Da_B] != c_val) {
                got_result = true;
                Serial.printf("✓ ALU result received: 0x%02X\n", varValues[Da_B]);
              }
            }
            
            if (!got_result) {
              Serial.println("✗ Timeout waiting for ALU_OUT");
            }
            
          } else {
            Serial.println(" ✗ ALU timeout on C_2_ALU!");
          }
        } else {
          Serial.println(" ✗ ALU timeout on B_2_ALU!");
        }
      } else {
        Serial.println(" ✗ ALU timeout on FCT_ALU!");
      }
      
      Serial.println("=== ALU operation complete ===\n");
    }
  }
}

// =====================================================================================================
// MEMORY
// =====================================================================================================
void MEMORY(int phase) {
  if (!is_t(em_Mem)) return;

  if (phase == 0) {
    if (is_asserted(MER)) {
      uint16_t addr = varValues[Ad_B];
      varValues[Da_B] = memArray[addr];
    }
    return;
  }

  if (phase == 1) {
    if (is_asserted(MEW)) {
      memArray[varValues[Ad_B]] = varValues[Da_B];
    }
  }
}

// =====================================================================================================
// PC AND IR CONTROL
// =====================================================================================================
void ctrl_pc_ir(int phase) {
  if(is_t(em_CPCIR)) {
    if (phase == 0) {
      uint8_t spc_state = is_asserted(SPC);
      broadcastOnChange(SPC, spc_state, &prevSig.SPC);
      
      if(spc_state) {
        varValues[Ad_B] = varValues[PC_R];
        sendBroadcast(ABUS, varValues[PC_R]);
      }
      
      uint8_t i2b_state = is_asserted(I2B);
      broadcastOnChange(I2B, i2b_state, &prevSig.I2B);
      
      if(i2b_state) {
        varValues[Da_B] = varValues[INST] & 0x000f;
        sendBroadcast(DBUS, (uint8_t)(varValues[INST] & 0x000f));
      }
    }
    if (phase == 1) {
      if(is_asserted(LPC)) {
        varValues[PC_R] = varValues[Ad_B];
        // Broadcast PC register update to monitor
        Serial.print("Broadcasting PC=0x");
        Serial.print(varValues[PC_R], HEX);
        Serial.print(" signal=");
        Serial.println(REG_PC);
        broadcastForMonitoring(REG_PC, varValues[PC_R]);
      } 
      if(is_asserted(LIN)) {
        varValues[INST] = varValues[Da_B];
        // Broadcast INST register update to monitor
        Serial.print("Broadcasting INST=0x");
        Serial.print(varValues[INST], HEX);
        Serial.print(" signal=");
        Serial.println(REG_INST);
        broadcastForMonitoring(REG_INST, varValues[INST]);
      } 
      if (state == 4) {  // INSTRUCTION DECODE
        uint8_t inst = varValues[8];
        if (decode_opcode_index(inst) == 0) {  // MOV Instruction 
          uint8_t ddd = (inst >> 3) & 0x07;
          uint8_t sss = inst & 0x07;
         // Serial.println(BitFieldNames[get_Load_line(ddd)]);
          set_bits(sigNameToIndex(BitFieldNames[get_Select_line(sss)]), 5, 2);  
          set_bits(sigNameToIndex(BitFieldNames[get_Load_line(ddd)]), 5, 1);  
          set_bits(AT08, 4, 18);
        }
        if(decode_opcode_index(inst) == 10) {  // MOV Instruction 
          set_bits(AT10, 4, 18);
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "ldi") {  // LDI Instruction 
          set_bits(AT12, 4, 18);
          set_bits(I2B, 8, 2);
          if ((varValues[INST] & 0b00100000) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLB, 8, 1);
          }
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "hlt") {  // HLT Instruction 
          set_bits(AT08, 4, 18);
          HALT = 1;
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "add") {  // ADD Instruction 
          set_bits(AT12, 4, 18);
          set_bits(ADD, 8, 8);
          set_bits(IALU, 8, 8);
          if ((varValues[INST] & (0b00001000)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "inc") {  // INC Instruction 
          set_bits(AT12, 4, 18);
          set_bits(INC, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0x00) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "and") {  // AND Instruction 
          set_bits(AT12, 4, 18);
          set_bits(AND, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "orr") {  // OR Instruction 
          set_bits(AT12, 4, 18);
          set_bits(ORR, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "eor") {  // XOR Instruction 
          set_bits(AT12, 4, 18);
          set_bits(XOR, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "not") {  // NOT Instruction 
          set_bits(AT12, 4, 18);
          set_bits(NOT, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "rol") {  // SHL Instruction 
          set_bits(AT12, 4, 18);
          set_bits(SHL, 8, 2);
          set_bits(IALU, 8, 2);
          if ((varValues[INST] & (1 << 3)) == 0) {
            set_bits(RLA, 8, 1);
          } else {
            set_bits(RLD, 8, 1);
          }
        } 
        if(inst_mnemonic[decode_opcode_index(inst)] == "rts") {  // RTS Instruction 
          set_bits(AT10, 4, 18);
          set_bits(SXY, 8, 2);
          set_bits(LPC, 8, 1);  
        }        
        if(inst_mnemonic[decode_opcode_index(inst)] == "lwi") {  // lwi ldi double to m
          set_bits(SPC, 8, 3);   set_bits(SPC, 15, 3);
          set_bits(MER, 8, 3);   set_bits(MER, 15, 3);
          if ((varValues[INST] & 0b00100000) == 0) {
            set_bits(LM1, 9, 1); 
            set_bits(LM2, 16, 1);
          } else {
            set_bits(LJ1, 9, 1); 
            set_bits(LJ2, 16, 1);           
          }
          set_bits(LIC, 9, 1);    set_bits(LIC, 16, 1);
          set_bits(SIC, 12, 2);   set_bits(SIC, 19, 2);
          set_bits(LPC, 12, 1);   set_bits(LPC, 19, 1);
          set_bits(SEJ, 22, 2);
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "b") {  // b (goto)
          set_bits(SPC, 8, 3);   set_bits(SPC, 15, 3);
          set_bits(MER, 8, 3);   set_bits(MER, 15, 3);
          set_bits(LJ1, 9, 1); 
          set_bits(LJ2, 16, 1);
          set_bits(LIC, 9, 1);    set_bits(LIC, 16, 1);
          set_bits(SIC, 12, 2);   set_bits(SIC, 19, 2);
          set_bits(LPC, 12, 1);   set_bits(LPC, 19, 1); set_bits(LPC, 22, 1);
          set_bits(SEJ, 22, 2);
        }
        if (inst_mnemonic[decode_opcode_index(inst)] == "bl") {  // bl register
          set_bits(SPC, 8, 3);   set_bits(SPC, 15, 3);
          set_bits(MER, 8, 3);   set_bits(MER, 15, 3);
          set_bits(LJ1, 9, 1); 
          set_bits(LJ2, 16, 1);
          set_bits(LIC, 9, 1);    set_bits(LIC, 16, 1);
          set_bits(SIC, 12, 2);   set_bits(SIC, 19, 2);
          set_bits(LPC, 12, 1);   set_bits(LPC, 19, 1); set_bits(LPC, 22, 1);
          set_bits(LXY, 19, 1);
          set_bits(SEJ, 22, 2);
        }  
        if(inst_mnemonic[decode_opcode_index(inst)] == "bcc") {  // bcc (goto)
          set_bits(SPC, 8, 3);   set_bits(SPC, 15, 3);
          set_bits(MER, 8, 3);   set_bits(MER, 15, 3);
          set_bits(LJ1, 9, 1); 
          set_bits(LJ2, 16, 1);
          set_bits(LIC, 9, 1);    set_bits(LIC, 16, 1);
          set_bits(SIC, 12, 2);   set_bits(SIC, 19, 2);
          set_bits(LPC, 12, 1);   set_bits(LPC, 19, 1); 
          if ((varValues[INST] & 0b00011110) == 0b00000100) {
            if (if_z() == true) {
              set_bits(LPC, 22, 1);
            }
          }
          if ((varValues[INST] & 0b00011110) == 0b00000010) {
            if (if_z() == false) {
              set_bits(LPC, 22, 1);
            }
          }
          if ((varValues[INST] & 0b00011110) == 0b00001000) {
            if (if_c() == true) {
              set_bits(LPC, 22, 1);
            }
          }
          if ((varValues[INST] & 0b00011110) == 0b00010000) {
            if (if_s() == true) {
              set_bits(LPC, 22, 1);
            }
          }
          if ((varValues[INST] & 0b00011110) == 0b00010100) {
            if ((if_s() == true) || (if_z() == true)) {
              set_bits(LPC, 22, 1);
            }
          }
          set_bits(SEJ, 22, 2);
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "str") {  // str store
          Serial.println(" got a Store INst");
          set_bits(AT12, 4, 18);
          set_bits(SEM, 8, 3);
          set_bits(MEW, 9, 1);
          Serial.println(varValues[INST]);
          if ((varValues[INST] % 0b00000011) == 0b00000000) set_bits(RSA, 8, 3);
          if ((varValues[INST] % 0b00000011) == 0b00000001) set_bits(RSB, 8, 3);
          if ((varValues[INST] % 0b00000011) == 0x00000010) set_bits(RSC, 8, 3);
          if ((varValues[INST] % 0b00000011) == 0x00000011) set_bits(RSD, 8, 3);
        }
        if(inst_mnemonic[decode_opcode_index(inst)] == "ldr") {  // ldr Load
          Serial.println(" got a Load INst");
          set_bits(AT12, 4, 18);
          set_bits(SEM, 8, 3);
          set_bits(MER, 8, 3);
          if ((varValues[INST] % 0b00000011) == 0x0) set_bits(RLA, 9, 1);
          if ((varValues[INST] % 0b00000011) == 0x1) set_bits(RLB, 9, 1);
          if ((varValues[INST] % 0b00000011) == 0x2) set_bits(RLC, 9, 1);
          if ((varValues[INST] % 0b00000011) == 0x3) set_bits(RLD, 9, 1);
        }
      }  
    }
  }  
}

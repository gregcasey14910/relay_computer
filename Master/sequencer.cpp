// sequencer.cpp - State Machine Implementation

#include "sequencer.h"
#include "definitions.h"
#include "display.h"

// =====================================================================================================
// CLOCK GENERATOR
// =====================================================================================================
void clock_gen() {
  if(is_t(em_CPCIR)) {
    state++;
    if ((is_asserted(AT08)) && (state == 8)) {
      state = 0; 
    } 
    if ((is_asserted(AT10)) && (state == 10)) {
      state = 0; 
    } 
    if ((is_asserted(AT12)) && (state == 12)) {
      state = 0; 
    } 
    if (clck == 1) {
      clck = 0;
    } else {
      clck = 1;
    }
    digitalWrite(clock_pin, clck); 
    if (state == 0) {
      delay(rest_time);
    } else {
      delay(state_time/2);
    }
  }  
}

// =====================================================================================================
// SEQUENCER
// =====================================================================================================
void sequencer() {
  if(is_t(em_CPCIR)) {
    show_leds(state);
    switch (state) {
      case 0:
        clear_bits(); 
        set_fetch_tbits();
        apply_bits();
        break;
      case 1:
      case 2:
      case 3:
      case 4:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
        apply_bits();
        break;
      case 5:
      case 14:
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
      case 20:
      case 21:
      case 22:
        break;
      case 23:
        state = -1;
        break;
      default:
        break;
    }
  }
}

// =====================================================================================================
// TIMING BIT MANIPULATION
// =====================================================================================================
void clear_bits() {
  for (uint8_t i = 0; i < 68; ++i) {
    tbits[i][0] = 0b00000000;   
    tbits[i][1] = 0b00000000; 
    tbits[i][2] = 0b00000000;    
  }
}

void set_bits(uint8_t name, uint8_t start, uint8_t count) {
  for (uint8_t j = start; j < (uint8_t)(start + count); j++) {
    if (j < 8)        tbits[name][0] |= (1 << (7 - j));
    else if (j < 16)  tbits[name][1] |= (1 << (7 - (j - 8)));
    else if (j < 24)  tbits[name][2] |= (1 << (7 - (j - 16)));
  }
}

void set_fetch_tbits() {
  set_bits(SPC, 1, 3);  
  set_bits(MER, 1, 3); 
  set_bits(LIN, 2, 1);
  set_bits(LIC, 3, 1);
  set_bits(SIC, 5, 2);
  set_bits(LPC, 5, 1);  
}

void apply_bits() {
  for (uint8_t i = 0; i < 67; ++i) {
    const char* name = BitFieldNames[i];
    uint8_t loc = conn_loc[i];
    uint8_t group = loc >> 5;
    uint8_t index = loc & 0b00011111;
    uint8_t bit = (tbits[i][0] >> (7 - state)) & 0x01;
  }
}

// =====================================================================================================
// INSTRUCTION DECODE
// =====================================================================================================
int decode_opcode_index(uint8_t opcode) {
  if ((opcode & 0b11000000) == 0x00000000) {
    return(0);  // mov
  }
  if ((opcode & 0b11000000) == 0b01000000) {
    return(1);  // ldi
  }
  if ((opcode >> 4) == 0b1000) {
    if ((opcode & 0b111) == 0b001)      {return(2);}   // add  
    if ((opcode & 0b111) == 0b010)      {return(20);}  // inc  NEW
    if ((opcode & 0b111) == 0b011)      {return(3);}   // and     
    if ((opcode & 0b111) == 0b100)      {return(21);}  // orr  new
    if ((opcode & 0b111) == 0b101)      {return(5);}   // xor  
    if ((opcode & 0b111) == 0b110)      {return(6);}   // not  
    if ((opcode & 0b111) == 0b111)      {return(7);}   // shr 
  }

  if ((opcode & 0b11110111) == 0b10000111) {
    return(4);  // clr
  }
  if (opcode == 0b10000110) {return(7);}
  if (opcode == 0b10101110) {return(8);}
  if (opcode == 0b10101010) {return(9);}
  if ((opcode & 0b11110001) == 0b10100000) {return(10);}
  if ((opcode & 0b11111100) == 0b10010000) {return(11);}
  if ((opcode & 0b11111100) == 0b10011000) {return(12);}
  if (opcode == 0b10110000) {return(13);}
  if (opcode == 0b11000000) {return(14);}
  if (opcode == 0b11100110) {return(15);}
  if (opcode == 0b11100111) {return(16);}
  if ((opcode & 0b11100001) == 0b11100000) {return(17);}
  if ((opcode & 0b11100001) == 0b11100001) {return(18);}
  if (opcode == 0b10100101) {return(19);}
  return -1;
}

void print_inst(uint8_t opcode) {
  Serial.println("============== Print INST ============");
  int idx = decode_opcode_index(opcode);
  Serial.print("Opcode: 0x");
  if (opcode < 0x10) Serial.print("0");
  Serial.print(opcode, HEX);

  if (idx >= 0) {
    Serial.print(" | Category: ");
    Serial.print(inst_category[idx]);
    Serial.print(" | Mnemonic: ");
    Serial.print(inst_mnemonic[idx]);
    Serial.print(" | Pattern: ");
    Serial.println(inst_pattern[idx]);
  } else {
    Serial.println(" | UNKNOWN");
  }
}

uint8_t get_Load_line(uint8_t ddd) {
  const uint8_t load_lines[] = {
    RLA, RLB, RLC, RLD, LM1, LM2, LDX, LDY
  };
  if (ddd < sizeof(load_lines)) {
    return load_lines[ddd];
  }
  return 0xFF;
}

uint8_t get_Select_line(uint8_t sss) {
  const uint8_t select_lines[] = {
    RSA, RSB, RSC, RSD, SM1, SM2, SEX, SEY
  };
  if (sss < sizeof(select_lines)) {
    return select_lines[sss];
  }
  return 0xFF;
}

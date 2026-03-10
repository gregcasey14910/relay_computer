// sequencer.h - State Machine and Instruction Decode
// Manages state transitions and timing bit arrays

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <Arduino.h>

// State machine
void sequencer();
void clock_gen();

// Timing bit manipulation
void clear_bits();
void set_bits(uint8_t name, uint8_t start, uint8_t count);
void set_fetch_tbits();
void apply_bits();

// Instruction decode helpers
int decode_opcode_index(uint8_t opcode);
void print_inst(uint8_t opcode);
uint8_t get_Load_line(uint8_t ddd);
uint8_t get_Select_line(uint8_t sss);

#endif // SEQUENCER_H

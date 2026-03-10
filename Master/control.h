// control.h - Control Units (PC, IR, ALU, Memory, Incrementer)
// Handles all control unit operations

#ifndef CONTROL_H
#define CONTROL_H

#include <Arduino.h>

// PC and IR control
void ctrl_pc_ir(int phase);

// Incrementer
void incr_inc(int phase);

// ALU operations
void alu(int phase);

// Memory operations
void MEMORY(int phase);

#endif // CONTROL_H

// registers.h - Register Operations (A, B, C, D, M, XY, J)
// Handles SELECT and LOAD operations for all registers

#ifndef REGISTERS_H
#define REGISTERS_H

#include <Arduino.h>
#include "definitions.h"

// Generic register handler for A, B, C, D
void handle_register(uint8_t phase, RegisterDescriptor* desc);

// Main register handler - handles all register types
void reg_INST(uint8_t phase, uint8_t type);

// Helper functions
void dst_src_equity(int dst, int src);

#endif // REGISTERS_H

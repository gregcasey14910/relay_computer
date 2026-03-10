// display.h - OLED Display and Trace Functions
// Handles screen output and serial bus tracing

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

// OLED display functions
void show_screen();
void show_leds(uint8_t state);
void drawStateDot(uint8_t state);

// Serial trace functions
void bus_trace();
void individual_bus(int load, int reg, int sel);
void individual_2bus(int load, int reg, int sel);
void individual_3bus(int load1, int load2, int load3, int reg, int sel1, int sel2, int sel3);

// Helper functions
uint8_t who_is_drv_DBUS();
uint8_t who_is_drv_ABUS();

#endif // DISPLAY_H

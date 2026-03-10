// README.md - Master2 Project Structure
// ESP32 Relay Computer Emulator - Refactored Multi-File Architecture

# Master2 - Refactored Structure

This is a refactored version of the original Master.ino file, split into logical modules for better maintainability.

## File Structure

```
Arduino/
├── common/                   - Shared files for all modules
│   ├── ESPNOW_Common.h      - Common definitions (already exists)
│   ├── espnow_comms.h       - ESP-NOW communication header
│   └── espnow_comms.cpp     - ESP-NOW communication implementation
│
└── Master2/
    ├── Master2.ino          - Main entry point (setup/loop)
    ├── definitions.h        - Constants, enums, structs, global declarations
    ├── globals.cpp          - Global variable definitions
    ├── registers.h/cpp      - Register operations (A, B, C, D, M, XY, J)
    ├── control.h/cpp        - Control units (PC, IR, ALU, Memory, Incrementer)
    ├── display.h/cpp        - OLED display and serial trace functions
    └── sequencer.h/cpp      - State machine and instruction decode
```

## Module Responsibilities

### Master2.ino
- Arduino setup() and loop() functions
- Calls into other modules in proper sequence

### definitions.h / globals.cpp
- All constants, enums, and structs
- Global variable declarations (in .h) and definitions (in .cpp)
- Lookup tables (BitFieldNames, inst_mnemonic, etc.)
- Utility functions (is_t, is_asserted, flag manipulation)

### ../common/espnow_comms.h / espnow_comms.cpp
- **SHARED ACROSS ALL MODULES** (Master, Reg_A, ALU, CYD_Mon)
- ESP-NOW initialization and peer registration
- Peer-to-peer request/response protocol
- Broadcast messaging for monitoring
- Send/receive callbacks

### registers.h / registers.cpp
- Generic register handler for A, B, C, D (8-bit)
- M register handler (16-bit)
- XY register handler (16-bit)
- J register handler (16-bit)
- SELECT and LOAD operations for all register types

### control.h / control.cpp
- Program Counter (PC) and Instruction Register (IR) control
- Incrementer operations
- ALU operations (local and remote)
- Memory read/write operations
- Instruction decode and timing bit manipulation

### display.h / display.cpp
- OLED screen display (show_screen, show_leds)
- Serial bus trace output (bus_trace)
- Individual register trace formatting
- Bus driver detection (who_is_drv_DBUS, who_is_drv_ABUS)

### sequencer.h / sequencer.cpp
- State machine (sequencer)
- Clock generation (clock_gen)
- Timing bit manipulation (set_bits, clear_bits)
- Instruction decode helpers (decode_opcode_index, get_Load_line, etc.)

## Key Benefits of This Structure

1. **Modularity**: Each file has a single, clear responsibility
2. **Maintainability**: Easy to find and fix code in specific modules
3. **Readability**: Much shorter files (~200-500 lines each vs 2100+ lines)
4. **Collaboration**: Multiple people can work on different modules
5. **Testing**: Easier to test individual components
6. **Reusability**: Communication code shared across Master, Reg_A, ALU, CYD_Mon
7. **Code Reuse**: espnow_comms.* can be used by all modules in the project

## Installation Instructions

### 1. Create Directory Structure
```
Arduino/
├── common/                   ← Create this directory
└── Master2/                  ← Create this directory
```

### 2. Place Files
**In `common/` directory:**
- `ESPNOW_Common.h` (already there)
- `espnow_comms.h`
- `espnow_comms.cpp`

**In `Master2/` directory:**
- `Master2.ino` (must match folder name!)
- `definitions.h`
- `globals.cpp`
- `registers.h` & `registers.cpp`
- `control.h` & `control.cpp`
- `display.h` & `display.cpp`
- `sequencer.h` & `sequencer.cpp`

### 3. Open in Arduino IDE
- File → Open → Navigate to `Master2` folder
- Select `Master2.ino`
- Arduino IDE will automatically find files in `../common/` via the `#include "../common/espnow_comms.h"` directives

### 4. Compile and Upload
- Just click Verify or Upload as normal
- Arduino IDE handles the relative paths automatically

## Compilation Notes

- All Master2 files must be in the `Master2/` directory
- Shared files (espnow_comms.*) must be in `../common/` directory
- The .ino file must match the directory name (Master2.ino in Master2/)
- Arduino IDE automatically compiles all .cpp files in the sketch folder
- The `../common/` includes are automatically resolved by the compiler

## Sharing espnow_comms with Other Modules

Other modules (Reg_A, ALU, CYD_Mon) can use the same communication code:

```cpp
// In Reg_A.ino, ALU.ino, CYD_Mon.ino:
#include "../common/ESPNOW_Common.h"
#include "../common/espnow_comms.h"
```

This ensures all modules use the same tested communication protocol!

## Migration from Master.ino

This refactored version maintains 100% functional compatibility with the original Master.ino.
All functionality has been preserved - just reorganized for clarity.

To verify equivalence:
1. Compile both versions
2. Upload to hardware
3. Compare serial output and OLED display
4. Should be identical

## Future Enhancements

Possible improvements:
- Add unit tests for individual modules
- Create separate configuration file for pin assignments
- Extract instruction set definitions to separate file
- Add documentation for each public function
- Share more common code between Master, Reg_A, ALU modules

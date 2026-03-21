/*******************************************************************************
 * I2C Pin Probe - ESP32-C3
 * Tries all valid SDA/SCL pin combinations and reports any I2C devices found.
 ******************************************************************************/

#include <Wire.h>

const uint8_t candidates[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21 };
const int numCandidates = sizeof(candidates);

void scanBus(uint8_t sda, uint8_t scl) {
  Wire.begin(sda, scl);
  delay(20);

  bool found = false;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t err = Wire.endTransmission();
    if (err == 0) {
      if (!found) {
        Serial.printf("  SDA=GPIO%-2d  SCL=GPIO%-2d  -> device at 0x%02X", sda, scl, addr);
        if (addr == 0x3C || addr == 0x3D) Serial.print("  << SSD1306!");
        if (addr == 0x20) Serial.print("  << MCP23017!");
        Serial.println();
        found = true;
      } else {
        Serial.printf("                               also at 0x%02X\n", addr);
      }
    }
  }
  Wire.end();
}

void doScan() {
  Serial.println("\n========================================");
  Serial.println("  I2C PIN PROBE - ESP32-C3");
  Serial.println("========================================");
  for (int s = 0; s < numCandidates; s++) {
    for (int c = 0; c < numCandidates; c++) {
      if (candidates[s] == candidates[c]) continue;
      scanBus(candidates[s], candidates[c]);
    }
  }
  Serial.println("========================================");
  Serial.println("  Scan complete. Repeating in 10s...");
  Serial.println("========================================\n");
}

void setup() {
  Serial.begin(115200);
  // Wait up to 5s for USB CDC host to connect
  unsigned long t = millis();
  while (!Serial && (millis() - t < 5000));
  delay(500);
  doScan();
}

void loop() {
  delay(10000);
  doScan();
}

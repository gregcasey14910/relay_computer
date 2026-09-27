// rc_simple - each SPACE runs one 300 ms cycle with the next address (0..255):
//   t =   0 ms  cycle start
//   t =  50 ms  A7..A0 (U3 GPB7..GPB0) = ADDR
//   t = 100 ms  ENHL (A_2_R, U7 GPB5) HIGH
//   t = 200 ms  ENHL LOW
//   t = 300 ms  cycle end (ADDR stays on the lines until the next cycle)
// SPACE = next address, / = same address again, 0 = restart at address 0.
// The SSD1306 OLED shows ADDR in hex, as large as fits.

#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_ADDR     0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MCP23X17 U3;     // the MCP23017 chip at I2C address 0x21 (A2..A0 = 001)
Adafruit_MCP23X17 U7;     // the MCP23017 chip at I2C address 0x22 (A2..A0 = 010)

const int A0_PIN = 8;     // pins 8..15 = GPB0..GPB7 on U3 (A0..A7)
const int ENHL   = 13;    // pin 13 = GPB5 on U7 (ENHL / A_2_R)

uint8_t addr = 0;         // first pass 0, then +1 each pass up to 255

// Wait until 'ms' milliseconds after 'start'
void waitUntil(unsigned long start, unsigned long ms) {
  while (millis() - start < ms) { }
}

// Show ADDR as 2 hex digits filling the OLED (2 x 48 = 96 wide, centered)
void showAddr() {
  char buf[3];
  sprintf(buf, "%02X", addr);
  display.clearDisplay();
  display.setCursor((SCREEN_WIDTH - 96) / 2, 0);
  display.print(buf);
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(3000);            // give the serial monitor time to connect

  Wire.begin(0, 1);       // I2C: SDA = GPIO0, SCL = GPIO1
  U3.begin_I2C(0x21);     // connect to U3
  U7.begin_I2C(0x22);     // connect to U7

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(8);             // 48 x 64 pixels per character

  for (int b = 0; b < 8; b++) {
    U3.pinMode(A0_PIN + b, OUTPUT);   // A0..A7 as outputs
  }
  U7.pinMode(ENHL, OUTPUT);
  U7.digitalWrite(ENHL, LOW);
}

void loop() {
  Serial.print("ADDR = ");
  Serial.printf("%02X\n", addr);      // A7..A0 as 2-digit hex (00..FF)
  showAddr();                         // OLED: ADDR in hex (before the timed cycle)

  unsigned long t0 = millis();
  waitUntil(t0, 50);
  U3.writeGPIOB(addr);                // t = 50 ms: apply ADDR
  waitUntil(t0, 100);
  U7.digitalWrite(ENHL, HIGH);        // t = 100 ms: ENHL HIGH
  waitUntil(t0, 200);
  U7.digitalWrite(ENHL, LOW);         // t = 200 ms: ENHL LOW
  waitUntil(t0, 300);                 // t = 300 ms: end of cycle

  // Wait here until SPACE (next address), '0' (restart at address 0)
  // or '/' (repeat the same address) is typed
  Serial.println("Press SPACE for next, / to repeat, 0 to reset ADDR");
  while (true) {
    int c = Serial.read();
    if (c == ' ') { addr++; break; }  // 0,1,...,255 then wraps back to 0
    if (c == '0') { addr = 0; break; }
    if (c == '/') break;              // same ADDR again
    delay(10);
  }
  }

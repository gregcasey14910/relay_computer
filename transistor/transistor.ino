/*
 * ESP32-C3 Flip-Flop Test with High-Impedance Mode
 * Using GPIO5 for SET (GPIO2 has internal pull-up issue)
 * 
 * GPIO5 = SET
 * GPIO3 = RESET
 * GPIO4 = MIRROR
 * GPIO8 = Built-in LED (active-low)
 */

#define SET_PIN     5  // Flip-flop SET (changed from GPIO2)
#define RESET_PIN   3  // Flip-flop RESET
#define MIRROR_PIN  4  // Mirror/test
#define LED_PIN     8  // Built-in LED (active-low)

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n========================================");
  Serial.println("  ESP32-C3 Flip-Flop Test");
  Serial.println("  GPIO5 = SET");
  Serial.println("  GPIO3 = RESET");
  Serial.println("  GPIO4 = MIRROR");
  Serial.println("========================================");
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);  // LED OFF
  
  // Start with all pins in INPUT mode (high-impedance)
  pinMode(SET_PIN, INPUT);
  pinMode(RESET_PIN, INPUT);
  pinMode(MIRROR_PIN, INPUT);
  
  Serial.println("\nCommands:");
  Serial.println("  5 - Pulse GPIO5 (SET) for 1 sec");
  Serial.println("  3 - Pulse GPIO3 (RESET) for 1 sec");
  Serial.println("  4 - Pulse GPIO4 (MIRROR) for 1 sec");
  Serial.println("  8 - Flash Built-in LED 5 times");
  Serial.println("  S - SET flip-flop (GPIO5 + GPIO4)");
  Serial.println("  R - RESET flip-flop (GPIO3)");
  Serial.println("  0 - All Hi-Z (disconnect)");
  Serial.println();
}

void loop() {
  if (Serial.available()) {
    char cmd = Serial.read();
    
    switch(cmd) {
      case '5':
        testGPIO5();
        break;
        
      case '3':
        testGPIO3();
        break;
        
      case '4':
        testGPIO4();
        break;
        
      case '8':
        flashBuiltinLED();
        break;
        
      case 'S':
      case 's':
        pulseSet();
        break;
        
      case 'R':
      case 'r':
        pulseReset();
        break;
        
      case '0':
        allHighZ();
        break;
    }
  }
}

void testGPIO5() {
  Serial.println("→ GPIO5 HIGH for 1 second");
  
  pinMode(SET_PIN, OUTPUT);
  digitalWrite(SET_PIN, HIGH);
  delay(1000);
  
  // Return to high-impedance
  pinMode(SET_PIN, INPUT);
  
  Serial.println("  GPIO5 now Hi-Z\n");
}

void testGPIO3() {
  Serial.println("→ GPIO3 HIGH for 1 second");
  
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  delay(1000);
  
  // Return to high-impedance
  pinMode(RESET_PIN, INPUT);
  
  Serial.println("  GPIO3 now Hi-Z\n");
}

void testGPIO4() {
  Serial.println("→ GPIO4 HIGH for 1 second");
  
  pinMode(MIRROR_PIN, OUTPUT);
  digitalWrite(MIRROR_PIN, HIGH);
  delay(1000);
  
  // Return to high-impedance
  pinMode(MIRROR_PIN, INPUT);
  
  Serial.println("  GPIO4 now Hi-Z\n");
}

void flashBuiltinLED() {
  Serial.println("→ Flashing Built-in LED 5 times");
  
  for(int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LOW);   // LOW = ON
    delay(200);
    digitalWrite(LED_PIN, HIGH);  // HIGH = OFF
    delay(200);
  }
  
  Serial.println("  Done!\n");
}

void pulseSet() {
  Serial.println("→ SET: Pulsing GPIO5 and GPIO4");
  
  // Configure as outputs and pulse HIGH
  pinMode(SET_PIN, OUTPUT);
  pinMode(MIRROR_PIN, OUTPUT);
  digitalWrite(SET_PIN, HIGH);
  digitalWrite(MIRROR_PIN, HIGH);
  
  Serial.println("  GPIO5 = HIGH, GPIO4 = HIGH");
  delay(100);
  
  // Return to high-impedance (stops interfering with circuit)
  pinMode(SET_PIN, INPUT);
  pinMode(MIRROR_PIN, INPUT);
  
  Serial.println("  GPIO5 = Hi-Z, GPIO4 = Hi-Z");
  Serial.println("  Flip-flop should be SET and HOLD\n");
}

void pulseReset() {
  Serial.println("→ RESET: Pulsing GPIO3");
  
  // Configure as output and pulse HIGH
  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  
  Serial.println("  GPIO3 = HIGH");
  delay(100);
  
  // Return to high-impedance
  pinMode(RESET_PIN, INPUT);
  
  Serial.println("  GPIO3 = Hi-Z");
  Serial.println("  Flip-flop should be RESET and HOLD\n");
}

void allHighZ() {
  pinMode(SET_PIN, INPUT);
  pinMode(RESET_PIN, INPUT);
  pinMode(MIRROR_PIN, INPUT);
  
  Serial.println("→ All GPIO pins = Hi-Z (high-impedance)\n");
}
// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(33, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(33, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(10);                      // wait for a second
  digitalWrite(33, LOW);   // turn the LED off by making the voltage LOW
  delay(10);                      // wait for a second
}

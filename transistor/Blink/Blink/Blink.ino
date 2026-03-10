int BUILTIN = 8;
int en = 20;
int d = 21;
int state;

int en_val = 0;
int d_val = 0;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(BUILTIN, OUTPUT);
  pinMode(en, OUTPUT);
  pinMode(d, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  for (int state = 1; state <= 6; state++) {
      switch (state) {
          case 1:
              digitalWrite(BUILTIN,1);
              en_val = 0; d_val = 0;
              break;
          case 2:
              en_val = 1; d_val = 0;
              break;
          case 3:
              en_val = 0; d_val = 0;
              break;
          case 4:
              digitalWrite(BUILTIN,0);
              en_val = 0; d_val = 1;
              break;
          case 5:
              en_val = 1; d_val = 1;
              break;
          case 6:
              en_val = 0; d_val = 1;
              break;
      }
      
      digitalWrite(en, en_val);
      digitalWrite(d, d_val);
  delay(200); 
  }                   
}

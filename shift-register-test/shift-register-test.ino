const int DataPin = 12;
const int LatchPin = 11;
const int ShiftPin = 10;

void setup() {
  digitalWrite(DataPin, LOW);
  digitalWrite(LatchPin, LOW);
  digitalWrite(ShiftPin, LOW);
  
  pinMode(DataPin, OUTPUT);
  pinMode(LatchPin, OUTPUT);
  pinMode(ShiftPin, OUTPUT);

  for (int i=0; i<65536; i++) {
    shiftOut(DataPin, ShiftPin, LSBFIRST, i&0xff);
    shiftOut(DataPin, ShiftPin, LSBFIRST, i>>8);
    digitalWrite(LatchPin, LOW);
    digitalWrite(LatchPin, HIGH);
    digitalWrite(LatchPin, LOW);
    delay(100);
  }
}

void loop() {
}

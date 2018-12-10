/*
   Hardware Definitions
*/

// data lines
const int DataPin0 = 2;
const int DataPin7 = 9;

// address lines
const int AddressPin0 = 12;
const int AddressPin7 = 19;

// program button
const int ButtonPin = 11;
const int ModePin = 10;


/*
   Low-Level
*/

// setup pins
void setupRAM() {
  Serial.println("Setting up for RAM population...");

  for (int pin = DataPin0; pin <= DataPin7; pin++) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }

  for (int pin = AddressPin0; pin <= AddressPin7; pin++) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }

  digitalWrite(ModePin, LOW);
  pinMode(ModePin, OUTPUT);

  digitalWrite(ButtonPin, HIGH);
  pinMode(ButtonPin, OUTPUT);
}

// set pins back to high-impedence state
void teardownRAM() {
  Serial.println("Setting up for RAM population...");

  for (int pin = DataPin0; pin <= DataPin7; pin++) {
    pinMode(pin, INPUT);
  }

  for (int pin = AddressPin0; pin <= AddressPin7; pin++) {
    pinMode(pin, INPUT);
  }

  pinMode(ModePin, INPUT);
  pinMode(ButtonPin, INPUT);
}

// write a byte to RAM
void writeRAM(byte addr, byte value) {
  Serial.print("Setting RAM[0x");
  Serial.print(addr, HEX);
  Serial.print("] to 0x");
  Serial.println(value, HEX);

  for (int pin = DataPin0; pin <= DataPin7; pin++) {
    digitalWrite(pin, value & 0x80);
    value <<= 1;
  }

  for (int pin = AddressPin0; pin <= AddressPin7; pin++) {
    digitalWrite(pin, addr & 1);
    addr >>= 1;
  }

  digitalWrite(ButtonPin, LOW);
  delay(1);
  digitalWrite(ButtonPin, HIGH);
  delay(1);
}


/*
   High-Level
*/

void populateRAM(byte values[]) {
  for (int i = 0; i < 32; i++) {
    writeRAM(i, values[i]);
    delay(1);
  }
}


/*
   Entry Point
*/

enum OpCode : byte {
  OP_NOP  = 0x00,
  OP_LDA  = 0x01,
  OP_LDB  = 0x02,
  OP_WRA  = 0x03,
  OP_WRB  = 0x04,
  OP_MOV  = 0x05,
  OP_ADD  = 0x06,
  OP_SUB  = 0x07,
  OP_JMP  = 0x08,
  OP_JZ   = 0x09,
  OP_JO   = 0x0A,
  OP_JNZ  = 0x0B,
  OP_JNO  = 0x0C,
  OP_STO  = 0x0D,
  OP_OUT  = 0x0E,
  OP_HLT  = 0x0F
};

byte ramContents[32] = {
  /*0x00 */ OP_LDA, 0x1F,
  /*0x02 */ OP_LDB, 0x1E,
  /*0x04 */ OP_ADD,
  /*0x05 */ OP_JNO, 0x4,
  /*0x07 */ OP_WRB, 18,
  /*0x09 */ OP_SUB,
  /*0x0A */ OP_JNZ, 0x09,
  /*0x0C */ OP_JZ,  0x10,
  /*0x0E */ OP_JMP, 0x14,
  /*0x10 */ OP_WRA, 42,
  /*0x12 */ OP_JMP, 0x16,
  /*0x14 */ OP_WRA, 66,
  /*0x16 */ OP_OUT,
  /*0x17 */ OP_HLT,
  /*0x18 */ OP_NOP,
  /*0x19 */ OP_NOP,
  /*0x1A */ OP_NOP,
  /*0x1B */ OP_NOP,
  /*0x1C */ OP_NOP,
  /*0x1D */ OP_NOP,
  /*0x1E */ 100,
  /*0x1F */ 10
};

void setup() {
  Serial.begin(115200);

  setupRAM();
  populateRAM(ramContents);
  teardownRAM();
}

void loop() { }

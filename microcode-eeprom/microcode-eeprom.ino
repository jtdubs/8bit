/*
 * Hardware Definitions
 * 
 * Compatible with EEPROM 28C32, 28C64 and 28C128
 * Compatible with shift register HC??HC??
 */

// Pins connected to 16-bit shift register that drives the EEPROM's address lines (A0 - A15)
const int AddressHighClockPin = A3;
const int AddressLowClockPin  = A4;
const int AddressDataPin      = A5;
const int AddressLatchPin     = 12;

// Sequential pins tied to the EEPROM's data lines (IO1 - IO7)
const int DataPin0 = 2;
const int DataPin7 = 9;

// Pins tied to EEPROM control lines
const int WriteEnablePin  = A0;
const int ChipEnablePin   = A1;
const int OutputEnablePin = A2;


/*
 * Low-Level functions that interface directly with the EEPROM and shift register
 */

// Constants representing whether or not the EEPROM is in READ or WRITE mode
enum EEPROMMode { WriteMode = 0, ReadMode = 1, UnknownMode = -1 };

// The current mode of the EEPROM
EEPROMMode CurrentMode = UnknownMode;

// Set default state of pins, and put EEPROM in read mode
void setupEEPROM() {
  Serial.println("Setting up EEPROM default pin states...");

  digitalWrite(AddressHighClockPin, LOW);
  digitalWrite(AddressLowClockPin, LOW);
  digitalWrite(AddressDataPin, LOW);
  digitalWrite(AddressLatchPin, LOW);
  
  pinMode(AddressHighClockPin, OUTPUT);
  pinMode(AddressLowClockPin, OUTPUT);
  pinMode(AddressDataPin, OUTPUT);
  pinMode(AddressLatchPin, OUTPUT);
  
  digitalWrite(ChipEnablePin, LOW);
  pinMode(ChipEnablePin, OUTPUT);

  setMode(ReadMode);
  
  pinMode(OutputEnablePin, OUTPUT);
  pinMode(WriteEnablePin, OUTPUT);
}

// Set the current mode of the EEPROM
void setMode(EEPROMMode mode) {
  if (CurrentMode == mode)
    return;

  CurrentMode = mode;

  switch (CurrentMode)
  {
    case ReadMode:
      Serial.println("Setting EEPROM to Read mode...");
      
      // OE should be LOW
      digitalWrite(OutputEnablePin, LOW);
      digitalWrite(WriteEnablePin, HIGH);
      // All data pins should be in INPUT mode
      for (int pin = DataPin0; pin <= DataPin7; pin++) {
        pinMode(pin, INPUT);
      }
      break;

    case WriteMode:
      Serial.println("Setting EEPROM to Write mode...");
    
      // OE should be HIGH
      digitalWrite(OutputEnablePin, HIGH);
      digitalWrite(WriteEnablePin, HIGH);
      // All data pins should be in OUTPUT mode
      for (int pin = DataPin0; pin <= DataPin7; pin++) {
        pinMode(pin, OUTPUT);
      }
      break;
  }
}

// Set the state of the address lines
void setAddress(int address) {
  static int lastHigh = 0xCE;
  
  byte high  = address >> 8;
  byte low = address & 0xFF;

  // Shift out the high byte, if it has changed
  if (high != lastHigh) {
    lastHigh = high;
    for (int i=0; i<8; i++) {
      digitalWrite(AddressDataPin, high & 1);
      digitalWrite(AddressHighClockPin, HIGH);
      digitalWrite(AddressHighClockPin, LOW);
      high >>= 1;
    }
  }

  // Shift out the low byte
  for (int i=0; i<8; i++) {
    digitalWrite(AddressDataPin, low & 1);
    digitalWrite(AddressLowClockPin, HIGH);
    digitalWrite(AddressLowClockPin, LOW);
    low >>= 1;
  }
  
  // Latch the new data
  digitalWrite(AddressLatchPin, HIGH);
  digitalWrite(AddressLatchPin, LOW);
}

// Set the state of the data lines
void setData(int data) {
  // Set DATA_P0 = LSB through DATA_P7 = MSB
  for (int pin = DataPin0; pin <= DataPin7; pin++) {
    digitalWrite(pin, data & 1);
    data >>= 1;
  }
}

// Read the current state of the data lines
byte readData() {
  // Read DATA_P7 (MSB) through DATA_P0 (LSB)
  byte data = 0;
  for (int pin = DataPin7; pin >= DataPin0; pin--) {
    data = (data << 1) | digitalRead(pin);
  }
  return data;
}

// Cause the EEPROM to store data
void storeData() {
  // 1us low pulse on the EN line
  digitalWrite(WriteEnablePin, LOW);
  delayMicroseconds(1);
  digitalWrite(WriteEnablePin, HIGH);

  // 10ms delay to allow writing to conclude
  delay(10);
}


/*
 * Medium-Level functions to read and write data to the EEPROM
 */

// Read the value in the EEPROM at the specified address
byte readEEPROM(int address) {
  setMode(ReadMode);
  setAddress(address);
  return readData();
}

// Write a byte to the EEPROM at the specified address
void writeEEPROM(int address, byte data) {
  setMode(WriteMode);
  setAddress(address);
  setData(data);
  storeData();
}

// remove software write-protection
void unlockEEPROM() {
    setMode(WriteMode);
    writeEEPROM(0x5555, 0xaa);
    writeEEPROM(0x2aaa, 0x55);
    writeEEPROM(0x5555, 0x80);
    writeEEPROM(0x5555, 0xaa);
    writeEEPROM(0x2aaa, 0x55);
    writeEEPROM(0x5555, 0x20);
}


/*
 * Generic utility functions
 */

// Dump the specified number of bytes the EEPROM's contents to the serial console
void dumpEEPROM(int bytes) {
  Serial.println("Dumping EEPROM...");

  for (int base = 0; base < bytes; base += 0x10) {
    byte data[16];
    for (int offset = 0; offset < 0x10; offset++) {
      data[offset] = readEEPROM(base + offset);
    }

    char buf[80];
    sprintf(buf, "%04x: %02x %02x %02x %02x %02x %02x %02x %02x    %02x %02x %02x %02x %02x %02x %02x %02x ",
      base,
      data[0x00], data[0x01], data[0x02], data[0x03], data[0x04], data[0x05], data[0x06], data[0x07],
      data[0x08], data[0x09], data[0x0A], data[0x0B], data[0x0C], data[0x0D], data[0x0E], data[0x0F]);

    Serial.println(buf);
  }
}

// Read a line of data from the serial connection.
int readLine(char* buffer, int len) {
    for (int ix = 0; (ix < len); ix++) {
         buffer[ix] = 0;
    }

    // read serial data until linebreak or buffer is full
    char c = ' ';
    int ix = 0;
    do {
        if (Serial.available()) {
            c = Serial.read();
            if ((c == '\b') && (ix > 0))
            {
                // Backspace, forget last character
                --ix;
            }
            buffer[ix++] = c;
            Serial.write(c);
        }
    } while ((c != '\n') && (ix < len));

    buffer[ix - 1] = 0;
    return ix - 1;
}


/*
 * High-level functions specific to the MicroCode EEPROM
 */


enum ControlWord : word {
  CW_NOP = 0x0000,
  CW_AI  = 0x0001,
  CW_AO  = 0x0002,
  CW_BI  = 0x0004,
  CW_CE  = 0x0008,
  CW_CI  = 0x0010,
  CW_CO  = 0x0020,
  CW_II  = 0x0040,
  CW_IO  = 0x0080,
  CW_OI  = 0x0100,
  CW_MI  = 0x0200,
  CW_RI  = 0x0400,
  CW_RO  = 0x0800,
  CW_EO  = 0x1000,
  CW_SUB = 0x2000,
  CW_FI  = 0x4000,
  CW_HLT = 0x8000
};

const word STEP_FETCH_OP1 = CW_CO | CW_MI;         // Move PC to MAR
const word STEP_FETCH_OP2 = CW_RO | CW_II | CW_CE; // Move RAM output to IR and increment the PC

const word STEP_FETCH_VAL1 = CW_CO | CW_MI; // Move PC to MAR
const word STEP_FETCH_VAL2 = CW_RO | CW_CE; // Move RAM output to SOMEWHERE and increment the PC


#define S_OP0() { STEP_FETCH_OP1, STEP_FETCH_OP2, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define S_OP1(a) { STEP_FETCH_OP1, STEP_FETCH_OP2, a, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define S_OP2(a, b) { STEP_FETCH_OP1, STEP_FETCH_OP2, a, b, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define S_OP3(a, b, c) { STEP_FETCH_OP1, STEP_FETCH_OP2, a, b, c, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }

#define D_OP0() { STEP_FETCH_OP1, STEP_FETCH_OP2, STEP_FETCH_VAL1, STEP_FETCH_VAL2, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define D_OP1(a) { STEP_FETCH_OP1, STEP_FETCH_OP2, STEP_FETCH_VAL1, STEP_FETCH_VAL2 | a, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define D_OP2(a, b) { STEP_FETCH_OP1, STEP_FETCH_OP2, STEP_FETCH_VAL1, STEP_FETCH_VAL2 | a, b, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }
#define D_OP3(a, b, c) { STEP_FETCH_OP1, STEP_FETCH_OP2, STEP_FETCH_VAL1, STEP_FETCH_VAL2 | a, b, c, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP, CW_NOP }

const word NOP[] = S_OP0();
const word LDA[] = D_OP2(CW_MI, CW_RO | CW_AI);
const word LDB[] = D_OP2(CW_MI, CW_RO | CW_BI);
const word WRA[] = D_OP1(CW_AI);
const word WRB[] = D_OP1(CW_BI);
const word MOV[] = S_OP1(CW_AO | CW_BI);
const word ADD[] = S_OP2(CW_FI, CW_EO | CW_AI);
const word SUB[] = S_OP2(CW_SUB | CW_FI, CW_EO | CW_SUB | CW_AI);
const word JMP[] = S_OP2(CW_CO | CW_MI, CW_RO | CW_CI);
const word JMP_FALLTHROUGH[] = S_OP1(CW_CE);
const word STO[] = D_OP2(CW_MI, CW_AO | CW_RI);
const word OUT[] = S_OP1(CW_AO | CW_OI);
const word HLT[] = S_OP1(CW_HLT);


#define BASIC(I)     { I, I, I, I, I, I, I, I }
#define IF_ZER(A, B) { B, A, B, A, B, A, B, A }
#define IF_UNK(A, B) { B, B, A, A, B, B, A, A }
#define IF_OVF(A, B) { B, B, B, B, A, A, A, A }

const word* MicroCode[16][8] = {
  BASIC(NOP),
  BASIC(LDA),
  BASIC(LDB),
  BASIC(WRA),
  BASIC(WRB),
  BASIC(MOV),
  BASIC(ADD),
  BASIC(SUB),
  BASIC(JMP),
  IF_ZER(JMP, JMP_FALLTHROUGH), // JZ
  IF_OVF(JMP, JMP_FALLTHROUGH), // JO
  IF_ZER(JMP_FALLTHROUGH, JMP), // JNZ
  IF_OVF(JMP_FALLTHROUGH, JMP), // JNO
  BASIC(STO),
  BASIC(OUT),
  BASIC(HLT)
};


void parseAddress(int address, word *opcode, word *opstep);
void parseAddress(int address, word *opcode, word *opstep) {
  *opstep = address & 0xf;
  *opcode = (address >> 4) & 0x0f;
}

word opcode, opstep;
char buffer[20];

void setup() {
  Serial.begin(115200);

  Serial.println("Performing initial setup...");
  
  setupEEPROM();
  setMode(WriteMode);  
}
  
void loop() {
  Serial.print("> ");
  int len = readLine(buffer, 20);

  if (strcmp(buffer, "h") == 0 || strcmp(buffer, "help") == 0) {
    Serial.println("Commands:");
    Serial.println("u  - unlock the EEPROM");
    Serial.println("d  - dump contents of EEPROM");
    Serial.println("w0 - write byte 0 of control words to EEPROM");
    Serial.println("w1 - write byte 0 of control words to EEPROM");
  } else if (strcmp(buffer, "u") == 0) {
    Serial.println("unlocking...");
    unlockEEPROM();
  } else if (strcmp(buffer, "d") == 0) {
    Serial.println("dumping microcode...");
    dumpEEPROM(0x100);
  } else if (strcmp(buffer, "w0") == 0) {
    Serial.println("writing microcode (#0)...");
    for (int f=0; f<8; f++) {
      Serial.println("writing flag combination...");
      for (int i=0; i<0x100; i++) {
        parseAddress(i, &opcode, &opstep);
        writeEEPROM((f << 12) | i, MicroCode[opcode][f][opstep] & 0xFF);
      }
    }
  } else if (strcmp(buffer, "w1") == 0) {
    Serial.println("writing microcode (#1)...");
    for (int f=0; f<8; f++) {
      Serial.println("writing flag combination...");
      for (int i=0; i<0x100; i++) {
        parseAddress(i, &opcode, &opstep);
        writeEEPROM((f << 12) | i, (MicroCode[opcode][f][opstep] >> 8) & 0xFF);
      }
    }
  }
}

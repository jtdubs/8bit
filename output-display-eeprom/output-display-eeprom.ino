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
 * Low-Lev  el functions that interface directly with the EEPROM and shift register
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
void unlockEEPROM()
{
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

 
/*
 * High-level functions specific to the Output Display EEPROM
 */
 
// Masks that define which segments of the 7-segment display should be on for various symbols
const byte BLANK_MASK   = 0x00;
const byte MINUS_MASK   = 0x01;
const byte DECIMAL_MASK = 0x80;
const byte DIGIT_MASK[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b, 0x77, 0x1f, 0x4e, 0x3d, 0x4f, 0x47 }; // 0-9, a-f

enum FormatFlags { None = 0x00, ZeroPadded = 0x01, ThirdDigit = 0x02, FourthDigit = 0x04, FixedNegativeSign = 0x08 };


/*
 * Fill 1024 bytes of EEPROM with digit masks for unsigned decimal numbers starting at the specified base address
 * 
 * Respects ZeroPadded and FourthDigit format flags as follows:
 * 
 * Value  ZeroPadded  FourthDigit  Output
 * 4      true        true         '0004'
 * 16     true        true         '0016'
 * 128    true        true         '0128'
 * 4      true        false        ' 004'
 * 16     true        false        ' 016'
 * 128    true        false        ' 128'
 * 4      false       *            '   4'
 * 16     false       *            '  16' 
 * 128    false       *            ' 128'
 */
void programUnsignedDecimalMasks(int baseAddress, FormatFlags flags);
void programUnsignedDecimalMasks(int baseAddress, FormatFlags flags=None) {
  Serial.println("Programming unsigned decimal masks...");

  bool zeroPadded = (flags & ZeroPadded) == ZeroPadded;
  bool fourthDigit = (flags & FourthDigit) == FourthDigit;

  for (int value = 0; value < 10; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value]);
    writeEEPROM(baseAddress + 0x100 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded && fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }

  for (int value = 10; value < 100; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value % 10]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[value / 10]);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded && fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }

  for (int value = 100; value < 256; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value % 10]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[(value / 10) % 10]);
    writeEEPROM(baseAddress + 0x200 + value, DIGIT_MASK[value / 100]);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded && fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }
}

/*
 * Fill 1024 bytes of EEPROM with digit masks for signed decimal numbers starting at the specified base address
 * 
 * Respects ZeroPadded and FixedNegativeSign format flags as follows:
 * 
 * Examples:
 * Value  ZeroPadded  FixedNegativeSign Output
 * -128   *           *                 '-128'
 * -64    true        *                 '-064'
 * -8     true        *                 '-008'
 * 8      true        *                 ' 008'
 * 16     true        *                 ' 016'
 * 128    true        *                 ' 128'
 * 8      false       *                 '   8'
 * 16     false       *                 '  16'
 * 128    false       *                 ' 128'
 * -64    false       true              '- 64'
 * -8     false       true              '-  8'
 * -64    false       false             ' -64'
 * -8     false       false             '  -8'
 */
void programSignedDecimalMasks(int baseAddress, FormatFlags flags);
void programSignedDecimalMasks(int baseAddress, FormatFlags flags=None) {
  Serial.println("Programming signed decimal masks...");

  bool zeroPadded = (flags & ZeroPadded) == ZeroPadded;
  bool fixedNegativeSign = (flags & FixedNegativeSign) == FixedNegativeSign;

  for (int value = -128; value < -99; value++) {
    int byteValue = (byte)value;
    int absValue = abs(value);
    writeEEPROM(baseAddress + 0x000 + byteValue, DIGIT_MASK[absValue % 10]);
    writeEEPROM(baseAddress + 0x100 + byteValue, DIGIT_MASK[(absValue / 10) % 10]);
    writeEEPROM(baseAddress + 0x200 + byteValue, DIGIT_MASK[absValue / 100]);
    writeEEPROM(baseAddress + 0x300 + byteValue, MINUS_MASK);
  }

  for (int value = -99; value < 9; value++) {
    int byteValue = (byte)value;
    int absValue = abs(value);
    writeEEPROM(baseAddress + 0x000 + byteValue, DIGIT_MASK[absValue % 10]);
    writeEEPROM(baseAddress + 0x100 + byteValue, DIGIT_MASK[absValue / 10]);
    writeEEPROM(baseAddress + 0x200 + byteValue, zeroPadded ? DIGIT_MASK[0] : (fixedNegativeSign ? BLANK_MASK : MINUS_MASK));
    writeEEPROM(baseAddress + 0x300 + byteValue, (zeroPadded || fixedNegativeSign) ? MINUS_MASK : BLANK_MASK);
  }

  for (int value = -9; value < 0; value++) {
    int byteValue = (byte)value;
    int absValue = abs(value);
    writeEEPROM(baseAddress + 0x000 + byteValue, DIGIT_MASK[absValue]);
    writeEEPROM(baseAddress + 0x100 + byteValue, zeroPadded ? DIGIT_MASK[0] : (fixedNegativeSign ? BLANK_MASK : MINUS_MASK));
    writeEEPROM(baseAddress + 0x200 + byteValue, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + byteValue, (zeroPadded || fixedNegativeSign) ? MINUS_MASK : BLANK_MASK);
  }

  for (int value = 0; value < 10; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value]);
    writeEEPROM(baseAddress + 0x100 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, BLANK_MASK);
  }

  for (int value = 10; value < 100; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value % 10]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[value / 10]);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, BLANK_MASK);
  }

  for (int value = 100; value < 128; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value % 10]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[(value / 10) % 10]);
    writeEEPROM(baseAddress + 0x200 + value, DIGIT_MASK[value / 100]);
    writeEEPROM(baseAddress + 0x300 + value , BLANK_MASK);
  }
}

/*
 * Fill 1024 bytes of EEPROM with digit masks for hex numbers starting at the specified base address
 * 
 * Respects ZeroPadded, ThirdDigit and FourthDigit format flags as follows:
 * 
 * Examples:
 * Value  ZeroPadded  ThirdDigit  FourthDigit Output
 * 10     false       *           *           '   A'
 * 10     true        false       false       '  0A'
 * 10     true        true        false       ' 00A'
 * 10     true        *           true        '000A'
 * 100    false       *           *           '  64'
 * 100    true        false       false       '  64'
 * 100    true        true        false       ' 064'
 * 100    true        *           true        '0064'
 */
void programHexMasks(int baseAddress, FormatFlags flags);
void programHexMasks(int baseAddress, FormatFlags flags=None) {
  Serial.println("Programming hex masks...");

  bool zeroPadded = (flags & ZeroPadded) == ZeroPadded;
  bool thirdDigit = (flags & ThirdDigit) == ThirdDigit;
  bool fourthDigit = (flags & FourthDigit) == FourthDigit;

  for (int value = 0x0; value < 0x10; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value]);
    writeEEPROM(baseAddress + 0x100 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x200 + value, (zeroPadded && (thirdDigit || fourthDigit)) ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded && fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }

  for (int value = 0x10; value < 0x100; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value & 0xF]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[value >> 4]);
    writeEEPROM(baseAddress + 0x200 + value, (zeroPadded && (thirdDigit || fourthDigit)) ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded && fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }
}

/*
 * Fill 1024 bytes of EEPROM with digit masks for octal numbers starting at the specified base address
 * 
 * Respects ZeroPadded and FourthDigit format flags as follows:
 * 
 * Examples:
 * Value  ZeroPadded  FourthDigit Output
 * 6      false       *           '   6'
 * 6      true        false       ' 006'
 * 6      true        true        '0006'
 * 16     false       *           '  20'
 * 16     true        false       ' 020'
 * 16     true        true        '0020'
 * 160    false       *           ' 240'
 * 160    true        false       ' 240'
 * 160    true        true        '0240'
 */
void programOctalMasks(int baseAddress, FormatFlags flags);
void programOctalMasks(int baseAddress, FormatFlags flags=None) {
  Serial.println("Programming octal masks...");
  
  bool zeroPadded = (flags & ZeroPadded) == ZeroPadded;
  bool fourthDigit = (flags & FourthDigit) == FourthDigit;

  for (int value = 0; value < 010; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value]);
    writeEEPROM(baseAddress + 0x100 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded & fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }
  
  for (int value = 010; value < 0100; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value & 0x7]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[value >> 3]);
    writeEEPROM(baseAddress + 0x200 + value, zeroPadded ? DIGIT_MASK[0] : BLANK_MASK);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded & fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }

  for (int value = 0100; value < 0400; value++) {
    writeEEPROM(baseAddress + 0x000 + value, DIGIT_MASK[value & 0x7]);
    writeEEPROM(baseAddress + 0x100 + value, DIGIT_MASK[(value >> 3) & 0x7]);
    writeEEPROM(baseAddress + 0x200 + value, DIGIT_MASK[value >> 6]);
    writeEEPROM(baseAddress + 0x300 + value, (zeroPadded & fourthDigit) ? DIGIT_MASK[0] : BLANK_MASK);
  }
}


/*
 * Entry Point
 */
 
void setup() {
  Serial.begin(115200 );
  
  setupEEPROM();

  programUnsignedDecimalMasks(0x000);
  programSignedDecimalMasks(0x400);
  programHexMasks(0x800, ZeroPadded);
  programOctalMasks(0xC00, ZeroPadded);
  
  dumpEEPROM(0x100);
}

void loop() { }

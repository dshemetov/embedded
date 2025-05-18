// Fixed width integer types, hell yea
// https://en.cppreference.com/w/cpp/types/integer
#include <Arduino.h>

// Arduino ItsyBitsy ESP32
// These are defined in the variant file
// ~/Library/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/adafruit_itsybitsy_esp32/pins_arduino.h
// Pinout here
// https://learn.adafruit.com/adafruit-itsybitsy-esp32?view=all#pinouts
#define D7 7
#define D5 5
#define EEWE T8  // Write Enable to EEPROM
#define EEOE T9  // Output Enable to EEPROM
#define SROE D7  // Output Enable to Shift Register
#define SRCLK D5 // Shift Clock to Shift Register
#define RCLK SCL // Storage Clock to Shift Register
#define SER SDA  // Serial Data to Shift Register

void setup() {
    // put your setup code here, to run once:
    // Setup so we can print debug messages
    Serial.begin(9600);

    pinMode(SER, OUTPUT);
    pinMode(RCLK, OUTPUT);
    pinMode(SRCLK, OUTPUT);
    // Ensure EEPROM is not in write mode and not in output mode
    // on startup
    digitalWrite(EEWE, HIGH);
    digitalWrite(EEOE, HIGH);
    pinMode(EEWE, OUTPUT);
    pinMode(EEOE, OUTPUT);
    pinMode(SROE, OUTPUT);

    // Start clock pin low
    digitalWrite(SRCLK, LOW);
    digitalWrite(SROE, LOW);

    // Clear pins
    shiftOut(SER, SRCLK, LSBFIRST, 0x00);
    shiftOut(SER, SRCLK, LSBFIRST, 0x00);
    shiftOut(SER, SRCLK, LSBFIRST, 0x00);

    // Write
    digitalWrite(RCLK, LOW);
    digitalWrite(RCLK, HIGH);
    digitalWrite(RCLK, LOW);

    chip_erase();
    // write_file();
    write_simple(50);
    read_simple(0, 100);
}

void write_simple(uint8_t offset) {
    uint16_t address = 0;
    // Do the execute program dance
    for (uint8_t i = 0 + offset; i <= 42 + offset; i++) {
        write_to_eeprom(address + i, i);
    }
}

void write_to_eeprom(uint16_t address, uint8_t data) {
    send_3bytes(0x5555, 0xAA);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x2AAA, 0x55);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x5555, 0xA0);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(address, data);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
}

// Chipper Ace
//  /\  /\
// |  '   '
// |   ------"
// |  -----
//
void chip_erase() {
    send_3bytes(0x5555, 0xAA);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x2AAA, 0x55);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x5555, 0x80);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x5555, 0xAA);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x2AAA, 0x55);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    send_3bytes(0x5555, 0x10);
    digitalWrite(EEWE, LOW);
    digitalWrite(EEWE, HIGH);
    delay(101);
}

void read_from_eeprom(uint16_t address) {
    // This is the LED approach
    // Send address to read
    send_3bytes(address, 0);
    // Impede SR output
    digitalWrite(SROE, HIGH);
    // Send OE to EEPROM
    digitalWrite(EEOE, LOW);
    delay(250);
    digitalWrite(EEOE, HIGH);
    digitalWrite(SROE, LOW);

    // This is the level shifter approach
    // Read one byte from the EEPROM
    // Send address to shift register
    // send_3bytes(0x0000, 0x00);
    // Toggle output enable
    // digitalWrite(EEOE, LOW);
    // uint8_t data = 0;
    // data += digitalRead(D0);
    // data = (data << 1) + digitalRead(D1);
    // data = (data << 1) + digitalRead(D2);
    // data = (data << 1) + digitalRead(D3);
    // data = (data << 1) + digitalRead(D4);
    // data = (data << 1) + digitalRead(D5);
    // data = (data << 1) + digitalRead(D6);
    // data = (data << 1) + digitalRead(D7);
    // Serial.println("Read: ", data);
}

void read_simple(uint8_t offset, uint16_t limit) {
    for (uint16_t i = 0 + offset; i <= limit; i++) {
        read_from_eeprom(i);
    }
}

// Sends 3 bytes to shift registers
void send_3bytes(uint16_t address, uint8_t data) {
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, (address >> 8));
    shiftOut(SER, SRCLK, MSBFIRST, (address));
    shiftOut(SER, SRCLK, MSBFIRST, data);
    digitalWrite(RCLK, HIGH);
}

void loop() {
    // put your main code here, to run repeatedly:
    // digitalWrite(SROE, LOW);

    // send_3bytes(0xaaaa, 0xaa);
    // delay(500);
    // send_3bytes(0, 0);
    // delay(500);
    // delayMicroseconds(3);
    // send_3bytes(0x55aa00, 128);
    // digitalWrite(SRCLK, LOW);
    // digitalWrite(SRCLK, HIGH);
    // digitalWrite(RCLK, LOW);
    // digitalWrite(RCLK, HIGH);
    // digitalWrite(SROE, LOW);
    // digitalWrite(SROE, HIGH);
}

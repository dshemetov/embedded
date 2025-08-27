// Fixed width integer types, hell yea
// https://en.cppreference.com/w/cpp/types/integer
#include <Arduino.h>

// Arduino ItsyBitsy ESP32
// Some are defined in the variant file
// ~/Library/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/adafruit_itsybitsy_esp32/pins_arduino.h
// Pinout here
// https://learn.adafruit.com/adafruit-itsybitsy-esp32?view=all#pinouts
#define EEPROM_WRITE_ENABLE T8  // Write Enable to EEPROM
#define EEPROM_OUTPUT_ENABLE T9 // Output Enable to EEPROM
#define SROE 7                 // Output Enable to Shift Register
#define SRCLK 5                // Shift Clock to Shift Register
#define RCLK SCL                // Storage Clock to Shift Register
#define SER SDA                 // Serial Data to Shift Register

void setup() {
    Serial.begin(115200);

    // Initialize pins
    pinMode(SER, OUTPUT);
    pinMode(RCLK, OUTPUT);
    pinMode(SRCLK, OUTPUT);
    pinMode(SROE, OUTPUT);
    pinMode(EEPROM_WRITE_ENABLE, OUTPUT);
    pinMode(EEPROM_OUTPUT_ENABLE, OUTPUT);

    // EEPROM is disabled on startup
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    digitalWrite(EEPROM_OUTPUT_ENABLE, HIGH);
    // Shift register output enable
    digitalWrite(SROE, HIGH);

    // Start clock pin low
    digitalWrite(SRCLK, LOW);
    // Enable shift register output
    digitalWrite(SROE, LOW);
    // Clear shift registers
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, 0x00);
    shiftOut(SER, SRCLK, MSBFIRST, 0x00);
    shiftOut(SER, SRCLK, MSBFIRST, 0x00);
    digitalWrite(RCLK, HIGH);

    // Erase the EEPROM
    chip_erase();

    // Basic test program.
    // write_to_eeprom(10, 0b10101010);
    // write_to_eeprom(11, 0b01010101);
    // write_to_eeprom(12, 0b11111111);
    // write_to_eeprom(13, 0b00000000);

    // Write 1 through 50 pattern starting at address 0
    write_simple_pattern(0, 1, 50);

    // Read the pattern
    read_simple_pattern(0, 0, 75);
}

void send_byte_to_shift_register(uint8_t data) {
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, data);
    digitalWrite(RCLK, HIGH);
}

void send_address_data_to_shift_register(uint16_t address, uint8_t data) {
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, (address >> 8));
    shiftOut(SER, SRCLK, MSBFIRST, (address));
    shiftOut(SER, SRCLK, MSBFIRST, data);
    digitalWrite(RCLK, HIGH);
}

void read_byte_from_eeprom(uint16_t address) {
    // Queue address to read
    send_address_data_to_shift_register(address, 0);
    // Impede shift register data output
    digitalWrite(SROE, HIGH);
    // Enable EEPROM output
    digitalWrite(EEPROM_OUTPUT_ENABLE, LOW);
    // Give time to view the data on the LEDs
    delay(250);
    // Disable EEPROM output
    digitalWrite(EEPROM_OUTPUT_ENABLE, HIGH);
    // Re-enable shift register data output
    digitalWrite(SROE, LOW);
}

void write_simple_pattern(uint16_t address, uint8_t offset, uint8_t count) {
    for (uint8_t i = offset; i <= count; i++) {
        write_to_eeprom(address + i, i);
    }
}

void read_simple_pattern(uint16_t address, uint8_t offset, uint8_t count) {
    for (uint8_t i = offset; i <= count; i++) {
        read_byte_from_eeprom(address + i);
    }
}

void write_to_eeprom(uint16_t address, uint8_t data) {
    // Write protection sequence
    send_address_data_to_shift_register(0x5555, 0xAA);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x2AAA, 0x55);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x5555, 0xA0);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    // Write data
    send_address_data_to_shift_register(address, data);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
}

// Chipper Ace
//  /\  /\
// |  '   '
// |   ------"
// |  -----
//
void chip_erase() {
    send_address_data_to_shift_register(0x5555, 0xAA);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x2AAA, 0x55);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x5555, 0x80);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x5555, 0xAA);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x2AAA, 0x55);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    send_address_data_to_shift_register(0x5555, 0x10);
    digitalWrite(EEPROM_WRITE_ENABLE, LOW);
    digitalWrite(EEPROM_WRITE_ENABLE, HIGH);
    delay(101);
}


void loop() {
    // Test bits.

    // Send a byte to the shift register
    // send_byte_to_shift_register(0b10101010);
    // delay(500);
    // send_byte_to_shift_register(0b01010101);
    // delay(500);
    // send_byte_to_shift_register(0b11111111);
    // delay(500);
    // send_byte_to_shift_register(0b00000000);
    // delay(500);

    // Read the data from the EEPROM
    // read_byte_from_eeprom(10);
    // read_byte_from_eeprom(11);
    // read_byte_from_eeprom(12);
    // read_byte_from_eeprom(13);
}

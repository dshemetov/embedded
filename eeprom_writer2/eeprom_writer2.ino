#include <Arduino.h>

// Protocol settings
static const uint16_t PAGE_SIZE = 1024;
// Protocol bytes
static const uint8_t SOF0 = 0xAA, SOF1 = 0x55;
static const uint8_t CMD_WRITE = 'W';
static const uint8_t CMD_READ = 'R';
static const uint8_t RPY_ACK = 'A';
static const uint8_t RPY_DATA = 'D';

// Checksum.
uint16_t sum16(const uint8_t *p, size_t n) {
    uint16_t s = 0;
    for (size_t i = 0; i < n; ++i)
        s += (uint16_t)p[i];
    return s & 0xFFFF;
}

// Serial read.
bool read_exact(uint8_t *buf, size_t n, uint32_t timeout_ms = 1000) {
    uint32_t t0 = millis();
    size_t got = 0;
    while (got < n) {
        if (Serial.available()) {
            got += Serial.readBytes(buf + got, n - got);
        } else if (millis() - t0 > timeout_ms) {
            return false;
        }
    }
    return true;
}

// 4 byte array -> 32 bit integer, little endian
uint32_t rd_le32(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}
// 2 byte array -> 16 bit integer, little endian
uint16_t rd_le16(const uint8_t *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
// 32 bit integer -> 4 byte array, little endian
void wr_le32(uint8_t *p, uint32_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
    p[2] = (v >> 16) & 0xFF;
    p[3] = (v >> 24) & 0xFF;
}
// 16 bit integer -> 2 byte array, little endian
void wr_le16(uint8_t *p, uint16_t v) {
    p[0] = v & 0xFF;
    p[1] = (v >> 8) & 0xFF;
}

// Much of the same code as in eeprom_writer.ino below.

// Arduino ItsyBitsy ESP32
// Some are defined in the variant file
// ~/Library/Arduino15/packages/esp32/hardware/esp32/3.2.0/variants/adafruit_itsybitsy_esp32/pins_arduino.h
// Pinout here
// https://learn.adafruit.com/adafruit-itsybitsy-esp32?view=all#pinouts
#define EEPROM_WRITE_ENABLE T8  // Write Enable to EEPROM
#define EEPROM_OUTPUT_ENABLE T9 // Output Enable to EEPROM
#define SROE 7                  // Output Enable to Shift Register
#define SRCLK 5                 // Shift Clock to Shift Register
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

    // Erase the EEPROM, if you wish
    chip_erase();
}

void loop() {
    // Seek SOF0 and SOF1
    while (Serial.available() < 2) {
        delay(1);
    }
    int b1 = Serial.read();
    if (b1 != SOF0) {
        delay(1);
        return;
    }
    int b2 = Serial.read();
    if (b2 != SOF1) {
        delay(1);
        return;
    }
    Serial.println("Found header.");

    // Header after SOF: [CMD:1] [ADDR:4 LE] [LEN:2 LE]
    uint8_t header_buf[1 + 4 + 2];
    if (!read_exact(header_buf, sizeof(header_buf))) {
        Serial.println("Header not found!");
        return;
    }
    uint8_t cmd = header_buf[0];
    uint32_t addr = rd_le32(header_buf + 1);
    if (addr > 0xFFFF) {
        Serial.println("Address currently restricted to 16 bits!");
        return;
    }
    uint16_t len = rd_le16(header_buf + 5);

    if (cmd == CMD_WRITE) {
        Serial.println("Write instruction found, reading data to write.");
        if (len == 0 || len > PAGE_SIZE) {
            Serial.println("Invalid length!");
            return;
        }
        Serial.println("Got length");
        Serial.println(len);
        uint8_t data_buf[PAGE_SIZE];
        if (!read_exact(data_buf, len)) {
            Serial.println("Data not found!");
            return;
        }
        Serial.println("Data received, verifying checksum...");
        uint8_t cksum_buf[2];
        if (!read_exact(cksum_buf, sizeof(cksum_buf))) {
            Serial.println("Checksum not found!");
            return;
        }
        uint16_t cksum = rd_le16(cksum_buf);
        Serial.println("Got checksum");
        Serial.println(cksum);
        if (cksum != sum16(data_buf, len)) {
            Serial.println("Checksum mismatch!");
            return;
        }
        Serial.println("Checksum verified.");
        // Debug echo
        Serial.println("Data received, checksum verified, echoing back data:");
        // for (int i = 0; i < len; i++) {
        //     Serial.println(data_buf[i], HEX);
        // }
        // Serial.println("Done echoing data.");
        // Write to EEPROM
        for (int i = 0; i < len; i++) {
            write_to_eeprom((uint16_t)(addr + i), data_buf[i]);
        }
        Serial.println("Done writing data to EEPROM.");
        // Send ACK to host
        Serial.println("Sending ACK to host...");
        Serial.write(SOF0);
        Serial.write(SOF1);
        Serial.write(RPY_ACK);
        Serial.write(sum16(data_buf, len));
        Serial.flush();
        Serial.println("Done sending ACK.");
    } else if (cmd == CMD_READ) {
        Serial.println("Read instruction found, fetching data to return.");
        // Fetch data from EEPROM.
        // TODO: I don't actually have a way to read the data from the
        // EEPROM aside from LEDs.
        for (int i = 0; i < len; i++) {
            // Get ready to read this from the LEDs.
            read_byte_from_eeprom((uint16_t)(addr + i));
        }
    } else {
        Serial.println("Invalid command found!");
        return;
    }
    delay(1000);
}

void send_byte_to_shift_register(uint8_t data) {
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, data);
    digitalWrite(RCLK, HIGH);
}

// We have three 8-bit shift registers.
// SR1 SR2 SR3 <- data starts on the right-most shift register and marches left.
// After loading we should have
// address[15:8] address[7:0] data
// Currently, the top 16 bits of the address are ignored, since shiftOut only
// takes 8 bits at a time.
void send_address_data_to_shift_register(uint32_t address, uint8_t data) {
    digitalWrite(RCLK, LOW);
    shiftOut(SER, SRCLK, MSBFIRST, (uint8_t)(address >> 8));
    shiftOut(SER, SRCLK, MSBFIRST, (uint8_t)(address & 0xFF));
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
    // TODO: Read the data from the data pins
    // Give time to view the data on the LEDs
    delay(1000);
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

void write_to_eeprom(uint32_t address, uint8_t data) {
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

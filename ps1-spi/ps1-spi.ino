// SPI Test Driver for PS1 Controller
// References: 
// - https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface
// - https://github.com/SukkoPera/PsxNewLib?tab=readme-ov-file
// - https://learn.adafruit.com/adafruit-itsybitsy-esp32/pinouts
// - https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
// - https://github.com/lemmingDev/ESP32-BLE-Gamepad
#include <Arduino.h>
#include <SPI.h>
#include <BleGamepad.h>

#define ATT 26 // A1
#define numButtons 16

// TODO: Deep sleep and wake
// TODO: More latency testing

SPISettings psx(250000, LSBFIRST, SPI_MODE3);
BleGamepad bleGamepad("PSX BLT", "dskel", 100);
byte previousButtonStates[numButtons];
byte currentButtonStates[numButtons];
// TODO: Map to something else?
const byte buttonMap[numButtons] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
// TODO: Unsigned long if you fear overflow?
int last_button_press = millis();
int sleep_time = (30 * 1000);
const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";

void setup() {
    Serial.begin(115200);
    delay(500);
    
    for (int i = 0; i < numButtons; i++) { 
        previousButtonStates[i] = HIGH;
        currentButtonStates[i] = HIGH;
    }
    pinMode(ATT, OUTPUT);
    digitalWrite(ATT, HIGH);
    
    SPI.begin(19, 22, 21, ATT); // SCK, MISO, MOSI
    Serial.println("Starting BLE work!");
    bleGamepad.begin();
    delay(100);
}

uint16_t poll_pad() {
    const uint8_t tx[5] = {0x01, 0x42, 0xff, 0xff, 0xff};
    uint8_t rx[5];

    digitalWrite(ATT, LOW);
    SPI.beginTransaction(psx);
    delayMicroseconds(20);
    // Expected: FF ID 5A RX RX
    for (int i = 0; i < 5; i++) {
        rx[i] = SPI.transfer(tx[i]);
        delayMicroseconds(5);          
    }
    SPI.endTransaction();
    
    digitalWrite(ATT, HIGH);
    digitalWrite(SCK, HIGH);
    digitalWrite(MOSI, HIGH);
    digitalWrite(MISO, HIGH);
    delayMicroseconds(20);
    
    log_rx(rx);
    
    if (rx[1] != 0x41 || rx[2] != 0x5A) return 0xffff;  // no pad?
    // RX RX (bits): L D R U St R3 L3 Se □ X O △ R1 L1 R2 L2
    return ~(rx[3]<<8 | rx[4]); // active-high
}

void log_rx(uint8_t* rx) {
    Serial.printf("RX: %02X %02X\n", rx[3], rx[4]);
    delay(1);
    Serial.print(CLEAR_SCREEN_ANSI);
}

void loop() {
    uint16_t rx;
    
    // Assume we're connected
    if (bleGamepad.isConnected()) {
        // Poll buttons
        rx = poll_pad();
        for (int i = 0; i < numButtons; i++) {
            // Unpack buttons
            currentButtonStates[i] = rx & (1 << i);
            if (currentButtonStates[i] != previousButtonStates[i]) {
                last_button_press = millis();
                
                if (currentButtonStates[i] == LOW) {
                    bleGamepad.release(buttonMap[i]);
                }
                else {
                    bleGamepad.press(buttonMap[i]);
                }
            }            
            // bleGamepad.sendReport();
        }
        if (currentButtonStates != previousButtonStates) {
            for (byte j = 0; j < numButtons; j++) {
                previousButtonStates[j] = currentButtonStates[j];
            }
        }
        
    }
}


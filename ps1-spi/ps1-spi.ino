// ESP32 Bluetooth PS1 Controller
//
// Code for my Adafruit ItsyBitsy ESP32 PS1 controller project.
// The ESP32 runs in device-mode, reading PS1 controller signals via SPI
// and communicate with hosts via BLE (Bluetooth).
//
// References:
// * https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface
// * https://github.com/SukkoPera/PsxNewLib?tab=readme-ov-file
// * https://learn.adafruit.com/adafruit-itsybitsy-esp32/pinouts
// * https://docs.arduino.cc/language-reference/en/functions/communication/SPI/
// * https://github.com/lemmingDev/ESP32-BLE-Gamepad
// * https://github.com/adafruit/Adafruit_NeoPixel
// TODO:
// * Map the buttons correctly.
// * Test deep sleep and wake.
// * More latency testing.
// * Test LED.
// * Connecting to start button might be annoying, consider an extra button?
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BleGamepad.h>
#include <SPI.h>

// Feature flags for incremental development. Comment out to disable features.
// #define USE_LED        // LED on pin 0
// #define USE_DEEP_SLEEP // Deep sleep functionality

// A0 is tied to ACK, but I don't actually poll it
#define ATT 26 // A1, controller select
#define numButtons 16

// DeepSleep has two modes EXT0 and EXT1.
// EXT0 allows 1 wake-up pin, low latency, and chosen level must be held until
// sleep re-arms. EXT1 allows up to 8 wake-up pins, can be triggered by any-high
// or all-low. Going with EXT0 here, for EXT1 see
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/ExternalWakeUp/ExternalWakeUp.ino
// This pin must be RTC GPIO
#define WAKE_PIN 4 // A2

SPISettings psx(250000, LSBFIRST, SPI_MODE3);
BleGamepad bleGamepad("PSX BLT", "dskel", 100);
byte previousButtonStates[numButtons];
byte currentButtonStates[numButtons];
// Bit order: L D R U St R3 L3 Se □ X O △ R1 L1 R2 L2
const byte buttonMap[numButtons] = {0, 1, 2,  3,  4,  5,  6,  7,
                                    8, 9, 10, 11, 12, 13, 14, 15};
const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
const int sleep_time = (3 * 60 * 1000); // 3 mins
int last_button_press = millis();
#ifdef USE_LED
Adafruit_NeoPixel pixel(1, 0, NEO_GRB + NEO_KHZ800);
#endif

void setup() {
    Serial.begin(115200);
    delay(500);
    pinMode(WAKE_PIN, INPUT_PULLUP); // idle HIGH

#ifdef USE_DEEP_SLEEP
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 0); // wake LOW
#endif

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
#ifdef USE_LED
    pixel.begin();
    pixel.clear();
#endif
}

uint16_t poll_pad() {
    // PS1 poll command and listen high
    const uint8_t tx[5] = {0x01, 0x42, 0xff, 0xff, 0xff};
    uint8_t rx[5];

    digitalWrite(ATT, LOW);
    SPI.beginTransaction(psx);
    delayMicroseconds(20);
    // Expected: FF ID 5A RX RX
    for (int i = 0; i < 5; i++) {
        rx[i] = SPI.transfer(tx[i]);
        delayMicroseconds(5); // ensure we don't just read MOSI back
    }
    SPI.endTransaction();

    digitalWrite(ATT, HIGH);
    digitalWrite(SCK, HIGH);
    digitalWrite(MOSI, HIGH);
    digitalWrite(MISO, HIGH);
    delayMicroseconds(20);

    if (rx[1] != 0x41 || rx[2] != 0x5A)
        return 0xffff;            // no pad?
    return ~(rx[3] << 8 | rx[4]); // convert to active-high
}

void loop() {
    uint16_t rx;

    // Assume we're connected
    if (bleGamepad.isConnected()) {
#ifdef USE_LED
        pixel.setPixelColor(0, 0x000033);
        pixel.show();
#endif

        rx = poll_pad();
        // Unpack buttons
        for (uint8_t i = 0; i < numButtons; i++) {
            currentButtonStates[i] = rx & (1 << i);
            if (currentButtonStates[i] != previousButtonStates[i]) {
                last_button_press = millis();

                // bleGamepad default sends a report every state change
                if (currentButtonStates[i] == LOW) {
                    bleGamepad.release(buttonMap[i]);
                } else {
                    bleGamepad.press(buttonMap[i]);
                }
            }
        }
        if (currentButtonStates != previousButtonStates) {
            // Log on update
            Serial.print(CLEAR_SCREEN_ANSI);
            Serial.printf("RX: %02X %02X\n", rx >> 8, rx & 0xff);
            for (uint8_t j = 0; j < numButtons; j++) {
                previousButtonStates[j] = currentButtonStates[j];
            }
        }
#ifdef USE_DEEP_SLEEP
        if (millis() - last_button_press > sleep_time) {
            delay(300);
            esp_deep_sleep_start();
        }
#endif
    }
}

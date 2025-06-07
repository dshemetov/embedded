// ESP32 Bluetooth PS1 Controller
//
// Code for my Adafruit ItsyBitsy ESP32 PSX controller project.
// The ESP32 runs in device-mode, reads PSX controller signals via SPI,
// and communicates with hosts via BLE (Bluetooth).
//
// Controller Pinout:
// | Pin | Typical Color | My Color | Function |
// | 1   | Brown         | Brown    | DATA/RX  |
// | 2   | Orange        | White    | CMD/ATT  |
// | 3   | ---           | ---      | NONE     |
// | 4   | Black         | Orange   | GND      |
// | 5   | Red           | Yellow   | 3.3V     |
// | 6   | Yellow        | Green    | SELECT   |
// | 7   | Blue          | Blue     | CLK      |
// | 8   | ---           | ---      | NONE     |
// | 9   | Green         | Black    | ACK      |
//
// References:
// * PSX SPI:
// https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface
// * PSX Arduino
// Library:https://github.com/SukkoPera/PsxNewLib?tab=readme-ov-file
// * ESP32 Pinouts: https://learn.adafruit.com/adafruit-itsybitsy-esp32/pinouts
// * Arduino SPI:
// https://docs.arduino.cc/language/reference/en/functions/communication/SPI/
// * ESP32 BLE Gamepad: https://github.com/lemmingDev/ESP32-BLE-Gamepad
// * Adafruit NeoPixel: https://github.com/adafruit/Adafruit_NeoPixel
// * HID Reports (not very useful)
// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
//
// TODO:
// * Connect R1 to GPIO for wake.
// * Test deep sleep and wake.
// * Solder wires directly to Arduino and remove breadboard.
// * Attach the USB-C port and the battery.
// * Attach the Arduino.
// * Angle so LED is exposed.
// * Get the Bluetooth to be less aggressive (currently re-pairs immediately
//   after disconnecting).
// * More latency testing.
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <BleGamepad.h>
#include <SPI.h>

// Feature flags for incremental development. Comment out to disable features.
#define USE_LED // LED on pin 0
// #define USE_DEEP_SLEEP // Deep sleep functionality
#define WIFI_UPDATES // Enable wireless firmware updates

#ifdef WIFI_UPDATES
#include "wifi_updates.h" // OTA WiFi Updates
#endif

// A0 is tied to ACK, but I don't actually poll it
#define ATT 26        // A1, controller select
#define SCK 19        // controller clock
#define MISO 22       // controller data out
#define MOSI 21       // controller data in
#define numButtons 20 // Increase to avoid D-pad button conflicts
// DeepSleep has two modes EXT0 and EXT1.
// EXT0 allows 1 wake-up pin, low latency, and chosen level must be held until
// sleep re-arms. EXT1 allows up to 8 wake-up pins, can be triggered by any-high
// or all-low. Going with EXT0 here, for EXT1 see
// https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/ExternalWakeUp/ExternalWakeUp.ino
// This pin must be RTC GPIO
#define WAKE_PIN 4 // A2

SPISettings psx(250000, LSBFIRST, SPI_MODE3);
BleGamepad bleGamepad("PSX BLT", "dskel", 100); // battery level
bool previousButtonStates[numButtons];
bool currentButtonStates[numButtons];
// Bit number:            15 14 13 12 11 10 9  8  7  6  5  4  3  2  1  0
// From controller (MSB): L  D  R  U  St R3 L3 Se □  X  O  △  R1 L1 R2 L2
// This map took some trial and error with
// https://gamepadtester.net
const byte buttonMap[numButtons] = {7,   // 0 L2
                                    8,   // 1 R2
                                    5,   // 2 L1
                                    6,   // 3 R1
                                    4,   // 4 △
                                    2,   // 5 O
                                    1,   // 6 X
                                    3,   // 7 □
                                    9,   // 8 Se
                                    9,   // 9 L3
                                    10,  // 10 R3
                                    10,  // 11 St
                                    14,  // 12 R
                                    16,  // 13 L
                                    15,  // 14 U
                                    13}; // 15 D

// PSX D-pad bit number
#define PSX_LEFT 15
#define PSX_DOWN 14
#define PSX_RIGHT 13
#define PSX_UP 12
#define PSX_START 11
#define PSX_SELECT 8

const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
const int sleep_time = (3 * 60 * 1000); // 3 mins
int last_button_press = millis();
#ifdef USE_LED
Adafruit_NeoPixel pixel(1, 0, NEO_GRB + NEO_KHZ800);
#endif

void setup() {
    Serial.begin(115200);
    delay(500);

#ifdef USE_DEEP_SLEEP
    pinMode(WAKE_PIN, INPUT_PULLUP);                       // idle HIGH
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 0); // wake LOW
#endif

    for (int i = 0; i < numButtons; i++) {
        previousButtonStates[i] = false;
        currentButtonStates[i] = false;
    }
    pinMode(ATT, OUTPUT);
    digitalWrite(ATT, HIGH);

    SPI.begin(SCK, MISO, MOSI, ATT);
    Serial.println("Starting BLE work!");
    BleGamepadConfiguration bleGamepadConfig;
    // Add start and select as special buttons (might be needed).
    bleGamepadConfig.setIncludeStart(true);
    bleGamepadConfig.setIncludeSelect(true);
    // Disable axes.
    bleGamepadConfig.setWhichAxes(false, false, false, false, false, false,
                                  false, false);
    bleGamepad.begin(&bleGamepadConfig);
    delay(100);
#ifdef USE_LED
    pixel.begin();
    pixel.clear();
#endif

#ifdef WIFI_UPDATES
    wifiInit();
    webServerInit();
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
    bool changed = false;

    // Assume we're connected
    if (bleGamepad.isConnected()) {
#ifdef USE_LED
        pixel.setPixelColor(0, 0x000033);
        pixel.show();
#endif

        rx = poll_pad();

        // Handle D-pad separately
        uint8_t dpadState = DPAD_CENTERED;
        if (rx & (1 << PSX_UP)) {        // D-pad Up
            if (rx & (1 << PSX_RIGHT)) { // D-pad Right
                dpadState = DPAD_UP_RIGHT;
            } else if (rx & (1 << PSX_LEFT)) { // D-pad Left
                dpadState = DPAD_UP_LEFT;
            } else {
                dpadState = DPAD_UP;
            }
        } else if (rx & (1 << PSX_DOWN)) { // D-pad Down
            if (rx & (1 << PSX_RIGHT)) {   // D-pad Right
                dpadState = DPAD_DOWN_RIGHT;
            } else if (rx & (1 << PSX_LEFT)) { // D-pad Left
                dpadState = DPAD_DOWN_LEFT;
            } else {
                dpadState = DPAD_DOWN;
            }
        } else if (rx & (1 << PSX_RIGHT)) { // D-pad Right
            dpadState = DPAD_RIGHT;
        } else if (rx & (1 << PSX_LEFT)) { // D-pad Left
            dpadState = DPAD_LEFT;
        }
        bleGamepad.setHat(dpadState);

        // Handle regular buttons
        for (uint8_t i = 0; i < numButtons; i++) {
            currentButtonStates[i] = rx & (1 << i);
            if (currentButtonStates[i] != previousButtonStates[i]) {
                last_button_press = millis();
                changed = true;

                // bleGamepad default sends a report every state change
                if (currentButtonStates[i] == LOW) {
                    bleGamepad.release(buttonMap[i]);
                    if (i == PSX_START) {
                        bleGamepad.releaseStart();
                    } else if (i == PSX_SELECT) {
                        bleGamepad.releaseSelect();
                    }
                } else {
                    bleGamepad.press(buttonMap[i]);
                    if (i == PSX_START) {
                        bleGamepad.pressStart();
                    } else if (i == PSX_SELECT) {
                        bleGamepad.pressSelect();
                    }
                }
            }
        }

        if (changed) {
            // // Log on update
            // Serial.print(CLEAR_SCREEN_ANSI);
            // // Print raw hex value
            // Serial.printf("Raw RX: 0x%04X\n", rx);
            // // Print binary representation with bit numbers
            // Serial.println("Binary: ");
            // for (int i = 15; i >= 0; i--) {
            //     Serial.print((rx & (1 << i)) ? "1  " : "0  ");
            // }
            // Serial.println();
            // for (uint8_t j = 0; j < numButtons; j++) {
            //     previousButtonStates[j] = currentButtonStates[j];
            // }
        }
#ifdef USE_DEEP_SLEEP
        if (millis() - last_button_press > sleep_time) {
            delay(300);
            esp_deep_sleep_start();
        }
#endif
    } else {
#ifdef USE_LED
        // Flash LED to indicate disconnected
        for (int i = 0; i < 3; i++) {
            pixel.setPixelColor(0, 0x000033);
            pixel.show();
            delay(300);
            pixel.setPixelColor(0, 0x000000);
            pixel.show();
            delay(300);
        }
#endif
    }
#ifdef WIFI_UPDATES
    server.handleClient();
#endif
}

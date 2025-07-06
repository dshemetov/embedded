// ESP32 Bluetooth PSX Controller
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <BleGamepad.h>

// Feature flags for incremental development.
#define USE_LED        // NeoPixel LED on pin 0.
#define USE_DEEP_SLEEP // Deep sleep after no input timeout.
#define WIFI_UPDATES   // Enable wireless firmware updates.

#ifdef WIFI_UPDATES
#include "wifi_updates.h"
#endif

// A0 is tied to ACK, but I don't actually poll it.
#define ATT 26  // A1, controller select.
#define SCK 19  // Controller clock.
#define MISO 22 // Controller data out.
#define MOSI 21 // Controller data in.
#ifdef USE_DEEP_SLEEP
#define WAKE_PIN 4                            // A2, pin must be RTC GPIO.
const int deep_sleep_after = (3 * 60 * 1000); // 3 mins.
int last_button_press = millis();
void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("Wakeup caused by external signal using RTC_IO");
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("Wakeup caused by external signal using RTC_CNTL");
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("Wakeup caused by timer");
        break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
        Serial.println("Wakeup caused by touchpad");
        break;
    case ESP_SLEEP_WAKEUP_ULP:
        Serial.println("Wakeup caused by ULP program");
        break;
    default:
        Serial.printf("Wakeup was not caused by deep sleep: %d\n",
                      wakeup_reason);
        break;
    }
}
#endif

// Start SPI at 250kHz.
// https://docs.arduino.cc/learn/communication/spi/
SPISettings psx(250000, LSBFIRST, SPI_MODE3);
// Start BLE and signal 100% battery.
// https://github.com/lemmingDev/ESP32-BLE-Gamepad
BleGamepad bleGamepad("PSX BLT", "dskel", 100);
const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
#ifdef USE_LED
Adafruit_NeoPixel pixel(1, 0, NEO_GRB + NEO_KHZ800);
#endif
// We keep track of 16 buttons, so we can map them from their position in the
// SPI 2-byte report to the HID gamepad buttons.
#define numButtons 16
bool previousButtonStates[numButtons];
bool currentButtonStates[numButtons];
int previousDpadState = DPAD_CENTERED;
// PSX button bit number (2 bytes received MSB first).
#define PSX_L2 0
#define PSX_R2 1
#define PSX_L1 2
#define PSX_R1 3
#define PSX_Y 4
#define PSX_B 5
#define PSX_A 6
#define PSX_X 7
#define PSX_SELECT 8
#define PSX_L3 9
#define PSX_R3 10
#define PSX_START 11
#define PSX_UP 12
#define PSX_RIGHT 13
#define PSX_DOWN 14
#define PSX_LEFT 15
// Try to get a decent default mapping; values here should be below 10.
const byte buttonMap[numButtons] = {
    // Counting is 1-16.
    1,  // 0 L2
    2,  // 1 R2
    3,  // 2 L1
    4,  // 3 R1
    5,  // 4 Y
    6,  // 5 B
    7,  // 6 A
    8,  // 7 X
    9,  // 8 Select
    10, // 9 L3
    11, // 10 R3
    12, // 11 Start
    13, // 12 Up
    14, // 13 Right
    15, // 14 Down
    16, // 15 Left
};

#define SPECIAL_SEQUENCE_TIMEOUT (3 * 1000) // 3 seconds
unsigned long bond_delete_start = 0;
bool bond_delete_sequence = false;
unsigned long restart_start = 0;
bool restart_sequence = false;

void setup() {
    // Initialize serial for debugging.
    Serial.begin(115200);
    delay(500);

#ifdef USE_DEEP_SLEEP
    print_wakeup_reason();
    pinMode(WAKE_PIN, INPUT_PULLUP);                       // Idle HIGH.
    esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKE_PIN, 0); // Wake LOW.
#endif

    // Initialize button states to off.
    for (int i = 0; i < numButtons; i++) {
        previousButtonStates[i] = false;
        currentButtonStates[i] = false;
    }

    pinMode(ATT, OUTPUT);
    digitalWrite(ATT, HIGH);
    // Note: MISO needs to be INPUT_PULLUP to idle high. The default SPI library
    // sets it to INPUT though, so I had to modify that in esp32-hal-spi.c.
    SPI.begin(SCK, MISO, MOSI, ATT);

    // Initialize BLE.
    Serial.println("Starting BLE work!");
    BleGamepadConfiguration bleGamepadConfig;
    bleGamepadConfig.setControllerType(CONTROLLER_TYPE_GAMEPAD); // Default.
    // This setting is default, but essential for the controller to be recognized by
    // https://hardwaretester.com/gamepad or
    // https://gamepadtester.net
    // bleGamepadConfig.setWhichAxes(true, true, true, true, true, true, true, true);
    // This is default, but also essential.
    // bleGamepadConfig.setHatSwitchCount(4); // Default, the D-pad.
    // Not default, but also essential.
    // TODO: Default, but necessary?
    // bleGamepadConfig.setButtonCount(16);
    bleGamepadConfig.setIncludeStart(true);
    bleGamepadConfig.setIncludeSelect(true);
    bleGamepad.begin(&bleGamepadConfig);
    // Set the axes to the center of the range?
    // const int16_t center = 2 << 15 / 2;
    // bleGamepad.setAxes(center, center, center, center, center, center, center, center);
    delay(100);

#ifdef USE_LED
    pixel.begin();
    pixel.clear();
#endif

#ifdef WIFI_UPDATES
    wifiInit();
    webServerInit();
    delay(1000);
#endif
}

uint16_t poll_pad() {
    // PSX poll command and listen high.
    const uint8_t tx[5] = {0x01, 0x42, 0xff, 0xff, 0xff};
    uint8_t rx[5];

    digitalWrite(ATT, LOW);
    SPI.beginTransaction(psx);
    delayMicroseconds(20);
    // Expected response: FF ID 5A RX RX.
    for (int i = 0; i < 5; i++) {
        rx[i] = SPI.transfer(tx[i]);
        delayMicroseconds(5); // Ensure we don't just read MOSI back.
    }
    SPI.endTransaction();

    // Idle high.
    digitalWrite(ATT, HIGH);
    digitalWrite(SCK, HIGH);
    digitalWrite(MOSI, HIGH);
    delayMicroseconds(20);

    if (rx[1] != 0x41 || rx[2] != 0x5A)
        return 0xffff;            // No pad?
    return ~(rx[3] << 8 | rx[4]); // Convert to active-high.
}

void handle_dpad(uint16_t rx, bool &changed) {
    uint8_t dpadState = DPAD_CENTERED;
    if (rx & (1 << PSX_UP)) {
        if (rx & (1 << PSX_RIGHT)) {
            dpadState = DPAD_UP_RIGHT;
        } else if (rx & (1 << PSX_LEFT)) {
            dpadState = DPAD_UP_LEFT;
        } else {
            dpadState = DPAD_UP;
        }
    } else if (rx & (1 << PSX_DOWN)) {
        if (rx & (1 << PSX_RIGHT)) {
            dpadState = DPAD_DOWN_RIGHT;
        } else if (rx & (1 << PSX_LEFT)) {
            dpadState = DPAD_DOWN_LEFT;
        } else {
            dpadState = DPAD_DOWN;
        }
    } else if (rx & (1 << PSX_RIGHT)) {
        dpadState = DPAD_RIGHT;
    } else if (rx & (1 << PSX_LEFT)) {
        dpadState = DPAD_LEFT;
    }
    bleGamepad.setHat(dpadState);
    changed = dpadState != previousDpadState;
    previousDpadState = dpadState;
}

void handle_buttons(uint16_t rx, bool &changed) {
    bool select_pressed = rx & (1 << PSX_SELECT);
    bool start_pressed = rx & (1 << PSX_START);
    bool l1_pressed = rx & (1 << PSX_L1);
    bool r1_pressed = rx & (1 << PSX_R1);
    bool l2_pressed = rx & (1 << PSX_L2);
    bool r2_pressed = rx & (1 << PSX_R2);

    // TODO: Re-enable this.
    // // Check for Bluetooth bond delete sequence (SELECT + START + L1 + R1).
    // if (select_pressed && start_pressed && l1_pressed && r1_pressed) {
    //     if (!bond_delete_sequence) {
    //         bond_delete_sequence = true;
    //         bond_delete_start = millis();
    //     } else if (millis() - bond_delete_start > SPECIAL_SEQUENCE_TIMEOUT) {
    //         Serial.println("Entering pairing mode!");
    //         bleGamepad.deleteBond();
    //         bleGamepad.enterPairingMode();
    //         bond_delete_sequence = false;
    //         return; // Skip normal button handling.
    //     }
    // } else {
    //     bond_delete_sequence = false;
    // }

    // Check for restart sequence (SELECT + START + L2 + R2).
    if (select_pressed && start_pressed && l2_pressed && r2_pressed) {
        if (!restart_sequence) {
            restart_sequence = true;
            restart_start = millis();
        } else if (millis() - restart_start > SPECIAL_SEQUENCE_TIMEOUT) {
            Serial.println("Restart sequence detected!");
            esp_restart();
        }
    } else {
        restart_sequence = false;
    }

    // Normal button handling.
    for (uint8_t i = 0; i < numButtons; i++) {
        // TODO: In theory the hat should handle these, but I'm no having luck
        // with it.
        // if (i >= PSX_UP && i <= PSX_LEFT) {
        //     continue;
        // }
        currentButtonStates[i] = rx & (1 << i);
        if (currentButtonStates[i] != previousButtonStates[i]) {
#ifdef USE_DEEP_SLEEP
            last_button_press = millis();
#endif
            changed = true;

            if (currentButtonStates[i] == LOW) {
                if (i == PSX_START) {
                    bleGamepad.releaseStart();
                } else if (i == PSX_SELECT) {
                    bleGamepad.releaseSelect();
                }
                bleGamepad.release(buttonMap[i]);
            } else {
                if (i == PSX_START) {
                    bleGamepad.pressStart();
                } else if (i == PSX_SELECT) {
                    bleGamepad.pressSelect();
                }
                bleGamepad.press(buttonMap[i]);
            }
        }
    }
}

void loop() {
    uint16_t rx;
    bool changed = false;

    if (bleGamepad.isConnected()) {
#ifdef USE_LED
        pixel.setPixelColor(0, 0x000033);
        pixel.show();
#endif

        rx = poll_pad();
        handle_dpad(rx, changed);
        handle_buttons(rx, changed);

        if (changed) {
            // Log on update.
            Serial.print(CLEAR_SCREEN_ANSI);
            // Print raw hex value.
            Serial.printf("Raw RX: 0x%04X\n", rx);
            // Print binary representation with bit numbers.
            Serial.println("Binary: ");
            for (int i = 15; i >= 0; i--) {
                Serial.print((rx & (1 << i)) ? "1  " : "0  ");
            }
            Serial.println();
            for (uint8_t j = 0; j < numButtons; j++) {
                previousButtonStates[j] = currentButtonStates[j];
            }
        }
    } else {
#ifdef USE_LED
        // Flash LED to indicate disconnected.
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
#ifdef USE_DEEP_SLEEP
    if (millis() - last_button_press > deep_sleep_after) {
        Serial.println("Entering deep sleep...");
        delay(300);
        esp_deep_sleep_start();
    }
#endif
#ifdef WIFI_UPDATES
    server.handleClient();
#endif
    delay(5); // ~200 Hz poll
}

# Embedded Projects

## PSX Bluetooth Controller

Arduino code for a PSX controller that uses the SPI interface to communicate
with the PSX console. The ESP32 runs in device-mode, reads PSX controller
signals over SPI, and communicates with hosts over BLE (Bluetooth).

See the [PSX-SPI](psx-spi) directory for the code.

### Features

- [x] OTA WiFi Updates (via ESP32 Arduino library)
- [x] LED (NeoPixel)
- [x] Deep Sleep (ESP32 draws 10mA in deep sleep)
- [x] Bluetooth bond delete sequence (SELECT + START + L1 + R1)
- [ ] Wake on R1 press
- [ ] Connect R1 to GPIO for wake.
- [ ] Test deep sleep and wake.
- [ ] Solder wires directly to Arduino and remove breadboard.
- [ ] Attach the USB-C port and the battery.
- [ ] Attach the Arduino.
- [ ] Angle so LED is exposed.
- [x] Get the Bluetooth to be less aggressive (currently re-pairs immediately
      after disconnecting).
- [ ] Latency testing with a real game.

### Arduino Pin Connections

This is how I wired the PSX controller to the ESP32 board.

| Arduino Pin | PSX Pin |
| ----------- | ------- |
| 26          | A1      |
| 19          | CLK     |
| 22          | MISO    |
| 21          | MOSI    |

### Controller Pinout

A reference for standard PSX controller pinout and my color coding.

| Pin | Typical Color | My Color | Function |
| --- | ------------- | -------- | -------- |
| 1   | Brown         | Brown    | DATA/RX  |
| 2   | Orange        | White    | CMD/ATT  |
| 3   | ---           | ---      | NONE     |
| 4   | Black         | Orange   | GND      |
| 5   | Red           | Yellow   | 3.3V     |
| 6   | Yellow        | Green    | SELECT   |
| 7   | Blue          | Blue     | CLK      |
| 8   | ---           | ---      | NONE     |
| 9   | Green         | Black    | ACK      |

### Further Notes

- ESP32 DeepSleep has two modes EXT0 and EXT1.
  - EXT0 allows 1 wake-up pin, low latency, and chosen level must be held until
    sleep re-arms.
  - EXT1 allows up to 8 wake-up pins, can be triggered by any-high or all-low.
    Going with EXT0 here, for EXT1 see
    [ExternalWakeUp.ino](https://github.com/espressif/arduino-esp32/blob/master/libraries/ESP32/examples/DeepSleep/ExternalWakeUp/ExternalWakeUp.ino)

### References

These references were helpful in building this project.

- [PSX SPI](https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface)
- [PSX Arduino Library](https://github.com/SukkoPera/PsxNewLib?tab=readme-ov-file)
- [ESP32 Pinouts](https://learn.adafruit.com/adafruit-itsybitsy-esp32/pinouts)
- [Arduino SPI](https://docs.arduino.cc/language/reference/en/functions/communication/SPI/)
- [ESP32 BLE Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
- [HID Reports](https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf) (not very useful)

## EEPROM Writer

Arduino microcontroller code for a device that writes to an EEPROM. See the
[eeprom-writer](eeprom-writer) directory for the code.

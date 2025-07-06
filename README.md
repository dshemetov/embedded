# Embedded Projects

## PSX Bluetooth Controller

Arduino code for a PSX controller that uses the SPI interface to communicate
with the PSX console. The ESP32 runs in device-mode, reads PSX controller
signals over SPI, and communicates with hosts over BLE (Bluetooth).

See the [PSX-SPI](psx-spi) directory for the code.

### Features

- [x] LED shows connected / disconnected mode (NeoPixel)
- [x] OTA WiFi Updates (via ESP32 Arduino library)
- [x] Deep Sleep (ESP32 draws 10mA in deep sleep), press R1 to wake.
- [x] Bluetooth bond delete sequence (SELECT + START + L1 + R1)
- [x] Bluetooth restart sequence (SELECT + START + L2 + R2)
- [x] Steam Link support (via Big Picture > Steam Input)
- [ ] Latency tested on [Celeste](https://www.celestegame.com)!
- [ ] Expose charge LED?

### Current Status

The controller connects properly but games don't recognize button mappings
correctly. These two sites correctly find the controller:
- https://gamepadtester.net/
- https://hardwaretester.com/gamepad

All the buttons work (not mapped right), but the directional buttons don't.

Some useful commands for debugging with MacOS:

```sh
# Shows the controller in the list of connected devices
system_profiler SPBluetoothDataType
# You can find it here too.
hidutil list
# The HID descriptor can be dumped with.
ioreg -l | grep -A 50 -B 5 "PSX BLT" > hid_descriptor.txt
```

Further findings:

- The BleGamepad defaults appear to be correct, minus the start/select buttons
  being necessary for the gamepad tester to work
- MacOS: Steam still doesn't detect button presses despite what appears to be a
  clean HID descriptor and gamepad tester working.
- Windows: Bluetooth connection unpairs immediately.
- Phone: Bluetooth connection unpairs immediately.

Next Steps:

1. Upgrading NimBLE to 2.2.3 -> 2.3.2.
1. Compare with known working controller - Capture HID reports from a working
   Xbox/PS4 controller to see the exact format Steam expects
2. Test with other games/apps - Try non-Steam games to isolate whether it's a
   Steam-specific issue
3. Monitor actual HID reports - Use tools to capture the raw HID data being sent
   during button presses

### PSX Controller and Arduino Pinout

Standard PSX controller pinout, my Arduino pin pairing, and color codings.

| Pin | Function | Arduino Pin | My Wire Color | Typical PSX Wire Color |
| --- | -------- | ----------- | ------------- | ---------------------- |
| 1   | DATA/RX  | MISO/22     | Brown         | Brown                  |
| 2   | CMD/ATT  | MOSI/21     | White         | Orange                 |
| 3   | NONE     | ---         | ---           | ---                    |
| 4   | GND      | GND         | Orange        | Black                  |
| 5   | 3.3V     | 3.3V        | Yellow        | Red                    |
| 6   | SELECT   | A1/10       | Green         | Yellow                 |
| 7   | CLK      | SCK/19      | Blue          | Blue                   |
| 8   | NONE     | ---         | ---           | ---                    |
| 9   | ACK      | A0/25       | Black         | Green                  |

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

# Embedded Projects

## PSX Bluetooth Controller

ESP32 Arduino code for a Bluetooth-enabled PSX controller. The Arduino runs in
device-mode, reads PSX controller signals over SPI, and communicates with hosts
over BLE (Bluetooth).

See the [PSX-SPI](psx-spi) directory for the code.

### Features

- [x] LED shows connected / disconnected mode (NeoPixel)
- [x] OTA WiFi Updates (via ESP32 Arduino library)
- [x] Deep Sleep (ESP32 draws 10mA in deep sleep), press R1 to wake
- [x] Bluetooth bond delete sequence (SELECT + START + L1 + R1)
- [x] Bluetooth restart sequence (SELECT + START + L2 + R2)
- [x] Tested on Windows with Steam and [Celeste](https://www.celestegame.com)!
- [ ] Tested on MacOS with Steam and [Celeste](https://www.celestegame.com)
- [ ] Steam Link support (via Big Picture > Steam Input)
- [ ] Expose charge LED?

### Challenges

- Soldering connections of more than 2 wires to an Arduino pin is a pain. I
  ended up twisting very thin wires together and threading them through.
- A lead burnt off the PSX controller, so I had to follow the trace to a
  resistor and connect there. This almost didn't work, I used tape to hold the
  wire in place.
- The controller also has a very small form factor, so fitting everything inside
  required shortening the wire lengths and spreading the components out.
- Figuring out which pins needed pullups was the first block to getting SPI
  working. I manually added resistors until I learned the Arduino has built-in
  pullups, so you can just set those to activate in the code.
- After that, I needed to add small delays between MOSI and MISO to get the
  controller to respond.
- Setting up OTA updates was surprisingly easy. It's still so funny to me that
  my controller has a small web server running on it.
- Adding Bluetooth support was surprisingly easy. Getting it functional and
  getting the button mappings right was the hardest part of the project.
  Bluetooth attribute caching can make it difficult to iterate on controller
  configuration. Steam appears to have some caching as well an needs to be
  restarted whenever you make changes to the controller settings.

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

Some useful commands for debugging with MacOS:

```sh
# Shows the controller in the list of connected devices
system_profiler SPBluetoothDataType
# You can find it here too.
hidutil list
# The HID descriptor can be dumped with.
ioreg -l | grep -A 50 -B 5 "PSX BLT" > hid_descriptor.txt
```

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

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
- [x] Tested on Windows with Steam and [Celeste](https://www.celestegame.com)
      (even some Farewell screens!) See [here](https://imgur.com/a/Pt0OeQK) for a short video.

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
- I learned that 350mAh gives about an hour of use (without aggressive power
  saving). Turning off the WiFi OTA and disabling the NeoPixel LED saves a lot
  of power.
- When the battery gets low, I experienced spurious inputs from the controller
  due to voltage sag.
- On MacOS, there is plenty of latency using Steam, so I recommend using Windows
  for the best experience.

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

- [Adafruit PSX Controller](https://learn.adafruit.com/esp32-playstation-controller/overview)
  - This guide gave me the initial idea for the project. It uses bit-banging to
    read the buttons, which I switched to SPI.
- [PSX SPI](https://hackaday.io/project/170365-blueretro/log/186471-playstation-playstation-2-spi-interface)
  - The notes here are really helpful for understanding the PS1 controller SPI
    interface.
- [PSX Arduino Library](https://github.com/SukkoPera/PsxNewLib?tab=readme-ov-file)
  - Was useful to get a sense of the SPI timing.
- [ESP32 Pinouts](https://learn.adafruit.com/adafruit-itsybitsy-esp32/pinouts)
  - Of course, you'll need to know the ESP32 pinout to use it.
- [Arduino SPI](https://docs.arduino.cc/language/reference/en/functions/communication/SPI/)
  - The Arduino SPI interface is pretty straightforward. Hooray for simple
    protocols!
- [ESP32 BLE Gamepad](https://github.com/lemmingDev/ESP32-BLE-Gamepad)
  - The main workhorse for the Bluetooth code. It mostly works out of the box,
    which was a nice surprise. The main difficulty was getting the OS to
    recognize and map the buttons correctly, so I ended up handling that at the
    software level (with Steam).
- [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel)
  - I used the NeoPixel to show the connected / disconnected mode.
- [HID Reports](https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf)
  - Wasn't useful to me, since the BLE Gamepad library generates the HID
    descriptor and the reports, but a good reference to understand the HID
    protocol.

## EEPROM Writer

Arduino microcontroller code for a device that writes to an EEPROM.
I used it to program an EEPROM that I later connected to a Game Boy.
See the [eeprom-writer](eeprom-writer) directory for the code.

### Usage

### Notes

- The EEPROM is SST39SF020A, see datasheet [here](https://www.mouser.com/datasheet/3/282/1/20005022C.pdf).
- You can see a picture of my breadboard setup here.
- I verified the basic functionality with eeprom_writer, which writes hard-coded patterns to the EEPROM and reads them back.
- Writing ROMs uses eeprom_writer2, which listens for a file over serial and writes it to the EEPROM. I designed a simple protocol, described below.
- A Python script is provided to read a file, break it into 1kB pages, and send it to the device over serial.

#### Protocol

Simple protocol that writes a file to the EEPROM.

```
[SOF=0xAA 0x55] [CMD:1] [ADDR:4 LE] [LEN:2 LE] [DATA:LEN] [CKSUM16:2 LE]
```

Host -> device:

- SOF: Start of frame
- CMD: Command (W 0x57: write, R 0x52: read)
- ADDR: Address (4 bytes, little endian)
- LEN: Length (2 bytes, little endian)
- DATA: Data (LEN bytes)
- CKSUM16: Checksum (2 bytes, little endian), 16-bit sum of all bytes modulo 0x10000

Device -> host:

- For W: `[0xAA, 0x55] ['A'] [STATUS:1] [CKSUM16]` where STATUS is 0x00 for success, 0x01 for error.
- For R: `[0xAA, 0x55] ['D'] [DATA:LEN] [CKSUM16]`

### References

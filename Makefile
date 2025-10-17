DEVICE=$(shell arduino-cli board list | grep -e "^.*usbserial[^\s]*" -o)
BOARD=esp32:esp32:adafruit_itsybitsy_esp32
UPLOAD_BAUDRATE=460800
SERIAL_BAUDRATE=115200
# PROJECT=psx-spi
PROJECT=eeprom_writer2
# empty or -v
VERBOSE=

compile:
	arduino-cli compile -b $(BOARD) $(VERBOSE) $(PROJECT)

upload:
	arduino-cli upload -p $(DEVICE) --fqbn $(BOARD) --upload-field upload.speed=$(UPLOAD_BAUDRATE) --upload-property "upload.speed=$(UPLOAD_BAUDRATE)" $(VERBOSE) $(PROJECT)

monitor:
	arduino-cli monitor -p $(DEVICE) -c baudrate=$(SERIAL_BAUDRATE) --fqbn $(BOARD)

send-serial:
	uv run eeprom_writer2/send_serial.py $(DEVICE)

send-serial-tetris:
	uv run eeprom_writer2/send_serial.py $(DEVICE) "tetris.gb"

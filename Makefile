.PHONY: sync compile upload monitor
DEVICE=/dev/cu.usbserial-59190089461
BOARD=esp32:esp32:adafruit_itsybitsy_esp32
UPLOAD_BAUDRATE=460800
SERIAL_BAUDRATE=115200
PROJECT=psx-spi
# empty or -v
VERBOSE=

compile:
	arduino-cli compile -b $(BOARD) $(VERBOSE) $(PROJECT)

upload:
	arduino-cli upload -p $(DEVICE) --fqbn $(BOARD) --upload-field upload.speed=$(UPLOAD_BAUDRATE) --upload-property "upload.speed=$(UPLOAD_BAUDRATE)" $(VERBOSE) $(PROJECT)

monitor:
	arduino-cli monitor -p $(DEVICE) -c baudrate=$(SERIAL_BAUDRATE) --fqbn $(BOARD)

sync:
	rsync -avzP --exclude="target" SteakWSL:/home/dskel/repos/embedded/ ~/Documents/Code/embedded/ >> sync-embedded.log 2>&1

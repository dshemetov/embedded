compile:
	arduino-cli compile -b espressif:esp32:adafruit_itsybitsy_esp32 ps1-spi

upload: compile
	arduino-cli upload -p /dev/ttyACM0 --fqbn espressif:esp32:adafruit_itsybitsy_esp32 -F upload.speed=115200 ps1-spi

monitor: upload
	arduino-cli monitor -p /dev/ttyACM0 -c baudrate=115200 --fqbn espressif:esp32:adafruit_itsybitsy_esp32


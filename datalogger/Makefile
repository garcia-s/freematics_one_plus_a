


compile:
	arduino-cli compile -b esp32:esp32:esp32 --libraries ./libraries

upload:
	arduino-cli upload -b esp32:esp32:esp32 --port /dev/ttyUSB0

attach: 
	arduino-cli monitor -b esp32:esp32:esp32 --port /dev/ttyUSB0 -c 115200

deploy:
	make compile && make upload && make attach




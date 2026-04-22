BOARD  = esp32:esp32:esp32s3
SKETCH = test/validate/validate.ino

ARDUINO_CLI = arduino-cli

.PHONY: validate install clean

install:
	$(ARDUINO_CLI) core install esp32:esp32

validate: install
	$(ARDUINO_CLI) compile \
		--fqbn $(BOARD) \
		--build-property "compiler.cpp.extra_flags=-I$(shell pwd)/src -std=gnu++23" \
		$(SKETCH)

clean:
	$(ARDUINO_CLI) compile \
		--fqbn $(BOARD) \
		--clean \
		$(SKETCH)
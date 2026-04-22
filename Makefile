# ─────────────────────────────────────────────
# Makefile — seam-lib
# ─────────────────────────────────────────────

BOARD       = esp32:esp32:esp32s3
SKETCH      = test/validate/validate.ino
TEST_SKETCH = test/unit/unit.ino
TEST_BAUD   = 115200
TEST_TIMEOUT = 10

ARDUINO_CLI = arduino-cli
PYTHON      = python3

# PORT can be overridden: make test TEST_PORT=/dev/cu.usbmodem101
TEST_PORT  ?= $(shell $(ARDUINO_CLI) board list 2>/dev/null \
                | grep -i "esp32" \
                | awk '{print $$1}' \
                | head -1)

.PHONY: install validate setup-test test clean

# ── Install core ──────────────────────────────

install:
	$(ARDUINO_CLI) core install esp32:esp32

# ── Compile validation ────────────────────────

validate: install
	$(ARDUINO_CLI) compile \
		--fqbn $(BOARD) \
		--build-property "compiler.cpp.extra_flags=-I$(shell pwd)/src -std=gnu++23" \
		$(SKETCH)

# ── Test environment setup ────────────────────

setup-test:
	@echo "==> Checking arduino-cli..."
	@which $(ARDUINO_CLI) > /dev/null || \
		(echo "ERROR: arduino-cli not found. Install from https://arduino.github.io/arduino-cli" && exit 1)
	@echo "    arduino-cli: ok"

	@echo "==> Checking Python..."
	@which $(PYTHON) > /dev/null || \
		(echo "ERROR: python3 not found" && exit 1)
	@echo "    python3: ok"

	@echo "==> Checking pyserial..."
	@$(PYTHON) -c "import serial" 2>/dev/null || \
		(echo "INFO: pyserial not found — installing..." && \
		 $(PYTHON) -m pip install pyserial --quiet)
	@echo "    pyserial: ok"

	@echo "==> Installing ESP32 core..."
	@$(ARDUINO_CLI) core install esp32:esp32 2>/dev/null
	@echo "    esp32 core: ok"

	@echo "==> Discovering connected boards..."
	@$(ARDUINO_CLI) board list
	@test -n "$(TEST_PORT)" || \
		(echo "ERROR: no ESP32 board detected. Connect a board or set TEST_PORT manually." && exit 1)
	@echo "    port: $(TEST_PORT)"

	@echo ""
	@echo "Test environment ready. Run 'make test' to execute unit tests."

# ── Unit tests ────────────────────────────────

test: validate setup-test
	@echo "==> Compiling unit tests..."
	$(ARDUINO_CLI) compile \
		--fqbn $(BOARD) \
		--build-property "compiler.cpp.extra_flags=-I$(shell pwd)/src -std=gnu++23" \
		$(TEST_SKETCH)

	@echo "==> Uploading to $(TEST_PORT)..."
	$(ARDUINO_CLI) upload \
		--fqbn $(BOARD) \
		--port $(TEST_PORT) \
		$(TEST_SKETCH)

	@echo "==> Running tests..."
	$(PYTHON) test/capture.py \
		--port $(TEST_PORT) \
		--baud $(TEST_BAUD) \
		--timeout $(TEST_TIMEOUT)

# ── Clean ─────────────────────────────────────

clean:
	$(ARDUINO_CLI) compile \
		--fqbn $(BOARD) \
		--clean \
		$(SKETCH)
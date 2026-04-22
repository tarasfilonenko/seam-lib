# ─────────────────────────────────────────────
# Makefile — seam-lib
# ─────────────────────────────────────────────

SKETCH      = test/validate/validate.ino
TEST_SKETCH = test/unit/unit.ino
TEST_BAUD   = 115200
TEST_TIMEOUT = 10

ARDUINO_CLI = arduino-cli
PYTHON      = python3

# Auto-detect board by FQBN pattern; override with BOARD=, CORE=, TEST_PORT=
_ACTIVE     := $(shell $(ARDUINO_CLI) board list 2>/dev/null | grep -E '[a-zA-Z0-9_-]+:[a-zA-Z0-9_-]+:[a-zA-Z0-9_-]+' | head -1)

TEST_PORT   ?= $(firstword $(_ACTIVE))
BOARD       ?= $(shell echo "$(_ACTIVE)" | grep -oE '[a-zA-Z0-9_-]+:[a-zA-Z0-9_-]+:[a-zA-Z0-9_-]+' | head -1)
CORE        ?= $(shell echo "$(BOARD)" | cut -d: -f1-2)
CXX_STD     ?= $(if $(filter esp32:%,$(CORE)),gnu++23,gnu++17)

.PHONY: install validate setup-test test clean

# ── Install core ──────────────────────────────

install:
	$(ARDUINO_CLI) core install $(CORE)

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

	@echo "==> Installing $(CORE) core..."
	@$(ARDUINO_CLI) core install $(CORE) 2>/dev/null
	@echo "    $(CORE) core: ok"

	@echo "==> Discovering connected boards..."
	@$(ARDUINO_CLI) board list
	@test -n "$(TEST_PORT)" || \
		(echo "ERROR: no supported board detected. Connect an ESP32 or Seeeduino XIAO, or set TEST_PORT and BOARD manually." && exit 1)
	@echo "    board: $(BOARD)"
	@echo "    port:  $(TEST_PORT)"

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

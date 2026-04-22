// ─────────────────────────────────────────────
// test/unit/unit.ino
//
// Entry point for seam-lib unit tests.
// Runs all test suites and prints results over Serial.
// Upload to an ESP32-S3, capture output with test/capture.py.
// ─────────────────────────────────────────────

#include <Arduino.h>
#include "protocol/Parser.h"
#include "protocol/wire/Event.h"
#include "protocol/caps/Caps.h"

using namespace seam::protocol;
using namespace seam::protocol::wire;
using namespace seam::protocol::caps;

// ── Test runner ───────────────────────────────

static int _passed = 0;
static int _failed = 0;

void check(const char* name, bool condition, const char* reason = "") {
    if (condition) {
        Serial.printf("TEST %s ... PASS\n", name);
        _passed++;
    } else {
        Serial.printf("TEST %s ... FAIL: %s\n", name, reason);
        _failed++;
    }
}

template<typename T>
void checkEqual(const char* name, const T& expected, const T& actual) {
    if (expected == actual) {
        Serial.printf("TEST %s ... PASS\n", name);
        _passed++;
    } else {
        // convert to string for printing
        Serial.printf("TEST %s ... FAIL: values did not match\n", name);
        _failed++;
    }
}

// specialisation for std::string — prints actual values
template<>
void checkEqual<std::string>(const char* name,
                              const std::string& expected,
                              const std::string& actual) {
    if (expected == actual) {
        Serial.printf("TEST %s ... PASS\n", name);
        _passed++;
    } else {
        Serial.printf("TEST %s ... FAIL: expected \"%s\" got \"%s\"\n",
                      name, expected.c_str(), actual.c_str());
        _failed++;
    }
}

template<>
void checkEqual<size_t>(const char* name,
                         const size_t& expected,
                         const size_t& actual) {
    if (expected == actual) {
        Serial.printf("TEST %s ... PASS\n", name);
        _passed++;
    } else {
        Serial.printf("TEST %s ... FAIL: expected %u got %u\n",
                      name, (unsigned)expected, (unsigned)actual);
        _failed++;
    }
}

template<>
void checkEqual<bool>(const char* name,
                       const bool& expected,
                       const bool& actual) {
    if (expected == actual) {
        Serial.printf("TEST %s ... PASS\n", name);
        _passed++;
    } else {
        Serial.printf("TEST %s ... FAIL: expected %s got %s\n",
                      name,
                      expected ? "true" : "false",
                      actual   ? "true" : "false");
        _failed++;
    }
}

// ── Helper ────────────────────────────────────

// feed a full string and return first event
std::optional<wire::Event> feedAndTake(Parser& p, const char* input) {
    p.feed(reinterpret_cast<const uint8_t*>(input), strlen(input));
    return p.takeEvent();
}

// ── Test suites ───────────────────────────────

#include "test_parser.h"
#include "test_caps.h"
#include "test_interleave.h"
#include "test_recovery.h"

// ── Entry point ───────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("==> seam-lib unit tests");

    runParserTests();
    runCapsTests();
    runInterleaveTests();
    runRecoveryTests();

    Serial.printf("\nDONE %d/%d passed\n", _passed, _passed + _failed);
}

void loop() {}
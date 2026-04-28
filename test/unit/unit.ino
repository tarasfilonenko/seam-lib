// ─────────────────────────────────────────────
// test/unit/unit.ino — seam-lib unit tests (AUnit)
// ─────────────────────────────────────────────

#include <Arduino.h>
#include <AUnit.h>
#include "protocol/Parser.h"
#include "protocol/Serializer.h"
#include "protocol/wire/Event.h"
#include "protocol/caps/Caps.h"

using namespace seam::protocol;
using namespace seam::protocol::wire;
using namespace seam::protocol::caps;

// ── Helper ────────────────────────────────────

std::optional<wire::Event> feedAndTake(Parser& p, const char* input) {
    p.feed(reinterpret_cast<const uint8_t*>(input), strlen(input));
    return p.takeEvent();
}

// ── Test suites ───────────────────────────────

#include "test_parser.h"
#include "test_serializer.h"
#include "test_caps.h"
#include "test_interleave.h"
#include "test_recovery.h"

// ── Entry point ───────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(500);

    while (true) {
        if (Serial.available()) {
            String cmd = Serial.readStringUntil('\n');
            cmd.trim();
            if (cmd == "start") break;
        }
    }

    aunit::TestRunner::setPrinter(&Serial);
}

void loop() {
    aunit::TestRunner::run();
}

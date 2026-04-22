#pragma once
// ─────────────────────────────────────────────
// test_recovery.h — error recovery tests
// ─────────────────────────────────────────────

void runRecoveryTests() {
    Serial.println("-- recovery --");

    // parser works after PARSE_ERROR
    {
        Parser p;

        // first — trigger a parse error
        p.feed(reinterpret_cast<const uint8_t*>("GARBAGE\r\n"), 9);
        auto ev1 = p.takeEvent();
        check("recovery_error_emitted",
            ev1 && ev1->type == EventType::PARSE_ERROR,
            "expected PARSE_ERROR");

        // then — valid command works
        p.feed(reinterpret_cast<const uint8_t*>("OK\r\n"), 4);
        auto ev2 = p.takeEvent();
        check("recovery_ok_after_error",
            ev2 && ev2->type == EventType::CMD_OK,
            "expected CMD_OK after recovery");
    }

    // parse error mid-CAPS resets — next CAPS parses cleanly
    {
        Parser p;

        // start a CAPS block then inject garbage
        {
            const char* s = "CAPS BEGIN\r\ntype:test\r\nGARBAGE\r\n";
            p.feed(reinterpret_cast<const uint8_t*>(s), strlen(s));
        }

        auto ev1 = p.takeEvent();
        check("recovery_mid_caps_error",
            ev1 && ev1->type == EventType::PARSE_ERROR,
            "expected PARSE_ERROR mid-CAPS");

        // fresh CAPS after reset
        auto ev2 = feedAndTake(p,
            "CAPS BEGIN\r\n"
            "type:servo_tester\r\n"
            "name:Servo Tester\r\n"
            "version:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n");

        check("recovery_caps_after_reset",
            ev2 && ev2->type == EventType::CAPS_READY,
            "expected CAPS_READY after reset");
        auto* pl = ev2 ? std::get_if<CapsReadyPayload>(&ev2->payload) : nullptr;
        checkEqual<std::string>("recovery_caps_device_type", "servo_tester",
            pl ? pl->caps.device_type : "");
    }

    // reset() clears all state
    {
        Parser p;

        // start a CAPS parse
        p.feed(reinterpret_cast<const uint8_t*>(
            "CAPS BEGIN\r\n"
            "type:test\r\n"), 22);

        // manually reset
        p.reset();

        // parser should be back in IDLE
        // a valid OK should work immediately
        auto ev = feedAndTake(p, "OK done\r\n");
        check("recovery_reset_clears_state",
            ev && ev->type == EventType::CMD_OK,
            "expected CMD_OK after manual reset");
        auto* pl = ev ? std::get_if<OkPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("recovery_reset_ok_text", "done", pl ? pl->text : "");
    }

    // multiple events in one feed() call
    {
        Parser p;
        auto input =
            std::string("CHANGED a\r\n") +
            std::string("CHANGED b\r\n") +
            std::string("OK\r\n");

        p.feed(reinterpret_cast<const uint8_t*>(input.c_str()), input.size());

        auto ev1 = p.takeEvent();
        auto ev2 = p.takeEvent();
        auto ev3 = p.takeEvent();

        check("recovery_multi_event_1",
            ev1 && ev1->type == EventType::CHANGED,
            "expected first CHANGED");
        check("recovery_multi_event_2",
            ev2 && ev2->type == EventType::CHANGED,
            "expected second CHANGED");
        check("recovery_multi_event_3",
            ev3 && ev3->type == EventType::CMD_OK,
            "expected CMD_OK");
    }
}
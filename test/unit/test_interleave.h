#pragma once
// ─────────────────────────────────────────────
// test_interleave.h — async interleave tests
// ─────────────────────────────────────────────

void runInterleaveTests() {
    Serial.println("-- interleave --");

    // CHANGED arrives mid-CAPS parse
    {
        Parser p;

        const char* part1 =
            "CAPS BEGIN\r\n"
            "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
            "GROUP BEGIN g\r\nlabel:G\r\n";

        const char* async =
            "CHANGED some_param\r\n";

        const char* part2 =
            "PARAM BEGIN p\r\n"
            "type:seam/int\r\naccess:r\r\nlabel:P\r\n"
            "PARAM END\r\n"
            "GROUP END\r\n"
            "CAPS END\r\n";

        p.feed(reinterpret_cast<const uint8_t*>(part1), strlen(part1));
        p.feed(reinterpret_cast<const uint8_t*>(async), strlen(async));
        p.feed(reinterpret_cast<const uint8_t*>(part2), strlen(part2));

        // first event should be CHANGED
        auto ev1 = p.takeEvent();
        check("interleave_changed_first",
            ev1 && ev1->type == EventType::CHANGED,
            "expected CHANGED before CAPS_READY");

        // second event should be CAPS_READY
        auto ev2 = p.takeEvent();
        check("interleave_caps_after_changed",
            ev2 && ev2->type == EventType::CAPS_READY,
            "expected CAPS_READY after CHANGED");
    }

    // VALUE binary frame
    {
        Parser p;
        const char* header = "VALUE pulse_width_us 4\r\n";
        const char* data   = "1500\r\n";
        p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
        p.feed(reinterpret_cast<const uint8_t*>(data),   strlen(data));

        auto ev = p.takeEvent();
        check("interleave_value_type",
            ev && ev->type == EventType::VALUE_RECEIVED,
            "expected VALUE_RECEIVED");
        auto* pl = ev ? std::get_if<ValueReceivedPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("interleave_value_id", "pulse_width_us", pl ? pl->id : "");
        check("interleave_value_data",
            pl && pl->data == std::vector<uint8_t>{'1','5','0','0'},
            "wrong data bytes");
    }

    // DATA binary frame
    {
        Parser p;
        const char* header = "DATA position 6\r\n";
        const char* data   = "1523.5\r\n";
        p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
        p.feed(reinterpret_cast<const uint8_t*>(data),   strlen(data));

        auto ev = p.takeEvent();
        check("interleave_data_type",
            ev && ev->type == EventType::STREAM_DATA,
            "expected STREAM_DATA");
        auto* pl = ev ? std::get_if<StreamDataPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("interleave_data_id", "position", pl ? pl->id : "");
        checkEqual<size_t>("interleave_data_size", 6, pl ? pl->data.size() : 0);
    }

    // multi-chunk feed — payload split across calls
    {
        Parser p;

        // split "CHANGED some_param\r\n" into 3 chunks
        const char* c1 = "CHAN";
        const char* c2 = "GED some";
        const char* c3 = "_param\r\n";

        p.feed(reinterpret_cast<const uint8_t*>(c1), strlen(c1));
        p.feed(reinterpret_cast<const uint8_t*>(c2), strlen(c2));
        p.feed(reinterpret_cast<const uint8_t*>(c3), strlen(c3));

        auto ev = p.takeEvent();
        check("interleave_chunked_type",
            ev && ev->type == EventType::CHANGED,
            "expected CHANGED from chunked feed");
        auto* pl = ev ? std::get_if<ChangedPayload>(&ev->payload) : nullptr;
        checkEqual<std::string>("interleave_chunked_id", "some_param", pl ? pl->id : "");
    }

    // binary payload split across feed() calls
    {
        Parser p;
        // VALUE split: header in one call, data split across two more
        const char* header = "VALUE temp 4\r\n";
        const char* d1     = "25";
        const char* d2     = ".5\r\n";

        p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
        p.feed(reinterpret_cast<const uint8_t*>(d1),     strlen(d1));
        p.feed(reinterpret_cast<const uint8_t*>(d2),     strlen(d2));

        auto ev = p.takeEvent();
        check("interleave_binary_chunked",
            ev && ev->type == EventType::VALUE_RECEIVED,
            "expected VALUE_RECEIVED from chunked binary");
        auto* pl = ev ? std::get_if<ValueReceivedPayload>(&ev->payload) : nullptr;
        checkEqual<size_t>("interleave_binary_chunked_size", 4, pl ? pl->data.size() : 0);
    }
}
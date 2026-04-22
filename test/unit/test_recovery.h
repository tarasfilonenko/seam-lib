#pragma once
#include <AUnit.h>

test(recovery_parse_error) {
    Parser p;
    p.feed(reinterpret_cast<const uint8_t*>("GARBAGE\r\n"), 9);
    auto ev1 = p.takeEvent();
    assertTrue(ev1.has_value());
    assertEqual((int)EventType::PARSE_ERROR, (int)ev1->type);
    p.feed(reinterpret_cast<const uint8_t*>("OK\r\n"), 4);
    auto ev2 = p.takeEvent();
    assertTrue(ev2.has_value());
    assertEqual((int)EventType::CMD_OK, (int)ev2->type);
}

test(recovery_mid_caps_error) {
    Parser p;
    {
        const char* s = "CAPS BEGIN\r\ntype:test\r\nGARBAGE\r\n";
        p.feed(reinterpret_cast<const uint8_t*>(s), strlen(s));
    }
    auto ev1 = p.takeEvent();
    assertTrue(ev1.has_value());
    assertEqual((int)EventType::PARSE_ERROR, (int)ev1->type);
    auto ev2 = feedAndTake(p,
        "CAPS BEGIN\r\n"
        "type:servo_tester\r\n"
        "name:Servo Tester\r\n"
        "version:1.0.0\r\n"
        "GROUP BEGIN g\r\nlabel:G\r\n"
        "GROUP END\r\n"
        "CAPS END\r\n");
    assertTrue(ev2.has_value());
    assertEqual((int)EventType::CAPS_READY, (int)ev2->type);
    auto* pl = ev2 ? std::get_if<CapsReadyPayload>(&ev2->payload) : nullptr;
    assertTrue(pl != nullptr);
    assertEqual("servo_tester", pl->caps.device_type.c_str());
}

test(recovery_manual_reset) {
    Parser p;
    {
        const char* s = "CAPS BEGIN\r\ntype:test\r\n";
        p.feed(reinterpret_cast<const uint8_t*>(s), strlen(s));
    }
    p.reset();
    auto ev = feedAndTake(p, "OK done\r\n");
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CMD_OK, (int)ev->type);
    auto* pl = std::get_if<OkPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("done", pl->text.c_str());
}

test(recovery_multi_event) {
    Parser p;
    auto input = std::string("CHANGED a\r\n") +
                 std::string("CHANGED b\r\n") +
                 std::string("OK\r\n");
    p.feed(reinterpret_cast<const uint8_t*>(input.c_str()), input.size());
    auto ev1 = p.takeEvent();
    auto ev2 = p.takeEvent();
    auto ev3 = p.takeEvent();
    assertTrue(ev1.has_value());
    assertEqual((int)EventType::CHANGED, (int)ev1->type);
    assertTrue(ev2.has_value());
    assertEqual((int)EventType::CHANGED, (int)ev2->type);
    assertTrue(ev3.has_value());
    assertEqual((int)EventType::CMD_OK, (int)ev3->type);
}

#pragma once
#include <AUnit.h>

test(interleave_changed_mid_caps) {
    Parser p;
    const char* part1 =
        "CAPS BEGIN\r\n"
        "type:test\r\nname:Test\r\nversion:1.0.0\r\n"
        "GROUP BEGIN g\r\nlabel:G\r\n";
    const char* async = "CHANGED some_param\r\n";
    const char* part2 =
        "PARAM BEGIN p\r\n"
        "type:seam/int\r\naccess:r\r\nlabel:P\r\n"
        "PARAM END\r\n"
        "GROUP END\r\n"
        "CAPS END\r\n";
    p.feed(reinterpret_cast<const uint8_t*>(part1), strlen(part1));
    p.feed(reinterpret_cast<const uint8_t*>(async), strlen(async));
    p.feed(reinterpret_cast<const uint8_t*>(part2), strlen(part2));
    auto ev1 = p.takeEvent();
    assertTrue(ev1.has_value());
    assertEqual((int)EventType::CHANGED, (int)ev1->type);
    auto ev2 = p.takeEvent();
    assertTrue(ev2.has_value());
    assertEqual((int)EventType::CAPS_READY, (int)ev2->type);
}

test(interleave_value_binary) {
    Parser p;
    const char* header = "VALUE pulse_width_us 4\r\n";
    const char* data   = "1500\r\n";
    p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
    p.feed(reinterpret_cast<const uint8_t*>(data),   strlen(data));
    auto ev = p.takeEvent();
    assertTrue(ev.has_value());
    assertEqual((int)EventType::VALUE_RECEIVED, (int)ev->type);
    auto* pl = std::get_if<ValueReceivedPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("pulse_width_us", pl->id.c_str());
    assertTrue((pl->data == std::vector<uint8_t>{'1','5','0','0'}));
}

test(interleave_data_binary) {
    Parser p;
    const char* header = "DATA position 6\r\n";
    const char* data   = "1523.5\r\n";
    p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
    p.feed(reinterpret_cast<const uint8_t*>(data),   strlen(data));
    auto ev = p.takeEvent();
    assertTrue(ev.has_value());
    assertEqual((int)EventType::STREAM_DATA, (int)ev->type);
    auto* pl = std::get_if<StreamDataPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("position", pl->id.c_str());
    assertEqual((size_t)6,  pl->data.size());
}

test(interleave_chunked_line) {
    Parser p;
    const char* c1 = "CHAN";
    const char* c2 = "GED some";
    const char* c3 = "_param\r\n";
    p.feed(reinterpret_cast<const uint8_t*>(c1), strlen(c1));
    p.feed(reinterpret_cast<const uint8_t*>(c2), strlen(c2));
    p.feed(reinterpret_cast<const uint8_t*>(c3), strlen(c3));
    auto ev = p.takeEvent();
    assertTrue(ev.has_value());
    assertEqual((int)EventType::CHANGED, (int)ev->type);
    auto* pl = std::get_if<ChangedPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual("some_param", pl->id.c_str());
}

test(interleave_chunked_binary) {
    Parser p;
    const char* header = "VALUE temp 4\r\n";
    const char* d1     = "25";
    const char* d2     = ".5\r\n";
    p.feed(reinterpret_cast<const uint8_t*>(header), strlen(header));
    p.feed(reinterpret_cast<const uint8_t*>(d1),     strlen(d1));
    p.feed(reinterpret_cast<const uint8_t*>(d2),     strlen(d2));
    auto ev = p.takeEvent();
    assertTrue(ev.has_value());
    assertEqual((int)EventType::VALUE_RECEIVED, (int)ev->type);
    auto* pl = std::get_if<ValueReceivedPayload>(&ev->payload);
    assertTrue(pl != nullptr);
    assertEqual((size_t)4, pl->data.size());
}

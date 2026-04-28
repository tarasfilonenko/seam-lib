#pragma once
#include <AUnit.h>

inline std::string toAscii(const Serializer::Buffer& buffer) {
    return std::string(buffer.begin(), buffer.end());
}

test(serializer_caps) {
    Serializer s;
    Command command;
    command.type = CommandType::CAPS;
    command.payload = CapsPayload{};

    auto encoded = s.serialize(command);
    assertTrue(encoded.has_value());
    assertEqual("CAPS\r\n", toAscii(*encoded).c_str());
}

test(serializer_get) {
    Serializer s;
    Command command;
    command.type = CommandType::GET;
    command.payload = GetPayload{ "pulse_width_us" };

    auto encoded = s.serialize(command);
    assertTrue(encoded.has_value());
    assertEqual("GET pulse_width_us\r\n", toAscii(*encoded).c_str());
}

test(serializer_set) {
    Serializer s;
    Command command;
    command.type = CommandType::SET;
    command.payload = SetPayload{
        "pulse_width_us",
        std::vector<uint8_t>{ '1', '5', '0', '0' }
    };

    auto encoded = s.serialize(command);
    assertTrue(encoded.has_value());
    assertEqual("SET pulse_width_us 4\r\n1500\r\n", toAscii(*encoded).c_str());
}

test(serializer_do) {
    Serializer s;
    Command command;
    command.type = CommandType::DO;
    command.payload = DoPayload{ "sweep", {} };

    auto encoded = s.serialize(command);
    assertTrue(encoded.has_value());
    assertEqual("DO sweep\r\n", toAscii(*encoded).c_str());
}

test(serializer_rejects_payload_mismatch) {
    Serializer s;
    Command command;
    command.type = CommandType::GET;
    command.payload = CapsPayload{};

    auto encoded = s.serialize(command);
    assertFalse(encoded.has_value());
}

test(serializer_rejects_do_args) {
    Serializer s;
    Command command;
    command.type = CommandType::DO;
    command.payload = DoPayload{
        "sweep",
        { In{ "start_us", std::vector<uint8_t>{ '5', '0', '0' } } }
    };

    auto encoded = s.serialize(command);
    assertFalse(encoded.has_value());
}

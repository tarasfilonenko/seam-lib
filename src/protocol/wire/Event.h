#pragma once
#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "../caps/Caps.h"

namespace seam {
namespace protocol {
namespace wire {

enum class EventType {
    CONNECTED,
    DISCONNECTED,
    CAPS_READY,
    CMD_OK,
    CMD_ERR,
    CMD_TIMEOUT,
    VALUE_RECEIVED,
    CHANGED,
    STREAM_DATA,
};

struct CapsReadyPayload {
    caps::Caps caps;
};

struct OkPayload {
    std::string id;
    std::string text;   // optional trailing text
};

struct ErrPayload {
    std::string          code;
    std::string          id;
    std::string          message;
    std::vector<uint8_t> min;     // OUT_OF_RANGE only
    std::vector<uint8_t> max;     // OUT_OF_RANGE only
    std::string          missing; // BAD_ARGS only
};

struct CmdTimeoutPayload {
    std::string id;
};

struct ValueReceivedPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct ChangedPayload {
    std::string id;
};

struct StreamDataPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct Event {
    EventType type;
    std::variant<
        std::monostate,
        CapsReadyPayload,
        OkPayload,
        ErrPayload,
        CmdTimeoutPayload,
        ValueReceivedPayload,
        ChangedPayload,
        StreamDataPayload
    > payload;
};

} // namespace wire
} // namespace protocol
} // namespace seam
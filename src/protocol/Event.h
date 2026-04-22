#pragma once
// ─────────────────────────────────────────────
// seam::protocol::SeamEventType
// seam::protocol::SeamEvent
//
// Models protocol events emitted by the protocol layer.
// Each event type carries only its relevant payload.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include <variant>
#include "Caps.h"

namespace seam {
namespace protocol {

enum class EventType {
    CONNECTED,
    DISCONNECTED,
    CAPS_READY,
    CMD_CONFIRMED,
    CMD_FAILED,
    CMD_TIMEOUT,
    VALUE_RECEIVED,
    CHANGED,
    STREAM_DATA,
};

// ── Payloads ──────────────────────────────────

struct CapsReadyPayload {
    Caps caps;
};

struct ValueReceivedPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct ChangedPayload {
    std::string id;
};

struct CmdConfirmedPayload {
    std::string id;
};

struct CmdFailedPayload {
    std::string id;
    std::string err_code;
};

struct CmdTimeoutPayload {
    std::string id;
};

struct StreamDataPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

// ── Event ─────────────────────────────────────

struct Event {
    EventType type;
    std::variant<
        std::monostate,       // CONNECTED, DISCONNECTED — no payload
        CapsReadyPayload,
        ValueReceivedPayload,
        ChangedPayload,
        CmdConfirmedPayload,
        CmdFailedPayload,
        CmdTimeoutPayload,
        StreamDataPayload
    > payload;
};

} // namespace protocol
} // namespace seam
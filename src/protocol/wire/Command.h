#pragma once
// ─────────────────────────────────────────────
// seam::protocol::wire::Command
//
// Models commands sent from controller to protocol layer.
// Each command type carries only its relevant payload.
//
// Wire format:
//   CAPS\r\n
//   GET <id>\r\n
//   SET <id> <length>\r\n<data>\r\n
//   DO BEGIN <id>\r\n[IN ...]\r\nDO END\r\n
//   STATUS\r\n
//   WATCH <id>\r\n
//   UNWATCH <id>\r\n
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include <variant>
#include <cstdint>
#include "In.h"

namespace seam {
namespace protocol {
namespace wire {

enum class CommandType {
    CAPS,
    GET,
    SET,
    DO,
    STATUS,
    WATCH,
    UNWATCH,
};

struct CapsPayload {};
struct StatusPayload {};

struct GetPayload {
    std::string id;
};

struct SetPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct DoPayload {
    std::string     id;
    std::vector<In> args;
};

struct WatchPayload {
    std::string id;
};

struct UnwatchPayload {
    std::string id;
};

struct Command {
    CommandType type;
    std::variant <
        CapsPayload,
        StatusPayload,
        GetPayload,
        SetPayload,
        DoPayload,
        WatchPayload,
        UnwatchPayload
    > payload;
};

} // namespace wire
} // namespace protocol
} // namespace seam

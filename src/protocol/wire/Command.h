#pragma once
// ─────────────────────────────────────────────
// seam::protocol::wire::Command
//
// Models commands sent from controller to protocol layer.
// Each command type carries only its relevant payload.
//
// Wire format:
//   GET <id>\r\n
//   SET <id> <length>\r\n<data>\r\n
//   DO BEGIN <id>\r\n[IN ...]\r\nDO END\r\n
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
    GET,
    SET,
    DO,
};

// ── Payloads ──────────────────────────────────

struct GetPayload {
    std::string id;
};

struct SetPayload {
    std::string          id;
    std::vector<uint8_t> data;
};

struct DoPayload {
    std::string      id;
    std::vector<In>  args;
};

// ── Command ───────────────────────────────────

struct Command {
    CommandType type;
    std::variant <
        GetPayload,
        SetPayload,
        DoPayload
    > payload;
};

} // namespace wire
} // namespace protocol
} // namespace seam
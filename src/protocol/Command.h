#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Command
//
// Models commands sent from controller to protocol layer.
// Each command type carries only its relevant payload.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include <variant>
#include "In.h"

namespace seam {
namespace protocol {

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
    std::variant<
        GetPayload,
        SetPayload,
        DoPayload
    > payload;
};

} // namespace protocol
} // namespace seam
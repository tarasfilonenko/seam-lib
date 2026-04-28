#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Serializer
//
// Serializes typed wire::Command objects into raw SEAM bytes.
// Stateless and reusable across transports.
//
// Returns nullopt when the command payload does not match the
// declared type, or when the command shape is not yet supported.
// ─────────────────────────────────────────────

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "wire/Command.h"

namespace seam {
namespace protocol {

class Serializer {
public:
    using Buffer = std::vector<uint8_t>;

    std::optional<Buffer> serialize(const wire::Command& command) const {
        Buffer message;

        switch (command.type) {
            case wire::CommandType::CAPS: {
                const auto* payload = std::get_if<wire::CapsPayload>(&command.payload);
                if (!payload) return std::nullopt;

                appendAscii(message, "CAPS");
                appendCrlf(message);
                return message;
            }

            case wire::CommandType::GET: {
                const auto* payload = std::get_if<wire::GetPayload>(&command.payload);
                if (!payload) return std::nullopt;

                appendAscii(message, "GET ");
                appendAscii(message, payload->id);
                appendCrlf(message);
                return message;
            }

            case wire::CommandType::SET: {
                const auto* payload = std::get_if<wire::SetPayload>(&command.payload);
                if (!payload) return std::nullopt;

                appendAscii(message, "SET ");
                appendAscii(message, payload->id);
                message.push_back(' ');
                appendAscii(message, std::to_string(payload->data.size()));
                appendCrlf(message);
                message.insert(message.end(), payload->data.begin(), payload->data.end());
                appendCrlf(message);
                return message;
            }

            case wire::CommandType::DO: {
                const auto* payload = std::get_if<wire::DoPayload>(&command.payload);
                if (!payload) return std::nullopt;
                if (!payload->args.empty()) return std::nullopt;

                appendAscii(message, "DO ");
                appendAscii(message, payload->id);
                appendCrlf(message);
                return message;
            }
        }

        return std::nullopt;
    }

private:
    static void appendAscii(Buffer& message, const std::string& text) {
        message.insert(message.end(), text.begin(), text.end());
    }

    static void appendCrlf(Buffer& message) {
        message.push_back('\r');
        message.push_back('\n');
    }
};

} // namespace protocol
} // namespace seam

#pragma once
// ─────────────────────────────────────────────
// seam::protocol::Parser
//
// Parses a raw SEAM byte stream into typed wire::Event objects.
// Feed raw bytes in chunks of any size via feed().
// Poll takeEvent() — returns nullopt when no event ready.
//
// On any parse error: emits PARSE_ERROR event then resets.
// No FreeRTOS dependency — pure C++ state machine.
// Threading: not thread-safe — call from one task only.
// ─────────────────────────────────────────────

#include <string>
#include <vector>
#include <queue>
#include <optional>
#include <sstream>
#include <cstdint>
#include <cstring>
#include "wire/Event.h"
#include "caps/Caps.h"

namespace seam {
namespace protocol {

class Parser {
public:
    Parser() = default;

    void feed(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            uint8_t byte = data[i];

            if (_binary_remaining > 0) {
                processBinaryByte(byte);
                continue;
            }

            if (_binary_skip_crlf) {
                if (byte == '\n') _binary_skip_crlf = false;
                continue;
            }

            if (byte == '\r') continue;

            if (byte == '\n') {
                processLine(_line_buf);
                _line_buf.clear();
                continue;
            }

            _line_buf += static_cast<char>(byte);
        }
    }

    std::optional<wire::Event> takeEvent() {
        if (_events.empty()) return std::nullopt;
        wire::Event ev = std::move(_events.front());
        _events.pop();
        return ev;
    }

    void reset() {
        _state            = State::IDLE;
        _line_buf.clear();
        _binary_id.clear();
        _binary_keyword.clear();
        _binary_remaining = 0;
        _binary_buf.clear();
        _binary_skip_crlf = false;
        _caps             = caps::Caps{};
        _current_group    = caps::Group{};
        _current_param    = caps::Param{};
        _current_action   = caps::Action{};
        _current_arg      = caps::Arg{};
        _current_stream   = caps::Stream{};
        _current_err      = wire::ErrPayload{};
        _pending_id.clear();
        while (!_events.empty()) _events.pop();
    }

private:
    // ── State Machine ─────────────────────────

    enum class State {
        IDLE,
        IN_CAPS,
        IN_GROUP,
        IN_PARAM,
        IN_ACTION,
        IN_ARG,
        IN_STREAM,
        IN_ERR,
    };

    State _state = State::IDLE;

    // ── Line Assembly ─────────────────────────

    std::string _line_buf;

    // ── Binary Frame ──────────────────────────

    std::string          _binary_id;
    std::string          _binary_keyword;
    size_t               _binary_remaining = 0;
    std::vector<uint8_t> _binary_buf;
    bool                 _binary_skip_crlf = false;

    // ── CAPS Build State ──────────────────────

    caps::Caps    _caps;
    caps::Group   _current_group;
    caps::Param   _current_param;
    caps::Action  _current_action;
    caps::Arg     _current_arg;
    caps::Stream  _current_stream;

    // ── ERR Build State ───────────────────────

    wire::ErrPayload _current_err;

    // ── Pending Command ───────────────────────

    std::string _pending_id;

    // ── Event Queue ───────────────────────────

    std::queue<wire::Event> _events;

    void emitEvent(wire::Event&& ev) {
        _events.push(std::move(ev));
    }

    // ── Error Handling ────────────────────────
    // Central failure point — emit PARSE_ERROR then reset.
    // Adding a new error condition anywhere is one line: fail(line, "reason").

    void fail(const std::string& line, const std::string& reason) {
        wire::Event ev;
        ev.type    = wire::EventType::PARSE_ERROR;
        ev.payload = wire::ParseErrorPayload{ line, reason };
        emitEvent(std::move(ev));
        reset();
    }

    // ── Helpers ───────────────────────────────

    static std::vector<std::string> tokenise(const std::string& line) {
        std::vector<std::string> tokens;
        std::istringstream ss(line);
        std::string token;
        while (ss >> token) tokens.push_back(token);
        return tokens;
    }

    static bool parseKeyValue(const std::string& line,
                              std::string& key,
                              std::string& value) {
        auto pos = line.find(':');
        if (pos == std::string::npos) return false;
        key   = line.substr(0, pos);
        value = line.substr(pos + 1);
        return true;
    }

    static bool parseBool(const std::string& value) {
        return value == "true";
    }

    static bool parseSize(const std::string& value, size_t& out) {
        try {
            out = std::stoul(value);
            return true;
        } catch (...) {
            return false;
        }
    }

    // ── Field Appliers ────────────────────────

    void applyCapsField(const std::string& key, const std::string& value) {
        if      (key == "type")    _caps.device_type = value;
        else if (key == "name")    _caps.device_name = value;
        else if (key == "version") _caps.version     = value;
    }

    void applyGroupField(const std::string& key, const std::string& value) {
        if      (key == "label")   _current_group.label        = value;
        else if (key == "enabled") _current_group.enabled_expr = value;
        else if (key == "visible") _current_group.visible_expr = value;
    }

    void applyParamField(const std::string& key, const std::string& value) {
        if      (key == "type")        _current_param.type         = value;
        else if (key == "access")      _current_param.access       = value.empty() ? 'r' : value[0];
        else if (key == "label")       _current_param.label        = value;
        else if (key == "description") _current_param.description  = value;
        else if (key == "default")     _current_param.default_val  = value;
        else if (key == "options")     _current_param.options      = value;
        else if (key == "flags")       _current_param.flags        = value;
        else if (key == "enabled")     _current_param.enabled_expr = value;
        else if (key == "visible")     _current_param.visible_expr = value;
        else if (key == "watchable")   _current_param.watchable    = parseBool(value);
        else if (key == "persist")     _current_param.persist      = parseBool(value);
        else if (key == "min") {
            _current_param.has_min = true;
            _current_param.min_val = std::stof(value);
        }
        else if (key == "max") {
            _current_param.has_max = true;
            _current_param.max_val = std::stof(value);
        }
    }

    void applyActionField(const std::string& key, const std::string& value) {
        if      (key == "label")       _current_action.label        = value;
        else if (key == "description") _current_action.description  = value;
        else if (key == "enabled")     _current_action.enabled_expr = value;
        else if (key == "visible")     _current_action.visible_expr = value;
        else if (key == "trigger")     _current_action.trigger      = value;
    }

    void applyArgField(const std::string& key, const std::string& value) {
        if      (key == "type")        _current_arg.type        = value;
        else if (key == "label")       _current_arg.label       = value;
        else if (key == "description") _current_arg.description = value;
        else if (key == "options")     _current_arg.options     = value;
        else if (key == "min") {
            _current_arg.has_min = true;
            _current_arg.min_val = std::stof(value);
        }
        else if (key == "max") {
            _current_arg.has_max = true;
            _current_arg.max_val = std::stof(value);
        }
    }

    void applyStreamField(const std::string& key, const std::string& value) {
        if      (key == "type")        _current_stream.type         = value;
        else if (key == "label")       _current_stream.label        = value;
        else if (key == "description") _current_stream.description  = value;
        else if (key == "enabled")     _current_stream.enabled_expr = value;
    }

    void applyErrField(const std::string& key, const std::string& value) {
        if      (key == "id")      _current_err.id      = value;
        else if (key == "message") _current_err.message = value;
        else if (key == "missing") _current_err.missing = value;
        else if (key == "min") {
            _current_err.min = std::vector<uint8_t>(value.begin(), value.end());
        }
        else if (key == "max") {
            _current_err.max = std::vector<uint8_t>(value.begin(), value.end());
        }
    }

    // ── Binary Byte Handler ───────────────────

    void processBinaryByte(uint8_t byte) {
        _binary_buf.push_back(byte);
        _binary_remaining--;

        if (_binary_remaining == 0) {
            if (_binary_keyword == "VALUE") {
                wire::Event ev;
                ev.type    = wire::EventType::VALUE_RECEIVED;
                ev.payload = wire::ValueReceivedPayload{
                    _binary_id,
                    std::move(_binary_buf)
                };
                emitEvent(std::move(ev));
            } else if (_binary_keyword == "DATA") {
                wire::Event ev;
                ev.type    = wire::EventType::STREAM_DATA;
                ev.payload = wire::StreamDataPayload{
                    _binary_id,
                    std::move(_binary_buf)
                };
                emitEvent(std::move(ev));
            }

            _binary_buf.clear();
            _binary_id.clear();
            _binary_keyword.clear();
            _binary_skip_crlf = true;
        }
    }

    // ── Main Line Processor ───────────────────

    void processLine(const std::string& line) {
        if (line.empty() || line[0] == '#') return;

        auto tokens = tokenise(line);
        if (tokens.empty()) return;

        const std::string& keyword = tokens[0];

        // ── Async notifications ───────────────

        if (keyword == "CHANGED") {
            if (tokens.size() < 2) {
                fail(line, "CHANGED missing id");
                return;
            }
            wire::Event ev;
            ev.type    = wire::EventType::CHANGED;
            ev.payload = wire::ChangedPayload{ tokens[1] };
            emitEvent(std::move(ev));
            return;
        }

        if (keyword == "DATA") {
            if (tokens.size() < 3) {
                fail(line, "DATA missing id or length");
                return;
            }
            size_t len = 0;
            if (!parseSize(tokens[2], len)) {
                fail(line, "DATA length is not a valid number");
                return;
            }
            _binary_keyword   = "DATA";
            _binary_id        = tokens[1];
            _binary_remaining = len;
            _binary_buf.clear();
            _binary_buf.reserve(len);
            return;
        }

        // ── State-specific handling ───────────

        switch (_state) {

            case State::IDLE: {
                if (keyword == "CAPS" && tokens.size() >= 2 && tokens[1] == "BEGIN") {
                    _caps  = caps::Caps{};
                    _state = State::IN_CAPS;
                }
                else if (keyword == "VALUE") {
                    if (tokens.size() < 3) {
                        fail(line, "VALUE missing id or length");
                        return;
                    }
                    size_t len = 0;
                    if (!parseSize(tokens[2], len)) {
                        fail(line, "VALUE length is not a valid number");
                        return;
                    }
                    _binary_keyword   = "VALUE";
                    _binary_id        = tokens[1];
                    _binary_remaining = len;
                    _binary_buf.clear();
                    _binary_buf.reserve(len);
                }
                else if (keyword == "OK") {
                    std::string text;
                    for (size_t i = 1; i < tokens.size(); i++) {
                        if (i > 1) text += ' ';
                        text += tokens[i];
                    }
                    wire::Event ev;
                    ev.type    = wire::EventType::CMD_OK;
                    ev.payload = wire::OkPayload{ _pending_id, text };
                    emitEvent(std::move(ev));
                }
                else if (keyword == "ERR") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "ERR missing BEGIN or code");
                        return;
                    }
                    _current_err      = wire::ErrPayload{};
                    _current_err.code = tokens[2];
                    _state            = State::IN_ERR;
                }
                else {
                    fail(line, "unexpected keyword in IDLE state");
                }
                break;
            }

            case State::IN_CAPS: {
                if (keyword == "CAPS" && tokens.size() >= 2 && tokens[1] == "END") {
                    wire::Event ev;
                    ev.type    = wire::EventType::CAPS_READY;
                    ev.payload = wire::CapsReadyPayload{ std::move(_caps) };
                    emitEvent(std::move(ev));
                    _state = State::IDLE;
                }
                else if (keyword == "GROUP") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "GROUP missing BEGIN or id");
                        return;
                    }
                    _current_group    = caps::Group{};
                    _current_group.id = tokens[2];
                    _state            = State::IN_GROUP;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in CAPS block");
                        return;
                    }
                    applyCapsField(key, value);
                }
                break;
            }

            case State::IN_GROUP: {
                if (keyword == "GROUP" && tokens.size() >= 2 && tokens[1] == "END") {
                    _caps.groups.push_back(std::move(_current_group));
                    _current_group = caps::Group{};
                    _state         = State::IN_CAPS;
                }
                else if (keyword == "PARAM") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "PARAM missing BEGIN or id");
                        return;
                    }
                    _current_param    = caps::Param{};
                    _current_param.id = tokens[2];
                    _state            = State::IN_PARAM;
                }
                else if (keyword == "ACTION") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "ACTION missing BEGIN or id");
                        return;
                    }
                    _current_action    = caps::Action{};
                    _current_action.id = tokens[2];
                    _state             = State::IN_ACTION;
                }
                else if (keyword == "STREAM") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "STREAM missing BEGIN or id");
                        return;
                    }
                    _current_stream    = caps::Stream{};
                    _current_stream.id = tokens[2];
                    _state             = State::IN_STREAM;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in GROUP block");
                        return;
                    }
                    applyGroupField(key, value);
                }
                break;
            }

            case State::IN_PARAM: {
                if (keyword == "PARAM" && tokens.size() >= 2 && tokens[1] == "END") {
                    _current_group.params.push_back(std::move(_current_param));
                    _current_param = caps::Param{};
                    _state         = State::IN_GROUP;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in PARAM block");
                        return;
                    }
                    applyParamField(key, value);
                }
                break;
            }

            case State::IN_ACTION: {
                if (keyword == "ACTION" && tokens.size() >= 2 && tokens[1] == "END") {
                    _current_group.actions.push_back(std::move(_current_action));
                    _current_action = caps::Action{};
                    _state          = State::IN_GROUP;
                }
                else if (keyword == "ARG") {
                    if (tokens.size() < 3 || tokens[1] != "BEGIN") {
                        fail(line, "ARG missing BEGIN or id");
                        return;
                    }
                    _current_arg    = caps::Arg{};
                    _current_arg.id = tokens[2];
                    _state          = State::IN_ARG;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in ACTION block");
                        return;
                    }
                    applyActionField(key, value);
                }
                break;
            }

            case State::IN_ARG: {
                if (keyword == "ARG" && tokens.size() >= 2 && tokens[1] == "END") {
                    _current_action.args.push_back(std::move(_current_arg));
                    _current_arg = caps::Arg{};
                    _state       = State::IN_ACTION;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in ARG block");
                        return;
                    }
                    applyArgField(key, value);
                }
                break;
            }

            case State::IN_STREAM: {
                if (keyword == "STREAM" && tokens.size() >= 2 && tokens[1] == "END") {
                    _current_group.streams.push_back(std::move(_current_stream));
                    _current_stream = caps::Stream{};
                    _state          = State::IN_GROUP;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in STREAM block");
                        return;
                    }
                    applyStreamField(key, value);
                }
                break;
            }

            case State::IN_ERR: {
                if (keyword == "ERR" && tokens.size() >= 2 && tokens[1] == "END") {
                    wire::Event ev;
                    ev.type    = wire::EventType::CMD_ERR;
                    ev.payload = std::move(_current_err);
                    emitEvent(std::move(ev));
                    _current_err = wire::ErrPayload{};
                    _state       = State::IDLE;
                }
                else {
                    std::string key, value;
                    if (!parseKeyValue(line, key, value)) {
                        fail(line, "expected key:value in ERR block");
                        return;
                    }
                    applyErrField(key, value);
                }
                break;
            }
        }
    }
};

} // namespace protocol
} // namespace seam
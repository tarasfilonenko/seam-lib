#pragma once
// ─────────────────────────────────────────────
// seam::cel::Value
//
// Runtime value type for the SEAM-CEL evaluator.
// Holds one of: Undefined, Bool, Number, String.
//
// Undefined propagates through operators — any operation involving an
// Undefined operand yields Undefined, so an expression that references a
// not-yet-loaded identifier evaluates to Undefined rather than failing.
// ─────────────────────────────────────────────

#include <string>
#include <string_view>
#include <variant>
#include <cstdint>

namespace seam {
namespace cel {

class Value {
public:
    enum class Kind : uint8_t { Undefined, Bool, Number, String };

    // ── Construction ──────────────────────────

    Value() = default;                                  // Undefined

    static Value undefined()               { return Value{}; }
    static Value boolean(bool v)           { Value x; x._data = v;        return x; }
    static Value number(double v)          { Value x; x._data = v;        return x; }
    static Value string(std::string v)     { Value x; x._data = std::move(v); return x; }

    // ── Inspection ────────────────────────────

    Kind kind() const {
        switch (_data.index()) {
            case 0: return Kind::Undefined;
            case 1: return Kind::Bool;
            case 2: return Kind::Number;
            case 3: return Kind::String;
        }
        return Kind::Undefined;
    }

    bool isUndefined() const { return kind() == Kind::Undefined; }
    bool isBool()      const { return kind() == Kind::Bool; }
    bool isNumber()    const { return kind() == Kind::Number; }
    bool isString()    const { return kind() == Kind::String; }

    // Accessors — caller must check kind() first. Returns defaulted value if
    // the variant doesn't currently hold the requested type, so a misuse is
    // bounded rather than UB.
    bool                asBool()   const { return isBool()   ? std::get<bool>(_data)        : false; }
    double              asNumber() const { return isNumber() ? std::get<double>(_data)      : 0.0; }
    std::string_view    asString() const { return isString() ? std::string_view{std::get<std::string>(_data)} : std::string_view{}; }

private:
    // Order matches Kind: Undefined (monostate), Bool, Number, String.
    std::variant<std::monostate, bool, double, std::string> _data;
};

} // namespace cel
} // namespace seam

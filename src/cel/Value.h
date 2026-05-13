#pragma once
// ─────────────────────────────────────────────
// seam::cel::Value
//
// Runtime value type for the SEAM-CEL evaluator.
// Holds one of: Undefined, Bool, Number, String, List.
//
// Undefined propagates through operators — any operation involving an
// Undefined operand yields Undefined, so an expression that references a
// not-yet-loaded identifier evaluates to Undefined rather than failing.
//
// List values back the `in` operator (CEL-style membership) and list
// literals (`[a, b, c]` in source). Backed by shared_ptr<ListData> so
// Value stays copyable cheaply — sharing the elements vector across
// copies. Lists are conceptually immutable after construction.
// ─────────────────────────────────────────────

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace seam {
namespace cel {

class Value;

// Forward decl; defined after Value so std::vector<Value> sees a
// complete type when it's instantiated.
struct ListData;

class Value {
public:
    enum class Kind : uint8_t { Undefined, Bool, Number, String, List };

    // ── Construction ──────────────────────────

    Value() = default;                                  // Undefined

    static Value undefined()              { return Value{}; }
    static Value boolean(bool v)          { Value x; x._data = v;                       return x; }
    static Value number(double v)         { Value x; x._data = v;                       return x; }
    static Value string(std::string v)    { Value x; x._data = std::move(v);            return x; }

    // List factories — out-of-line because ListData isn't complete yet.
    static Value list(std::shared_ptr<ListData> data);
    static Value list(std::initializer_list<Value> items);
    static Value list(std::vector<Value> items);

    // ── Inspection ────────────────────────────

    Kind kind() const noexcept {
        switch (_data.index()) {
            case 0: return Kind::Undefined;
            case 1: return Kind::Bool;
            case 2: return Kind::Number;
            case 3: return Kind::String;
            case 4: return Kind::List;
        }
        return Kind::Undefined;
    }

    bool isUndefined() const { return kind() == Kind::Undefined; }
    bool isBool()      const { return kind() == Kind::Bool; }
    bool isNumber()    const { return kind() == Kind::Number; }
    bool isString()    const { return kind() == Kind::String; }
    bool isList()      const { return kind() == Kind::List; }

    // Accessors — caller must check kind() first. Returns a defaulted
    // value if the variant doesn't currently hold the requested type, so
    // a misuse is bounded rather than UB.
    bool                asBool()   const { return isBool()   ? std::get<bool>(_data)        : false; }
    double              asNumber() const { return isNumber() ? std::get<double>(_data)      : 0.0; }
    std::string_view    asString() const { return isString() ? std::string_view{std::get<std::string>(_data)} : std::string_view{}; }
    std::shared_ptr<const ListData> asList() const;     // defined out-of-line

private:
    // shared_ptr<ListData> keeps Value cheaply copyable. ListData is an
    // incomplete type here; shared_ptr<incomplete> is fine in a variant
    // alternative because shared_ptr's size is fixed regardless of T.
    // ListData becomes complete below before any Value destructor or
    // get<> call is instantiated by a consumer including this header.
    std::variant<std::monostate, bool, double, std::string, std::shared_ptr<ListData>> _data;
};

// ── ListData ──────────────────────────────────
//
// Owns the elements of a List value. Defined here so std::vector<Value>
// inside it sees a complete Value type.
struct ListData {
    std::vector<Value> items;
};

// ── List factories (out-of-line) ──────────────

inline Value Value::list(std::shared_ptr<ListData> data) {
    Value v;
    v._data = std::move(data);
    return v;
}

inline Value Value::list(std::initializer_list<Value> items) {
    auto data = std::make_shared<ListData>();
    data->items.assign(items);
    return Value::list(std::move(data));
}

inline Value Value::list(std::vector<Value> items) {
    auto data = std::make_shared<ListData>();
    data->items = std::move(items);
    return Value::list(std::move(data));
}

inline std::shared_ptr<const ListData> Value::asList() const {
    if (!isList()) return nullptr;
    return std::get<std::shared_ptr<ListData>>(_data);
}

} // namespace cel
} // namespace seam

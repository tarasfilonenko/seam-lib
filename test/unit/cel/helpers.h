#pragma once
// ─────────────────────────────────────────────
// test/unit/cel/helpers — shared scaffolding for the cel test files
//
// The `cel_test::` namespace provides:
//   - emptyEnv()                — Env whose resolver returns Undefined
//   - Registry / envFor(reg)    — in-memory binding store + Env factory
//   - Compiled / compileSrc()   — compile, bundling expr + err + ok
//   - eval() / evalEmpty()      — compile then evaluate, return Value
//                                  (Undefined on compile failure so a
//                                   typo'd test source surfaces
//                                   loudly instead of silently passing)
//
// The cel_expect_* macros below wrap compile+evaluate+assert in one
// line. They are macros (not functions) because AUnit's assertion
// macros bind to the enclosing test's `this`, so a failure stops the
// test and is reported with the right name + line.
//
// Per-feature test files (test/unit/cel/<feature>.h) include this
// header and contribute their own test() cases. test/unit/test_cel.h
// pulls them all in for the unit sketch.
// ─────────────────────────────────────────────

#include <AUnit.h>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "cel/cel.h"

namespace cel_test {

inline seam::cel::Value lookupAlwaysUndefined(void* /*ctx*/, std::string_view /*id*/) {
    return seam::cel::Value::undefined();
}

inline seam::cel::Env emptyEnv() {
    return seam::cel::Env{ nullptr, &lookupAlwaysUndefined };
}

// Small in-memory binding store for tests. Linear scan, copies values
// out — fine for a few entries per test.
struct Registry {
    std::vector<std::pair<std::string, seam::cel::Value>> entries;

    seam::cel::Value lookup(std::string_view id) const {
        for (const auto &e : entries) {
            if (e.first == id) return e.second;
        }
        return seam::cel::Value::undefined();
    }
};

inline seam::cel::Value lookupRegistry(void *ctx, std::string_view id) {
    return static_cast<const Registry *>(ctx)->lookup(id);
}

inline seam::cel::Env envFor(const Registry &reg) {
    return seam::cel::Env{
        const_cast<Registry *>(&reg),   // Env stores void*; we never mutate
        &lookupRegistry,
    };
}

// Bundled compile result. Move-only because Expression is move-only.
struct Compiled {
    seam::cel::Expression   expr;
    seam::cel::CompileError err;
    bool                    ok = false;
};

inline Compiled compileSrc(std::string_view src) {
    Compiled c;
    c.ok = seam::cel::compile(src, c.expr, c.err);
    return c;
}

// Compile-and-evaluate against an Env. Returns Undefined on compile
// failure (rather than the always-true reset Expression's Value::
// boolean(true)) so a typo in a test source surfaces as a wrong-kind
// assertion failure instead of silently passing.
inline seam::cel::Value eval(std::string_view src, const seam::cel::Env &env) {
    seam::cel::Expression  expr;
    seam::cel::CompileError err;
    if (!seam::cel::compile(src, expr, err)) {
        return seam::cel::Value::undefined();
    }
    return seam::cel::evaluate(expr, env);
}

inline seam::cel::Value evalEmpty(std::string_view src) {
    seam::cel::Env env = emptyEnv();
    return eval(src, env);
}

} // namespace cel_test

// ── Expectation macros ────────────────────────────────────────

#define cel_expect_bool(src, expected)                                  \
    do {                                                                \
        auto _v = cel_test::evalEmpty(src);                             \
        assertTrue(_v.isBool());                                        \
        if (expected) assertTrue(_v.asBool()); else assertFalse(_v.asBool()); \
    } while (0)

#define cel_expect_number(src, expected)                                \
    do {                                                                \
        auto _v = cel_test::evalEmpty(src);                             \
        assertTrue(_v.isNumber());                                      \
        assertNear((double)(expected), _v.asNumber(), 1e-9);            \
    } while (0)

#define cel_expect_string(src, expected)                                \
    do {                                                                \
        auto _v = cel_test::evalEmpty(src);                             \
        assertTrue(_v.isString());                                      \
        assertEqual((expected), std::string(_v.asString()).c_str());    \
    } while (0)

// Eval against a Registry (must be a local of type cel_test::Registry).
#define cel_expect_bool_in(src, reg, expected)                          \
    do {                                                                \
        auto _v = cel_test::eval(src, cel_test::envFor(reg));           \
        assertTrue(_v.isBool());                                        \
        if (expected) assertTrue(_v.asBool()); else assertFalse(_v.asBool()); \
    } while (0)

#define cel_expect_number_in(src, reg, expected)                        \
    do {                                                                \
        auto _v = cel_test::eval(src, cel_test::envFor(reg));           \
        assertTrue(_v.isNumber());                                      \
        assertNear((double)(expected), _v.asNumber(), 1e-9);            \
    } while (0)

#define cel_expect_string_in(src, reg, expected)                        \
    do {                                                                \
        auto _v = cel_test::eval(src, cel_test::envFor(reg));           \
        assertTrue(_v.isString());                                      \
        assertEqual((expected), std::string(_v.asString()).c_str());    \
    } while (0)

#define cel_expect_undefined_in(src, reg)                               \
    do {                                                                \
        auto _v = cel_test::eval(src, cel_test::envFor(reg));           \
        assertTrue(_v.isUndefined());                                   \
    } while (0)

#define cel_expect_undefined(src)                                       \
    do {                                                                \
        auto _v = cel_test::evalEmpty(src);                             \
        assertTrue(_v.isUndefined());                                   \
    } while (0)

// Compile-only checks.
#define cel_expect_compile_error(src, pos)                              \
    do {                                                                \
        auto _c = cel_test::compileSrc(src);                            \
        assertFalse(_c.ok);                                             \
        assertEqual((size_t)(pos), _c.err.position);                    \
    } while (0)

#define cel_expect_compile_ok(src)                                      \
    do {                                                                \
        auto _c = cel_test::compileSrc(src);                            \
        assertTrue(_c.ok);                                              \
    } while (0)

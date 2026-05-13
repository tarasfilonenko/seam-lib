# seam::cel

Tiny CEL-subset evaluator used by the SEAM CAPS protocol.

`caps::Group`, `caps::Param`, `caps::Action`, and `caps::Stream` carry
`visible_expr` and `enabled_expr` strings. Hosts (e.g. the dock) evaluate
these on every relevant value change to decide whether to show/enable a UI
row. This module is the parser + evaluator for that expression language.

This is **not** full Google CEL — only the subset that actually appears
in real caps. The grammar is intentionally minimal so the library fits on
ESP32-class targets and stays allocation-free at evaluation time.

## Public surface

```cpp
#include "cel/cel.h"

seam::cel::Expression expr;
seam::cel::CompileError err;
if (!seam::cel::compile("mode == \"advanced\" && gain > 0", expr, err)) {
    // log err.position + err.message; treat as always-true
}

seam::cel::Env env{ ctx, &resolve_identifier };
seam::cel::Value v = seam::cel::evaluate(expr, env);
// v.isUndefined() → "not ready" (some referenced id is unbound)
// v.isBool()      → the answer
// otherwise       → caller policy (typically fail-open to true)
```

## Grammar

Precedence low → high:

```
expr       := or_expr
or_expr    := and_expr ( "||" and_expr )*
and_expr   := cmp_expr ( "&&" cmp_expr )*
cmp_expr   := not_expr ( ( "==" | "!=" | "<" | "<=" | ">" | ">=" | "in" ) not_expr )?
not_expr   := "!" not_expr | primary
primary    := number | string | "true" | "false" | identifier | "(" expr ")" | list_lit
list_lit   := "[" ( expr ( "," expr )* )? "]"
identifier := bare_id | "@" bare_id
bare_id    := [A-Za-z_][A-Za-z0-9_]*
number     := -?[0-9]+("."[0-9]+)?
string     := "([^"\\]|\\.)*"
```

Precedence: `!` binds tighter than comparison (so `!a == b` is `(!a) ==
b`), and comparisons bind tighter than `&&` / `||`. Comparisons are
non-chainable: `a == b == c` is a parse error — use parens to group
explicitly.

Bare identifiers resolve to caps param ids. `@`-prefixed identifiers are
reserved for host-provided state (e.g. `@connected`, `@preset_index`).
The grammar reserves the prefix even though no `@`-vars are emitted yet —
adding them later won't break older docks.

### `in` operator + list literals

`item in list` tests membership using CEL semantics. The right operand
must be a List (either a literal `[a, b, c]` or an identifier that
resolves to a List). Element-wise equality drives membership, using the
same type-strict rules as `==`:

```
1 in [1, 2, 3]               // → true
"x" in ["a", "b"]            // → false
1 in ["1", "2"]              // → Undefined (every comparison is a type mismatch)
1 in [1, "x"]                // → true (a match wins over later type errors)
```

Error absorption: any matching element wins (returns `true`), even if
earlier element comparisons were `Undefined`. If no element matches and
at least one comparison was `Undefined`, the result is `Undefined`.
Otherwise (all defined-false), the result is `false`.

Non-List right operand (string, number, bool, ...) → `Undefined`.

Real modules emit expressions like:

```
"{{CHANNEL_PREFIX}}" in enabled_channels
```

where `{{CHANNEL_PREFIX}}` is module-side template substitution that
happens **before** the string reaches cel, and `enabled_channels`
resolves (via the host's `Env`) to a List value containing the
allowed channel identifiers.

### List literals

Lists are written `[expr, expr, ...]`. Empty `[]` is allowed; trailing
commas are not (`[1, 2,]` is a parse error). Elements are full
expressions, so list literals nest naturally:

```
[1, 2, 3]
["fast", "slow"]
[gain, max_gain, 0]
[1 == 1, true, mode == "advanced"]
[[1, 2], [3, 4]]
```

List values currently exist primarily to feed the `in` operator. Other
operations on Lists (`==`, `<`, `!`, etc.) yield `Undefined` — there's
no recursive list equality or ordering. Expand if a real module needs
it.

## Semantics

- **Three-valued logic.** Identifiers may be `Undefined` (not yet bound).
  Undefined propagates: any operator with an Undefined operand yields
  Undefined. Hosts treat Undefined as "not ready" — typically hide the
  row until a real value arrives, avoiding a visible→hidden flicker on
  first display.
- **Type-strict comparisons.** `==` / `!=` between mismatched types yield
  Undefined (not false). Ordering ops require both operands to be Number.
  No implicit conversions — module authors get a loud "doesn't work"
  rather than a silently-wrong answer.
- **Total evaluator.** No exceptions. Bad expressions caught at compile
  time; bad runtime types caught as Undefined. Safe to call from tight
  loops.
- **Empty source = always true.** caps fields use `""` to mean "always".
  `compile("", out, err)` succeeds with `out.isAlwaysTrue() == true`.

## Error handling policy (recommended)

`compile()` returns false on parse error. Recommended host behaviour:

1. Log the source text + `err.position` + `err.message`.
2. Reset `out` (already done by `compile`) — it becomes always-true.
3. Continue rendering the param/action as if the expression were absent.

Hiding things on parse error makes the dock look broken when a module
ships a minor syntax issue, and the user has no recourse. Fail-open
keeps the device usable while giving the module author a logged signal.

## Why this is in seam-lib

The evaluator is bound to caps expression semantics, so it ships with
seam-lib for now. If a second consumer appears (host-side caps linter,
module SDK self-validation, web tooling), it's a `git mv` away from
becoming its own `seam-cel` library — the namespace is already set up
for that move.

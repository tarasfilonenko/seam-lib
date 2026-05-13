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
and_expr   := not_expr ( "&&" not_expr )*
not_expr   := "!" not_expr | cmp_expr
cmp_expr   := primary ( ( "==" | "!=" | "<" | "<=" | ">" | ">=" ) primary )?
primary    := number | string | "true" | "false" | identifier | "(" expr ")"
identifier := bare_id | "@" bare_id
bare_id    := [A-Za-z_][A-Za-z0-9_]*
number     := -?[0-9]+("."[0-9]+)?
string     := "([^"\\]|\\.)*"
```

Bare identifiers resolve to caps param ids. `@`-prefixed identifiers are
reserved for host-provided state (e.g. `@connected`, `@preset_index`).
The grammar reserves the prefix even though no `@`-vars are emitted yet —
adding them later won't break older docks.

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

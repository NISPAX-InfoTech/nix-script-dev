#!/usr/bin/env bash
set -euo pipefail

NIX="${1:-./build/nix}"
ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT
mkdir -p "$ROOT/modules"

cat > "$ROOT/modules/left.ns" <<'EOF'
var localValue = 40;
class Hidden {
    var stored;
    fn Hidden(initial) { this.stored = initial; }
    fn value() { return this.stored; }
};
fn helper() { return Hidden(localValue).value(); }
async fn plusOne(value) { return value + 1; }
export fn leftAnswer() { return helper() + 2; }
export async fn delayedAnswer(value) {
    var next = await plusOne(value);
    return next + helper();
}
export fn leftLabel() { return "value=${localValue}"; }
export var sharedCount = 3;
export int typedCount = 7;
export class PublicBox {
    var stored;
    fn PublicBox(initial) { this.stored = initial; }
    fn combined() { return helper() + this.stored; }
};
EOF

cat > "$ROOT/modules/right.ns" <<'EOF'
var localValue = 100;
class Hidden {
    var stored;
    fn Hidden(initial) { this.stored = initial; }
    fn value() { return this.stored; }
};
fn helper() { return Hidden(localValue).value(); }
export fn rightAnswer() { return helper() + 1; }
export fn rightLabel() { return "value=${localValue}"; }
EOF

cat > "$ROOT/positive.ns" <<'EOF'
import { leftAnswer, leftLabel, delayedAnswer as delayed, PublicBox as Box, sharedCount as count, typedCount as typed } from "./modules/left";
import { rightAnswer as computeRight, rightLabel } from "./modules/right";
nix main() {
    println("left=", leftAnswer());
    println("right=", computeRight());
    var asyncAnswer = await delayed(1);
    println("async-left=", asyncAnswer);
    println("left-label=", leftLabel());
    println("right-label=", rightLabel());
    var box = Box(2);
    println("public-box=", box.combined());
    println("exported-global=", count);
    count = count + 4;
    typed = typed + 1;
    println("updated-exported-global=", count);
    println("updated-typed-global=", typed);
}
EOF
actual="$("$NIX" "$ROOT/positive.ns")"
expected="$(printf 'left=42\nright=101\nasync-left=42\nleft-label=value=40\nright-label=value=100\npublic-box=42\nexported-global=3\nupdated-exported-global=7\nupdated-typed-global=8')"
if [[ "$actual" != "$expected" ]]; then
    echo "Private-module helper isolation failed." >&2
    echo "Expected: [$expected]" >&2
    echo "Actual:   [$actual]" >&2
    exit 1
fi

cat > "$ROOT/private_leak.ns" <<'EOF'
import { leftAnswer } from "./modules/left";
nix main() {
    println(helper());
}
EOF
if "$NIX" "$ROOT/private_leak.ns" >"$ROOT/private-leak.log" 2>&1; then
    echo "A module-private helper was callable from the importer." >&2
    exit 1
fi
grep -Eq "Unknown (function|variable) 'helper'" "$ROOT/private-leak.log" || {
    echo "Private-helper rejection had an unexpected diagnostic:" >&2
    cat "$ROOT/private-leak.log" >&2
    exit 1
}
cat > "$ROOT/unimported_export.ns" <<'EOF'
import { leftAnswer } from "./modules/left";
import { rightAnswer as computeRight } from "./modules/right";
nix main() {
    println(rightAnswer());
}
EOF
if "$NIX" "$ROOT/unimported_export.ns" >"$ROOT/unimported-export.log" 2>&1; then
    echo "An exported symbol was accessible without importing that binding." >&2
    exit 1
fi
grep -Eq "Unknown (variable|function) 'rightAnswer'" "$ROOT/unimported-export.log" || {
    echo "Unimported-export rejection had an unexpected diagnostic:" >&2
    cat "$ROOT/unimported-export.log" >&2
    exit 1
}

cat > "$ROOT/private_global_leak.ns" <<'EOF'
import { leftAnswer } from "./modules/left";
nix main() {
    println(localValue);
}
EOF
if "$NIX" "$ROOT/private_global_leak.ns" >"$ROOT/private-global-leak.log" 2>&1; then
    echo "A module-private global was visible to the importer." >&2
    exit 1
fi
grep -Eq "Unknown (variable|function) 'localValue'" "$ROOT/private-global-leak.log" || {
    echo "Private-global rejection had an unexpected diagnostic:" >&2
    cat "$ROOT/private-global-leak.log" >&2
    exit 1
}


cat > "$ROOT/private_class_leak.ns" <<'EOF'
import { leftAnswer } from "./modules/left";
nix main() {
    var item = Hidden(1);
    println(item.value());
}
EOF
if "$NIX" "$ROOT/private_class_leak.ns" >"$ROOT/private-class-leak.log" 2>&1; then
    echo "A module-private class was visible to the importer." >&2
    exit 1
fi
grep -Eq "Unknown variable 'Hidden'|Unknown function 'Hidden'" "$ROOT/private-class-leak.log" || {
    echo "Private-class rejection had an unexpected diagnostic:" >&2
    cat "$ROOT/private-class-leak.log" >&2
    exit 1
}

cat > "$ROOT/legacy_and_named.ns" <<'EOF'
use left;
import { leftAnswer } from "./left";
nix main() { println(leftAnswer()); }
EOF
cp "$ROOT/modules/left.ns" "$ROOT/left.ns"
if "$NIX" "$ROOT/legacy_and_named.ns" >"$ROOT/mixed-mode.log" 2>&1; then
    echo "Mixing legacy and named imports of one source unexpectedly succeeded." >&2
    exit 1
fi
grep -q "both named imports and legacy 'use'" "$ROOT/mixed-mode.log" || {
    echo "Mixed-import rejection had an unexpected diagnostic:" >&2
    cat "$ROOT/mixed-mode.log" >&2
    exit 1
}

cat > "$ROOT/modules/overloads.ns" <<'EOF'
export fn convert(int value) { return value + 1; }
export fn convert(string value) { return value + "!"; }
EOF

cat > "$ROOT/exported_overloads.ns" <<'EOF'
import { convert as convertValue } from "./modules/overloads";
nix main() {
    println(convertValue(41));
    println(convertValue("ok"));
}
EOF
overload_actual="$("$NIX" "$ROOT/exported_overloads.ns")"
overload_expected="$(printf '42
ok!')"
if [[ "$overload_actual" != "$overload_expected" ]]; then
    echo "Exported overload set failed across a named module boundary." >&2
    echo "Expected: [$overload_expected]" >&2
    echo "Actual:   [$overload_actual]" >&2
    exit 1
fi

cat > "$ROOT/modules/once.ns" <<'EOF'
var initCount = 1;
export fn initRuns() { return initCount; }
EOF

cat > "$ROOT/reimport_once.ns" <<'EOF'
import { initRuns as first } from "./modules/once";
import { initRuns as second } from "./modules/once";
nix main() {
    println(first());
    println(second());
}
EOF
once_actual="$("$NIX" "$ROOT/reimport_once.ns")"
once_expected="$(printf '1
1')"
if [[ "$once_actual" != "$once_expected" ]]; then
    echo "A module imported twice did not initialize exactly once." >&2
    echo "Expected: [$once_expected]" >&2
    echo "Actual:   [$once_actual]" >&2
    exit 1
fi

echo "Module private-function isolation passed."

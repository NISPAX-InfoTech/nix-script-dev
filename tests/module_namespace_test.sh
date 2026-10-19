#!/usr/bin/env bash
set -euo pipefail

NIX="${1:-./build/nix}"
ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT
mkdir -p "$ROOT/modules"

cat > "$ROOT/modules/left.ns" <<'EOF'
var localValue = 40;
class Hidden {
    fn value() { return localValue; }
}
fn helper() { return Hidden().value(); }
export fn leftAnswer() { return helper() + 2; }
EOF

cat > "$ROOT/modules/right.ns" <<'EOF'
var localValue = 100;
class Hidden {
    fn value() { return localValue; }
}
fn helper() { return Hidden().value(); }
export fn rightAnswer() { return helper() + 1; }
EOF

cat > "$ROOT/positive.ns" <<'EOF'
import { leftAnswer } from "./modules/left";
import { rightAnswer } from "./modules/right";
nix main() {
    println("left=", leftAnswer());
    println("right=", rightAnswer());
}
EOF
actual="$("$NIX" "$ROOT/positive.ns")"
expected="$(printf 'left=42\nright=101')"
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

echo "Module private-function isolation passed."

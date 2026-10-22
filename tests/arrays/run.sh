#!/usr/bin/env bash
set -euo pipefail

BIN="${1:-./build/nix}"
tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

check_output() {
  local file="$1"
  local expected="$2"
  "$BIN" "$file" >"$tmp/actual.txt"
  printf '%s' "$expected" >"$tmp/expected.txt"
  if ! diff -u "$tmp/expected.txt" "$tmp/actual.txt"; then
    echo "Array regression failed: $file" >&2
    exit 1
  fi
  echo "PASS $file"
}

check_error() {
  local file="$1"
  local expected="$2"
  if "$BIN" "$file" >"$tmp/actual.txt" 2>&1; then
    echo "Expected failure but program succeeded: $file" >&2
    exit 1
  fi
  if ! grep -Fq "$expected" "$tmp/actual.txt"; then
    cat "$tmp/actual.txt" >&2
    echo "Expected error text '$expected' from $file" >&2
    exit 1
  fi
  echo "PASS $file (expected error)"
}

check_output tests/arrays/01_empty_dynamic_array.ns $'0\ntrue\n'
check_output tests/arrays/02_push_pop.ns $'2\nx\n10\ntrue\n'
check_output tests/arrays/03_insert_positions.ns $'5\n10\n20\n30\n40\n'
check_output tests/arrays/04_remove_positions.ns $'20\n2\n30\n'
check_output tests/arrays/05_get_set.ns $'10\n99\n'
check_output tests/arrays/06_contains_index_of.ns $'true\nfalse\n0\n-1\n'
check_output tests/arrays/07_clear.ns $'0\ntrue\n4\n'
check_output tests/arrays/08_nested_and_dynamic_growth.ns $'5\n7\n4\n4\n'
check_error tests/arrays/09_invalid_index.ns "Array index must be a non-negative whole number."
check_error tests/arrays/10_invalid_insert.ns "Array.insert() position is out of range."
check_error tests/arrays/11_wrong_arity.ns "Array.push() expects 1 argument(s)."
check_error tests/arrays/12_extra_aliases_rejected.ns "Array has no method 'append'."
check_output tests/arrays/13_map_compatibility.ns $'Nix\n3\n0\n'

echo "All array regression cases passed."

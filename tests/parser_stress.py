#!/usr/bin/env python3
"""Deterministic malformed-source stress test: parser must reject safely, not crash."""

from __future__ import annotations

import random
import subprocess
import sys
import tempfile
from pathlib import Path

SEED = 20261016
TIMEOUT_SECONDS = 2
UNKNOWN_CHARS = ("@", "#", "$", "?", chr(96), "^", "[", "]")


def make_cases() -> list[tuple[str, str]]:
    cases: list[tuple[str, str]] = []
    for char in UNKNOWN_CHARS:
        cases.append((
            f"unknown character {char!r}",
            f'nix main() {{ println(10 {char} 20); }}',
        ))

    cases.extend([
        ("single ampersand", "nix main() { if (true & false) { println(1); } }"),
        ("single pipe", "nix main() { if (true | false) { println(1); } }"),
        ("missing expression", "nix main() { var value = ; }"),
        ("unclosed grouping", "nix main() { println((1 + 2); }"),
        ("extra closing grouping", "nix main() { println(1 + 2)); }"),
        ("unclosed main block", 'nix main() { println("x");'),
        ("extra top-level closing brace", 'nix main() { println("x"); }}'),
        ("missing function parameter close", "fn f(value { return value; } nix main() {}"),
        ("missing class body close", "class C { public: int value = 1; ; nix main() {}"),
        ("entry point with parameter", "nix main(value) { println(value); }"),
        ("missing declaration semicolon", "nix main() { var value = 1 println(value); }"),
        ("operator without right operand", "nix main() { println(1 + ); }"),
        ("missing return expression", "fn f() { return 1 + ; } nix main() {}"),
        ("unterminated string", 'nix main() { println("unclosed); }'),
        ("invalid interpolation", 'nix main() { println("{$"); }'),
        ("typed top-level parameter", "fn f(int value) { return value; } nix main() {}"),
    ])

    # Deterministically mutate operators/delimiters around otherwise valid tokens.
    rng = random.Random(SEED)
    expressions = [
        "1 + 2", "(3 * 4)", "5 / 1", "true and false",
        '"quoted text"', "sqrt(9)", "item.field", "f(1, 2)",
    ]
    mutations = [
        lambda e: e + ")",
        lambda e: "(" + e,
        lambda e: e + ",",
        lambda e: e + " ; ;",
        lambda e: e.replace(" ", "  ", 1) + " @",
        lambda e: e.replace("(", "", 1) if "(" in e else "(" + e,
        lambda e: e.replace(")", "", 1) if ")" in e else e + ")",
    ]
    for i in range(96):
        expression = rng.choice(expressions)
        mutation = rng.choice(mutations)
        damaged = mutation(expression)
        cases.append((f"mutated expression {i + 1}", f"nix main() {{ println({damaged}); }}"))

    return cases


def main() -> int:
    if len(sys.argv) > 2:
        print("Usage: parser_stress.py [path/to/nix]", file=sys.stderr)
        return 2

    binary = Path(sys.argv[1] if len(sys.argv) == 2 else "./build/nix").resolve()
    if not binary.is_file():
        print(f"Interpreter not found: {binary}", file=sys.stderr)
        return 2

    cases = make_cases()
    rejected = 0
    strict_count = len(cases) - 96
    for index, (label, source) in enumerate(cases, 1):
        with tempfile.TemporaryDirectory(prefix="nixscript-parser-") as directory:
            script = Path(directory) / "malformed.ns"
            script.write_text(source + "\n", encoding="utf-8")
            try:
                process = subprocess.run(
                    [str(binary), str(script)],
                    check=False,
                    capture_output=True,
                    text=True,
                    timeout=TIMEOUT_SECONDS,
                )
            except subprocess.TimeoutExpired:
                print(f"FAIL case {index} ({label}): parser timed out", file=sys.stderr)
                print(source, file=sys.stderr)
                return 1

        if process.returncode < 0:
            print(f"FAIL case {index} ({label}): process terminated by signal {-process.returncode}", file=sys.stderr)
            print(source, file=sys.stderr)
            print(process.stderr, file=sys.stderr)
            return 1
        if process.returncode == 0 and index <= strict_count:
            print(f"FAIL case {index} ({label}): malformed source was accepted", file=sys.stderr)
            print(source, file=sys.stderr)
            return 1
        if process.returncode != 0:
            rejected += 1

    if rejected < strict_count:
        print(f"FAIL: rejected only {rejected} of {strict_count} malformed hand-authored cases", file=sys.stderr)
        return 1

    print(f"PASS: safely handled {len(cases)} malformed-source cases ({rejected} rejected; seed={SEED})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

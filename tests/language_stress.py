#!/usr/bin/env python3
"""Black-box regression/stress tests for the NixScript interpreter."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass
class Case:
    name: str
    source: str
    expected_stdout: str = ""
    stdin: str = ""
    should_succeed: bool = True
    expected_error: str = ""


CASES = [
    Case("empty main", 'nix main() { }'),
    Case("integer arithmetic", 'nix main() { println(2 + 3 * 4); }', "14\n"),
    Case("parentheses", 'nix main() { println((2 + 3) * 4); }', "20\n"),
    Case("division and subtraction", 'nix main() { println(20 / 5 - 2); }', "2\n"),
    Case("unary minus", 'nix main() { println(-7 + 10); }', "3\n"),
    Case("assignment and reassignment", 'nix main() { var n = 2; n = n + 5; println(n); }', "7\n"),
    Case("multiple variables", 'nix main() { var a = 3; var b = 4; println(a + b); }', "7\n"),
    Case("string output", 'nix main() { print("alpha"); println(" beta"); }', "alpha beta\n"),
    Case("string interpolation", 'nix main() { var who = "Nix"; println("Hello, {$who}!"); }', "Hello, Nix!\n"),
    Case("escape sequences", r'nix main() { print("A\nB\tC"); }', "A\nB\tC"),
    Case("boolean literals and condition", 'nix main() { if (true) { println("yes"); } else { println("no"); } }', "yes\n"),
    Case("comparison less-than", 'nix main() { println(2 < 3); }', "1\n"),
    Case("comparison equality", 'nix main() { println(5 == 5); }', "1\n"),
    Case("logical and", 'nix main() { if (1 and 2) { println("yes"); } }', "yes\n"),
    Case("logical not", 'nix main() { if (not 0) { println("yes"); } }', "yes\n"),
    Case("elif chain", 'nix main() { var n = 2; if (n == 1) { println("one"); } elif (n == 2) { println("two"); } else { println("other"); } }', "two\n"),
    Case("else-if chain", 'nix main() { var n = 3; if (n == 1) { println("one"); } else if (n == 3) { println("three"); } else { println("other"); } }', "three\n"),
    Case("repeat zero times", 'nix main() { repeat(0) { println("bad"); } println("done"); }', "done\n"),
    Case("repeat three times", 'nix main() { var i = 0; repeat(3) { println(i); i = i + 1; } }', "0\n1\n2\n"),
    Case("for loop", 'nix main() { for (var i = 0; i < 4; i = i + 1) { print(i); } println(""); }', "0123\n"),
    Case("function return", 'fn add(a, b) { return a + b; } nix main() { println(add(4, 8)); }', "12\n"),
    Case("recursive factorial", 'fn fact(n) { if (n <= 1) { return 1; } return n * fact(n - 1); } nix main() { println(fact(6)); }', "720\n"),
    Case("function local scope", 'var x = 10; fn local() { var x = 20; return x; } nix main() { println(local()); println(x); }', "20\n10\n"),
    Case("prototype before main definition after", 'fn calc(a); nix main() { println(calc(9)); } fn calc(x) { return x * 2; }', "18\n"),
    Case("void-like function end", 'fn say() { println("ok"); } nix main() { say(); println("after"); }', "ok\nafter\n"),
    Case("string lowercase", 'nix main() { println(toLower("MiXeD")); }', "mixed\n"),
    Case("string uppercase", 'nix main() { println(toUpper("MiXeD")); }', "MIXED\n"),
    Case("string predicate", 'nix main() { println(isUpper("ABC")); println(isLower("abc")); }', "1\n1\n"),
    Case("square helper", 'nix main() { println(sq(9)); }', "81\n"),
    Case("power helper", 'nix main() { println(pow(2, 5)); }', "32\n"),
    Case("absolute helper", 'nix main() { println(abs(-12)); }', "12\n"),
    Case("square root helper", 'nix main() { println(sqrt(81)); }', "9\n"),
    Case("string input", 'nix main() { var name; uin(name); println(name); }', "Ada\n", "Ada\n"),
    Case("integer input", 'nix main() { var age = uin("Age: ", int); println(age); }', "Age: 27\n", "27\n"),
    Case("boolean input", 'nix main() { var ok = uin("OK: ", bool); println(ok); }', "OK: 1\n", "true\n"),
    Case("missing main rejected", 'fn helper() { return 1; }', should_succeed=False),
    Case("undefined function rejected", 'nix main() { missing(); }', should_succeed=False),
    Case("unresolved prototype rejected on call", 'fn missing(); nix main() { missing(); }', should_succeed=False),
    Case("wrong function arity rejected", 'fn one(a) { return a; } nix main() { one(1, 2); }', should_succeed=False),
    Case("duplicate function definition rejected", 'fn f(a) { return a; } fn f(a) { return a; } nix main() { println(f(1)); }', should_succeed=False),
    Case("private access rejected", 'class Secret { private: int value = 3; }; nix main() { Secret s; println(s.value); }', should_succeed=False),
    Case("invalid typed integer input rejected", 'nix main() { var age = uin("Age: ", int); println(age); }', stdin="not-a-number\n", should_succeed=False),
]


def run_case(binary: Path, case: Case, root: Path) -> tuple[bool, str]:
    source_path = root / "case.ns"
    source_path.write_text(case.source + "\n", encoding="utf-8")
    try:
        proc = subprocess.run(
            [str(binary), str(source_path)],
            input=case.stdin,
            text=True,
            capture_output=True,
            timeout=5,
            check=False,
        )
    except subprocess.TimeoutExpired:
        return False, "timed out after 5 seconds (possible infinite loop)"
    combined = proc.stdout + proc.stderr
    if case.should_succeed:
        if proc.returncode != 0:
            return False, f"exit={proc.returncode}; output={combined!r}"
        if proc.stdout != case.expected_stdout:
            return False, f"stdout expected {case.expected_stdout!r}, got {proc.stdout!r}; stderr={proc.stderr!r}"
        return True, ""
    if proc.returncode == 0:
        return False, f"expected failure but exited 0; stdout={proc.stdout!r}"
    if case.expected_error and case.expected_error not in combined:
        return False, f"expected error containing {case.expected_error!r}, got {combined!r}"
    return True, ""


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} PATH_TO_NIX_BINARY", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1]).resolve()
    if not binary.is_file():
        print(f"interpreter not found: {binary}", file=sys.stderr)
        return 2

    passed = 0
    failures: list[tuple[str, str]] = []
    with tempfile.TemporaryDirectory(prefix="nixscript-stress-") as tmp:
        root = Path(tmp)
        for case in CASES:
            ok, detail = run_case(binary, case, root)
            if ok:
                passed += 1
                print(f"PASS {case.name}")
            else:
                failures.append((case.name, detail))
                print(f"FAIL {case.name}: {detail}", file=sys.stderr)

    print(f"\nLanguage stress suite: {passed}/{len(CASES)} passed.")
    if failures:
        print("\nFailures:", file=sys.stderr)
        for name, detail in failures:
            print(f"- {name}: {detail}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

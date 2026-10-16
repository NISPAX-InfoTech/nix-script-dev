#!/usr/bin/env python3
"""Regression tests for NixScript function prototype/definition ordering."""

from __future__ import annotations

import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class Case:
    name: str
    source: str
    expected_stdout: str = ""
    expected_error: str = ""


CASES = [
    Case(
        "definition before main needs no prototype",
        """fn Add(a, b) { return a + b; }
nix main() { println(Add(10, 20)); }""",
        "30\n",
    ),
    Case(
        "pre-main prototype permits post-main definition",
        """fn Add(a, b);
nix main() { println(Add(10, 20)); }
fn Add(x, y) { return x + y; }""",
        "30\n",
    ),
    Case(
        "post-main definition requires pre-main prototype",
        """nix main() { println(late()); }
fn late() { return 1; }""",
        expected_error="must have a prototype before 'nix main()'",
    ),
    Case(
        "prototype after main is rejected",
        """nix main() { println("ok"); }
fn late();
fn late() { return 1; }""",
        expected_error="prototypes must be declared before 'nix main()'",
    ),
    Case(
        "post-main definition still requires fn",
        """fn late();
nix main() { println(late()); }
late() { return 1; }""",
        expected_error="Expected a top-level function declaration/definition",
    ),
    Case(
        "prototype and definition parameter counts must match",
        """fn blend(a, b);
nix main() { println("unreachable"); }
fn blend(x) { return x; }""",
        expected_error="declared with 2 parameter(s)",
    ),
    Case(
        "unused pre-main prototype is allowed",
        """fn notCalled(value);
nix main() { println("ok"); }""",
        "ok\n",
    ),
    Case(
        "called prototype without definition is rejected",
        """fn later();
nix main() { later(); }""",
        expected_error="declared but has no definition",
    ),
    Case(
        "duplicate function definitions remain rejected",
        """fn value() { return 1; }
nix main() { println(value()); }
fn value() { return 2; }""",
        expected_error="more than one definition",
    ),
    Case(
        "class method out-of-class definition remains valid",
        """class Meter {
public:
    int read();
};
nix main() {
    var meter = Meter();
    println(meter.read());
}
int Meter::read() { return 7; }""",
        "7\n",
    ),
]


def main() -> int:
    if len(sys.argv) != 2:
        print(f"usage: {Path(sys.argv[0]).name} PATH_TO_NIX_BINARY", file=sys.stderr)
        return 2

    binary = Path(sys.argv[1]).resolve()
    if not binary.is_file():
        print(f"interpreter not found: {binary}", file=sys.stderr)
        return 2

    passed = 0
    failures: list[tuple[str, str]] = []
    with tempfile.TemporaryDirectory(prefix="nixscript-function-rules-") as tmp:
        source_path = Path(tmp) / "case.ns"
        for case in CASES:
            source_path.write_text(case.source + "\n", encoding="utf-8")
            try:
                proc = subprocess.run(
                    [str(binary), str(source_path)],
                    text=True,
                    capture_output=True,
                    timeout=5,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                failures.append((case.name, "timed out after 5 seconds"))
                continue

            output = proc.stdout + proc.stderr
            if case.expected_error:
                if proc.returncode == 0:
                    failures.append((case.name, f"expected error containing {case.expected_error!r}, but program succeeded"))
                elif case.expected_error not in output:
                    failures.append((case.name, f"expected error containing {case.expected_error!r}, got {output!r}"))
                else:
                    passed += 1
            elif proc.returncode != 0:
                failures.append((case.name, f"exit={proc.returncode}; output={output!r}"))
            elif proc.stdout != case.expected_stdout:
                failures.append((case.name, f"expected stdout {case.expected_stdout!r}, got {proc.stdout!r}; stderr={proc.stderr!r}"))
            else:
                passed += 1

    for case in CASES:
        result = "PASS" if not any(failure[0] == case.name for failure in failures) else "FAIL"
        print(f"{result} {case.name}")
    print(f"\nFunction declaration rules: {passed}/{len(CASES)} passed.")
    for name, detail in failures:
        print(f"- {name}: {detail}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())

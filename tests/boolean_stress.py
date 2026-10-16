#!/usr/bin/env python3
"""Deterministic generated boolean/operator parser regression test."""

from __future__ import annotations

import random
import subprocess
import sys
import tempfile
from pathlib import Path

SEED = 20261016
CASE_COUNT = 180
MAX_DEPTH = 5
COMPARISONS = ("==", "!=", "<", "<=", ">", ">=")
LOGICAL = ("and", "or", "&&", "||")


def generate(rng: random.Random, depth: int) -> tuple:
    if depth <= 0 or rng.random() < 0.28:
        if rng.random() < 0.38:
            return ("bool", rng.choice((True, False)))
        return ("cmp", rng.randint(-9, 9), rng.choice(COMPARISONS), rng.randint(-9, 9))

    if rng.random() < 0.22:
        return ("not", rng.choice(("not", "!")), generate(rng, depth - 1))
    return (
        "logic",
        rng.choice(LOGICAL),
        generate(rng, depth - 1),
        generate(rng, depth - 1),
    )


def render(node: tuple) -> str:
    kind = node[0]
    if kind == "bool":
        return "true" if node[1] else "false"
    if kind == "cmp":
        return f"({node[1]} {node[2]} {node[3]})"
    if kind == "not":
        op = node[1]
        return f"{op} ({render(node[2])})" if op == "not" else f"!({render(node[2])})"
    if kind == "logic":
        return f"({render(node[2])} {node[1]} {render(node[3])})"
    raise ValueError(f"unknown generated node: {node!r}")


def evaluate(node: tuple) -> bool:
    kind = node[0]
    if kind == "bool":
        return node[1]
    if kind == "cmp":
        left, op, right = node[1], node[2], node[3]
        if op == "==":
            return left == right
        if op == "!=":
            return left != right
        if op == "<":
            return left < right
        if op == "<=":
            return left <= right
        if op == ">":
            return left > right
        return left >= right
    if kind == "not":
        return not evaluate(node[2])
    if kind == "logic":
        op, left, right = node[1], node[2], node[3]
        if op in ("and", "&&"):
            return evaluate(left) and evaluate(right)
        return evaluate(left) or evaluate(right)
    raise ValueError(f"unknown generated node: {node!r}")


def main() -> int:
    if len(sys.argv) > 2:
        print("Usage: boolean_stress.py [path/to/nix]", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1] if len(sys.argv) == 2 else "./build/nix").resolve()
    if not binary.is_file():
        print(f"Interpreter not found: {binary}", file=sys.stderr)
        return 2

    rng = random.Random(SEED)
    cases = [(generate(rng, MAX_DEPTH), None) for _ in range(CASE_COUNT)]
    cases = [(tree, evaluate(tree)) for tree, _ in cases]
    source = "nix main() {\n" + "".join(
        f"    println({render(tree)});\n" for tree, _ in cases
    ) + "}\n"

    with tempfile.TemporaryDirectory(prefix="nixscript-boolean-") as directory:
        script = Path(directory) / "generated.ns"
        script.write_text(source, encoding="utf-8")
        try:
            process = subprocess.run(
                [str(binary), str(script)],
                check=False,
                capture_output=True,
                text=True,
                timeout=20,
            )
        except subprocess.TimeoutExpired:
            print("FAIL: generated boolean stress program timed out", file=sys.stderr)
            return 1

    if process.returncode != 0:
        print("FAIL: interpreter rejected generated boolean program", file=sys.stderr)
        print(process.stderr, file=sys.stderr)
        return 1

    actual_lines = process.stdout.splitlines()
    if len(actual_lines) != len(cases):
        print(
            f"FAIL: expected {len(cases)} outputs, received {len(actual_lines)}",
            file=sys.stderr,
        )
        print(process.stdout, file=sys.stderr)
        return 1

    for index, ((tree, expected), actual) in enumerate(zip(cases, actual_lines), 1):
        wanted = "true" if expected else "false"
        if actual != wanted:
            print(
                f"FAIL case {index}: {render(tree)}; expected {wanted}, received {actual!r}",
                file=sys.stderr,
            )
            return 1

    print(f"PASS: {len(cases)} generated boolean expressions (seed={SEED})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

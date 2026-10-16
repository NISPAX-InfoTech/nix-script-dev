#!/usr/bin/env python3
"""Deterministic property-style stress test for arithmetic parsing and evaluation."""

from __future__ import annotations

import ast
import math
import random
import subprocess
import sys
import tempfile
from pathlib import Path

SEED = 20261016
CASE_COUNT = 160
MAX_DEPTH = 4


def make_expression(rng: random.Random, depth: int) -> str:
    if depth <= 0 or rng.random() < 0.24:
        sign = "-" if rng.random() < 0.18 else ""
        return sign + str(rng.randint(0, 9))

    left = make_expression(rng, depth - 1)
    right = make_expression(rng, depth - 1)
    op = rng.choice(("+", "-", "*", "/"))
    expression = f"{left} {op} {right}"
    if rng.random() < 0.62:
        expression = f"({expression})"
    return expression


def evaluate(node: ast.AST) -> float:
    if isinstance(node, ast.Expression):
        return evaluate(node.body)
    if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
        return float(node.value)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
        return -evaluate(node.operand)
    if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.UAdd):
        return evaluate(node.operand)
    if isinstance(node, ast.BinOp):
        left = evaluate(node.left)
        right = evaluate(node.right)
        if isinstance(node.op, ast.Add):
            return left + right
        if isinstance(node.op, ast.Sub):
            return left - right
        if isinstance(node.op, ast.Mult):
            return left * right
        if isinstance(node.op, ast.Div):
            if right == 0:
                raise ZeroDivisionError
            return left / right
    raise ValueError(f"unexpected arithmetic AST node: {ast.dump(node)}")


def main() -> int:
    if len(sys.argv) > 2:
        print("Usage: arithmetic_stress.py [path/to/nix]", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1] if len(sys.argv) == 2 else "./build/nix").resolve()
    if not binary.is_file():
        print(f"Interpreter not found: {binary}", file=sys.stderr)
        return 2

    rng = random.Random(SEED)
    cases: list[tuple[str, float]] = []
    attempts = 0
    while len(cases) < CASE_COUNT:
        attempts += 1
        if attempts > CASE_COUNT * 100:
            raise RuntimeError("could not generate enough safe arithmetic expressions")
        expression = make_expression(rng, MAX_DEPTH)
        try:
            expected = evaluate(ast.parse(expression, mode="eval"))
        except ZeroDivisionError:
            continue
        if not math.isfinite(expected):
            continue
        cases.append((expression, expected))

    source = "nix main() {\n" + "".join(
        f"    println({expression});\n" for expression, _ in cases
    ) + "}\n"

    with tempfile.TemporaryDirectory(prefix="nixscript-arithmetic-") as directory:
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
            print("FAIL: arithmetic stress program timed out", file=sys.stderr)
            return 1

    if process.returncode != 0:
        print("FAIL: interpreter rejected generated arithmetic program", file=sys.stderr)
        print(process.stderr, file=sys.stderr)
        return 1

    output_lines = process.stdout.splitlines()
    if len(output_lines) != len(cases):
        print(
            f"FAIL: expected {len(cases)} outputs, received {len(output_lines)}",
            file=sys.stderr,
        )
        print(process.stdout, file=sys.stderr)
        return 1

    for index, ((expression, expected), actual_text) in enumerate(zip(cases, output_lines), 1):
        try:
            actual = float(actual_text)
        except ValueError:
            print(
                f"FAIL case {index}: {expression} produced non-number {actual_text!r}",
                file=sys.stderr,
            )
            return 1
        if not math.isclose(actual, expected, rel_tol=2e-5, abs_tol=2e-5):
            print(
                f"FAIL case {index}: {expression}; expected {expected}, received {actual_text}",
                file=sys.stderr,
            )
            return 1

    print(f"PASS: {len(cases)} generated arithmetic expressions (seed={SEED})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

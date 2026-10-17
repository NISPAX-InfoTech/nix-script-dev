#!/usr/bin/env python3
"""Deterministic 1,200-case hardcore regression suite for Nix Script.

Case budget:
  400 generated arithmetic expressions, checked against Python's AST evaluator
  400 generated boolean expressions, checked against an independent evaluator
  200 arithmetic precedence/boundary expressions, independently evaluated
  200 malformed programs, which must be rejected without crashing or hanging

The generator is deterministic so every failure is reproducible.
"""
from __future__ import annotations

import ast
import math
import random
import subprocess
import sys
import tempfile
from pathlib import Path

SEED = 20261017
ARITHMETIC_CASES = 400
BOOLEAN_CASES = 400
BOUNDARY_CASES = 200
MALFORMED_CASES = 200
TIMEOUT_SECONDS = 20
OPS = ("+", "-", "*", "/")


def make_arithmetic(rng: random.Random, depth: int) -> str:
    if depth <= 0 or rng.random() < 0.25:
        return ("-" if rng.random() < 0.15 else "") + str(rng.randint(1, 9))
    left = make_arithmetic(rng, depth - 1)
    right = make_arithmetic(rng, depth - 1)
    op = rng.choice(OPS)
    if op == "/" and eval_ast(right) == 0:
        right = "1"
    expression = f"{left} {op} {right}"
    return f"({expression})" if rng.random() < 0.7 else expression


def eval_ast(expression: str) -> float:
    def visit(node: ast.AST) -> float:
        if isinstance(node, ast.Expression):
            return visit(node.body)
        if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
            return float(node.value)
        if isinstance(node, ast.UnaryOp) and isinstance(node.op, ast.USub):
            return -visit(node.operand)
        if isinstance(node, ast.BinOp):
            left, right = visit(node.left), visit(node.right)
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
        raise ValueError(f"unsupported generated expression: {ast.dump(node)}")
    return visit(ast.parse(expression, mode="eval"))


def make_boolean(rng: random.Random, depth: int) -> tuple:
    comparisons = ("==", "!=", "<", "<=", ">", ">=")
    if depth <= 0 or rng.random() < 0.28:
        if rng.random() < 0.25:
            return ("bool", rng.choice((True, False)))
        return ("cmp", rng.randint(-20, 20), rng.choice(comparisons), rng.randint(-20, 20))
    if rng.random() < 0.18:
        return ("not", make_boolean(rng, depth - 1))
    return ("logic", rng.choice(("and", "or", "&&", "||")),
            make_boolean(rng, depth - 1), make_boolean(rng, depth - 1))


def render_boolean(node: tuple) -> str:
    kind = node[0]
    if kind == "bool":
        return "true" if node[1] else "false"
    if kind == "cmp":
        return f"({node[1]} {node[2]} {node[3]})"
    if kind == "not":
        return f"not ({render_boolean(node[1])})"
    return f"({render_boolean(node[2])} {node[1]} {render_boolean(node[3])})"


def eval_boolean(node: tuple) -> bool:
    kind = node[0]
    if kind == "bool":
        return node[1]
    if kind == "cmp":
        left, op, right = node[1:]
        return {"==": left == right, "!=": left != right, "<": left < right,
                "<=": left <= right, ">": left > right, ">=": left >= right}[op]
    if kind == "not":
        return not eval_boolean(node[1])
    op, left, right = node[1:]
    if op in ("and", "&&"):
        return eval_boolean(left) and eval_boolean(right)
    return eval_boolean(left) or eval_boolean(right)


def make_boundary_expressions() -> list[tuple[str, float]]:
    # Explicitly exercise precedence, associativity, unary minus, nesting,
    # fractional division and zero-valued intermediate results.
    patterns = [
        "1 + 2 * 3", "(1 + 2) * 3", "9 - 4 - 2", "9 - (4 - 2)",
        "8 / 2 / 2", "8 / (2 / 2)", "-3 * 4 + 2", "(-3) * (4 + 2)",
        "7 + 8 / 4", "(7 + 8) / 4", "0 - 9", "0 * 9 + 4",
        "6 / 4", "1 / 4", "9 / 3 + 2", "9 / (3 + 2)",
        "2 * 3 * 4", "2 * (3 * 4)", "5 - 8", "-5 - 8",
        "3 + 4 * 2 / 2", "(3 + 4) * (2 / 2)",
    ]
    cases: list[tuple[str, float]] = []
    for i in range(BOUNDARY_CASES):
        expr = patterns[i % len(patterns)]
        # Vary constants deterministically while preserving expression shape.
        if i >= len(patterns):
            rng = random.Random(SEED + i)
            a, b, c = rng.randint(1, 9), rng.randint(1, 9), rng.randint(1, 9)
            expr = rng.choice([
                f"{a} + {b} * {c}",
                f"({a} + {b}) * {c}",
                f"{a} - ({b} - {c})",
                f"({a} * {b}) / {c}",
                f"-{a} + {b} * {c}",
                f"({a} + {b} * {c}) / {c}",
            ])
        cases.append((expr, eval_ast(expr)))
    return cases


def make_malformed_cases() -> list[tuple[str, str]]:
    cases = []
    # Keep the mutation controlled: each input is otherwise a valid program,
    # with an unknown character inserted before the closing main brace.
    invalid_chars = ("@", "#", "$", "?", "`", "^", "[", "]")
    for i in range(MALFORMED_CASES):
        value = i % 97
        invalid = invalid_chars[i % len(invalid_chars)]
        source = f"nix main() {{ println({value}); {invalid} }}"
        cases.append((f"malformed-{i + 1}", source))
    return cases


def run_valid_program(binary: Path, source: str, expected: list[tuple[str, object]],
                      label: str, numeric: bool) -> int:
    with tempfile.TemporaryDirectory(prefix="nixscript-hardcore-") as directory:
        script = Path(directory) / "generated.ns"
        script.write_text(source, encoding="utf-8")
        try:
            process = subprocess.run([str(binary), str(script)], capture_output=True,
                                     text=True, timeout=TIMEOUT_SECONDS, check=False)
        except subprocess.TimeoutExpired:
            print(f"FAIL: {label} program timed out", file=sys.stderr)
            return 1
    if process.returncode != 0:
        print(f"FAIL: {label} program exited {process.returncode}", file=sys.stderr)
        print(process.stderr, file=sys.stderr)
        return 1
    lines = process.stdout.splitlines()
    if len(lines) != len(expected):
        print(f"FAIL: {label}: expected {len(expected)} outputs, got {len(lines)}",
              file=sys.stderr)
        return 1
    for i, ((case, wanted), actual) in enumerate(zip(expected, lines), 1):
        if numeric:
            try:
                got = float(actual)
            except ValueError:
                print(f"FAIL: {label} case {i}: {case} produced {actual!r}", file=sys.stderr)
                return 1
            if not math.isfinite(got) or not math.isclose(got, float(wanted),
                                                           rel_tol=2e-5, abs_tol=2e-5):
                print(f"FAIL: {label} case {i}: {case}; expected {wanted}, got {actual}",
                      file=sys.stderr)
                return 1
        elif actual != wanted:
            print(f"FAIL: {label} case {i}: {case}; expected {wanted}, got {actual!r}",
                  file=sys.stderr)
            return 1
    print(f"PASS: {len(expected)} {label} cases")
    return 0


def main() -> int:
    if len(sys.argv) > 2:
        print("Usage: hardcore_stress.py [path/to/nix]", file=sys.stderr)
        return 2
    binary = Path(sys.argv[1] if len(sys.argv) == 2 else "./build/nix").resolve()
    if not binary.is_file():
        print(f"Interpreter not found: {binary}", file=sys.stderr)
        return 2

    rng = random.Random(SEED)
    arithmetic: list[tuple[str, float]] = []
    attempts = 0
    while len(arithmetic) < ARITHMETIC_CASES:
        attempts += 1
        if attempts > ARITHMETIC_CASES * 100:
            print("FAIL: couldn't generate enough safe arithmetic expressions", file=sys.stderr)
            return 1
        expr = make_arithmetic(rng, 4)
        try:
            value = eval_ast(expr)
        except ZeroDivisionError:
            continue
        if math.isfinite(value):
            arithmetic.append((expr, value))

    boundary = make_boundary_expressions()
    booleans = [(tree, "true" if eval_boolean(tree) else "false")
                for tree in (make_boolean(rng, 5) for _ in range(BOOLEAN_CASES))]
    arithmetic_all = arithmetic + boundary
    arithmetic_source = "nix main() {\n" + "".join(
        f"  println({expr});\n" for expr, _ in arithmetic_all) + "}\n"
    boolean_source = "nix main() {\n" + "".join(
        f"  println({render_boolean(tree)});\n" for tree, _ in booleans) + "}\n"

    if run_valid_program(binary, arithmetic_source, arithmetic_all,
                         "arithmetic/precedence", numeric=True):
        return 1
    if run_valid_program(binary, boolean_source,
                         [(render_boolean(tree), expected) for tree, expected in booleans],
                         "boolean", numeric=False):
        return 1

    malformed = make_malformed_cases()
    rejected = 0
    for index, (label, source) in enumerate(malformed, 1):
        with tempfile.TemporaryDirectory(prefix="nixscript-hardcore-invalid-") as directory:
            script = Path(directory) / "invalid.ns"
            script.write_text(source + "\n", encoding="utf-8")
            try:
                process = subprocess.run([str(binary), str(script)], capture_output=True,
                                         text=True, timeout=2, check=False)
            except subprocess.TimeoutExpired:
                print(f"FAIL: {label}: parser timed out", file=sys.stderr)
                return 1
        if process.returncode < 0:
            print(f"FAIL: {label}: process terminated by signal {-process.returncode}",
                  file=sys.stderr)
            print(process.stderr, file=sys.stderr)
            return 1
        if process.returncode == 0:
            print(f"FAIL: {label}: malformed source was accepted", file=sys.stderr)
            print(source, file=sys.stderr)
            return 1
        rejected += 1

    print(f"PASS: {rejected} malformed-source cases (rejected without crash/timeout)")
    total = len(arithmetic) + len(boundary) + len(booleans) + rejected
    if total != 1200:
        print(f"FAIL: internal case-count error: expected 1200, got {total}", file=sys.stderr)
        return 1
    print(f"PASS: HARDCORE SUITE {total}/1200 cases (seed={SEED})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

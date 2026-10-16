#!/usr/bin/env bash
# Broad language regression suite. Run from repository root after building:
#   bash tests/language_stress.sh [path/to/nix]
set -uo pipefail

BIN="${1:-./build/nix}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

passed=0
failed=0

run_file_ok() {
    local name="$1" source_file="$2" expected="$3" input="${4:-}"
    local actual="$TMP/$name.actual" wanted="$TMP/$name.expected" log="$TMP/$name.log"

    if ! printf '%s' "$input" | "$BIN" "$source_file" >"$actual" 2>"$log"; then
        echo "FAIL: $name (program returned an error)" >&2
        cat "$log" >&2
        failed=$((failed + 1))
        return
    fi

    printf '%s' "$expected" >"$wanted"
    if diff -u "$wanted" "$actual"; then
        echo "PASS: $name"
        passed=$((passed + 1))
    else
        echo "FAIL: $name (output mismatch)" >&2
        failed=$((failed + 1))
    fi
}

run_ok() {
    local name="$1" expected="$2" input="${3:-}"
    local source_file="$TMP/$name.ns"
    cat >"$source_file"
    run_file_ok "$name" "$source_file" "$expected" "$input"
}

run_file_error() {
    local name="$1" source_file="$2" expected_message="$3" input="${4:-}"
    local log="$TMP/$name.error.log"

    if printf '%s' "$input" | "$BIN" "$source_file" >"$log" 2>&1; then
        echo "FAIL: $name (expected a non-zero exit)" >&2
        cat "$log" >&2
        failed=$((failed + 1))
    elif grep -Fq "$expected_message" "$log"; then
        echo "PASS: $name"
        passed=$((passed + 1))
    else
        echo "FAIL: $name (expected diagnostic: $expected_message)" >&2
        cat "$log" >&2
        failed=$((failed + 1))
    fi
}

run_error() {
    local name="$1" expected_message="$2" input="${3:-}"
    local source_file="$TMP/$name.ns"
    cat >"$source_file"
    run_file_error "$name" "$source_file" "$expected_message" "$input"
}

echo "=== Arithmetic, values, and operator precedence ==="

run_ok arithmetic_precedence $'14\n20\n5\n2.5\n-2\n' <<'NS'
nix main() {
    println(2 + 3 * 4);
    println((2 + 3) * 4);
    println(10 - 2 - 3);
    println(5 / 2);
    println(-3 + 1);
}
NS

run_ok numeric_and_literal_formatting $'3\n0.75\ntrue\nfalse\nnull\n' <<'NS'
nix main() {
    println(3.0);
    println(0.25 + 0.5);
    println(true);
    println(false);
    println(null);
}
NS

run_ok explicit_null_is_a_value $'null\nnull compares equal\nnot truthy\n' <<'NS'
fn nothing() {
    return null;
}
nix main() {
    var maybe = null;
    println(maybe);
    if (maybe == nothing()) {
        println("null compares equal");
    }
    if (maybe) {
        println("bad");
    } else {
        println("not truthy");
    }
}
NS

run_ok string_concatenation_and_interpolation $'Hello, Nix!\nNixScript 42\nNixScript!\n' <<'NS'
nix main() {
    var language = "Nix";
    var version = 42;
    print("Hello, ");
    println("{$language}!");
    println(language + "Script " + version);
    println("{$language}Script!");
}
NS

run_ok string_escapes_and_exact_printing $'A\tB\nLine one\nLine two\nend' <<'NS'
nix main() {
    print("A\tB\n");
    print("Line one\nLine two\n");
    print("end");
}
NS

echo "=== Variables, scopes, and mutation ==="

run_ok inferred_variables_and_increment $'10\n11\n10\nChanged\n' <<'NS'
nix main() {
    var count;
    count = 10;
    println(count);
    count++;
    println(count);
    count--;
    println(count);
    var label = "Initial";
    label = "Changed";
    println(label);
}
NS

run_ok local_block_scope_and_outer_assignment $'9\n5\n' <<'NS'
nix main() {
    var total = 4;
    if (true) {
        var local = 5;
        total = total + local;
    }
    println(total);
    println(5);
}
NS

run_ok null_literal_and_boolean_conditions $'not null\nzero is false\nnegative is true\nempty is false\n' <<'NS'
nix main() {
    if (null) {
        println("bad");
    } else {
        println("not null");
    }
    if (0) {
        println("bad");
    } else {
        println("zero is false");
    }
    if (-1) {
        println("negative is true");
    }
    if ("") {
        println("bad");
    } else {
        println("empty is false");
    }
}
NS

echo "=== Comparisons, logic, and branching ==="

run_ok relational_and_equality_operators $'true\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\ntrue\n' <<'NS'
nix main() {
    println(4 == 4);
    println(4 != 5);
    println(2 < 3);
    println(2 <= 2);
    println(3 > 2);
    println(3 >= 3);
    println("a" < "b");
    println("same" == "same");
    println("x" != "y");
}
NS

run_ok word_and_symbol_logical_operators $'true\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\ntrue\n' <<'NS'
nix main() {
    println(true and true);
    println(true and false);
    println(true or false);
    println(false or false);
    println(!false);
    println(!true);
    println(not false);
    println(true && (false || true));
}
NS

run_ok logical_short_circuiting $'false\ntrue\nfalse\ntrue\n' <<'NS'
nix main() {
    println(false and missing_function());
    println(true or another_missing_function());
    println(false && missing_function());
    println(true || another_missing_function());
}
NS

run_ok if_elif_else_if_and_else $'first\nsecond\nfallback\n' <<'NS'
nix main() {
    if (true) {
        println("first");
    } elif (true) {
        println("bad");
    } else {
        println("bad");
    }

    if (false) {
        println("bad");
    } else if (true) {
        println("second");
    } else {
        println("bad");
    }

    if (false) {
        println("bad");
    } elif (false) {
        println("bad");
    } else {
        println("fallback");
    }
}
NS

echo "=== Loops ==="

run_ok repeat_and_for_loops $'4\n0\n1\n2\n3\n4\n3\n2\n1\n' <<'NS'
nix main() {
    var repeats = 0;
    repeat(4) {
        repeats = repeats + 1;
    }
    println(repeats);

    for (var i = 0; i < 5; i++) {
        print(i);
        if (i < 4) {
            print("\n");
        }
    }
    println();

    for (var j = 3; j > 0; j--) {
        println(j);
    }
}
NS

run_ok empty_repeat_and_nested_loops $'done\n' <<'NS'
nix main() {
    repeat(0) {
        println("bad");
    }
    var count = 0;
    repeat(3) {
        repeat(2) {
            count = count + 1;
        }
    }
    if (count == 6) {
        println("done");
    }
}
NS

echo "=== Functions and return values ==="

run_ok calls_recursion_and_null_returns $'5\n15\n120\nfinished\nnull\n' <<'NS'
fn identity(value) {
    return value;
}
fn sum3(a, b, c) {
    return a + b + c;
}
fn factorial(n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}
fn no_result() {
    println("finished");
}
nix main() {
    println(identity(5));
    println(sum3(identity(3), 4, 8));
    println(factorial(5));
    println(no_result());
}
NS

run_ok function_local_scope_and_early_return $'inner\n7\nouter\n' <<'NS'
fn choose(value) {
    var word = "outer";
    if (value > 0) {
        var word = "inner";
        println(word);
        return value + 1;
    }
    return 0;
}
nix main() {
    var word = "outer";
    println(choose(6));
    println(word);
}
NS

run_ok default_function_fallthrough $'null\n' <<'NS'
fn implicit_null() {
    var x = 1;
}
nix main() {
    println(implicit_null());
}
NS

echo "=== Built-in math and string functions ==="

run_ok math_builtins $'9\n9\n8\n2.5\n2\n3\n3\n' <<'NS'
nix main() {
    println(sqrt(81));
    println(sq(-3));
    println(pow(2, 3));
    println(abs(-2.5));
    println(floor(2.8));
    println(ceil(2.2));
    println(round(2.5));
}
NS

run_ok string_case_and_predicate_builtins $'NIX SCRIPT 3\nnix script 3\n1\n0\n1\n0\n0\n1\n0\n' <<'NS'
nix main() {
    println(toUpper("Nix Script 3"));
    println(toLower("NIX SCRIPT 3"));
    println(isUpper("ABC123"));
    println(isUpper("AbC"));
    println(isLower("abc"));
    println(isLower("Abc"));
    println(isUpper("123"));
    println(isSpace(" \t"));
    println(isSpace(""));
}
NS

echo "=== Input ==="

run_ok string_input_and_prompt $'Name: Ada Lovelace\n' $'Ada Lovelace\n' <<'NS'
nix main() {
    var name = uin("Name: ");
    println(name);
}
NS

run_ok typed_integer_input_truncates $'Age: 17\n' $'17.9\n' <<'NS'
nix main() {
    var age = uin("Age: ", int);
    println(age);
}
NS

run_ok typed_float_input $'Value: 2.75\n' $'2.75\n' <<'NS'
nix main() {
    var value = uin("Value: ", float);
    println(value);
}
NS

run_ok typed_boolean_input $'Ready: true\n' $'true\n' <<'NS'
nix main() {
    var ready = uin("Ready: ", bool);
    println(ready);
}
NS

run_ok input_into_existing_variables_preserves_category $'18.5\nnew name\nfalse\n' $'18.5\nnew name\n0\n' <<'NS'
nix main() {
    var number = 7;
    var name = "old";
    var ready = true;
    uin(number);
    uin(name);
    uin(ready);
    println(number);
    println(name);
    println(ready);
}
NS

echo "=== Classes, methods, inheritance, and access control ==="

run_ok object_identity_and_aliasing $'false\ntrue\n' <<'NS'
class Box {
public:
    int value = 1;
};
nix main() {
    var first = Box();
    var second = Box();
    var alias = first;
    println(first == second);
    println(first == alias);
}
NS

run_ok objects_constructors_methods_and_fields $'Point object\n30\n15\n15\n' <<'NS'
class Point {
public:
    int x = 0;
    int y = 0;
    Point(int startX, int startY) {
        this.x = startX;
        this.y = startY;
    }
    int total() {
        return x + y;
    }
    int shift(int dx) {
        this.x = this.x + dx;
        return this.x;
    }
};

nix main() {
    var point = Point(10, 20);
    println(point);
    println(point.total());
    println(point.shift(5));
    println(point.x);
}
NS

run_ok protected_member_access_from_subclass $'9\n9\n' <<'NS'
class Parent {
protected:
    int secret = 9;
public:
    int revealBase() {
        return secret;
    }
};
class Child : Parent {
public:
    int revealChild() {
        return this.secret;
    }
};
nix main() {
    var child = Child();
    println(child.revealChild());
    println(child.revealBase());
}
NS

run_ok static_fields_methods_and_multiple_instances $'2\n' <<'NS'
class Marker {
public:
    static int total = 0;
    Marker() {
        Marker::total = Marker::total + 1;
    }
    static int count() {
        return Marker::total;
    }
};
nix main() {
    var one = Marker();
    var two = Marker();
    println(Marker::count());
}
NS

run_ok automatic_class_field_defaults $'0\n[]\nfalse\n' <<'NS'
class Defaults {
public:
    int count;
    string label;
    bool active;
};
nix main() {
    var defaults = Defaults();
    println(defaults.count);
    print("[");
    print(defaults.label);
    println("]");
    println(defaults.active);
}
NS

run_ok destructor_runs_at_block_scope_exit $'inside\ndestroyed\nafter\n' <<'NS'
class Notice {
public:
    ~Notice() {
        println("destroyed");
    }
};
nix main() {
    if (true) {
        var item = Notice();
        println("inside");
    }
    println("after");
}
NS

echo "=== Source import loader ==="

mkdir -p "$TMP/import-comments/lib"
cat >"$TMP/import-comments/lib/calc.ns" <<'NS'
fn imported_answer() {
    return 40 + 2;
}
NS
cat >"$TMP/import-comments/main.ns" <<'NS'
use calc; // trailing comments should not disable an import
nix main() {
    println(imported_answer());
}
NS
NIXSCRIPT_LIB_PATH="$TMP/nonexistent-installed-root" run_file_ok import_with_trailing_comment "$TMP/import-comments/main.ns" $'42\n'

mkdir -p "$TMP/nested-import/lib"
cat >"$TMP/nested-import/lib/inner.ns" <<'NS'
fn nested_value() {
    return 7;
}
NS
cat >"$TMP/nested-import/lib/outer.ns" <<'NS'
use inner;
fn outer_value() {
    return nested_value() * 6;
}
NS
cat >"$TMP/nested-import/main.ns" <<'NS'
use outer;
use inner;
nix main() {
    println(outer_value());
    println(nested_value());
}
NS
NIXSCRIPT_LIB_PATH="$TMP/nonexistent-installed-root" run_file_ok nested_import_and_deduplication "$TMP/nested-import/main.ns" $'42\n7\n'

mkdir -p "$TMP/libraries-folder/libraries"
cat >"$TMP/libraries-folder/libraries/tools.ns" <<'NS'
fn tool_answer() {
    return 6 * 7;
}
NS
cat >"$TMP/libraries-folder/main.ns" <<'NS'
// comments before imports remain in the import section

use tools;
nix main() {
    println(tool_answer());
}
NS
NIXSCRIPT_LIB_PATH="$TMP/nonexistent-installed-root" run_file_ok libraries_directory_resolution "$TMP/libraries-folder/main.ns" $'42\n'

mkdir -p "$TMP/adjacent-import"
cat >"$TMP/adjacent-import/adjacent.ns" <<'NS'
fn adjacent_answer() {
    return 84 / 2;
}
NS
cat >"$TMP/adjacent-import/main.ns" <<'NS'
use adjacent;
nix main() {
    println(adjacent_answer());
}
NS
NIXSCRIPT_LIB_PATH="$TMP/nonexistent-installed-root" run_file_ok adjacent_source_directory_fallback "$TMP/adjacent-import/main.ns" $'42\n'

echo "=== Expected diagnostics and invalid programs ==="

run_error uninitialized_variable "has not been initialized" <<'NS'
nix main() {
    var pending;
    println(pending);
}
NS

run_error unknown_variable "Unknown variable 'missing'" <<'NS'
nix main() {
    println(missing);
}
NS

run_error unknown_function "Unknown function 'missing'" <<'NS'
nix main() {
    missing();
}
NS

run_error division_by_zero "Division by zero" <<'NS'
nix main() {
    println(12 / 0);
}
NS

run_error sqrt_negative "sqrt() requires a non-negative number" <<'NS'
nix main() {
    println(sqrt(-1));
}
NS

run_error builtin_wrong_argument_count "sqrt() expects 1 argument(s)" <<'NS'
nix main() {
    println(sqrt());
}
NS

run_error builtin_wrong_argument_type "sqrt() expects numeric arguments" <<'NS'
nix main() {
    println(sqrt("not a number"));
}
NS

run_error incompatible_arithmetic "Arithmetic operators require numbers" <<'NS'
nix main() {
    println("left" - "right");
}
NS

run_error incompatible_comparison "Comparison requires two numbers or two strings" <<'NS'
nix main() {
    println(1 < "2");
}
NS

run_error private_constructor_access "not accessible here" <<'NS'
class Locked {
private:
    Locked() {
    }
};
nix main() {
    var object = Locked();
    println(object);
}
NS

run_error duplicate_method_parameters "appears more than once" <<'NS'
class DuplicateParameters {
public:
    int combine(value, value) {
        return value;
    }
};
nix main() {
    println("unreachable");
}
NS

run_error duplicate_class_member "declared more than once" <<'NS'
class DuplicateMember {
public:
    int value = 1;
    int value = 2;
};
nix main() {
    println("unreachable");
}
NS

run_error mismatched_out_of_class_method_signature "definition has 1 parameter(s)" <<'NS'
class Calculator {
public:
    int combine(left, right);
};
nix main() {
    var calculator = Calculator();
    println(calculator.combine(1, 2));
}
int Calculator::combine(only) {
    return only;
}
NS

run_error duplicate_out_of_class_method_definition "more than one definition" <<'NS'
class Calculator {
public:
    int value();
};
nix main() {
    var calculator = Calculator();
    println(calculator.value());
}
int Calculator::value() {
    return 1;
}
int Calculator::value() {
    return 2;
}
NS

run_error typed_function_parameters_are_rejected "function parameters use inferred values" <<'NS'
fn typed(int value) {
    return value;
}
nix main() {
    println("unreachable");
}
NS

run_error class_method_without_definition "declared but has no definition" <<'NS'
class Incomplete {
public:
    int calculate(value);
};
nix main() {
    var object = Incomplete();
    println(object.calculate(2));
}
NS

run_error static_method_called_through_instance "Static methods must be called through ClassName::method()" <<'NS'
class StaticOnly {
public:
    static int get() {
        return 1;
    }
};
nix main() {
    var object = StaticOnly();
    println(object.get());
}
NS

run_error instance_field_called_through_class "is not static" <<'NS'
class InstanceOnly {
public:
    int value = 4;
};
nix main() {
    println(InstanceOnly::value);
}
NS

run_error implicit_private_access "not accessible here" <<'NS'
class HiddenByDefault {
    int secret = 4;
};
nix main() {
    var object = HiddenByDefault();
    println(object.secret);
}
NS

run_error wrong_function_arity "expects 2 argument(s), received 1" <<'NS'
fn add(a, b) {
    return a + b;
}
nix main() {
    println(add(1));
}
NS

run_error duplicate_function_parameter "appears more than once" <<'NS'
fn duplicated(value, value) {
    return value;
}
nix main() {
    println("unreachable");
}
NS

run_error reserved_function_name "is reserved" <<'NS'
fn print(value) {
    return value;
}
nix main() {
    println("unreachable");
}
NS

run_error return_outside_function "return' can only be used inside a function or method" <<'NS'
nix main() {
    return 3;
}
NS

run_error invalid_repeat_count "repeat() expects a non-negative whole number" <<'NS'
nix main() {
    repeat(-1) {
        println("unreachable");
    }
}
NS

run_error fractional_repeat_count "repeat() expects a non-negative whole number" <<'NS'
nix main() {
    repeat(1.5) {
        println("unreachable");
    }
}
NS

run_error invalid_boolean_input "Expected true/false or 1/0 for bool input" "sometimes"$'\n' <<'NS'
nix main() {
    println(uin("Ready: ", bool));
}
NS

run_error invalid_numeric_input "Invalid numeric input" "seventeen"$'\n' <<'NS'
nix main() {
    println(uin("Age: ", int));
}
NS

run_error private_field_access "not accessible here" <<'NS'
class Vault {
private:
    int code = 123;
};
nix main() {
    var vault = Vault();
    println(vault.code);
}
NS

run_error private_method_access "not accessible here" <<'NS'
class Vault {
private:
    int code() {
        return 123;
    }
};
nix main() {
    var vault = Vault();
    println(vault.code());
}
NS

run_error constructor_arity "expects 1 argument(s), received 0" <<'NS'
class NeedsValue {
public:
    NeedsValue(value) {
        this.value = value;
    }
};
nix main() {
    var value = NeedsValue();
    println(value);
}
NS

run_error unknown_base_class "Unknown base class 'MissingBase'" <<'NS'
class Child : MissingBase {
public:
    int value = 1;
};
nix main() {
    println("unreachable");
}
NS

run_error malformed_expression "Expected an expression" <<'NS'
nix main() {
    var value = ;
}
NS

run_error missing_statement_semicolon "Expected ';' after print" <<'NS'
nix main() {
    println("missing semicolon")
}
NS

run_error unexpected_character "Unexpected character: '@'" <<'NS'
nix main() {
    println(@);
}
NS

run_error unterminated_string "Unterminated string" <<'NS'
nix main() {
    println("unterminated);
}
NS

run_error missing_entry_point "Every executable NixScript file must contain 'nix main()'" <<'NS'
fn helper() {
    return 1;
}
NS

run_error class_requires_trailing_semicolon "Expected ';' after class declaration" <<'NS'
class MissingSemicolon {
public:
    int value = 1;
}
nix main() {
    println("unreachable");
}
NS

run_error duplicate_class_name "already declared" <<'NS'
class Same {
public:
    int value = 1;
};
class Same {
public:
    int value = 2;
};
nix main() {
    println("unreachable");
}
NS

run_error unresolved_prototype "declared but has no definition" <<'NS'
fn declared_only(value);
nix main() {
    println(declared_only(3));
}
NS

printf '\nLanguage stress suite: %d passed, %d failed\n' "$passed" "$failed"
if (( failed > 0 )); then
    exit 1
fi

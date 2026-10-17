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

run_ok numeric_edge_semantics $'false\ntrue\n-6\n2\n' <<'NS'
nix main() {
    println(true == 1);
    println(null == null);
    println(-2 * 3);
    println(8 / 4);
}
NS

run_ok escaped_quotes_backslashes_and_literal_braces $'say "hi"\npath\\folder\n{literal}\n' <<'NS'
nix main() {
    println("say \"hi\"");
    println("path\\folder");
    println("{literal}");
}
NS

run_ok block_shadowing_and_local_lifetime $'outer\ninner\nouter\n' <<'NS'
nix main() {
    var label = "outer";
    println(label);
    if (true) {
        var label = "inner";
        println(label);
    }
    println(label);
}
NS

run_ok bare_return_stops_function $'before\n' <<'NS'
fn stop_early() {
    println("before");
    return;
    println("bad");
}
nix main() {
    stop_early();
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

run_ok typed_declaration_defaults $'0\n[]\nfalse\n' <<'NS'
nix main() {
    int count;
    string label;
    bool active;
    println(count);
    print("[");
    print(label);
    println("]");
    println(active);
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

run_ok for_loop_optional_initializer_and_increment $'0\n1\n2\n' <<'NS'
nix main() {
    var i = 0;
    for (; i < 3; ) {
        println(i);
        i++;
    }
}
NS

run_ok repeated_compatible_prototypes_and_parameter_renaming $'42\n' <<'NS'
fn combine(left, right);
fn combine(first, second);
nix main() {
    println(combine(20, 22));
}
fn combine(one, two) {
    return one + two;
}
NS

run_ok forward_call_from_function_to_later_definition $'later\n' <<'NS'
fn first() {
    return second();
}
fn second() {
    return "later";
}
nix main() {
    println(first());
}
NS

run_ok overloaded_functions_by_arity_and_runtime_type $'text\ninteger\nfraction\ngeneric\n3\n6\n' <<'NS'
fn label(value) {
    return "generic";
}
fn label(string value) {
    return "text";
}
fn label(int value) {
    return "integer";
}
fn label(float value) {
    return "fraction";
}
fn combine(first, second) {
    return first + second;
}
fn combine(first, second, third) {
    return first + second + third;
}
nix main() {
    println(label("hello"));
    println(label(5));
    println(label(5.25));
    println(label(true));
    println(combine(1, 2));
    println(combine(1, 2, 3));
}
NS

run_ok overloaded_function_prototypes_and_late_definitions $'string:hi\nnumber:7\n' <<'NS'
fn display(string value);
fn display(int value);
nix main() {
    display("hi");
    display(7);
}
fn display(string text) {
    println("string:" + text);
}
fn display(int count) {
    println("number:" + count);
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

run_ok function_arguments_evaluate_left_to_right $'12\n' <<'NS'
fn next_value() {
    counter = counter + 1;
    return counter;
}
fn pair(left, right) {
    return left * 10 + right;
}
nix main() {
    var counter = 0;
    println(pair(next_value(), next_value()));
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

run_ok overloaded_methods_by_type_and_arity $'string:hello\ninteger:8\npair:hi!\n' <<'NS'
class Printer {
    fn show(string value) {
        return "string:" + value;
    }
    fn show(int value) {
        return "integer:" + value;
    }
    fn show(first, second) {
        return "pair:" + first + second;
    }
};
nix main() {
    var p = Printer();
    println(p.show("hello"));
    println(p.show(8));
    println(p.show("hi", "!"));
}
NS

run_ok overloaded_constructors $'default\ninteger:9\nstring:hey\n' <<'NS'
class Choice {
    var description;
    Choice() {
        this.description = "default";
    }
    Choice(int value) {
        this.description = "integer:" + value;
    }
    Choice(string value) {
        this.description = "string:" + value;
    }
};
nix main() {
    var empty = Choice();
    var number = Choice(9);
    var text = Choice("hey");
    println(empty.description);
    println(number.description);
    println(text.description);
}
NS

run_error constructor_overload_no_match "No overload of constructor 'Choice' accepts 1 argument(s)" <<'NS'
class Choice {
    Choice(int value, int other) {
    }
};
nix main() {
    var bad = Choice("wrong type");
}
NS

run_ok multiple_inheritance_and_base_constructor_arguments $'left:4\nright:hi\nown:9\n' <<'NS'
class Left {
    var left;
    Left(int value) {
        this.left = value;
    }
    fn readLeft() {
        return "left:" + left;
    }
};
class Right {
    var right;
    Right(string value) {
        this.right = value;
    }
    fn readRight() {
        return "right:" + right;
    }
};
class Combined : Left, Right {
    var own;
    Combined(int leftValue, string rightValue, int ownValue)
        : Left(leftValue), Right(rightValue) {
        this.own = ownValue;
    }
    fn readOwn() {
        return "own:" + own;
    }
};
nix main() {
    var c = Combined(4, "hi", 9);
    println(c.readLeft());
    println(c.readRight());
    println(c.readOwn());
}
NS

run_ok shared_diamond_base_constructed_once $'root\nleft\nright\ndiamond\n' <<'NS'
class Root {
    Root() {
        println("root");
    }
};
class Left : Root {
    Left() {
        println("left");
    }
};
class Right : Root {
    Right() {
        println("right");
    }
};
class Diamond : Left, Right {
    Diamond() {
        println("diamond");
    }
};
nix main() {
    var d = Diamond();
}
NS

run_error ambiguous_multiple_inheritance_method "Ambiguous inherited overload of method 'name'" <<'NS'
class First {
    fn name() {
        return "first";
    }
};
class Second {
    fn name() {
        return "second";
    }
};
class Both : First, Second {
};
nix main() {
    var b = Both();
    println(b.name());
}
NS

run_error ambiguous_multiple_inheritance_field "Ambiguous inherited member 'value'" <<'NS'
class FirstField {
    var value = 1;
};
class SecondField {
    var value = 2;
};
class BothFields : FirstField, SecondField {
};
nix main() {
    var b = BothFields();
    println(b.value);
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

run_ok contained_object_destructor_order $'before\ncontainer destroyed\ncomponent destroyed\n' <<'NS'
class Component {
public:
    ~Component() {
        println("component destroyed");
    }
};
class Container {
public:
    var component = Component();
    ~Container() {
        println("container destroyed");
    }
};
nix main() {
    var container = Container();
    println("before");
}
NS

run_ok contained_object_alias_remains_alive_when_referenced_outside $'container destroyed\nstill alive\ncomponent destroyed\n' <<'NS'
class SharedComponent {
public:
    ~SharedComponent() {
        println("component destroyed");
    }
};
class OwnsComponent {
public:
    var component = SharedComponent();
    ~OwnsComponent() {
        println("container destroyed");
    }
};
nix main() {
    var component;
    if (true) {
        var owner = OwnsComponent();
        component = owner.component;
    }
    println("still alive");
}
NS

run_ok destructor_chain_runs_through_all_base_classes $'child destroyed\nparent destroyed\ngrandparent destroyed\n' <<'NS'
class Grandparent {
public:
    ~Grandparent() {
        println("grandparent destroyed");
    }
};
class Parent : Grandparent {
public:
    ~Parent() {
        println("parent destroyed");
    }
};
class Child : Parent {
public:
    ~Child() {
        println("child destroyed");
    }
};
nix main() {
    var child = Child();
}
NS

run_ok returned_object_survives_nested_scope_cleanup $'created\ncaller owns it\ndestroyed\n' <<'NS'
class ReturnLife {
public:
    ~ReturnLife() {
        println("destroyed");
    }
};
fn makeLife() {
    if (true) {
        var object = ReturnLife();
        println("created");
        return object;
    }
}
nix main() {
    var object = makeLife();
    println("caller owns it");
}
NS

run_ok returned_object_survives_loop_scope_cleanup $'created\ncaller owns it\ndestroyed\n' <<'NS'
class LoopLife {
public:
    ~LoopLife() {
        println("destroyed");
    }
};
fn makeLoopLife() {
    for (var i = 0; i < 1; i++) {
        var object = LoopLife();
        println("created");
        return object;
    }
}
nix main() {
    var object = makeLoopLife();
    println("caller owns it");
}
NS

run_ok object_alias_keeps_instance_alive_until_last_scope $'inside\nstill alive\ndestroyed\n' <<'NS'
class Life {
public:
    ~Life() {
        println("destroyed");
    }
};
nix main() {
    var item = Life();
    if (true) {
        var innerAlias = item;
        println("inside");
    }
    println("still alive");
}
NS

run_ok private_method_can_be_called_inside_class $'secret\n' <<'NS'
class Safe {
private:
    int reveal() {
        return 42;
    }
public:
    int run() {
        println("secret");
        return this.reveal();
    }
};
nix main() {
    var safe = Safe();
    safe.run();
}
NS

run_ok class_method_can_call_another_method $'42\n' <<'NS'
class Calculator {
public:
    int doubleValue(value) {
        return value * 2;
    }
    int calc(value) {
        return this.doubleValue(value + 10) + 2;
    }
};
nix main() {
    var calculator = Calculator();
    println(calculator.calc(10));
}
NS

run_ok class_field_initializers_use_prior_fields $'8\n' <<'NS'
class InitOrder {
public:
    int base = 4;
    int doubled = base * 2;
};
nix main() {
    var value = InitOrder();
    println(value.doubled);
}
NS

run_ok constructor_chain_runs_base_to_derived $'grandparent constructor\nparent constructor\nchild constructor\n' <<'NS'
class Grandparent {
public:
    Grandparent() {
        println("grandparent constructor");
    }
};
class Parent : Grandparent {
public:
    Parent() {
        println("parent constructor");
    }
};
class Child : Parent {
public:
    Child() {
        println("child constructor");
    }
};
nix main() {
    var child = Child();
}
NS

run_ok derived_constructor_can_call_protected_base_constructor $'base constructor\nderived constructor\n' <<'NS'
class ProtectedBase {
protected:
    ProtectedBase() {
        println("base constructor");
    }
};
class Derived : ProtectedBase {
public:
    Derived() {
        println("derived constructor");
    }
};
nix main() {
    var child = Derived();
}
NS

run_ok derived_method_overrides_base_method $'2\n' <<'NS'
class Parent {
public:
    int value() {
        return 1;
    }
};
class Child : Parent {
public:
    int value() {
        return 2;
    }
};
nix main() {
    var child = Child();
    println(child.value());
}
NS

run_ok bound_method_identity $'true\nfalse\n' <<'NS'
class Actions {
public:
    int go() {
        return 1;
    }
};
nix main() {
    var first = Actions();
    var second = Actions();
    println(first.go == first.go);
    println(first.go == second.go);
}
NS

run_ok explicit_fn_class_method_prototype_and_definition $'null × _×Hey, x = 6 now!' <<'NS'
class Test {
    var x;
    Test() {
        print("${x} × _×");
    }
    fn Put();
};

fn Test::Put() {
    x = 6;
    print("Hey, x = ", x, " now!");
}

nix main() {
    Test t;
    t.Put();
}
NS

run_ok polymorphic_dispatch_through_base_typed_variable $'dog\n' <<'NS'
class Animal {
public:
    fn speak() {
        println("animal");
    }
};
class Dog : Animal {
public:
    fn speak() {
        println("dog");
    }
};
nix main() {
    Animal pet = Dog();
    pet.speak();
}
NS

run_ok abstract_class_concrete_implementation $'dog\n' <<'NS'
abstract class Animal {
    abstract fn speak();
};
class Dog : Animal {
    fn speak() {
        println("dog");
    }
};
nix main() {
    Animal pet = Dog();
    pet.speak();
}
NS

run_error cannot_instantiate_abstract_class "Cannot instantiate abstract class 'Animal'" <<'NS'
abstract class Animal {
    abstract fn speak();
};
nix main() {
    var pet = Animal();
}
NS

run_error concrete_class_missing_abstract_method "must implement abstract method 'speak'" <<'NS'
abstract class Animal {
    abstract fn speak();
};
class SilentDog : Animal {
};
nix main() {
    var pet = SilentDog();
}
NS

run_ok abstract_class_can_extend_abstract_class $'bird\n' <<'NS'
abstract class Animal {
    abstract fn speak();
};
abstract class Bird : Animal {
};
class Sparrow : Bird {
    fn speak() {
        println("bird");
    }
};
nix main() {
    var bird = Sparrow();
    bird.speak();
}
NS

run_ok out_of_class_constructor_method_and_destructor $'stored\nitem destroyed\n' <<'NS'
class Item {
public:
    var value;
    Item(value);
    string read();
    ~Item();
};
Item::Item(initial) {
    this.value = initial;
}
string Item::read() {
    return this.value;
}
Item::~Item() {
    println("item destroyed");
}
nix main() {
    var item = Item("stored");
    println(item.read());
}
NS

run_ok static_field_initializer_and_scope_lookup $'5\n10\n' <<'NS'
class Counts {
public:
    static int initial = 2 + 3;
    static int doubleInitial() {
        return Counts::initial * 2;
    }
};
nix main() {
    println(Counts::initial);
    println(Counts::doubleInitial());
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

run_error duplicate_variable_same_scope "already declared in this scope" <<'NS'
nix main() {
    var value = 1;
    var value = 2;
    println(value);
}
NS

run_error unknown_assignment_target "Unknown variable 'missing'" <<'NS'
nix main() {
    missing = 3;
}
NS

run_error method_call_on_field "This expression is not callable" <<'NS'
class Data {
public:
    int value = 42;
};
nix main() {
    var data = Data();
    data.value();
}
NS

run_error malformed_interpolation "Unclosed interpolation" <<'NS'
nix main() {
    var value = 1;
    println("value={$value");
}
NS

run_error logical_operator_single_ampersand "Use '&&' for a logical AND" <<'NS'
nix main() {
    if (true & false) {
        println("unreachable");
    }
}
NS

run_error logical_operator_single_pipe "Use '||' for a logical OR" <<'NS'
nix main() {
    if (true | false) {
        println("unreachable");
    }
}
NS

run_error second_entry_point_is_rejected "Expected a top-level function declaration/definition or qualified class method definition after 'nix main()'" <<'NS'
nix main() {
    println("first");
}
nix main() {
    println("second");
}
NS

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

run_error derived_constructor_cannot_call_private_base_constructor "not accessible here" <<'NS'
class PrivateBase {
private:
    PrivateBase() {
    }
};
class Derived : PrivateBase {
public:
    Derived() {
    }
};
nix main() {
    var child = Derived();
}
NS

run_error protected_field_access_from_outside "not accessible here" <<'NS'
class ProtectedVault {
protected:
    int code = 123;
};
nix main() {
    var vault = ProtectedVault();
    println(vault.code);
}
NS

run_error too_many_function_arguments "expects 1 argument(s), received 2" <<'NS'
fn one(value) {
    return value;
}
nix main() {
    println(one(1, 2));
}
NS

run_error method_wrong_argument_count "expects 1 argument(s), received 0" <<'NS'
class NeedsArgument {
public:
    int method(value) {
        return value;
    }
};
nix main() {
    var object = NeedsArgument();
    println(object.method());
}
NS

run_error duplicate_constructor_declarations "declared more than once" <<'NS'
class DoubleConstruct {
public:
    DoubleConstruct() {
    }
    DoubleConstruct(value) {
    }
};
nix main() {
    println("unreachable");
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

run_error mismatched_out_of_class_method_signature "the definition has 1 parameter(s)" <<'NS'
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

run_error ambiguous_typed_function_overload "Ambiguous overload of function 'choose'" <<'NS'
fn choose(int value) {
    return "int";
}
fn choose(long value) {
    return "long";
}
nix main() {
    println(choose(7));
}
NS

run_error no_function_overload_matches_type "No matching overload of function 'pick'" <<'NS'
fn pick(int value) {
    return value;
}
fn pick(bool value) {
    return value;
}
nix main() {
    println(pick("not a number"));
}
NS

run_error wrong_overload_arity "No overload of function 'pick' accepts 2 argument(s)" <<'NS'
fn pick(value) {
    return value;
}
nix main() {
    println(pick(1, 2));
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

run_ok implicit_public_access $'4\n' <<'NS'
class PublicByDefault {
    int value = 4;
};
nix main() {
    var object = PublicByDefault();
    println(object.value);
}
NS

run_error secured_field_access "not accessible here" <<'NS'
class HiddenByChoice {
secured:
    int secret = 4;
};
nix main() {
    var object = HiddenByChoice();
    println(object.secret);
}
NS

run_error secured_method_access "not accessible here" <<'NS'
class HiddenMethod {
secured:
    fn reveal() {
        return 42;
    }
};
nix main() {
    var object = HiddenMethod();
    println(object.reveal());
}
NS

run_ok secured_method_can_access_secured_field $'42\n' <<'NS'
class Encapsulated {
secured:
    var secret = 42;
    fn reveal() {
        return secret;
    }
public:
    fn value() {
        return reveal();
    }
};
nix main() {
    var object = Encapsulated();
    println(object.value());
}
NS

run_ok explicit_public_section_remains_supported $'7\n' <<'NS'
class ExplicitPublic {
public:
    var value = 7;
};
nix main() {
    var object = ExplicitPublic();
    println(object.value);
}
NS

run_error repeat_iteration_limit "repeat() limit is 1,000,000 iterations" <<'NS'
nix main() {
    repeat(1000001) {
        println("unreachable");
    }
}
NS

run_error out_of_class_method_without_class "Unknown class 'Missing' in qualified method definition" <<'NS'
int Missing::value() {
    return 1;
}
nix main() {
    println("unreachable");
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

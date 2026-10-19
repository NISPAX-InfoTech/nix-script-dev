#!/usr/bin/env python3
"""Generated NixScript regression corpus: 1,200 independent samples per family + 50 integrations."""
from __future__ import annotations
import argparse, math, subprocess, sys, tempfile
from pathlib import Path
from typing import Callable

Case = Callable[[int, int], tuple[str, str, str, str | None]]

def build(i: int, n: int, src: str, expected: str, stdin: str = "", module: str | None = None):
    src = src.replace("__CASE__", str(i)).replace("__N__", str(n))
    if module is not None:
        module = module.replace("__CASE__", str(i)).replace("__N__", str(n))
    return src, expected.replace("__N__", str(n)), stdin.replace("__N__", str(n)), module

def variables(i,n):
    return build(i,n,r"""
// sample __CASE__
nix main() {
    var dynamicValue = __N__;
    int integerValue = __N__;
    float floatingValue = __N__;
    double doubleValue = __N__;
    string label = "value__N__";
    bool enabled = true;
    char letter = "A";
    println(dynamicValue + integerValue);
    println(floatingValue + doubleValue);
    println(label);
    println(letter);
    if (enabled) { println("enabled"); }
}
""",f"{2*n}\n{2*n}\nvalue{n}\nA\nenabled\n")

def output(i,n):
    return build(i,n,r"""
// sample __CASE__
nix main() {
    var value = "__N__";
    print("prefix:");
    println("{$value}:suffix");
    println("first\nsecond");
}
""","prefix:__N__:suffix\nfirst\nsecond\n")

def arithmetic(i,n):
    return build(i,n,r"""
// sample __CASE__
nix main() {
    println((__N__ + 1) * 2);
    println(__N__ - 1);
    println((__N__ * 2) / 2);
    println(-__N__ + 2 * __N__);
}
""",f"{(n+1)*2}\n{n-1}\n{n}\n{n}\n")

def comparisons(i,n):
    return build(i,n,r"""
// sample __CASE__
nix main() {
    if (__N__ <= __N__ and __N__ >= 0 and not false) { println("pass"); }
    else { println("fail"); }
    if (__N__ != 0 or false) { println("or-pass"); }
}
""","pass\nor-pass\n")

def branching(i,n):
    return build(i,n,r"""
nix main() {
    var value = __N__;
    if (value < __N__) { println("low"); }
    elif (value == __N__) { println("equal"); }
    else if (value > __N__) { println("high"); }
    else { println("unreachable"); }
}
""","equal\n")

def repeat_loop(i,n):
    return build(i,n,r"""
nix main() {
    var count = 0;
    repeat(__N__) { count = count + 1; }
    println(count);
}
""","__N__\n")

def for_loop(i,n):
    return build(i,n,r"""
nix main() {
    var sum = 0;
    for (var index = 0; index < __N__; index++) { sum = sum + index; }
    println(sum);
}
""",f"{n*(n-1)//2}\n")

def functions(i,n):
    return build(i,n,r"""
fn add(int first, int second) { return first + second; }
fn greet(string name) { println("hello {$name}"); }
nix main() {
    println(add(__N__, __N__ + 1));
    greet("sample__N__");
}
""",f"{2*n+1}\nhello sample{n}\n")

def recursion(i,n):
    k=1+(i%9)
    return build(i,k,r"""
fn factorial(int value) {
    if (value <= 1) { return 1; }
    return value * factorial(value - 1);
}
nix main() { println(factorial(__N__)); }
""",f"{math.factorial(k)}\n")

def prototypes(i,n):
    return build(i,n,r"""
fn cube(int value);
nix main() { println(cube(__N__)); }
fn cube(int value) { return value * value * value; }
""",f"{n**3}\n")

def overloads(i,n):
    return build(i,n,r"""
fn choose(int value) { return value + 1; }
fn choose(string value) { return value + "!"; }
nix main() {
    println(choose(__N__));
    println(choose("item__N__"));
}
""",f"{n+1}\nitem{n}!\n")

def local_input(i,n):
    return build(i,n,r"""
nix main() {
    var name;
    uin(name);
    int age;
    uin(age);
    println(name);
    println(age);
}
""","user__N__\n__N__\n","user__N__\n__N__\n")

def class_input(i,n):
    return build(i,n,r"""
class InputRecord {
public:
    var name;
    int age;
    InputRecord() { uin(name); uin(age); }
};
nix main() {
    var record = InputRecord();
    println(record.name);
    println(record.age);
}
""","user__N__\n__N__\n","user__N__\n__N__\n")

def string_builtins(i,n):
    return build(i,n,r"""
nix main() {
    println(toUpper("abc__N__"));
    println(toLower("NIX"));
    println(len("abc__N__"));
    println(isUpper("ABC"));
    println(isLower("abc"));
    println(isSpace(" "));
}
""",f"ABC{n}\nnix\n{len('abc'+str(n))}\n1\n1\n1\n")

def math_builtins(i,n):
    return build(i,n,r"""
nix main() {
    println(sqrt(__N__ * __N__));
    println(sq(__N__));
    println(pow(__N__, 2));
    println(abs(-__N__));
    println(floor(2.8));
    println(ceil(2.2));
    println(round(2.5));
}
""",f"{n}\n{n*n}\n{n*n}\n{n}\n2\n3\n3\n")

def arrays(i,n):
    return build(i,n,r"""
nix main() {
    var values = [__N__, __N__ + 1];
    values[1] = __N__ + 2;
    values.push(__N__ + 3);
    println(values[0]);
    println(values[1]);
    println(values.length);
    println(values.pop());
}
""",f"{n}\n{n+2}\n3\n{n+3}\n")

def maps(i,n):
    return build(i,n,r"""
nix main() {
    var profile = {value: __N__, label: "item__N__"};
    profile.value = profile.value + 1;
    profile["extra"] = __N__ + 2;
    println(profile.value);
    println(profile.get("label"));
    println(profile.length);
    println(profile["extra"]);
}
""",f"{n+1}\nitem{n}\n3\n{n+2}\n")

def callbacks(i,n):
    return build(i,n,r"""
fn doubleValue(int value) { return value * 2; }
fn apply(function callback, int value) { return callback(value); }
nix main() {
    var operation = doubleValue;
    println(operation(__N__));
    println(apply(operation, __N__ + 1));
    println(apply(abs, -__N__));
}
""",f"{2*n}\n{2*(n+1)}\n{n}\n")

def classes(i,n):
    return build(i,n,r"""
class Box {
public:
    int value = 0;
    Box(int initial) { this.value = initial; }
    int add(int amount) { this.value = this.value + amount; return this.value; }
    int get() { return value; }
};
nix main() {
    var box = Box(__N__);
    println(box.get());
    println(box.add(2));
    println(box.value);
}
""",f"{n}\n{n+2}\n{n+2}\n")

def constructor_overloads(i,n):
    return build(i,n,r"""
class ValueBox {
public:
    int value = 0;
    ValueBox(int initial) { this.value = initial; }
    ValueBox(string initial) { this.value = len(initial); }
};
nix main() {
    var numeric = ValueBox(__N__);
    var textual = ValueBox("abc");
    println(numeric.value);
    println(textual.value);
}
""",f"{n}\n3\n")

def static_and_out_of_class(i,n):
    return build(i,n,r"""
class Counter {
public:
    static int created = 0;
    int value = 0;
    Counter(int initial) {
        this.value = initial;
        Counter::created = Counter::created + 1;
    }
    static int count() { return Counter::created; }
    int add(int amount);
};
int Counter::add(int amount) {
    value = value + amount;
    return value;
}
nix main() {
    var counter = Counter(__N__);
    println(counter.add(2));
    println(Counter::count());
}
""",f"{n+2}\n1\n")

def single_inheritance(i,n):
    return build(i,n,r"""
class Base {
public:
    int baseValue = __N__;
    int getBase() { return baseValue; }
};
class Child : Base {
public:
    int childValue = __N__ + 1;
    int sum() { return this.getBase() + childValue; }
};
nix main() {
    var child = Child();
    println(child.getBase());
    println(child.sum());
}
""",f"{n}\n{2*n+1}\n")

def multiple_inheritance(i,n):
    return build(i,n,r"""
class Left {
public:
    int leftValue = __N__;
    int getLeft() { return leftValue; }
};
class Right {
public:
    int rightValue = __N__ + 1;
    int getRight() { return rightValue; }
};
class Both : Left, Right {
public:
    int sum() { return this.getLeft() + this.getRight(); }
};
nix main() { var both = Both(); println(both.sum()); }
""",f"{2*n+1}\n")

def modules(i,n):
    return build(i,n,r"""
use corpus_mod;
nix main() { println(moduleValue()); }
""",f"{n+5}\n",module="fn moduleValue() { return __N__ + 5; }\n")

def async_await(i,n):
    return build(i,n,r"""
async fn plusOne(value) { return value + 1; }
async fn twice(value) {
    var first = await plusOne(value);
    var second = await plusOne(first);
    return second;
}
nix main() {
    var task = twice(__N__);
    println(await task);
}
""",f"{n+2}\n")

def comments_escapes(i,n):
    return build(i,n,r"""
// sample __CASE__: line comments are ignored
nix main() {
    // escaped tabs and newlines are preserved
    println("left\tright");
    println("line__N__\nend");
}
""",f"left\tright\nline{n}\nend\n")

def abstract_methods(i,n):
    return build(i,n,r"""
abstract class Shape {
public:
    abstract fn area();
};
class Square : Shape {
public:
    int side = __N__;
    fn area() { return side * side; }
};
nix main() { var square = Square(); println(square.area()); }
""",f"{n*n}\n")

def destructors(i,n):
    return build(i,n,r"""
class Tracked {
public:
    ~Tracked() { println("released__N__"); }
};
nix main() {
    var item = Tracked();
    println("created__N__");
}
""",f"created{n}\nreleased{n}\n")

FEATURES: list[tuple[str, Case]] = [
    ("variables_explicit_types_initialization",variables),
    ("print_interpolation_and_escapes",output),
    ("arithmetic_operators",arithmetic),
    ("comparisons_and_logical_operators",comparisons),
    ("if_elif_else_branching",branching),
    ("repeat_loops",repeat_loop),
    ("for_loops",for_loop),
    ("functions_and_return",functions),
    ("recursion",recursion),
    ("prototypes_and_definitions_after_main",prototypes),
    ("overload_dispatch",overloads),
    ("input_into_local_variables",local_input),
    ("input_into_class_fields",class_input),
    ("string_builtins",string_builtins),
    ("math_builtins",math_builtins),
    ("arrays_and_lists",arrays),
    ("maps_and_dictionaries",maps),
    ("first_class_callbacks",callbacks),
    ("classes_fields_constructors_methods",classes),
    ("overloaded_constructors",constructor_overloads),
    ("static_members_and_out_of_class_methods",static_and_out_of_class),
    ("single_inheritance",single_inheritance),
    ("multiple_inheritance",multiple_inheritance),
    ("module_imports",modules),
    ("async_and_nested_await",async_await),
    ("comments_and_escapes",comments_escapes),
    ("abstract_methods",abstract_methods),
    ("destructor_lifecycle",destructors),
]

LARGE = r"""
use corpus_mod;
fn add(int left, int right) { return left + right; }
fn doubleValue(int value) { return value * 2; }
fn apply(function callback, int value) { return callback(value); }
async fn plusOne(value) { return value + 1; }
class ParentRecord {
public:
    int score = 0;
    int increase(int amount) {
        this.score = this.score + amount;
        return this.score;
    }
};
class Metadata {
public:
    string label = "";
    int tagCount() { return 2; }
};
class Record : ParentRecord, Metadata {
public:
    static int created = 0;
    Record() { Record::created = Record::created + 1; }
    int bump(int amount) { score = score + amount; return score; }
    static int count() { return Record::created; }
};
class Premium : Record {
public:
    int extended() { return this.bump(1) * 2; }
};
fn triple(int value);
nix main() {
    var value = __N__;
    int typed = __N__;
    string title = "large__N__";
    bool enabled = true;
    var values = [value, value + 1, value + 2];
    values.push(value + 3);
    var info = {label: title, score: values[1], history: values};
    info.history.push(value + 4);
    var record = Premium();
    record.score = info.score;
    record.label = title;
    var bumped = record.bump(value);
    var total = 0;
    for (var index = 0; index < 3; index++) {
        total = total + values[index];
    }
    repeat(2) { total = total + 1; }
    var callbackResult = apply(doubleValue, value);
    var task = plusOne(value);
    var asyncResult = await task;
    var moduleResult = moduleValue();
    if (enabled and asyncResult == value + 1 and info.history.length == 5) {
        println("pass");
    } else { println("fail"); }
    println(record.label);
    println(add(total, bumped));
    println(callbackResult);
    println(asyncResult);
    println(moduleResult);
    println(Premium::count());
    println(triple(value));
    println(toUpper(title));
    println(sqrt(value * value));
}
fn triple(int value) { return value * 3; }
"""

def run_one(executable: Path, directory: Path, source: str, expected: str,
            stdin: str, module: str | None, label: str, timeout: float):
    path = directory / "corpus_case.ns"
    path.write_text(source, encoding="utf-8")
    module_path = directory / "corpus_mod.ns"
    if module is not None:
        module_path.write_text(module, encoding="utf-8")
    elif module_path.exists():
        module_path.unlink()
    try:
        result = subprocess.run([str(executable), str(path)], input=stdin, text=True,
                                capture_output=True, timeout=timeout, cwd=directory)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f"{label}: timed out\nSOURCE:\n{source}\n{error}") from error
    if result.returncode != 0 or result.stdout != expected:
        raise RuntimeError(
            f"{label}: expected exit=0 and exact stdout.\nexit={result.returncode}\n"
            f"expected stdout={expected!r}\nactual stdout={result.stdout!r}\n"
            f"stderr={result.stderr!r}\nSOURCE:\n{source}"
        )

def main() -> int:
    parser=argparse.ArgumentParser()
    parser.add_argument("--nix",required=True,type=Path)
    parser.add_argument("--small-per-feature",type=int,default=1200)
    parser.add_argument("--large-count",type=int,default=50)
    parser.add_argument("--timeout",type=float,default=4.0)
    args=parser.parse_args()
    executable=args.nix.resolve()
    if not executable.exists(): parser.error(f"interpreter does not exist: {executable}")
    if args.small_per_feature < 1 or args.large_count < 0:
        parser.error("small-per-feature must be positive and large-count non-negative")
    count=0
    try:
        with tempfile.TemporaryDirectory(prefix="nixscript-regression-") as temp:
            directory=Path(temp)
            for family,factory in FEATURES:
                for index in range(args.small_per_feature):
                    n=1+(index%37)
                    source,expected,stdin,module=factory(index,n)
                    run_one(executable,directory,source,expected,stdin,module,
                            f"feature={family} case={index+1}",args.timeout)
                    count+=1
                print(f"PASS {family}: {args.small_per_feature} programs",flush=True)
            for index in range(args.large_count):
                n=1+(index%50)
                source,_,stdin,module=build(index,n,LARGE,"",
                                            module="fn moduleValue() { return __N__ + 5; }\n")
                expected=("pass\n"+f"large{n}\n"+f"{5*n+6}\n"+f"{2*n}\n"+
                          f"{n+1}\n"+f"{n+5}\n"+"1\n"+f"{3*n}\n"+
                          f"LARGE{n}\n"+f"{n}\n")
                run_one(executable,directory,source,expected,stdin,module,
                        f"large-integration case={index+1}",args.timeout)
                count+=1
            print(f"PASS large integration: {args.large_count} programs",flush=True)
    except RuntimeError as error:
        print(str(error),file=sys.stderr)
        return 1
    small_total=args.small_per_feature*len(FEATURES)
    print(f"ALL PASS: {small_total} small programs across {len(FEATURES)} feature families "
          f"+ {args.large_count} large programs = {count} programs total.")
    return 0

if __name__=="__main__":
    raise SystemExit(main())

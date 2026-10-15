# NixScript Language Notes

> Working draft. This describes the current prototype and its intended direction; syntax and semantics may still evolve.

## Goal

NixScript is a readable, multi-paradigm, general-purpose language with predictable memory use and a small core. It should be powerful without becoming needlessly verbose. The planned Discord library should provide Discord-specific types and functionality instead of baking them into the language.

## File and module direction

- Source files use the `.ns` extension.
- `use library_name;` brings a library or module into a file.
- Module visibility and export rules are not decided yet.

```ns
use discord;
```

## Variables and type inference

- Use `var` to declare inferred variables.
- A declaration without an initial value is allowed: `var name;`.
- Exact initialization and type-checking rules are still to be defined.

```ns
var score = 10;
var name;
```

## Input

Implemented `uin()` behavior:
- `uin()` reads one line as a string.
- `uin("Enter age: ", int)` reads numeric input and truncates it to a whole number.
- `uin("Enter value: ", float)` and `uin("Enter value: ", double)` read numeric input.
- `uin("Continue? ", bool)` accepts `true`/`false` or `1`/`0`.
- `uin(existing_variable)` reads directly into an existing variable, preserving its basic string/number/boolean category.
- Invalid typed input raises a readable runtime error.
- Input is read from standard input; interactive input in the Android app still needs a UI integration.

```ns
var name;
uin(name);

var age = uin("Enter your age: ", int);
```

## Output and string interpolation

`print()` accepts one or more comma-separated values and ends with a newline. Values are printed exactly as supplied; the language should not silently add spaces between them. Double-quoted strings support interpolation with `{$variable}`.

```ns
var name = "Alex";
print("Hello ", name, "!");
print("Hello, {$name}!");
```

## Functions and program entry point

Ordinary functions use `fn`. Every executable NixScript program must have exactly one `nix main()` entry point; `nix` is reserved for that purpose.

```ns
fn greet(name) {
    print("Hello, {$name}!");
}

nix main() {
    greet("Alex");
}
```

Top-level functions support both definitions and C-style declarations. You can define a function before `nix main()`, or declare its signature before `main` and place its implementation after `main`. Every declared function must have exactly one definition, and a declaration and definition must agree on parameter count. Repeated compatible declarations are allowed; parameter names in a declaration do not need to match the names used by the definition. Functions support positional arguments, local scope, recursion, and `return`; parameters use inferred values and do not require type annotations. A function that reaches its end without returning produces `null`. Function values and callback passing are not implemented yet, and `return` outside a function or class method raises an error.

## Classes and objects

**Decision: use C++-style class syntax and core OOP concepts**, rather than inventing a custom class construct for now.

Planned syntax includes:
- `class Name { ... };` — the semicolon after the class body is required.
- `Name object;` and `Name object(args);` — object declarations and construction.
- A constructor named after the class, e.g. `Player(name)`.
- A destructor named `~Player()`.
- `public`, `private`, and `protected` access specifiers.
- Inheritance, member functions, and static members.
- `ClassName::member` scope resolution and out-of-class member definitions.
- `this` to refer to the current object.

The prototype implements the core of this syntax. For example:

```ns
class Player {
public:
    int health = 100;
    static int created = 0;

    Player(int startingHealth) {
        this.health = startingHealth;
        Player::created = Player::created + 1;
    }

    ~Player() {
        print("Player destroyed");
    }

    int damage(int amount);

    static int count() {
        return Player::created;
    }
};

int Player::damage(int amount) {
    health = health - amount;
    return health;
}

nix main() {
    Player player(100);
    player.damage(20);
    print(player.health);
    print(Player::count());
}
```

Implemented class features include fields, typed or inferred field declarations, constructors, destructors, methods with parameters and `return`, `public`/`private`/`protected`, single inheritance, static fields/methods, `this`, `ClassName::member`, and out-of-class method definitions. Constructors and destructors are invoked by the interpreter; destructor timing follows local scope cleanup in this prototype.

This is an interpreter implementation, not full C++ compatibility. Multiple inheritance, overloaded methods/operators, references, C++ initializer lists for base constructors, and complete C++ object-lifetime rules are not implemented. Use `this.member` where clarity matters; unqualified access to the current class's fields is also supported inside methods.

## Native built-in functions

NixScript should provide useful built-ins for common string and math operations without requiring a library import.

### String and character helpers

Implemented functions:
- `isUpper(value)` — returns numeric `1` if the string has at least one letter and no lowercase letters; otherwise `0`.
- `isLower(value)` — returns numeric `1` if the string has at least one letter and no uppercase letters; otherwise `0`.
- `toUpper(value)` — converts ASCII letters to uppercase.
- `toLower(value)` — converts ASCII letters to lowercase.
- `isSpace(value)` — returns numeric `1` if the non-empty string consists entirely of whitespace; otherwise `0`.

Predicate functions return numeric `0` or `1`; conditions treat zero as false and non-zero as true.

Examples of intended usage:

```ns
print(isUpper("A"));       // 1
print(toLower("HELLO"));   // hello

if (isSpace(" ")) {
    print("Whitespace");
}
```

### Math helpers

Implemented math built-ins include:
- `sqrt(value)` — square root of a non-negative number.
- `sq(value)` — square of a value.
- `pow(base, exponent)` — exponentiation.
- `abs(value)`, `floor(value)`, `ceil(value)`, and `round(value)`.

These validate argument counts and types and report readable runtime errors for invalid input, including a negative square-root argument.

## Conditions and loops

`if` / `elif` / `else if` / `else`, comparisons (`==`, `!=`, `<`, `<=`, `>`, `>=`), and logical operators (`and`, `or`, `not`, `&&`, `||`, `!`) are implemented. `elif` and `else if` behave identically; either can be used in a chain.
```ns
if (score >= 90) {
    print("A");
} elif (score >= 75) {
    print("B");
} else if (score >= 50) {
    print("C");
} else {
    print("Try again");
}
```


A `repeat` loop and C-style `for` loop are implemented.

```ns
repeat(5) {
    print("Hello!");
}

for (var i = 0; i < 10; i = i + 1) {
    print(i);
}
```

## Arrays

Fixed-size arrays are wanted for predictable storage.

```ns
var numbers: int[10];

numbers[0] = 42;
numbers[1] = 99;
```

## Multi-paradigm direction

- Procedural programming with functions and control flow.
- Object-oriented programming with classes.
- Functional capabilities without forcing all code into classes.
- Modules for code reuse.

## Discord library direction

Discord-specific types such as `Bot`, `Message`, and `Embed` should live in the Discord library. The language core should stay general-purpose and small.

## Implementation status

The current C++17 interpreter prototype supports the required `nix main()` entry point, variable declarations, output/interpolation, arithmetic, comparisons, logical conditions, `if`/`elif`/`else`, top-level functions with arguments and return values, `repeat`, C-style `for`, `uin()`, common string/math built-ins, and a subset of C++-style classes/objects. Function values, arrays, and module loading remain unimplemented.

## Open decisions

1. Exact type system and uninitialized-variable rules.
2. Module exports and visibility.
3. Full type checking and exact return-type rules.
4. Array syntax and memory/layout guarantees.
5. Module loading and top-level function semantics.
6. Full C++-style class details not covered by the current subset.
7. Unicode behavior for string case conversion; current implementation is ASCII-only.

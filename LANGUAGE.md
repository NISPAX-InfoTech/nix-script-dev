# NixScript Language Notes

> Early working draft. This records the current design direction, not a finalized language specification. Features marked planned are not necessarily implemented in the interpreter.

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

Planned `uin()` behavior:
- `uin()` reads input as a string by default.
- `uin("Enter age: ", int)` reads typed input.
- `uin(existing_variable)` reads input directly into that variable.
- Invalid typed input behavior still needs a decision.

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

General function declarations, calls, parameters, and return values are planned; they are not yet implemented in the current prototype.

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

Illustrative syntax only; classes are not implemented in the current prototype:

```ns
class Player {
public:
    var name;
    var health = 100;

    Player(name) {
        this.name = name;
    }

    ~Player() {
        print("Player destroyed");
    }

    fn damage(amount) {
        health = health - amount;
    }
};

fn Player::damage(amount) {
    health = health - amount;
}

nix main() {
    Player player("Knight");
    player.damage(20);
    print(player.health);
}
```

The exact interaction between C++-style object lifetime and NixScript's intended automatic memory cleanup must be specified carefully. C++ syntax is the design reference; it does not mean the interpreter already has C++ semantics.

## Native built-in functions

NixScript should provide useful built-ins for common string and math operations without requiring a library import.

### String and character helpers

Planned functions:
- `isUpper(value)` — checks whether a character/string is uppercase.
- `isLower(value)` — checks whether a character/string is lowercase.
- `toUpper(value)` — converts letters to uppercase.
- `toLower(value)` — converts letters to lowercase.
- `isSpace(value)` — checks whether a character is whitespace.

Predicate functions should produce numeric `0` or `1` in ordinary output/arithmetic contexts, while being usable naturally as conditions (displayed as `false`/`true` when rendered as a boolean condition). The precise type/coercion rule needs to be implemented consistently, rather than being special-cased only for printing.

Examples of intended usage:

```ns
print(isUpper("A"));       // 1
print(toLower("HELLO"));   // hello

if (isSpace(" ")) {
    print("Whitespace");
}
```

### Math helpers

Planned built-ins include:
- `sqrt(value)` — square root.
- `sq(value)` — square of a value.
- `pow(base, exponent)` — exponentiation.

These should validate argument count and types and report readable runtime errors for invalid input (for example, a negative square-root argument if NixScript chooses real-number-only math).

## Loops

A simple `repeat` loop and a flexible `for` loop are wanted.

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

The current C++17 interpreter prototype supports the required `nix main()` entry point, basic `var` declarations, output/interpolation, arithmetic, and `repeat`. The following are planned, not yet implemented: user-defined functions, classes/objects, `uin()`, `if`/general `for`, arrays, modules, and the built-in functions listed above.

## Open decisions

1. Exact type system and uninitialized-variable rules.
2. Module exports and visibility.
3. Invalid typed-input behavior for `uin()`.
4. Exact function return-type rules.
5. Consistent predicate result/coercion behavior (`0`/`1` versus boolean conditions).
6. Object lifetime/destructor timing under automatic memory cleanup.
7. Full C++-style class details, including inheritance and static member rules.
8. Unicode behavior for string case conversion; initial implementation may be ASCII-only.

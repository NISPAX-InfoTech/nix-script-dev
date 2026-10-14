# NixScript Language Notes

> Early working draft. This records our current direction, not a finalized language specification.

## Goal

NixScript is a readable, multi-paradigm language with C/JavaScript familiarity, predictable memory use, and a small core. The planned Discord library should provide most Discord-specific features; those features should not need to be built into the language itself.

## File and module direction

- Source files use the `.ns` extension.
- `use library_name;` brings a library or module into a file.
- Module visibility and export rules are not decided yet.

Example:

```ns
use discord;
```

## Variables and type inference

- Use `var` to declare variables.
- The compiler should infer a variable's type from its assigned value where possible.
- A declaration without an initial value should be allowed, for example `var name;`.
- Exact initialization and type-checking rules are still to be defined.

```ns
var score = 10;
var name;
```

## Input

- `uin()` reads input as a string by default.
- A prompt and an expected type can be supplied, for example `uin("Enter age: ", int)`.
- `uin(existing_variable)` should read input directly into that variable.

```ns
var name;
uin(name);

var age = uin("Enter your age: ", int);
```

The behavior for invalid typed input still needs a decision.

## Functions and program entry point

Function direction:

```ns
fn greet(name) {
    print("Hello, " + name);
}
```

Every executable NixScript program must have exactly one `nix main()` entry point. The keyword `nix` is reserved for this purpose only; ordinary functions continue to use `fn`.

## Loops

Both a simple repeat loop and a flexible `for` loop are wanted.

```ns
repeat(5) {
    print("Hello!");
}

for (var i = 0; i < 10; i = i + 1) {
    print(i);
}
```

## Arrays

Fixed-size arrays like C arrays are wanted for predictable storage.

```ns
var numbers: int[10];

numbers[0] = 42;
numbers[1] = 99;
```

## Multi-paradigm direction

- Procedural code with functions and control flow.
- Object-oriented programming with classes.
- Functional capabilities can be added without forcing all code into classes.
- Modules for code reuse.

## Discord library direction

Discord-specific types such as `Bot`, `Message`, and `Embed` should live in the Discord library. For example, an embed can be represented by a class supplied by that library. The core language should stay small.

## Open decisions

2. The exact type system and rules for uninitialized variables.
3. How modules export and import names.
4. What happens when typed `uin` input is invalid.
5. Class-construction syntax.

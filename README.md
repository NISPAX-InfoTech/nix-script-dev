# NixScript

NixScript is an early experiment in a readable, multi-paradigm, general-purpose programming language. Its source files use the `.ns` extension.

The current implementation is a small **C++17 interpreter prototype**, not a native compiler. It does not yet implement the full language design.

## Build

Requires CMake 3.16+ and a C++17 compiler.

```sh
cmake -S . -B build
cmake --build build
```

## Run an example

```sh
./build/nix examples/hello.ns
```

On Windows, the executable may be at `build/Debug/nix.exe` or `build/Release/nix.exe`, depending on the generator.

## Currently implemented

- `nix main() { ... }` as the required executable entry point
- `var name = value;` and uninitialized `var name;`
- `print(...);` with multiple values and string interpolation
- Basic arithmetic, comparisons, logical conditions, and `+` string concatenation
- `repeat(count) { ... }` and C-style `for` loops, including `i++`/`i--`
- `if` / `else if` / `else`
- `uin()` string input, direct input into an existing variable, and typed numeric/boolean input
- Native string helpers: `isUpper()`, `isLower()`, `toUpper()`, `toLower()`, `isSpace()`
- Native math helpers: `sqrt()`, `sq()`, `pow()`, `abs()`, `floor()`, `ceil()`, and `round()`
- C++-style class declarations, fields, constructors/destructors, methods, access labels, single inheritance, static members, and `ClassName::member`
- `this`, typed method parameters, `return`, in-class and out-of-class method definitions
- `//` line comments and parser/runtime errors with line numbers

## Still planned

- General top-level user-defined functions
- Fixed-size arrays
- Module loading and `use` imports

Try the expanded features with `./build/nix examples/features.ns`. For input, run `./build/nix examples/input.ns`. Input reads from standard input in the command-line interpreter.

The interpreter is still an evolving prototype: arrays, module loading, and general top-level functions are not implemented yet. String case conversion currently operates on ASCII characters.

See [LANGUAGE.md](LANGUAGE.md) for syntax notes and limitations.

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
- Basic arithmetic and `+` string concatenation
- `repeat(count) { ... }`
- `//` line comments
- Parser/runtime errors with line numbers

## Planned

- C++-style classes and objects, including constructors, destructors, access specifiers, inheritance, static members, and `::` scope resolution
- Native string helpers: `isUpper()`, `isLower()`, `toUpper()`, `toLower()`, `isSpace()`
- Native math helpers: `sqrt()`, `sq()`, `pow()`, and other useful math functions
- `uin()`, user-defined functions, `if`/else, general `for` loops, arrays, and module loading

See [LANGUAGE.md](LANGUAGE.md) for the current design notes and implementation status.

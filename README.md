# NixScript

NixScript is an early experiment in a readable, multi-paradigm programming language. Its source files use the `.ns` extension.

The current implementation is a small **C++17 interpreter prototype**. It is not yet a compiler and does not implement the full language design.

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

## Prototype features

- `fn main() { ... }` as the executable entry point
- `var name = value;` and uninitialized `var name;`
- `print(expression);`
- arithmetic with numbers and `+` string concatenation
- `repeat(count) { ... }`
- `//` line comments

Module loading, `uin()`, arrays, classes, `if`/loops beyond `repeat`, and static type checking are not implemented yet.

See [LANGUAGE.md](LANGUAGE.md) for the working design notes.

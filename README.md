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
- Top-level function prototypes before `main` and definitions before or after `main`; unused prototypes are allowed, while calling a prototype without a definition fails clearly
- `var name = value;` and uninitialized `var name;`
- `print(...);` without an automatic newline and `println(...);` with an automatic newline, both supporting multiple values and string interpolation
- Basic arithmetic, comparisons, logical conditions, and `+` string concatenation
- Top-level `fn` functions with parameters, calls, local scope, recursion, and `return`
- `repeat(count) { ... }` and C-style `for` loops, including `i++`/`i--`
- `if` / `elif` / `else if` / `else`
- `uin()` string input, direct input into an existing variable, and typed numeric/boolean input
- Native string helpers: `isUpper()`, `isLower()`, `toUpper()`, `toLower()`, `isSpace()`
- Native math helpers: `sqrt()`, `sq()`, `pow()`, `abs()`, `floor()`, `ceil()`, and `round()`
- C++-style class declarations, fields, constructors/destructors, methods, access labels, single inheritance, static members, and `ClassName::member`
- `this`, typed method parameters, `return`, in-class and out-of-class method definitions
- `\n` and `\t` string escapes; `//` line comments and parser/runtime errors with line numbers

## Still planned

- Function values and callback passing
- Fixed-size arrays
- Export/visibility rules for modules and package-manager support

Try output behavior with `./build/nix examples/output.ns`. Try the expanded features with `./build/nix examples/features.ns`. For functions, run `./build/nix examples/functions.ns` or `./build/nix examples/prototypes.ns` to see declarations before `main` and implementations after it. For input, run `./build/nix examples/input.ns`. Input reads from standard input in the command-line interpreter.

The interpreter is still an evolving prototype: function values/callback passing, arrays, module export/visibility rules, and package-manager support are not implemented yet. String case conversion currently operates on ASCII characters.

## Libraries

`use name;` loads `name.ns` from the project's `lib/` or `libraries/` folder, or from the standard installed-library directory. Imports can be nested; a file is loaded once, and circular imports are reported. Library files contain declarations (such as `fn` functions and `class` definitions), not `nix main()`. Imported declarations currently share the program's global namespace; explicit exports and aliases are not implemented yet.

**Standard installed-library directory:**
- Linux: `~/.local/share/nixscript/libraries/` (or `$XDG_DATA_HOME/nixscript/libraries/` when `XDG_DATA_HOME` is set)
- macOS: `~/.local/share/nixscript/libraries/` (or `$XDG_DATA_HOME/nixscript/libraries/` when set)
- Windows: `%APPDATA%\\NixScript\\libraries\\`

Set `NIXSCRIPT_LIB_PATH` to override the installed-library directory. Project-local `lib/` and `libraries/` folders are checked first; installed libraries are checked next; the importing file's directory is a final fallback for simple local examples.

```ns
use greeting;

nix main() {
    greet("Alex");
}
```

For a working example, run `./build/nix examples/use_library.ns`.

See [LANGUAGE.md](LANGUAGE.md) for syntax notes and limitations.

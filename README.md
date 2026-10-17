# NixScript

NixScript is an early experiment in a readable, multi-paradigm, general-purpose programming language. Its source files use the `.ns` extension.

The current implementation is a **C++17 interpreter prototype**, not a native compiler. The core C++-style OOP arc includes multiple inheritance, overload resolution by arity and parameter types, and explicit base-constructor initialization; full C++ compatibility is not a goal.

## Release status

NixScript is currently an **early preview**, not a stable 1.0 release. The CLI release workflow packages tested Linux and Windows builds from a semantic-version tag (for example, `v0.1.0`) pushed to the private `nix-script-dev` repository, then publishes the artifacts to the public [NixScript releases repository](https://github.com/NISPAX-InfoTech/nix-script). The workflow requires the `NIXSCRIPT_RELEASE_TOKEN` Actions secret in the development repository. See [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md) for setup, release gates, supported artifacts, and remaining stability decisions. Do not assume planned features are implemented; the limitations below and in [LANGUAGE.md](LANGUAGE.md) are authoritative.

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
- Top-level function prototypes and overloads, with resolution by argument count and optional parameter-type annotations; definitions may appear before or after `main`, and unresolved declared overloads fail clearly
- `var name = value;` and uninitialized `var name;`
- `print(...);` without an automatic newline and `println(...);` with an automatic newline, both supporting multiple values and string interpolation
- Basic arithmetic, comparisons, logical conditions, and `+` string concatenation
- Top-level `fn` functions with parameters, calls, local scope, recursion, and `return`
- `repeat(count) { ... }` and C-style `for` loops, including `i++`/`i--`
- `if` / `elif` / `else if` / `else`
- `uin()` string input, direct input into an existing variable, and typed numeric/boolean input
- Native string helpers: `isUpper()`, `isLower()`, `toUpper()`, `toLower()`, `isSpace()`
- Native math helpers: `sqrt()`, `sq()`, `pow()`, `abs()`, `floor()`, `ceil()`, and `round()`
- C++-style classes, fields, overloaded constructors/methods, destructors, public/private/protected access, multiple inheritance, static members, and `ClassName::member`
- `this`, typed parameters for overload selection, `return`, in-class and out-of-class method definitions, and base-constructor initializer lists
- Dynamic arrays/lists with literals, nested values, checked indexing/assignment, `.length`, `push`, `append`, and `pop`
- Dictionary/map literals, ordered key/value storage, primitive key normalization, indexed and dot-property access, and `get`/`set`/`has`/`remove`/`clear`
- First-class function values and callbacks, including bound instance methods and selected built-ins
- `\n` and `\t` string escapes; `//` line comments and parser/runtime errors with line numbers

## Still planned

- Fixed-size typed arrays, array type enforcement, and fixed-size declaration syntax
- Export/visibility rules for modules and package dependency management

Try output behavior with `./build/nix examples/output.ns`. Try the expanded features with `./build/nix examples/features.ns`. For functions, run `./build/nix examples/functions.ns` or `./build/nix examples/prototypes.ns`. For mutable arrays, maps, and callbacks, run `./build/nix examples/collections.ns`, `./build/nix examples/maps.ns`, and `./build/nix examples/callbacks.ns`. For input, run `./build/nix examples/input.ns`. Input reads from standard input in the command-line interpreter.

The interpreter is still an evolving prototype. Fixed-size typed arrays, full assignment/return type enforcement, closures/lambdas, module export/visibility rules, package dependency management, and package integrity signatures remain future work. Arrays are dynamic lists with bounds-checked non-negative integer indexes. Maps preserve insertion order and normalize primitive keys to strings; missing keys return `null`. Function values refer to top-level overload groups or bound methods (not closures/lambdas yet). String case conversion currently operates on ASCII characters.

## Libraries

`use name;` loads `name.ns` from the project's `lib/` or `libraries/` folder, or from the standard installed-library directory. Imports can be nested; a file is loaded once, and circular imports are reported. Library files contain declarations (such as `fn` functions and `class` definitions), not `nix main()`. Imported declarations currently share the program's global namespace; explicit exports and aliases are not implemented yet.

**Standard installed-library directory:**
- Linux: `~/.local/share/nixscript/libraries/` (or `$XDG_DATA_HOME/nixscript/libraries/` when `XDG_DATA_HOME` is set)
- macOS: `~/Library/Application Support/NixScript/libraries/`
- Windows: `%APPDATA%\\NixScript\\libraries\\`

Set `NIXSCRIPT_LIB_PATH` to override the installed-library directory. Project-local `lib/` and `libraries/` folders are checked first; installed libraries are checked next; the importing file's directory is a final fallback for simple local examples.

```ns
use greeting;

nix main() {
    greet("Alex");
}
```

For a working example, run `./build/nix examples/use_library.ns`.

## Official library packages

The CLI installs packages from the public [official NixScript library registry](https://github.com/NISPAX-InfoTech/nixscript-libraries). Its `index.txt` maps each package to a version, and released files live at `libraries/<name>/<version>/<name>.ns` within that registry. The installer constructs downloads from this fixed source; registry entries cannot select arbitrary repositories or URLs.

```sh
nix install numbers
nix list
nix update numbers
nix update
```

`nix install <name>` downloads the latest version listed in the official index and records the installed version. `nix update <name>` updates one installed package; `nix update` updates all installed packages. `nix list` shows installed official packages and their recorded versions. Currently, `numbers` is the starter package. A package is installable only after it appears in the public registry's `index.txt` and its versioned `.ns` file is published there.

The CLI uses `curl` for HTTPS downloads. Installed libraries share the standard library directory used by `use`; set `NIXSCRIPT_LIB_PATH` to override that directory during development or testing. Packages are regular `.ns` files, so importing one uses the existing `use numbers;` syntax.

The initial package format supports single-file libraries and version tracking. Dependency installation and cryptographic package signatures are future improvements.

See [LANGUAGE.md](LANGUAGE.md) for syntax notes and limitations.

# NixScript Language Notes

> Working draft. This describes the current prototype and its intended direction; syntax and semantics may still evolve.

## Goal

NixScript is a readable, multi-paradigm, general-purpose language with predictable memory use and a small core. It should be powerful without becoming needlessly verbose. The planned Discord library should provide Discord-specific types and functionality instead of baking them into the language.

## File and module direction

- Source files use the `.ns` extension.
- `use library_name;` loads a `.ns` library before the importing program is parsed.
- Project-local resolution checks `lib/` and `libraries/` beside the importing file first, then the standard installed-library directory, then the importing file's directory as a fallback.
- Standard installed-library directory: Linux `~/.local/share/nixscript/libraries/` (or `$XDG_DATA_HOME/nixscript/libraries/` when set); macOS `~/Library/Application Support/NixScript/libraries/`; Windows `%APPDATA%/NixScript/libraries/`.
- Set `NIXSCRIPT_LIB_PATH` to override the installed-library directory.
- Imports can be nested. Each resolved file is loaded once; circular imports and missing libraries produce readable errors.
- Library files contain top-level `fn` and `class` declarations and optional method definitions, but must not define `nix main()`.
- Imported declarations share the global namespace for now. Export visibility, aliases, dependency handling, and package integrity signatures are future work.

```ns
use discord;
```

## Official package commands

Only libraries listed by the team in the [public official registry](https://github.com/NISPAX-InfoTech/nixscript-libraries) are installable. The CLI reads `index.txt` at the registry root, where each line is a package name and a `MAJOR.MINOR.PATCH` version. The matching source file is stored at `libraries/<name>/<version>/<name>.ns`. Published version directories should not be edited in place; add a new version and update the index to release a package update.

```sh
nix install numbers  # Install the latest indexed official version.
nix list             # Show locally installed package versions.
nix update numbers   # Update one installed package.
nix update           # Update every installed official package.
```

The first official package is `numbers`, which provides `clamp(value, minimum, maximum)`. After installing it, source code can use:

```ns
use numbers;

nix main() {
    println(clamp(14, 0, 10));  // 10
}
```

The installer downloads files from the fixed official GitHub registry over HTTPS using `curl`; the registry does not accept arbitrary download URLs. Versions are recorded in `installed.txt` alongside installed libraries. The initial format supports single-file libraries only; dependency resolution and cryptographic signatures are future work.

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

- `print(...)` writes all supplied values exactly as given, without adding a newline.
- `println(...)` writes all supplied values and then adds one newline.
- Both accept zero or more comma-separated values and do not silently insert spaces.
- Double-quoted strings support interpolation with either `{$variable}` or `${variable}`, and escapes such as `\n` (newline) and `\t` (tab).

```ns
var name = "Alex";
print("Hello, ");
println("{$name}!");
print("First line\nSecond line\n");
```

## Functions and program entry point

Ordinary functions use `fn`. Every executable NixScript program must have exactly one `nix main()` entry point; `nix` is reserved for that purpose. Identifiers are case-sensitive: `main` is reserved, but `maIn` is a different identifier.

```ns
fn greet(name) {
    println("Hello, {$name}!");
}

nix main() {
    greet("Alex");
}
```

Top-level functions support definitions, prototypes, and overloads. A function may be defined before `nix main()` without a separate prototype; a definition after `main` can use a matching `fn Name(parameters);` prototype before `main`. The `fn` keyword is required on ordinary function declarations and definitions. Overloads may differ by argument count or typed parameters, for example `fn show(string value)` and `fn show(int value)`. Untyped parameters are generic fallbacks. Dispatch chooses the most specific compatible signature and reports clear no-match or ambiguity errors rather than selecting arbitrarily. Prototypes and definitions must agree on their full parameter-type signature, though parameter names may differ. Unused prototypes do not require a definition; calling an unresolved declared overload is an error. Functions support local scope, recursion, and `return`; a function that reaches its end without returning produces `null`. Function names can be stored in variables, returned, placed in arrays, and passed as callbacks. A call selects the best compatible overload when the value refers to an overloaded function group. Bound instance methods can also be passed as callbacks. Anonymous functions/lambdas and closures are not implemented yet; `return` outside a function or class method raises an error.

```ns
fn doubleIt(int value) { return value * 2; }
fn apply(function callback, int value) { return callback(value); }

nix main() {
    var operation = doubleIt;
    println(apply(operation, 6)); // 12
}
```

## Classes and objects

**Decision: use C++-style class syntax and core OOP concepts**, rather than inventing a custom class construct for now.

Planned syntax includes:
- `class Name { ... };` — the semicolon after the class body is required.
- `Name object;` and `Name object(args);` — object declarations and construction.
- A constructor named after the class, e.g. `Player(name)`.
- A destructor named `~Player()`.
- Public members by default; optional `public:` is accepted for clarity.
- `secured:` makes following members private. Legacy `private:` is also accepted.
- `protected:` makes members accessible from the class and its derived classes.
- `fn Method(...);` declares a class method; `fn ClassName::Method(...)` defines it outside the class.
- `abstract class Name` declares a class that cannot be instantiated directly.
- `abstract fn method(...);` declares a required method with no body. Concrete derived classes must implement every inherited abstract overload; abstract derived classes may defer the implementation.
- Methods and constructors can be overloaded by argument count and optional parameter types.
- Multiple inheritance uses `class Name : BaseA, BaseB { ... };`. Constructors can pass arguments to bases using an initializer list, such as `Child(int value) : Left(value), Right("tag") { ... }`.
- Method overriding, polymorphic dispatch, and static members.
- `ClassName::member` scope resolution and `this` to refer to the current object.

The implemented OOP subset includes encapsulation through access checks, overloaded constructors and methods, constructors/destructors, single and multiple inheritance, method overriding, dynamic dispatch (including calls through a base-typed variable), abstract-method enforcement, and static fields/methods. For example:

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
        println("Player destroyed");
    }

    fn damage(amount);

    static fn count() {
        return Player::created;
    }
};

fn Player::damage(amount) {
    health = health - amount;
    return health;
}

nix main() {
    Player player(100);
    player.damage(20);
    println(player.health);
    println(Player::count());
}
```

Implemented class features include fields, typed or inferred field declarations, overloaded constructors and methods, constructors/destructors, public-by-default visibility, `secured:`/`private:`, `protected:`, explicit `public:`, single and multiple inheritance, method overriding, dynamic method lookup, abstract classes/methods, static fields/methods, `this`, `ClassName::member`, and out-of-class method definitions. Base constructor initializer lists can pass values to each direct base. Shared ancestors in a diamond are initialized and destroyed once; conflicting initializer choices for a shared ancestor are diagnosed. If unrelated bases contribute the same member signature/name, the access is ambiguous until the derived class redeclares or overrides it. Constructors and destructors run during object construction and scope cleanup.

Important limits: this is an interpreter subset, not complete C++ compatibility. Values still use dynamic runtime storage, so `int` and `float` are overload-selection constraints rather than distinct runtime numeric representations. There is no separate interface/trait system, operator overloading, references, or C++ ABI/layout compatibility yet. Encapsulation, abstract-method enforcement, overload selection, constructor chaining, multiple inheritance and polymorphic dispatch have regression coverage. Use `this.member` where clarity matters; unqualified access to current-class fields is also supported inside methods.

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
println(isUpper("A"));       // 1
println(toLower("HELLO"));   // hello

if (isSpace(" ")) {
    println("Whitespace");
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
    println("A");
} elif (score >= 75) {
    println("B");
} else if (score >= 50) {
    println("C");
} else {
    println("Try again");
}
```


A `repeat` loop and C-style `for` loop are implemented.

```ns
repeat(5) {
    println("Hello!");
}

for (var i = 0; i < 10; i = i + 1) {
    println(i);
}
```

## Arrays and lists

Array literals create mutable dynamic lists. Arrays can contain mixed values or nested arrays, and assigning an array to another variable shares the same mutable array.

```ns
var numbers = [10, 20, 30];
numbers[1] = 42;
numbers.push(40);   // returns the new length
numbers.append(50); // returns null
println(numbers[1]);
println(numbers.length);
println(len(numbers));
println(numbers.pop());
```

Indexes must be non-negative whole numbers and must be in range; invalid indexes produce a line-numbered runtime error. `push`/`append` add one element, while `pop` removes and returns the final element. The equivalent built-ins are `push(array, value)`, `append(array, value)`, and `pop(array)`. Strings also support `len(string)`. Fixed-size typed declarations such as `int values[10]` are not implemented yet.

## Multi-paradigm direction

- Procedural programming with functions and control flow.
- Object-oriented programming with classes.
- Functional capabilities without forcing all code into classes.
- Modules for code reuse.

## Discord library direction

Discord-specific types such as `Bot`, `Message`, and `Embed` should live in the Discord library. The language core should stay general-purpose and small.

## Implementation status

The current C++17 interpreter prototype supports the required `nix main()` entry point, variable declarations, output/interpolation, arithmetic, comparisons, logical conditions, `if`/`elif`/`else`, top-level functions with arguments and return values, `repeat`, C-style `for`, `uin()`, common string/math built-ins, a subset of C++-style classes/objects, and source-file library loading with `use`. Function values, arrays, module exports, package dependency management, and package integrity signatures remain unimplemented.

## Open decisions

1. Exact type system and uninitialized-variable rules.
2. Module exports and visibility.
3. Full type checking and exact return-type rules.
4. Array syntax and memory/layout guarantees.
5. Module loading and top-level function semantics.
6. Full C++-style class details not covered by the current subset.
7. Module exports, aliases, package dependency manifests, and registry integrity signatures.
7. Unicode behavior for string case conversion; current implementation is ASCII-only.

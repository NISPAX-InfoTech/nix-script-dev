# NS1 modules and async/await implementation contract

This document defines the acceptance contract for the NS1 module system and native asynchronous execution. It is not a claim that the features are implemented.

## Modules

### Source syntax

```ns
// math.ns
export fn add(a, b) {
    return a + b;
}
```

```ns
// main.ns
import { add } from "./math";

nix main() {
    println(add(2, 3));
}
```

The exact import grammar must be implemented consistently and documented before release. Existing `use library_name;` support must remain compatible for installed libraries.

### Required behavior

- Resolve relative imports from the importing source file, not the process working directory.
- Resolve project-local libraries and installed libraries using the existing loader rules.
- Support explicit named exports and imports; declarations not exported by a module are not visible to importers.
- Cache a module after successful initialization so it executes once per program.
- Detect import cycles and report the full or useful cycle path instead of recursing indefinitely.
- Detect missing modules, missing exports, duplicate imports/name conflicts and invalid syntax with source-aware diagnostics.
- Keep classes, functions, overloads and static members consistent across module boundaries.
- Never allow an imported module's entry point to run as the application's entry point.
- Preserve existing `use` syntax and current package installation behavior.

## Native async/await

### Proposed syntax

```ns
async fn fetchUser(id) {
    var response = await http.get("/users/" + id);
    return response;
}

async nix main() {
    var user = await fetchUser(42);
    println(user);
}
```

The snippet describes the intended design; it must not be documented as working until the runtime and underlying I/O support it.

### Required semantics

- `async` is a language feature, not a separately installed library.
- Calling an async function returns a task/future value; it does not silently block until completion.
- `await` is only legal in async functions and in the async entry point. Invalid use is rejected clearly.
- Await suspends the current task and allows the runtime to make progress on other ready tasks.
- Returning values, nested awaits, thrown errors, and task failures propagate deterministically.
- The runtime must define task scheduling, shutdown, unhandled failures, cancellation and timeout behavior.
- Concurrent work must have an explicit supported API; merely writing multiple `await` expressions sequentially must not be advertised as concurrent execution.
- Existing synchronous functions and existing HTTP APIs keep their current behavior unless an intentional, documented compatibility change is made.
- Async HTTP/file operations must use non-blocking I/O or a clearly bounded worker pool. Wrapping blocking I/O in a keyword alone is not sufficient.
- No detached tasks may be silently abandoned when `main` completes.

## Integration and release gates

- Modules and async functions must interoperate, including imported async functions and classes.
- Add positive and negative parser/runtime tests for every rule above.
- Preserve the existing OOP, overload-resolution, package-loader and Android build behavior.
- Build and run regression tests on desktop CI and build the Android APK.
- Add runnable examples and update README/LANGUAGE.md only after behavior is verified.
- NS1 is not release-ready until tests demonstrate real asynchronous suspension/resumption and module visibility; parsing the keywords alone does not satisfy the release gate.

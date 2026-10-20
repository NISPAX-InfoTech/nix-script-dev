# Cooperative async runtime (C++ foundation)

Nix Script now supports top-level `async fn` declarations and `await`
expressions in the C++ interpreter. Async functions return task values and run
cooperatively, advancing one statement per scheduler turn. The scheduler is
single-threaded: this does not claim parallel execution or non-blocking I/O.

## Task lifecycle

A task transitions from `Pending` to `Running`, then to exactly one terminal
state: `Succeeded`, `Failed`, or `Cancelled`. Results are available only
after success. Failure is rethrown by `Task::result()`; cancellation throws
`TaskCancelled`. The scheduler propagates awaited failures and cancellation to
the waiting task.

## One-shot work

```cpp
nix::async::Scheduler scheduler;
auto answer = scheduler.spawn<int>("answer", [] { return 42; });
scheduler.runUntil(answer);
int value = answer.result();
```

`spawn` is for work that runs to completion in one scheduler step. It is not
suitable for a long-running computation or blocking I/O because it cannot yield
until its callable returns.

## Resumable work and await

A resumable callback must retain its program counter / local state between
invocations. When `await` returns false, return from the callback immediately;
the scheduler will queue it again when the dependency completes. Reaching the
same await point again after resumption observes the completed dependency.

```cpp
nix::async::Scheduler scheduler;
nix::async::Task<int> child;
child = scheduler.spawn<int>("child", [] { return 21; });

nix::async::Task<int> parent;
parent = scheduler.create<int>("parent", [&](nix::async::TaskContext& context) {
    if (!context.await(child)) return; // suspend this step
    context.complete(parent, child.result() * 2);
});
scheduler.runUntil(parent);
int value = parent.result(); // 42
```

Use `context.yield()` to schedule the current task for another step. Avoid
blocking system calls inside callbacks. External I/O integration needs a
non-blocking event source or a bounded worker pool that posts completions back
to the scheduler.

## Safety guarantees

- A task can be completed only once.
- Direct and indirect await cycles fail rather than silently deadlock.
- `runUntil` reports when no runnable task can complete the requested task.
- Awaited errors and cancellations propagate to waiters.
- Cancellation and scheduler shutdown release callbacks and their captures.
- The scheduler is single-threaded and must not be pumped concurrently.

## Language syntax and current boundaries

```ns
async fn plusOne(value) {
    return value + 1;
}

async fn nested(value) {
    var first = await plusOne(value);
    var second = await plusOne(first);
    return second;
}

nix main() {
    var answer = await nested(40);
    println(answer); // 42
}
```

The nested-await example is covered by `tests/async_language.ns` and the
`async_language_test` CTest target. Awaiting a non-task is a runtime error.
The runtime is cooperative, not parallel; synchronous work and blocking input
still block the interpreter. External I/O futures, timers, and a worker pool are
not connected yet. Async methods and async constructors are not implemented.
The current interpreter resumes at statement boundaries; avoid putting
suspending awaits inside complex nested control-flow bodies until that
continuation path has dedicated regression coverage. Module visibility remains
a separate release blocker because modules are still flattened by the loader.

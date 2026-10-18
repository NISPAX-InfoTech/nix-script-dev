# Cooperative async runtime (C++ foundation)

This header provides the execution primitive intended to back Nix Script async
functions. It is deliberately cooperative and single-threaded: a task advances
only when the scheduler invokes its resume callback. It does not claim parallel
execution or non-blocking I/O by itself.

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

## Language integration status

This is a C++ runtime foundation, not yet the Nix Script language feature. The
parser, expression evaluator, function-call path, async entry point, and language
regression suite must be wired to these primitives before `async`/`await` can
be advertised as supported in `.ns` source files.

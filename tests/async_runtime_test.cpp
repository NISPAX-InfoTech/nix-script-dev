#include "async_runtime.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

using namespace nix::async;

int main() {
    Scheduler scheduler;
    std::vector<int> order;
    Task<int> dependency;
    int dependencySteps = 0;
    dependency = scheduler.create<int>("dependency", [&](TaskContext& ctx) {
        order.push_back(1);
        if (++dependencySteps == 1) { ctx.yield(); return; }
        ctx.complete(dependency, 21);
    });
    Task<int> parent;
    parent = scheduler.create<int>("parent", [&](TaskContext& ctx) {
        order.push_back(2);
        if (!ctx.await(dependency)) return;
        ctx.complete(parent, dependency.result() * 2);
    });
    scheduler.runUntil(parent);
    assert((order == std::vector<int>{1, 2, 1, 2}));
    assert(parent.status() == TaskStatus::Succeeded);
    assert(parent.result() == 42);

    Task<int> failed;
    failed = scheduler.create<int>("failure", [&](TaskContext& ctx) {
        ctx.fail(failed, std::runtime_error("expected failure"));
    });
    scheduler.runUntilIdle();
    bool propagated = false;
    try { (void)failed.result(); }
    catch (const std::runtime_error& error) {
        propagated = std::string(error.what()) == "expected failure";
    }
    assert(propagated);

    Task<void> cancelled;
    cancelled = scheduler.create<void>("cancelled", [&](TaskContext&) {
        assert(false && "cancelled task must not run");
    });
    scheduler.cancel(cancelled);
    scheduler.runUntilIdle();
    assert(cancelled.status() == TaskStatus::Cancelled);
    bool cancellationReported = false;
    try { cancelled.result(); }
    catch (const TaskCancelled&) { cancellationReported = true; }
    assert(cancellationReported);

    // Multiple consumers of one task must all resume when it completes.
    Task<int> shared;
    shared = scheduler.create<int>("shared", [&](TaskContext& ctx) {
        ctx.complete(shared, 7);
    });
    Task<int> consumerA, consumerB;
    consumerA = scheduler.create<int>("consumer-a", [&](TaskContext& ctx) {
        if (!ctx.await(shared)) return;
        ctx.complete(consumerA, shared.result() + 1);
    });
    consumerB = scheduler.create<int>("consumer-b", [&](TaskContext& ctx) {
        if (!ctx.await(shared)) return;
        ctx.complete(consumerB, shared.result() + 2);
    });
    scheduler.runUntilIdle();
    assert(consumerA.result() == 8);
    assert(consumerB.result() == 9);

    // Indirect wait cycles fail deterministically instead of deadlocking.
    Task<void> cycleA, cycleB;
    cycleA = scheduler.create<void>("cycle-a", [&](TaskContext& ctx) {
        if (!ctx.await(cycleB)) return;
        ctx.complete(cycleA);
    });
    cycleB = scheduler.create<void>("cycle-b", [&](TaskContext& ctx) {
        if (!ctx.await(cycleA)) return;
        ctx.complete(cycleB);
    });
    scheduler.runUntilIdle();
    assert(cycleA.status() == TaskStatus::Failed);
    assert(cycleB.status() == TaskStatus::Failed);

    // A pending dependency with no runnable producer is diagnosed, not spun on.
    Task<void> never, blocked;
    never = scheduler.create<void>("never-completes", [&](TaskContext&) {});
    blocked = scheduler.create<void>("blocked", [&](TaskContext& ctx) {
        if (!ctx.await(never)) return;
        ctx.complete(blocked);
    });
    bool deadlockReported = false;
    try { scheduler.runUntil(blocked); }
    catch (const std::runtime_error& error) {
        deadlockReported = std::string(error.what()).find("cannot make progress") != std::string::npos;
    }
    assert(deadlockReported);

    // Exceptions after a terminal transition must not corrupt the scheduler.
    Task<int> completedThenThrows;
    completedThenThrows = scheduler.create<int>("completed-then-throws", [&](TaskContext& ctx) {
        ctx.complete(completedThenThrows, 123);
        throw std::runtime_error("cleanup failure after completion");
    });
    scheduler.runUntilIdle();
    assert(completedThenThrows.status() == TaskStatus::Succeeded);
    assert(completedThenThrows.result() == 123);

    auto spawned = scheduler.spawn<int>("spawned-value", [] { return 42; });
    scheduler.runUntil(spawned);
    assert(spawned.result() == 42);
    auto spawnedVoid = scheduler.spawn<void>("spawned-void", [] {});
    scheduler.runUntil(spawnedVoid);
    assert(spawnedVoid.status() == TaskStatus::Succeeded);
    auto spawnedFailure = scheduler.spawn<int>("spawned-failure", []() -> int {
        throw std::runtime_error("spawn failure");
    });
    bool spawnFailureReported = false;
    try { scheduler.runUntil(spawnedFailure); }
    catch (const std::runtime_error& error) {
        spawnFailureReported = std::string(error.what()) == "spawn failure";
    }
    assert(spawnFailureReported);

    scheduler.shutdown();
    bool rejected = false;
    try {
        (void)scheduler.create<void>("late", [](TaskContext&) {});
    } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    return 0;
}

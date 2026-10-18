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

    scheduler.shutdown();
    bool rejected = false;
    try {
        (void)scheduler.create<void>("late", [](TaskContext&) {});
    } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    return 0;
}

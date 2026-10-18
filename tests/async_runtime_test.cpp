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
    dependency = scheduler.create<int>("dependency", [&](TaskContext& ctx) {
        order.push_back(1);
        ctx.complete(dependency, 21);
    });
    Task<int> parent;
    parent = scheduler.create<int>("parent", [&](TaskContext& ctx) {
        order.push_back(2);
        if (!ctx.await(dependency)) return;
        ctx.complete(parent, dependency.result() * 2);
    });
    scheduler.runUntilIdle();
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

    scheduler.shutdown();
    bool rejected = false;
    try {
        (void)scheduler.create<void>("late", [](TaskContext&) {});
    } catch (const std::logic_error&) { rejected = true; }
    assert(rejected);
    return 0;
}

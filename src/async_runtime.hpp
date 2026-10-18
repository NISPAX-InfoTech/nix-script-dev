#pragma once

// Small deterministic cooperative task scheduler used by the NS async runtime.
// This is deliberately thread-free: language tasks are resumed only by the
// scheduler, so shutdown and failure propagation have a single owner.
#include <any>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nix::async {

enum class TaskStatus { Pending, Running, Succeeded, Failed, Cancelled };

struct TaskState;
class Scheduler;
class TaskContext;

struct TaskState : std::enable_shared_from_this<TaskState> {
    TaskStatus status = TaskStatus::Pending;
    std::any result;
    std::exception_ptr failure;
    std::function<void(TaskContext&)> resume;
    std::vector<std::weak_ptr<TaskState>> waiters;
    std::weak_ptr<TaskState> waitingOn;
    std::string label;
    bool queued = false;
};

class TaskCancelled final : public std::runtime_error {
public:
    explicit TaskCancelled(const std::string& label)
        : std::runtime_error("Task cancelled" + (label.empty() ? std::string{} : ": " + label)) {}
};

template <typename T>
class Task {
public:
    Task() = default;
    explicit Task(std::shared_ptr<TaskState> state) : state_(std::move(state)) {}

    TaskStatus status() const noexcept {
        return state_ ? state_->status : TaskStatus::Cancelled;
    }
    bool ready() const noexcept {
        const auto s = status();
        return s == TaskStatus::Succeeded || s == TaskStatus::Failed || s == TaskStatus::Cancelled;
    }
    const std::string& label() const {
        static const std::string empty;
        return state_ ? state_->label : empty;
    }
    T result() const {
        requireState();
        if (state_->status == TaskStatus::Failed) std::rethrow_exception(state_->failure);
        if (state_->status == TaskStatus::Cancelled) throw TaskCancelled(state_->label);
        if (state_->status != TaskStatus::Succeeded)
            throw std::logic_error("Cannot read the result of a task that is not complete.");
        if constexpr (std::is_void_v<T>) {
            return;
        } else {
            return std::any_cast<T>(state_->result);
        }
    }

private:
    std::shared_ptr<TaskState> state_;
    void requireState() const {
        if (!state_) throw std::logic_error("Invalid task handle.");
    }
    friend class Scheduler;
    friend class TaskContext;
};

class TaskContext {
public:
    void yield() { enqueue_(current_); }

    bool await(const std::shared_ptr<TaskState>& dependency) {
        if (!dependency) throw std::logic_error("Cannot await an invalid task.");
        if (dependency->status == TaskStatus::Succeeded) return true;
        if (dependency->status == TaskStatus::Failed) std::rethrow_exception(dependency->failure);
        if (dependency->status == TaskStatus::Cancelled) throw TaskCancelled(dependency->label);
        // Reject direct and indirect wait cycles before adding an edge.
        for (auto cursor = dependency; cursor; cursor = cursor->waitingOn.lock()) {
            if (cursor.get() == current_.get())
                throw std::logic_error("Await dependency cycle detected.");
        }

        // Avoid registering the same waiter more than once if a resumed step
        // reaches the same await point again.
        const bool alreadyWaiting = current_->waitingOn.lock() == dependency;
        current_->waitingOn = dependency;
        if (!alreadyWaiting) dependency->waiters.emplace_back(current_);
        return false;
    }

    template <typename T>
    bool await(const Task<T>& dependency) { return await(dependency.state_); }

    template <typename T>
    void complete(const Task<T>& task, T value) {
        finish(task.state_, std::any(std::move(value)), nullptr, TaskStatus::Succeeded);
    }
    void complete(const Task<void>& task) {
        finish(task.state_, std::any{}, nullptr, TaskStatus::Succeeded);
    }
    template <typename T>
    void fail(const Task<T>& task, std::exception_ptr error) {
        if (!error) error = std::make_exception_ptr(std::runtime_error("Task failed without an exception."));
        finish(task.state_, {}, error, TaskStatus::Failed);
    }
    template <typename T, typename E,
              std::enable_if_t<!std::is_same_v<std::decay_t<E>, std::exception_ptr>, int> = 0>
    void fail(const Task<T>& task, E&& error) {
        fail(task, std::make_exception_ptr(std::forward<E>(error)));
    }

private:
    explicit TaskContext(std::shared_ptr<TaskState> current,
                         std::function<void(const std::shared_ptr<TaskState>&)> enqueue)
        : current_(std::move(current)), enqueue_(std::move(enqueue)) {}
    std::shared_ptr<TaskState> current_;
    std::function<void(const std::shared_ptr<TaskState>&)> enqueue_;

    void finish(const std::shared_ptr<TaskState>& state, std::any result,
                std::exception_ptr failure, TaskStatus status) {
        if (!state) throw std::logic_error("Invalid task handle.");
        if (state->status == TaskStatus::Succeeded || state->status == TaskStatus::Failed ||
            state->status == TaskStatus::Cancelled)
            throw std::logic_error("A task may complete only once.");
        state->result = std::move(result);
        state->failure = std::move(failure);
        state->status = status;
        state->waitingOn.reset();
        for (const auto& weak : state->waiters) {
            if (auto waiter = weak.lock()) {
                waiter->waitingOn.reset();
                enqueue_(waiter);
            }
        }
        state->waiters.clear();
    }
    friend class Scheduler;
};

class Scheduler {
public:
    Scheduler() = default;
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    ~Scheduler() { shutdown(); }

    template <typename T, typename Step>
    Task<T> create(std::string label, Step&& step) {
        ensureRunning();
        auto state = std::make_shared<TaskState>();
        state->label = std::move(label);
        state->resume = std::forward<Step>(step);
        tasks_.emplace_back(state);
        enqueue(state);
        return Task<T>(std::move(state));
    }

    bool runOne() {
        while (!ready_.empty()) {
            auto state = ready_.front();
            ready_.pop_front();
            if (!state) continue;
            state->queued = false;
            if (state->status == TaskStatus::Succeeded || state->status == TaskStatus::Failed ||
                state->status == TaskStatus::Cancelled || !state->resume) continue;
            state->status = TaskStatus::Running;
            TaskContext context(state, [this](const std::shared_ptr<TaskState>& task) { enqueue(task); });
            try {
                state->resume(context);
                if (state->status == TaskStatus::Running) state->status = TaskStatus::Pending;
            } catch (...) {
                context.fail(Task<void>(state), std::current_exception());
            }
            return true;
        }
        return false;
    }

    void runUntilIdle() {
        while (runOne()) {}
    }

    template <typename T>
    void runUntil(const Task<T>& task) {
        ensureRunning();
        while (!task.ready()) {
            if (!runOne())
                throw std::runtime_error("Task cannot make progress: pending work is waiting on an unavailable event or dependency.");
        }
        // Surface failures/cancellation at the scheduler boundary as well.
        (void)task.result();
    }

    void cancel(const Task<void>& task) { cancelState(task.state_); }

    template <typename T>
    void cancel(const Task<T>& task) { cancelState(task.state_); }

    void shutdown() noexcept {
        if (shuttingDown_) return;
        shuttingDown_ = true;
        ready_.clear();
        for (auto& weak : tasks_) {
            if (auto state = weak.lock()) {
                if (state->status == TaskStatus::Pending || state->status == TaskStatus::Running) {
                    state->status = TaskStatus::Cancelled;
                    state->resume = {};
                    state->waitingOn.reset();
                    state->waiters.clear();
                }
            }
        }
        tasks_.clear();
    }

private:
    std::deque<std::shared_ptr<TaskState>> ready_;
    std::vector<std::weak_ptr<TaskState>> tasks_;
    bool shuttingDown_ = false;

    void ensureRunning() const {
        if (shuttingDown_) throw std::logic_error("Cannot schedule work after scheduler shutdown.");
    }
    void enqueue(const std::shared_ptr<TaskState>& state) {
        if (shuttingDown_ || !state || state->queued ||
            state->status == TaskStatus::Succeeded || state->status == TaskStatus::Failed ||
            state->status == TaskStatus::Cancelled) return;
        state->queued = true;
        ready_.push_back(state);
    }
    void cancelState(const std::shared_ptr<TaskState>& state) {
        if (!state || state->status == TaskStatus::Succeeded || state->status == TaskStatus::Failed ||
            state->status == TaskStatus::Cancelled) return;
        state->status = TaskStatus::Cancelled;
        state->resume = {};
        state->waitingOn.reset();
        for (const auto& weak : state->waiters) {
            if (auto waiter = weak.lock()) enqueue(waiter);
        }
        state->waiters.clear();
    }
};

} // namespace nix::async

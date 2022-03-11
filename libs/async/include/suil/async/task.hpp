/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-06
 */

#pragma once


#include <suil/async/detail/promise.hpp>
#include <suil/async/scheduler.hpp>

namespace suil {
    template <typename T = void, bool Joinable = false>
    class task
    {
    public:
        using promise_type = detail::promise_t<task, T, Joinable>;

        task() noexcept = default;
        task(std::coroutine_handle<promise_type> coroutine) noexcept
            : _coroutine{coroutine}
        {}

        DISABLE_COPY(task);

        task(task&& other) noexcept
            : _coroutine{std::exchange(other._coroutine, nullptr)}
        {}

        task& operator=(task&& other) noexcept {
            if (this != &other) {
                // For join-able tasks, the coroutine is destroyed in the final
                // awaiter to support fire-and-forget semantics
                if constexpr (!Joinable) {
                    if (_coroutine) {
                        _coroutine.destroy();
                    }
                }
                _coroutine       = other._coroutine;
                other._coroutine = nullptr;
            }
            return *this;
        }

        ~task() noexcept {
            if constexpr (!Joinable) {
                if (_coroutine) {
                    _coroutine.destroy();
                }
            }
        }

        // The de-referencing operators below return the data contained in the
        // associated promise
        [[nodiscard]] auto operator*() noexcept
        {
            static_assert(
                    !std::is_same_v<T, void>, "This task doesn't contain any data");
            return std::ref(promise().data);
        }

        [[nodiscard]] auto operator*() const noexcept {
            static_assert(
                    !std::is_same_v<T, void>, "This task doesn't contain any data");
            return std::cref(promise().data);
        }

        // A Task is truthy if it is not associated with an outstanding
        // coroutine or the coroutine it is associated with is complete
        [[nodiscard]] operator bool() const noexcept {
            return await_ready();
        }

        [[nodiscard]] bool await_ready() const noexcept {
            return !_coroutine || _coroutine.done();
        }

        void join() {
            static_assert(Joinable,
                          "Cannot join a task without the Joinable type "
                          "parameter "
                          "set");
            _coroutine.promise().join_sem.acquire();
        }

        // When suspending from a coroutine *within* this task's coroutine, save
        // the resume point (to be resumed when the inner coroutine finalizes)
        std::coroutine_handle<> await_suspend(std::coroutine_handle<> coroutine) noexcept {
            return detail::await_suspend(_coroutine, coroutine);
        }

        // The return value of await_resume is the final result of `co_await
        // this_task` once the coroutine associated with this task completes
        auto await_resume() const noexcept {
            if constexpr (std::is_same_v<T, void>) {
                return;
            }
            else {
                return std::move(promise().data);
            }
        }

    protected:
        [[nodiscard]] promise_type& promise() const noexcept {
            return _coroutine.promise();
        }

        std::coroutine_handle<promise_type> _coroutine = nullptr;
    };

    /**
     * Suspend the current coroutine to be scheduled for execution on a different
     * thread by the supplied scheduler. Remember to `co_await` this function's
     * returned value.
     *
     * The least significant bit of the CPU mask, corresponds to CPU 0. A non-zero
     * mask will prevent this coroutine from being scheduled on CPUs corresponding
     * to bits that are set
     *
     * Thread safe only if Scheduler::schedule is thread safe (the default one
     * provided is thread safe).
     */
    template <SchedulerInf S = Scheduler>
    inline auto schedule(S& scheduler,
                         uint16 tid) noexcept {
        struct awaiter_t {
            Scheduler &scheduler;
            uint16 tid;

            bool await_ready() const noexcept {
                return false;
            }

            void await_resume() const noexcept {
            }

            void await_suspend(std::coroutine_handle<> coroutine) const noexcept {
                scheduler.schedule(coroutine, tid);
            }
        };

        return awaiter_t{scheduler, tid};
    }

    inline auto schedule(uint16 tid = AFFINITY_ANY) {
        return schedule(Scheduler::instance(), tid);
    }

    template <bool Joinable = true>
    using VoidTask = task<void, Joinable>;

    template <typename T = void>
    using JoinableTask = task<T, true>;

    template <typename T = void>
    using Task = task<T, false>;
}
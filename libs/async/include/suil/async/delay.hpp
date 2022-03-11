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

#include <suil/async/coroutine.hpp>

#include <optional>
#include <set>

namespace suil {
    struct Event;

    class Timer {
    public:
        typedef enum {
            tsCREATED,
            tsSCHEDULED,
            tsABANDONED,
            tsFIRED
        } State;

        struct Entry {
            steady_clock::time_point timerDeadline;
            Event *targetEvent{nullptr};
            Timer *targetTimer{nullptr};

            bool operator<(const Entry& rhs) const;

            bool operator<=(const std::chrono::steady_clock::time_point& tp) const {
                return timerDeadline <= tp;
            }

        };
        using List = std::set<Entry>;
        using Handle = List::iterator;

        Timer(milliseconds timeout, uint16_t tid = AFFINITY_ANY);
        ~Timer() noexcept;

        MOVE_CTOR(Timer) noexcept;
        MOVE_ASSIGN(Timer) noexcept;

        DISABLE_COPY(Timer);

        bool await_ready() const noexcept { return _state == tsFIRED; }

        void await_suspend(std::coroutine_handle<> coroutine) noexcept;

        bool await_resume() noexcept { return _state.exchange(tsCREATED) == tsFIRED; }

    private:
        friend struct Scheduler;
        friend struct Thread;
        std::atomic<State> _state{tsCREATED};
        milliseconds _timeout{0ms};
        std::coroutine_handle<> _coro{nullptr};
        std::optional<Handle> _handle{};
        uint16 _tid{AFFINITY_ANY};
    };

    inline auto asyncDelay(milliseconds ms) {
        return Timer{ms};
    }
}

#define delay(ms) do { auto ret = co_await suil::asyncDelay(ms); SUIL_ASSERT(ret); } while (0)

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
#include <suil/async/detail/list.hpp>

#include <optional>
#include <set>
#include <variant>

namespace suil {
    struct Event;
    class  Delay;

    struct Timer {
        struct mill_list_item item{};
        int64_t dd{0};
        std::variant<std::monostate, Event*, Delay*> target{};
        inline operator bool() const { return !holds_alternative<std::monostate>(target); }
    };

    class Delay {
    public:
        typedef enum {
            tsCREATED,
            tsSCHEDULED,
            tsABANDONED,
            tsFIRED
        } State;

        Delay(int64_t timeout, uint16_t tid = THREAD_ID_ANY);
        ~Delay() noexcept;

        MOVE_CTOR(Delay) noexcept;
        MOVE_ASSIGN(Delay) noexcept;

        DISABLE_COPY(Delay);

        bool await_ready() const noexcept { return _state == tsFIRED; }

        void await_suspend(std::coroutine_handle<> coroutine) noexcept;

        bool await_resume() noexcept { return _state.exchange(tsCREATED) == tsFIRED; }

    private:
        friend struct Scheduler;
        friend struct Thread;
        Timer _timer{};
        std::atomic<State> _state{tsCREATED};
        std::coroutine_handle<> _coro{nullptr};
        uint16 _tID{THREAD_ID_ANY};
    };

    inline auto asyncDelay(milliseconds ms) {
        return Delay{ms.count()};
    }
}

#define delay(ms) do { auto ret = co_await suil::asyncDelay(ms); SUIL_ASSERT(ret); } while (0)

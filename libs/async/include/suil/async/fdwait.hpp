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
#include <suil/async/delay.hpp>

#include <optional>

namespace suil {

    struct Event {
        typedef enum {
            esCREATED,
            esFIRED,
            esERROR,
            esSCHEDULED,
            esABANDONED,
            esTIMEOUT,
        } State;

        typedef enum {
            IN,
            OUT
        } IO;

        struct Handle {
            int fd{INVALID_FD};
            uint16_t tid{AFFINITY_ANY};
            std::atomic<State> state{esCREATED};
            steady_clock::time_point eventDeadline{};
            IO  ion{IN};
            std::optional<Timer::Handle> timerHandle{std::nullopt};
            std::coroutine_handle<> coro{nullptr};
        };

        Event(int fd, uint16_t affinity = AFFINITY_ANY) noexcept;

        ~Event() noexcept;

        MOVE_CTOR(Event) noexcept;

        MOVE_ASSIGN(Event) noexcept;

        DISABLE_COPY(Event);

        bool await_ready() const noexcept {
            return _handle.state == esFIRED;
        }

        void await_suspend(std::coroutine_handle<> coroutine) noexcept;

        State await_resume() noexcept {
            _handle.coro = nullptr;
            return _handle.state.exchange(esCREATED);
        }

        Event& operator()(steady_clock::time_point dd) {
            SUIL_ASSERT(_handle.state == esCREATED);
            _handle.eventDeadline = dd;
            return Ego;
        }

        Event& operator()(IO ion) {
            SUIL_ASSERT(_handle.state == esCREATED);
            _handle.ion = ion;
            return Ego;
        }

        uint16 affinity() const { return _handle.tid; }
        Handle& handle() { return _handle; }

    private:
        friend class Scheduler;
        Handle _handle{};
    };

    inline auto fdwait(int fd, Event::IO io, steady_clock::time_point dd = {})
    {
        Event event(fd, AFFINITY_ANY);
        event(io)(dd);
        return event;
    }
}
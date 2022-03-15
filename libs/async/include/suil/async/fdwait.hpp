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
            Timer timerHandle;
            int fd{INVALID_FD};
            uint16_t tid{THREAD_ID_ANY};
            std::atomic<State> state{esCREATED};
            IO  ion{IN};
            std::coroutine_handle<> coro{nullptr};
        };

        explicit Event(int fd, uint16_t affinity = THREAD_ID_ANY) noexcept;

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

        Event& operator()(int64_t dd) {
            SUIL_ASSERT(_handle.state == esCREATED);
            _handle.timerHandle.dd = dd;
            return Ego;
        }

        Event& operator()(IO ion) {
            SUIL_ASSERT(_handle.state == esCREATED);
            _handle.ion = ion;
            return Ego;
        }

        [[nodiscard]] uint16 tid() const { return _handle.tid; }
        Handle& handle() { return _handle; }

    private:
        friend class Scheduler;
        Handle _handle{};
    };

    inline auto fdwait(int fd, Event::IO io, int64_t dd = -1, uint16 affinity = THREAD_ID_ANY)
    {
        Event event(fd, affinity);
        event(io)(dd);
        return event;
    }
}
/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-06
 */

#include "suil/async/fdwait.hpp"
#include "suil/async/scheduler.hpp"

namespace suil {

    Event::Event(int fd, uint16_t affinity) noexcept
            : _handle{{}, fd, affinity}
    {
        SUIL_ASSERT(fd != INVALID_FD && "Poll events needs a valid file descriptor");
    }

    Event::Event(Event &&other) noexcept
    {
        *this = std::move(other);
    }

    Event& Event::operator=(Event &&other) noexcept
    {
        if (this != &other) {
            SUIL_ASSERT(_handle.state == esCREATED);
            _handle.state = other._handle.state.exchange(esABANDONED);
            _handle.tid = std::exchange(other._handle.tid, 0);
            _handle.fd = std::exchange(other._handle.fd, INVALID_FD);
            _handle.timerHandle = std::exchange(other._handle.timerHandle, {});
            _handle.coro = std::exchange(other._handle.coro, nullptr);
        }

        return *this;
    }

    Event::~Event() noexcept
    {
        // event must not be scheduled when it is destroyed
        SUIL_ASSERT(_handle.state == esCREATED);
    }

    void Event::await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        SUIL_ASSERT(_handle.state == esCREATED);
        _handle.coro = coroutine;
        Scheduler::instance().schedule(this, _handle.tid);
    }
}
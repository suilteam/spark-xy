/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * @author Carter
 * @date 2022-03-06
 */

#include "suil/async/delay.hpp"
#include "suil/async/scheduler.hpp"

namespace suil {

    Delay::Delay(int64_t timeout, uint16_t tid)
        : _tID{tid}
    {
        _timer.dd = fastnow() + timeout;
    }

    Delay::Delay(Delay &&other) noexcept
    {
        Ego = std::move(other);
    }

    Delay& Delay::operator=(Delay &&other) noexcept
    {
        if (this != &other) {
            SUIL_ASSERT(_state == tsCREATED);
            _state = other._state.exchange(tsABANDONED);
            _coro = std::exchange(other._coro, nullptr);
            _tID = std::exchange(other._tID, 0);
            _timer = std::exchange(other._timer, {});
        }

        return Ego;
    }

    Delay::~Delay() noexcept
    {
        SUIL_ASSERT(_state == tsCREATED);
    }

    void Delay::await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        SUIL_ASSERT(_state == tsCREATED);
        _coro = coroutine;
        Scheduler::instance().schedule(this, _tID);
    }
}
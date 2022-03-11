/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-09
 */

#pragma once

#include <suil/utils/utils.hpp>
#include <suil/async/fdwait.hpp>
#include <suil/async/detail/concurrentqueue.h>

namespace suil {

    class Thread {
        struct Stats {
            std::atomic<uint64> inflight{0};
            std::atomic<uint64> maxInflight{0};
            std::atomic<uint64> totalQueued{0};
            uint64 maxPolled{0};
        };
    public:
        Thread(uint16 id);

        DISABLE_COPY(Thread);
        DISABLE_MOVE(Thread);

        void start();
        Stats getStats() const;
        void abort();
        bool isActive() const { return _active; }
        void schedule(std::coroutine_handle<> coro);
        bool add(Event *event);
        void add(Timer *timer);
        void remove(Event *event);
        void remove(Timer *timer);
        uint64 load() const;
        ~Thread();

    private:
        void signal();
        void handleEvent(Event *event);
        void cancelTimer(Timer::Handle handle);
        Timer::Handle addTimer(Timer::Entry entry);
        void fireExpiredTimers();
        int computeWaitTimeout();
        void handleThreadEvent();
        void record();
        Timer::List _timers;
        std::mutex _timersLock;
        uint16_t _id{0};
        Stats _stats{};
        std::atomic<bool> _active{false};
        std::atomic<bool> _signaling{false};
        std::thread _thread;
        int _epfd{INVALID_FD};
        int _evfd{INVALID_FD};
        moodycamel::ConcurrentQueue<std::coroutine_handle<>> _scheduleQ;
    };
}
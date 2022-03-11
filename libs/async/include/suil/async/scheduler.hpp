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
#include <suil/async/thread.hpp>
#include <suil/async/detail/concurrentqueue.h>


namespace suil {
    struct Event;

    template<typename S>
    concept SchedulerInf = requires(S scheduler,
                                    std::coroutine_handle<> coroutine,
                                    uint16 cpu)
    {
        scheduler.schedule(coroutine, cpu);
    };

    class Scheduler {
    public:
        DISABLE_MOVE(Scheduler);
        DISABLE_COPY(Scheduler);

        static Scheduler& instance();
        static void init(uint16 threadCount);
        void schedule(std::coroutine_handle<> coro, uint16 tid = AFFINITY_ANY);
        void schedule(Event *event, uint16 tid = AFFINITY_ANY);
        void schedule(Timer *timer, uint16 tid = AFFINITY_ANY);
        void dumpStats();
        ~Scheduler();
    private:
        Scheduler() = default;
        uint16 minLoadSchedule();
        void run(uint16 threadCount);
        Thread *_threads{nullptr};
        uint16 _threadCount{0};
        std::atomic_bool _initialized{false};
        std::atomic<uint64> _totalScheduled{0};
    };
}
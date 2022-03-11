/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-06
 */

#include "suil/async/scheduler.hpp"
#include "suil/async/thread.hpp"
#include "suil/async/fdwait.hpp"

#include <sys/eventfd.h>
#include <sys/epoll.h>

#ifndef SUIL_ASYNC_MAXIMUM_CONCURRENCY
#define SUIL_ASYNC_MAXIMUM_CONCURRENCY 64u
#endif

namespace suil {

    Scheduler &Scheduler::instance()
    {
        static Scheduler scheduler;
        return scheduler;
    }

    void Scheduler::run(uint16 threadCount)
    {
        auto concurrency = std::thread::hardware_concurrency();
        SUIL_ASSERT(concurrency >= threadCount);
        _threadCount = (threadCount == 0? concurrency : threadCount);
        if (unlikely(_threadCount > SUIL_ASYNC_MAXIMUM_CONCURRENCY)) {
            ASYNC_TRACE("[%zu]: async concurrency capped to %u",
                        ASYNC_LOCATION, tid(), SUIL_ASYNC_MAXIMUM_CONCURRENCY);
            _threadCount = SUIL_ASYNC_MAXIMUM_CONCURRENCY;
        }

        ASYNC_TRACE("[%zu]: spawning scheduler with %u threads", ASYNC_LOCATION, tid(), _threadCount);

        void *raw = operator new[](sizeof(Thread) * _threadCount);
        _threads = static_cast<Thread *>(raw);
        for (auto i = 0; i != _threadCount; i++) {
            new(_threads +i) Thread(i);
            _threads[i].start();
        }

        auto dd = after(60s);
        for (auto i = 0; i < _threadCount; i++) {
            while (!_threads[i].isActive() &&  (now() < dd)) {
                std::this_thread::sleep_for(500ms);
            }
            SUIL_ASSERT(_threads[i].isActive());
        }
    }

    void Scheduler::init(uint16 threadCount)
    {
        bool expected{false};
        if (Scheduler::instance()._initialized.compare_exchange_weak(expected, true)) {
            Scheduler::instance().run(threadCount);
        }
    }

    void Scheduler::schedule(std::coroutine_handle<> coro, uint16 tid)
    {
        if (tid == AFFINITY_ANY) {
            tid = minLoadSchedule();
        }
        SUIL_ASSERT(tid < _threadCount);
        _threads[tid].schedule(coro);
        _totalScheduled++;
    }

    void Scheduler::schedule(Event *event, uint16 tid)
    {
        if (tid == AFFINITY_ANY) {
            tid = minLoadSchedule();
        }

        SUIL_ASSERT(tid < _threadCount);
        _threads[tid].add(event);
        _totalScheduled++;
    }

    void Scheduler::schedule(Timer *timer, uint16 tid)
    {
        if (tid == AFFINITY_ANY) {
            tid = minLoadSchedule();
        }
        SUIL_ASSERT(tid < _threadCount);
        _threads[tid].add(timer);
        _totalScheduled++;
    }

    uint16 Scheduler::minLoadSchedule()
    {
        uint64 minLoad = UINT64_MAX;
        uint16 tid = 0;
        for (int i = 0; i < _threadCount; i++) {
            auto load = _threads[i].load();
            if (load == 0) return i;

            if (load < minLoad) {
                minLoad = load;
                tid = i;
            }
        }

        return tid;
    }

    void Scheduler::dumpStats()
    {
        if (!_initialized) return;
        std::printf("Queue Statistics\n");
        std::printf("Total scheduled: %lu\n", _totalScheduled.load());
        std::printf("| Queue | Inflight | MaxInflight | TotalQueued | MaxPolled | Usage  |\n");
        std::printf("|-------+----------+-------------+-------------+-----------+--------|\n");
        for (int i = 0; i < _threadCount; i++) {
            std::printf("| %5d | ", i);
            auto stats = _threads[i].getStats();
            std::printf("%8lu | %11lu | %11lu | %9lu | %7.2f |\n",
                        stats.inflight.load(), stats.maxInflight.load(), stats.totalQueued.load(),
                        stats.maxPolled, float(stats.totalQueued * 100) / float(_totalScheduled));
        }
    }

    Scheduler::~Scheduler()
    {
        if (_threads != nullptr) {
            for (auto i = 0u; i != +_threadCount; i++) {
                _threads[i].~Thread();
            }
            operator delete[](static_cast<void *>(_threads));
            _threads = nullptr;
        }
    }
}
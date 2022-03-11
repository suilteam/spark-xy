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
#define SUIL_ASYNC_MAXIMUM_CONCURRENCY 512u
#endif

namespace suil {

    Scheduler::Scheduler()
    {}

    void Scheduler::run(int cpuCount)
    {
        auto concurrency = std::thread::hardware_concurrency();
        SUIL_ASSERT(concurrency >= cpuCount);
        _cpuCount = (cpuCount == 0? concurrency : cpuCount);
        if (unlikely(_cpuCount > SUIL_ASYNC_MAXIMUM_CONCURRENCY)) {
            ASYNC_TRACE("[%zu]: async concurrency capped to %u", ASYNC_LOCATION, tid(), SUIL_ASYNC_MAXIMUM_CONCURRENCY);
            _cpuCount = SUIL_ASYNC_MAXIMUM_CONCURRENCY;
        }

        _cpuMask = (1 << (_cpuCount + 1)) - 1;
        ASYNC_TRACE("[%zu]: spawning scheduler with %u threads", ASYNC_LOCATION, tid(), _cpuCount);

        void *raw = operator new[](sizeof(WorkQueue) * _cpuCount);
        _queues = static_cast<WorkQueue *>(raw);
        for (auto i = 0; i != _cpuCount; i++) {
            new(_queues +i) WorkQueue(*this, (i+1));
        }

        _epfd = epoll_create1(EPOLL_CLOEXEC);
        SUIL_ASSERT(_epfd != INVALID_FD);

        _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        SUIL_ASSERT(_evfd != INVALID_FD);

        struct epoll_event ev{.events = EPOLLERR | EPOLLIN | EPOLLHUP, .data = {.fd = _evfd}};
        int rc = epoll_ctl(_epfd, EPOLL_CTL_ADD, _evfd, &ev);
        SUIL_ASSERT(rc != -1);

        _eventThread = std::thread([this] {
            _active = true;
            while (_active) {
                // wait for events on the event loop
                if (waitForEvents()) {
                    // check expired timers
                    scheduleExpiredTimers();
                }
            }

            if (_evfd != INVALID_FD) {
                ::close(_evfd);
                _evfd = INVALID_FD;
            }
            if (_epfd != INVALID_FD) {
                ::close(_epfd);
                _evfd = INVALID_FD;
            }
        });

        auto maxWait = 6;
        while (!_active && maxWait-- > 0) {
            std::this_thread::sleep_for(milliseconds{500});
        }
        SUIL_ASSERT(_active);
    }

    void Scheduler::schedule(std::coroutine_handle<> coroutine,
                             uint64_t affinity,
                             uint32_t priority,
                             AsyncLocation location)
    {
        if (affinity == 0) {
            affinity = ~affinity & _cpuMask;
        }

        uint64 minInflight = UINT64_MAX;
        uint32_t q{0};
        for (auto i = 0u; i != _cpuCount; i++) {
            auto inflight = _queues[i].size_approx();
            if (inflight == 0) {
                q = i;
                break;
            }

            if (inflight < minInflight) {
                minInflight = inflight;
                q = i;
            }
        }

        ASYNC_TRACEn(2, "[%zu] scheduling coro into queue %u", ASYNC_LOCATION, tid(), q);
        _totalScheduled++;
        _queues[q].enqueue(coroutine, priority, location);
    }

    void Scheduler::relocate(std::coroutine_handle<> coroutine,
                             uint16_t cpu,
                             AsyncLocation location)
    {
        SUIL_ASSERT(cpu < _cpuCount);
        ASYNC_TRACEn(2, "[%zu] scheduling coro into empty queue %u", ASYNC_LOCATION, tid(), cpu);
        _queues[cpu].enqueue(coroutine, PRIO_0, location);
    }

    bool Scheduler::schedule(Event *event)
    {
        auto& handle = event->_handle;
        SUIL_ASSERT((handle.state != Event::esSCHEDULED) &&
                    (handle.fd != INVALID_FD) &&
                    (handle.coro != nullptr));

        int op = EPOLL_CTL_ADD, ec = 0;

        TRY_OP:
        struct epoll_event ev {
                .events = EPOLLHUP | EPOLLERR | (handle.ion == Event::IN ? EPOLLIN : EPOLLOUT),
                .data = {.ptr = event }
        };

        ec = epoll_ctl(_epfd, op,handle.fd, &ev);
        if (ec == 0) {
            if (handle.eventDeadline != steady_clock::time_point{}) {
                handle.timerHandle = addTimer({.timerDeadline = handle.eventDeadline, .targetEvent = event});
            }
            handle.state = Event::esSCHEDULED;
            return true;
        }
        if (errno == EEXIST) {
            // already added, modify
            op = EPOLL_CTL_MOD;
            goto TRY_OP;
        }

        return false;
    }

    void Scheduler::schedule(Timer *timer)
    {
        SUIL_ASSERT((timer->_state != Timer::tsSCHEDULED) &&
                    (timer->_coro != nullptr));
        timer->_state = Timer::tsSCHEDULED;
        timer->_handle =
                addTimer({.timerDeadline = now() + timer->_timeout, .targetTimer = timer});
    }

    void Scheduler::unschedule(Event *event)
    {
        auto& handle = event->_handle;
        auto state = Event::esSCHEDULED;
        if (handle.state.compare_exchange_weak(state, Event::esABANDONED)) {
            // allow only 1 thread to unschedule event
            if (handle.timerHandle) {
                cancelTimer(*handle.timerHandle);
                handle.timerHandle = std::nullopt;
            }
            struct epoll_event ev{};
            epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);
            schedule(handle.coro, handle.affinity, handle.priority);
        }
    }

    void Scheduler::unschedule(Timer *timer)
    {
        auto state = Timer::tsSCHEDULED;
        if (timer->_state.compare_exchange_weak(state, Timer::tsSCHEDULED)) {
            // only 1 thread allowed to unschedule event
            SUIL_ASSERT(timer->_state == Timer::tsSCHEDULED);
            if (timer->_handle) {
                cancelTimer(*timer->_handle);
                timer->_handle = std::nullopt;
            }
            schedule(timer->_coro, timer->_affinity, timer->_priority);
        }
    }

    bool Scheduler::waitForEvents()
    {
        struct epoll_event events[SUIL_ASYNC_MAXIMUM_CONCURRENCY];
        auto count = epoll_wait(_epfd,
                                events,
                                SUIL_ASYNC_MAXIMUM_CONCURRENCY,
                                computeWaitTimeout());
        if (!_active) {
            return false;
        }

        if (count == -1) {
            SUIL_ASSERT(errno == EINTR);
        }

        int found{0};
        for (int i = 0; i < count; i++) {
            auto& triggered = events[i];
            if (triggered.data.fd == _evfd) {
                handleThreadEvent();
                continue;
            }

            auto event = static_cast<Event *>(events[i].data.ptr);
            auto& handle = event->_handle;
            auto state = Event::esSCHEDULED;
            if (handle.state.compare_exchange_weak(state, Event::esFIRED)) {
                if (triggered.events & (EPOLLERR | EPOLLHUP)) {
                    handle.state = Event::esERROR;
                } else {
                    auto ev = (handle.ion == Event::IN ? EPOLLIN : EPOLLOUT);
                    SUIL_ASSERT((triggered.events & ev) == ev);
                    handle.state = Event::esFIRED;
                }
                found++;
                handleEvent(event);
            }
        }
        _maxEventsPolled = std::max(_maxEventsPolled, std::uint64_t(found));
        return true;
    }

    void Scheduler::handleEvent(Event *event)
    {
        auto& handle =event->_handle;

        if (handle.timerHandle) {
            cancelTimer(*handle.timerHandle);
            handle.timerHandle = std::nullopt;
        }

        struct epoll_event ev{};
        epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);
        schedule(handle.coro, handle.affinity, handle.priority);
    }

    void Scheduler::handleThreadEvent() const
    {
        eventfd_t count{0};
        auto nrd = read(_evfd, &count, sizeof(count));
        SUIL_ASSERT(nrd == sizeof(count));
    }

    void Scheduler::signalThread()
    {
        bool expected = false;
        // only need to signal EPOLL once
        if (_threadSignaling.compare_exchange_weak(expected, true)) {
            eventfd_t count{0x01};
            auto nwr = ::write(_evfd, &count, sizeof(count));
            SUIL_ASSERT(nwr == 8);
            _threadSignaling = false;
        }
    }

    void Scheduler::cancelTimer(Timer::Handle handle)
    {
        std::lock_guard<std::mutex> lg(_timersLock);
        if (handle != _timers.end()) {
            _timers.erase(handle);
        }
    }

    Timer::Handle Scheduler::addTimer(Timer::Entry entry)
    {
        _timersLock.lock();
        auto it = _timers.insert(entry);
        _timersLock.unlock();

        signalThread();

        SUIL_ASSERT(it.second);
        return it.first;
    }

    void Scheduler::scheduleExpiredTimers()
    {
        auto tp = now();

        _timersLock.lock();
        while (!_timers.empty()) {
            auto it = _timers.begin();
            if (it->timerDeadline <= tp) {
                auto handle = *it;
                _timers.erase(it);
                _timersLock.unlock();
                if (handle.targetEvent) {
                    auto state = Event::esSCHEDULED;
                    auto& event = handle.targetEvent->_handle;
                    if (event.state.compare_exchange_weak(state, Event::esTIMEOUT)) {
                        event.timerHandle = std::nullopt;
                        struct epoll_event ev{};
                        epoll_ctl(_epfd, EPOLL_CTL_DEL, event.fd, &ev);
                        schedule(event.coro, event.affinity, event.priority);
                    }
                }
                else {
                    auto& timer = *handle.targetTimer;
                    auto state = Timer::tsSCHEDULED;
                    if (timer._state.compare_exchange_weak(state, Timer::tsFIRED)) {
                        timer._handle = std::nullopt;
                        schedule(timer._coro, timer._affinity, timer._affinity);
                    }
                }
                _timersLock.lock();
            } else {
                break;
            }
        }
        _timersLock.unlock();
    }

    int Scheduler::computeWaitTimeout()
    {
        auto it = _timers.begin();
        if (it != _timers.end()) {
            auto at = std::chrono::duration_cast<std::chrono::milliseconds>(it->timerDeadline - now()).count();
            if (at < 0) {
                return 0;
            }
            return int(at);
        }

        return -1;
    }

    Scheduler& Scheduler::instance() noexcept
    {
        static Scheduler scheduler;
        return scheduler;
    }

    void Scheduler::init(int cpuCount)
    {
        SUIL_ASSERT(!Scheduler::instance()._active);
        Scheduler::instance().run(cpuCount);
    }

    void Scheduler::abort()
    {
        bool isActive{true};
        if (Scheduler::instance()._active.compare_exchange_weak(isActive, false)) {
            Scheduler::instance().signalThread();
        }
    }

    void Scheduler::dumpStats()
    {
        if (!_active) return;
        std::printf("Queue Statistics\n");
        std::printf("Maximum events polled: %lu\n", _maxEventsPolled);
        std::printf("| Queue | Inflight | MaxInflight | TotalHandled |  Usage  |\n");
        std::printf("|-------+----------+-------------+--------------+---------|\n");
        for (int i = 0; i < _cpuCount; i++) {
            std::printf("| %5d | ", i);
            auto stats = _queues[i].getStats();
            std::printf("%8lu | %11lu | %12lu | %7.2f |\n",
                        stats.inflight.load(), stats.maxInflight.load(), stats.totalHandled,
                        float(stats.totalHandled*100)/float(_totalScheduled));
        }
    }

    Scheduler::~Scheduler() noexcept
    {
        Scheduler::abort();
        if (_eventThread.joinable()) {
            _eventThread.join();
        }

        if (_queues != nullptr) {
            for (auto i = 0u; i != +_cpuCount; i++) {
                _queues[i].~WorkQueue();
            }
            operator delete[](static_cast<void *>(_queues));
            _queues = nullptr;
        }
    }
}
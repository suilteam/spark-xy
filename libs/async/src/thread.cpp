/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-08
 */

#include "suil/async/thread.hpp"

#include <sys/eventfd.h>
#include <sys/epoll.h>

#ifndef SUIL_ASYNC_MAXIMUM_CONCURRENCY
#define SUIL_ASYNC_MAXIMUM_CONCURRENCY 256u
#endif

namespace suil {

    static __thread int16 QueueId = -1;

    int16 qid() {
        return QueueId;
    }

    Thread::Thread(uint16 id)
        :_id{id}
    {
        mill_list_init(&_timers);
    }

    void Thread::start()
    {
        _thread = std::thread([this] {
            QueueId = int16(_id);
            pthread_t thread = pthread_self();
            char label[64];
            snprintf(label, sizeof(label), "Async-Thread/%i", _id);
            int result = pthread_setname_np(thread, label);
            SUIL_ASSERT(result == 0);

            ASYNC_TRACE("[%zu] queue #%d created", ASYNC_LOCATION, tid(), _id);

            _epfd = epoll_create1(EPOLL_CLOEXEC);
            SUIL_ASSERT(_epfd != INVALID_FD);

            _evfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
            SUIL_ASSERT(_evfd != INVALID_FD);

            struct epoll_event evfd{.events = EPOLLERR | EPOLLIN | EPOLLHUP, .data = {.fd = _evfd}};
            int rc = epoll_ctl(_epfd, EPOLL_CTL_ADD, _evfd, &evfd);
            SUIL_ASSERT(rc != -1);

            _active = true;
            while (_active) {
                struct epoll_event events[SUIL_ASYNC_MAXIMUM_CONCURRENCY];
                auto count = epoll_wait(_epfd,
                                        events,
                                        SUIL_ASYNC_MAXIMUM_CONCURRENCY,
                                        computeWaitTimeout());
                if (!_active) {
                    break;
                }

                if (count == -1) {
                    SUIL_ASSERT(errno == EINTR);
                }

                {
                    std::coroutine_handle<> coro;
                    while (_active && _scheduleQ.try_dequeue(coro)) {
                        // resume all coroutines scheduled to this queue
                        _stats.inflight--;
                        coro.resume();
                    }
                }

                int found{0};
                for (int i = 0; _active && (i < count); i++) {
                    auto &triggered = events[i];
                    if (triggered.data.fd == _evfd) {
                        handleThreadEvent();
                        continue;
                    }

                    auto event = static_cast<Event *>(events[i].data.ptr);
                    auto &handle = event->handle();
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
                _stats.maxPolled = std::max(_stats.maxPolled, std::uint64_t(found));
                if (_active) {
                    fireExpiredTimers();
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
    }

    void Thread::handleEvent(Event *event)
    {
        auto& handle =event->handle();
        auto coro = handle.coro;
        if (handle.timerHandle) {
            cancelTimer(handle.timerHandle);
            handle.timerHandle = {};
        }

        struct epoll_event ev{};
        epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);
        _stats.inflight--;
        coro.resume();
    }

    void Thread::handleThreadEvent()
    {
        eventfd_t count{0};
        auto nrd = read(_evfd, &count, sizeof(count));
        SUIL_ASSERT(nrd == sizeof(count));
    }

    void Thread::signal()
    {
        bool expected = false;
        // only need to signal EPOLL once
        if (_signaling.compare_exchange_weak(expected, true)) {
            eventfd_t count{0x01};
            auto nwr = ::write(_evfd, &count, sizeof(count));
            SUIL_ASSERT(nwr == 8);
            _signaling = false;
        }
    }

    bool Thread::add(Event *event)
    {
        auto& handle = event->handle();
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
            if (handle.timerHandle.dd > 0) {
                handle.timerHandle.target = event;
                addTimer(handle.timerHandle);
            }
            handle.state = Event::esSCHEDULED;
            record();
            return true;
        }
        if (errno == EEXIST) {
            // already added, modify
            op = EPOLL_CTL_MOD;
            goto TRY_OP;
        }

        return false;
    }

    void Thread::add(Delay *dly)
    {
        SUIL_ASSERT((dly->_state != Delay::tsSCHEDULED) &&
                    (dly->_coro != nullptr));
        dly->_state = Delay::tsSCHEDULED;
        dly->_timer.target = dly;
        addTimer(dly->_timer);
        record();
    }

    void Thread::schedule(std::coroutine_handle<> coro)
    {
        _scheduleQ.enqueue(coro);
        record();
        signal();
    }

    void Thread::record()
    {
        _stats.totalQueued++;
        _stats.inflight++;
        _stats.maxInflight = std::max(_stats.inflight.load(), _stats.maxInflight.load());
    }

    void Thread::remove(Event *event)
    {
        auto& handle = event->handle();
        auto state = Event::esSCHEDULED;
        if (handle.state.compare_exchange_weak(state, Event::esABANDONED)) {
            // allow only 1 thread to unschedule event
            if (handle.timerHandle) {
                cancelTimer(handle.timerHandle);
                handle.timerHandle = {};
            }
            struct epoll_event ev{};
            epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);

            _stats.inflight--;
            handle.coro.resume();
        }
    }

    void Thread::remove(Delay *dly)
    {
        auto state = Delay::tsSCHEDULED;
        if (dly->_state.compare_exchange_weak(state, Delay::tsABANDONED)) {
            cancelTimer(dly->_timer);
            dly->_timer = {};
            _stats.inflight--;
            dly->_coro.resume();
        }
    }

    uint64 Thread::load() const
    {
        return _stats.inflight;
    }

    void Thread::addTimer(Timer& timer)
    {
        _timersLock.lock();
        auto it = mill_list_begin(&_timers);
        while (it) {
            auto tm = mill_cont(it, Timer, item);
            if (timer.dd < tm->dd)
                break;
            it = mill_list_next(it);
        }
        mill_list_insert(&_timers, &timer.item, it);
        _timersLock.unlock();

        signal();
    }

    void Thread::cancelTimer(Timer& handle)
    {
        std::lock_guard<std::mutex> lg(_timersLock);
        if (handle) {
            mill_list_erase(&_timers, &handle.item);
        }
    }

    void Thread::fireExpiredTimers()
    {
        auto tp = fastnow();

        _timersLock.lock();
        while (!mill_list_empty(&_timers)) {
            auto it = mill_cont(mill_list_begin(&_timers), Timer, item);
            if (it == nullptr || it->dd > tp) {
                break;
            }

            mill_list_erase(&_timers, mill_list_begin(&_timers));
            _timersLock.unlock();
            if (holds_alternative<Event*>(it->target)) {
                auto state = Event::esSCHEDULED;
                auto& event = get<Event*>(it->target)->handle();
                if (event.state.compare_exchange_weak(state, Event::esTIMEOUT)) {
                    struct epoll_event ev{};
                    epoll_ctl(_epfd, EPOLL_CTL_DEL, event.fd, &ev);

                    _stats.inflight--;
                    event.coro.resume();
                }
            }
            else if (holds_alternative<Delay*>(it->target)) {
                auto& timer = *get<Delay*>(it->target);
                auto state = Delay::tsSCHEDULED;
                if (timer._state.compare_exchange_weak(state, Delay::tsFIRED)) {
                    _stats.inflight--;
                    timer._coro.resume();
                }
            }
            else {
                SUIL_ASSERT(false);
            }
            _timersLock.lock();
        }
        _timersLock.unlock();
    }

    int Thread::computeWaitTimeout()
    {
        std::lock_guard<std::mutex> lg{_timersLock};

        if (!mill_list_begin(&_timers)) {
            return -1;
        }

        auto tm = mill_cont(mill_list_begin(&_timers), Timer, item);
        auto at =  tm->dd - fastnow();
        if (at <= 0) {
            return 0;
        }
        return int(at);
    }

    Thread::Stats Thread::getStats() const
    {
        return  {
            _stats.inflight.load(),
            _stats.maxInflight.load(),
            _stats.totalQueued.load(),
            _stats.maxPolled,
        };
    }

    void Thread::abort()
    {
        _active = false;
        signal();
        if (_thread.joinable()) {
            _thread.join();
        }
    }

    Thread::~Thread()
    {
        abort();
    }
}
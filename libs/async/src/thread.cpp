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
    {}

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
            cancelTimer(*handle.timerHandle);
            handle.timerHandle = std::nullopt;
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
            if (handle.eventDeadline != steady_clock::time_point{}) {
                handle.timerHandle = addTimer({.timerDeadline = handle.eventDeadline, .targetEvent = event});
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

    void Thread::add(Timer *timer)
    {
        SUIL_ASSERT((timer->_state != Timer::tsSCHEDULED) &&
                    (timer->_coro != nullptr));
        timer->_state = Timer::tsSCHEDULED;
        timer->_handle =
                addTimer({.timerDeadline = now() + timer->_timeout, .targetTimer = timer});
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
                cancelTimer(*handle.timerHandle);
                handle.timerHandle = std::nullopt;
            }
            struct epoll_event ev{};
            epoll_ctl(_epfd, EPOLL_CTL_DEL, handle.fd, &ev);

            _stats.inflight--;
            handle.coro.resume();
        }
    }

    void Thread::remove(Timer *timer)
    {
        auto state = Timer::tsSCHEDULED;
        if (timer->_state.compare_exchange_weak(state, Timer::tsSCHEDULED)) {
            // only 1 thread allowed to unschedule event
            SUIL_ASSERT(timer->_state == Timer::tsSCHEDULED);
            if (timer->_handle) {
                cancelTimer(*timer->_handle);
                timer->_handle = std::nullopt;
            }

            _stats.inflight--;
            timer->_coro.resume();
        }
    }

    uint64 Thread::load() const
    {
        return _stats.inflight;
    }

    Timer::Handle Thread::addTimer(Timer::Entry entry)
    {
        _timersLock.lock();
        auto it = _timers.insert(entry);
        _timersLock.unlock();

        signal();

        SUIL_ASSERT(it.second);
        return it.first;
    }

    void Thread::cancelTimer(Timer::Handle handle)
    {
        std::lock_guard<std::mutex> lg(_timersLock);
        if (handle != _timers.end()) {
            _timers.erase(handle);
        }
    }

    void Thread::fireExpiredTimers()
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
                    auto& event = handle.targetEvent->handle();
                    if (event.state.compare_exchange_weak(state, Event::esTIMEOUT)) {
                        event.timerHandle = std::nullopt;
                        struct epoll_event ev{};
                        epoll_ctl(_epfd, EPOLL_CTL_DEL, event.fd, &ev);

                        _stats.inflight--;
                        event.coro.resume();
                    }
                }
                else {
                    auto& timer = *handle.targetTimer;
                    auto state = Timer::tsSCHEDULED;
                    if (timer._state.compare_exchange_weak(state, Timer::tsFIRED)) {
                        timer._handle = std::nullopt;

                        _stats.inflight--;
                        timer._coro.resume();
                    }
                }
                _timersLock.lock();
            } else {
                break;
            }
        }
        _timersLock.unlock();
    }

    int Thread::computeWaitTimeout()
    {
        _timersLock.lock();
        auto it = _timers.begin();
        if (it != _timers.end()) {
            _timersLock.unlock();
            auto at = std::chrono::duration_cast<std::chrono::milliseconds>(it->timerDeadline - now()).count();
            if (at < 0) {
                return 0;
            }
            return int(at);
        }
        _timersLock.unlock();

        return -1;
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
/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-05
 *
 *  Modified from https://github.com/jeremyong/coop/blob/main/include/coop/detail/work_queue.hpp
 */

#pragma once

#include <suil/utils/utils.hpp>
#include <suil/async/coroutine.hpp>

#include <suil/async/detail/concurrentqueue.h>

// Currently, async supports exactly two priority levels, 0 (default) and 1
// (high)
#ifndef SUIL_ASYNC_PRIORITY_COUNT
#define SUIL_ASYNC_PRIORITY_COUNT 2
#endif

namespace suil {

    class Scheduler;

    class WorkQueue {
        struct Stats {
            std::atomic<uint64> inflight{0};
            std::atomic<uint64> maxInflight{0};
            uint64 totalHandled{0};
        };

    public:
        WorkQueue(Scheduler& scheduler, uint16_t idx);

        ~WorkQueue() noexcept;


        DISABLE_COPY(WorkQueue);
        DISABLE_MOVE(WorkQueue);

        // Returns the approximate size across all queues of any priority
        size_t size_approx() const noexcept {
            size_t out = 0;
            for (const auto & queue : queues_) {
                out += queue.size_approx();
            }
            return out;
        }

        void enqueue(std::coroutine_handle<> coroutine,
                     uint32_t priority = 0,
                     AsyncLocation location = {});

        Stats getStats() const;

    private:
        Scheduler &scheduler_;
        std::thread thread_;
        std::atomic<bool> active_;
        std::counting_semaphore<> sem_;
        Stats _stats{};
        moodycamel::ConcurrentQueue<std::coroutine_handle<>> queues_[SUIL_ASYNC_PRIORITY_COUNT];
    };
}

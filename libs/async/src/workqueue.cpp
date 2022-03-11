/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-05
 */

#include "suil/async/work_queue.hpp"

#include <algorithm>
#include <cstdio>

namespace suil {

    WorkQueue::WorkQueue(Scheduler& scheduler, uint16_t id)
            : scheduler_{scheduler}
            , sem_{0}
    {
        active_ = true;
        thread_ = std::thread([this, id] {
            pthread_t thread = pthread_self();
            char label[64];
            snprintf(label, sizeof(label), "Async-Q:%i", id);
            int result = pthread_setname_np(thread, label);
            SUIL_ASSERT(result == 0);

            ASYNC_TRACE("[%zu] queue #%d created", ASYNC_LOCATION, tid(), id);

            while (true) {
                sem_.acquire();
                if (!active_) {
                    return;
                }

                bool did_dequeue = false;

                // Dequeue in a loop because the concurrent queue isn't sequentially
                // consistent
                while (!did_dequeue)
                {
                    for (int i = SUIL_ASYNC_PRIORITY_COUNT - 1; i >= 0; --i)
                    {
                        std::coroutine_handle<> coroutine;
                        if (queues_[i].try_dequeue(coroutine))
                        {
                            ASYNC_TRACEn(2, "[%zu]: de-queueing coroutine %p (%u)\n",
                                        ASYNC_LOCATION,
                                        tid(),
                                        coroutine.address(),
                                        id);

                            did_dequeue = true;
                            _stats.inflight--;
                            _stats.totalHandled++;
                            coroutine.resume();
                            break;
                        }
                    }
                }
            }
        });
    }

    WorkQueue::~WorkQueue() noexcept
    {
        active_ = false;
        sem_.release();
        thread_.join();
    }

    void WorkQueue::enqueue(std::coroutine_handle<> coroutine,
                               uint32_t priority,
                               AsyncLocation location)
    {
        priority = std::clamp<uint32_t>(priority, 0, SUIL_ASYNC_PRIORITY_COUNT - 1);
        queues_[priority].enqueue(coroutine);
        _stats.inflight++;
        _stats.maxInflight = std::max(_stats.inflight, _stats.maxInflight).load();
        sem_.release();
    }

    WorkQueue::Stats WorkQueue::getStats() const
    {
        return  {
            _stats.inflight.load(),
            _stats.maxInflight.load(),
            _stats.totalHandled
        };
    }
}
/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-17
 */

#pragma once

#include <suil/async/task.hpp>

#include <suil/utils/utils.hpp>

#include <span>
#include <string>

namespace suil {

    class Socket {
    public:
        Socket(Socket&&) noexcept;
        Socket& operator=(Socket&&) noexcept;
        virtual ~Socket() noexcept;

        DISABLE_COPY(Socket);

        virtual void close() noexcept;
        virtual int detach();
        virtual bool isValid() const { return _fd != INVALID_FD; }

        int fd() const { return _fd; }

        int getLastError() const { return _error; }

        auto send(const void* buf, std::size_t size, milliseconds timeout = DELAY_INF) -> Task<int>;

        auto send(const std::span<const char>& buf, milliseconds timeout = DELAY_INF) {
            return send(buf.data(), buf.size(), timeout);
        }

        auto sendAll(const void* buf, std::size_t size, milliseconds timeout = DELAY_INF) -> Task<int>;

        auto sendAll(const std::span<const char>& buf, milliseconds timeout = DELAY_INF) {
            return sendAll(buf.data(), buf.size(), timeout);
        }

        auto receive(void *buf, std::size_t size, milliseconds timeout = DELAY_INF) -> Task<int>;

        auto receive(std::span<char> buf, milliseconds timeout = DELAY_INF) {
            return receive(buf.data(), buf.size(), timeout);
        }

        auto receiveAll(void* buf, std::size_t size, milliseconds timeout = DELAY_INF) -> Task<int>;

        auto receiveAll(std::span<char> buf, milliseconds timeout = DELAY_INF) {
            return receiveAll(buf.data(), buf.size(), timeout);
        }

        void bindToThread(uint16 tID);

    protected:
        Socket(int fd, int err) : _fd{fd}, _error{0}
        {}

        Socket() = default;

        int _fd{INVALID_FD};
        int16  _error{0};
        uint16  _tID{THREAD_ID_ANY};
    };
}
/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-14
 */

#include "suil/async/scheduler.hpp"
#include "suil/async/tcp.hpp"
#include "suil/async/scope.hpp"
#include "suil/async/sync.hpp"
#include "suil/async/fdwait.hpp"

#include <cstring>
#include <cmath>
#include <thread>
#include <iostream>

using namespace suil;
using namespace std::string_literals;

#define SYX_TCP_PORT 8080

#ifdef SYX_TCP_ECHO_SVR

auto handler(int id, TcpSocket sock) -> Task<>
{
    while (sock) {
        char buf[1024];
        auto ec = co_await sock.receive({buf, sizeof(buf)});
        if (ec > 0) {
            ec = co_await sock.send({buf, std::size_t(ec)});
        }

        if (ec == 0) {
            sock.close();
        }
        else if (ec < 0) {
            std::printf("\t[%d] -> error: %s\n", sock.address().port(), strerror(sock.getLastError()));
            sock.close();
        }
    }
}

auto dumpFairness() -> Task<>
{
    while (true) {
        delay(5s);
        Scheduler::instance().dumpStats();
    }
}

auto accept(int thread, TcpListener& listener) -> Task<>
{
    AsyncScope connectionScope;
    int i = 0;

    while (listener) {
        auto sock = co_await listener.accept();
        if (!sock) {
            printf("accept failed: %s\n", strerror(errno));
            break;
        }

        connectionScope.spawn(handler(i++, std::move(sock)));
    }

    // join the open utils scope
    co_await connectionScope.join();
}

auto acceptor(int threads, TcpListener& ls) -> VoidTask<>
{
    AsyncScope scope;

    scope.spawn(dumpFairness());
    for (int i = 0; i < threads; i++) {
        scope.spawn(accept(i, ls));
    }

    co_await scope.join();
}

void multiThreadServer(int threads = 1)
{
    auto listener = TcpListener::listen(SocketAddress::local("0.0.0.0", SYX_TCP_PORT), 127);
    SUIL_ASSERT(listener);
    printf("listening on 0.0.0.0:%d\n", SYX_TCP_PORT);
    auto sa = acceptor(threads, listener);

    sa.join();
}

#else
auto connection(int id, long roundTrips) -> Task<>
{
    auto conn = co_await TcpSocket::connect(SocketAddress::local("0.0.0.0", SYX_TCP_PORT));
    SUIL_ASSERT(conn);

#define MESSAGE     "Hello World"
#define MESSAGE_LEN (sizeof(MESSAGE)-1)
    for (int i = 0; i < roundTrips; ++i) {
        auto ec = co_await conn.send({MESSAGE, MESSAGE_LEN});
        SUIL_ASSERT(ec == MESSAGE_LEN);
        char buf[16];
        ec = co_await conn.receive({buf, sizeof(buf)});
        if (ec != MESSAGE_LEN) {
            break;
        }
    }
#undef MESSAGE
#undef MESSAGE_LEN
}

auto clients(long conns, long roundTrips) -> VoidTask<>
{
    AsyncScope connectionsScope;

    for (int i = 0; i < conns; ++i) {
        connectionsScope.spawn(connection(i, roundTrips));
    }

    co_await connectionsScope.join();
}

void multiThreadedClients(long conns, long roundTrips)
{
    auto start = nowms();

    auto handle = clients(conns, roundTrips);
    handle.join();

    auto duration = nowms() - start;

    auto requests = conns * roundTrips;
    printf("%ld requests in %ld ms\n",  requests, duration);
    printf("Performance: %.2f req/s\n", double(requests * 1000)/double(duration));
}

#endif

int main(int argc, const char *argv[])
{
    int threads = 0;
#ifdef SYX_TCP_ECHO_SVR
    if (argc > 1) {
        threads = strtol(argv[1], nullptr, 10);
    }
    Scheduler::init(threads);
    multiThreadServer(threads);
#else
    constexpr const char* usage = "Usage: syx-tcp-echo-cli <conns> <roundtrips>\n";
    if (argc < 3) {
        printf(usage);
        return EXIT_FAILURE;
    }

    auto conns = strtol(argv[1], nullptr, 10);
    SUIL_ASSERT(conns != 0);
    auto roundTrips = strtol(argv[2], nullptr, 10);
    if (argc > 3) {
        threads = strtol(argv[3], nullptr, 10);
    }
    Scheduler::init(threads);
    multiThreadedClients(conns, roundTrips);
#endif
    Scheduler::instance().dumpStats();
    return EXIT_SUCCESS;
}
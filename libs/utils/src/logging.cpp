/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/logging.hpp"

#include "suil/utils/console.hpp"
#include "suil/utils/datetime.hpp"
#include "suil/utils/exception.hpp"

#include <syslog.h>

#if SUIL_ENABLE_BACKTRACE==1
#include <execinfo.h> // for backtrace
#include <dlfcn.h>    // for dladdr
#include <cxxabi.h>   // for __cxa_demangle

namespace suil {

    void backtrace(char *buf, size_t size) {
        void *callstack[128];
        const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
        if (size == 0 or buf == nullptr) {
            // use static buffer
            static __thread char LOCAL[1024];
            buf = LOCAL; size = sizeof(LOCAL);
        }
        int nFrames = ::backtrace(callstack, nMaxFrames);
        char **symbols = backtrace_symbols(callstack, nFrames);

        std::ostringstream trace_buf;
        for (int i = 1; i < nFrames; i++) {
            printf("%s\n", symbols[i]);

            Dl_info info;
            if (dladdr(callstack[i], &info) && info.dli_sname) {
                char *demangled = nullptr;
                int status = -1;
                if (info.dli_sname[0] == '_')
                    demangled = abi::__cxa_demangle(info.dli_sname, nullptr, 0, &status);
                snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
                         i, int(2 + sizeof(void*) * 2), callstack[i],
                         status == 0 ? demangled :
                         info.dli_sname == nullptr ? symbols[i] : info.dli_sname,
                         (char *)callstack[i] - (char *)info.dli_saddr);
                free(demangled);
            } else {
                snprintf(buf, sizeof(buf), "%-3d %*p %s\n",
                         i, int(2 + sizeof(void*) * 2), callstack[i], symbols[i]);
            }
            trace_buf << buf;
        }
        free(symbols);
        printf("%s\n", trace_buf.str().data());
    }
}

#endif
namespace suil {

    _Logger::_Logger()
    {
        reset();
    }

    bool _Logger::isValid() const {
        return _formatter != nullptr && _writer != nullptr;
    }

    Level _Logger::getLevel() const {
        return Ego._lvl;
    }

    void _Logger::forwardLogs(const char *log, size_t sz, Level l, const char* tag)
    {
        if (_writer == nullptr) {
            return;
        }

        try {
            _writer->write(log, sz, l, tag);
        }
        catch (...) {
            auto ex = Exception::fromCurrent();
            char buf[SUIL_LOG_BUFFER_SIZE];

            auto size = DefaultFormatter{}.format(
                    buf,
                    Level::ERROR,
                    tag,
                    "Log writer exception: %s (Line = %s)",
                    ex.what(), log);

            DefaultWriter{}(buf, size, Level::ERROR);
        }
    }

    size_t _Logger::format(char *out, Level l, const char *tag, const char *fmt, va_list args)
    {
        if (_formatter != nullptr) {
            try {
                return _formatter(out, l, tag, fmt, args);
            }
            catch (...) {
                auto ex = Exception::fromCurrent();
                char buf[SUIL_LOG_BUFFER_SIZE];
                DefaultFormatter{}(buf, l, tag, fmt, args);

                return DefaultFormatter{}.format(
                        out,
                        Level::ERROR,
                        tag,
                        "Formatter exception: %s (Line = %s)",
                        ex.what(), buf);
            }
        }

        return 0;
    }

    void _Logger::reset()
    {
        _writer = LogWriter::mkunique();
        _formatter =
                [&](char *out, Level l, const char *tag, const char *fmt, va_list args) {
                    return DefaultFormatter()(out, l, tag, fmt, args);
                };
    }

    static _Logger SYS_LOGGER;
    _Logger &_Log = SYS_LOGGER;

    size_t DefaultFormatter::operator()(
            char *out,
            Level l,
            const char *tag,
            const char *fmt,
            va_list args) {
        const static char *LOGLVL_STR[] = {
                "TRC", "DBG", "INF", "NTC", "WRN", "ERR", "CRT"
        };

        char worker[64];
        size_t sz = SUIL_LOG_BUFFER_SIZE-8;
        char *tmp = out;
        const char *name = _Log.name() ? _Log.name() : "global";

        int wr = 0;

        switch (l) {
            case Level::DEBUG:
            case Level::ERROR:
            case Level::CRITICAL:
            case Level::WARNING:
                wr = snprintf(tmp, sz, "%s/%05d: [%s] [%3s] [%10.10s] ",
                              name, getpid(), Datetime()(), LOGLVL_STR[(unsigned char) l],
                              tag);
                break;
            default:
                wr = snprintf(tmp, sz, "%s: ", name);
                break;
        }

        sz -= wr;
        wr += vsnprintf(tmp + wr, sz, fmt, args);
        tmp[wr++] = '\033';
        tmp[wr++] = '[';
        tmp[wr++] = '0';
        tmp[wr++] = 'm';
        tmp[wr++] = '\n';
        tmp[wr] = '\0';

        return (size_t) wr;
    }

    size_t DefaultFormatter::format(
            char *out,
            Level l,
            const char *tag,
            const char *fmt,
            ...)
    {
        va_list args;
        va_start(args, fmt);
        auto sz = Ego(out, l, tag, fmt, args);
        va_end(args);
        return sz;
    }

    void DefaultWriter::operator()(const char *log, size_t sz, Level l) {
        auto c = Console::DEFAULT;
        int bold = 0;
        switch (l) {
            case Level::CRITICAL:
            case Level::ERROR:
                c = Console::RED;
                break;
            case Level::WARNING:
                c = Console::YELLOW;
                break;
            case Level::DEBUG:
                c = Console::MAGENTA;
                break;
            case Level::INFO:
                c = Console::WHITE;
                break;
            case Level::NOTICE:
                c = Console::GREEN;
                bold = 1;
                break;
            default:
                break;
        }
        cprint(c, bold, log);
    }

    void LogWriter::write(const char *log, size_t size, Level level, const char *tag)
    {
        DefaultWriter{}(log, size, level);
    }

    Syslog::Syslog(const char *name)
    {
        openlog(name, LOG_PID | LOG_CONS, LOG_USER);
    }

    void Syslog::close() {
        closelog();
    }

    void Syslog::write(const char *msg, size_t, Level l, const  char* tag) {
        int prio{LOG_INFO};
        switch (l) {
            case Level::DEBUG:
            case Level::TRACE:
                prio = LOG_DEBUG;
                break;
            case Level::NOTICE:
                prio = LOG_NOTICE;
                break;
            case Level::ERROR:
                prio = LOG_ERR;
                break;
            case Level::WARNING:
                prio = LOG_WARNING;
                break;
            case Level::CRITICAL:
                prio = LOG_CRIT;
                break;
            default:
                break;
        }
        syslog(prio, "%s", msg);
    }
}

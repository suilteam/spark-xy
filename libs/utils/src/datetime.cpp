/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/datetime.hpp"

namespace suil {

    Datetime::Datetime(time_t t)
            : _time(t)
    {
        gmtime_r(&_time, &_tm);
    }

    Datetime::Datetime()
            : Datetime(time(nullptr))
    {}

    Datetime::Datetime(const char *fmt, const char *str)
    {
        const char *tmp = HTTP_FMT;
        if (fmt) {
            tmp = fmt;
        }
        strptime(str, tmp, &_tm);
    }

    Datetime::Datetime(const char *http_time)
            : Datetime(HTTP_FMT, http_time)
    {}

    const char* Datetime::str(char *out, size_t sz, const char *fmt)  {
        if (!out || !sz || !fmt) {
            return nullptr;
        }
        (void) strftime(out, sz, fmt, &_tm);
        return out;
    }

    Datetime::operator time_t()  {
        if (_time == 0) {
            _time = timegm(&_tm);
        }
        return _time;
    }
}

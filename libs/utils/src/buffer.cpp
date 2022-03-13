/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/buffer.hpp"

namespace {

    inline std::ostream& getCachedThreadOStream(std::streambuf* sb) {
        static thread_local std::ostream _suil_cached_os(nullptr);
        SUIL_ASSERT(sb != nullptr);
        _suil_cached_os.rdbuf(sb);
        return _suil_cached_os;
    }
}

namespace suil {


    StringBuffer::StringBuffer(size_t size)
    {
        _buffer.reserve(size);
    }

    StringBuffer::int_type StringBuffer::overflow(int_type __c)
    {
        _buffer.append(1, __c);
        return 1;
    }

    std::streamsize StringBuffer::xsputn(const char_type *__s, std::streamsize __n)
    {
        _buffer.append(__s, __n);
        return __n;
    }

    std::ostream &StringBuffer::operator()()
    {
        return getCachedThreadOStream(this);
    }
}
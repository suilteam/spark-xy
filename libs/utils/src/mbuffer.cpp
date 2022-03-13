/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/mbuffer.hpp"

namespace suil {

    MemoryBuffer::MemoryBuffer(size_t size)
    {
        _buffer.reserve(size);
    }

    MemoryBuffer::int_type MemoryBuffer::overflow(int_type __c)
    {
        _buffer.append(1, __c);
        return 1;
    }

    std::streamsize MemoryBuffer::xsputn(const char_type *__s, std::streamsize __n)
    {
        _buffer.append(__s, __n);
        return __n;
    }
}
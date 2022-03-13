/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#pragma once

#include <suil/utils/utils.hpp>
#include <streambuf>

namespace suil {

    class MemoryBuffer : public std::streambuf {
    public:
        MemoryBuffer() = default;
        explicit MemoryBuffer(size_t size);

        std::string str() { return std::exchange(_buffer, {}); }
        [[nodiscard]] std::string_view view() const { return _buffer; }

        MOVE_CTOR(MemoryBuffer) = default;
        MOVE_ASSIGN(MemoryBuffer) =  default;

        DISABLE_COPY(MemoryBuffer);

    protected:
        std::streamsize xsputn(const char_type *__s, std::streamsize __n) override;
        int_type overflow(int_type __c = traits_type::eof()) override;

    private:
        std::string _buffer{};
    };
}
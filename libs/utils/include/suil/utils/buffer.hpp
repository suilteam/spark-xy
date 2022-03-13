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

    class StringBuffer : public std::streambuf {
    public:
        StringBuffer() = default;
        explicit StringBuffer(size_t size);

        std::string str() { return std::exchange(_buffer, {}); }
        [[nodiscard]] std::string_view view() const { return _buffer; }

        MOVE_CTOR(StringBuffer) = default;
        MOVE_ASSIGN(StringBuffer) =  default;

        DISABLE_COPY(StringBuffer);

        /**
         * @warning Do not use this operator unless you know what you are doing. It
         * returns a per thread stream. It is VERY unsafe since it overrides the internal
         * buffer of the stream.
         */
        std::ostream& operator()();

        template <typename T>
        StringBuffer& operator<<(const T& value) {
            Ego() << value;
            return Ego;
        }

    protected:
        std::streamsize xsputn(const char_type *__s, std::streamsize __n) override;
        int_type overflow(int_type __c = traits_type::eof()) override;

    private:
        std::string _buffer{};
    };
}
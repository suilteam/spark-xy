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

namespace suil::B64 {

    void encode(std::ostream& os, const uint8_t *, size_t);

    std::string encode(const uint8_t *, size_t);

    static std::string encode(const std::string_view& str) {
        return encode((const uint8_t *) str.data(), str.size());
    }

    void decode(std::ostream& os, const uint8_t *in, size_t len);

    std::string decode(const uint8_t *in, size_t len);

    static std::string decode(const std::string_view &sv) {
        return decode((const uint8_t *) sv.data(), sv.size());
    }
}
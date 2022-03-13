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

#include <cstdint>
#include <string>

namespace suil {

    struct Uuid {
        Uuid(unsigned char val[16]);
        Uuid(const std::string_view& str = {});
        std::string toString(bool lower = true) const;
        bool operator==(const Uuid& other) const;
        bool operator!=(const Uuid& other) const;
        bool empty() const;
        const unsigned char* raw() const { return &_bin[0]; }
    private:
        union {
            struct {
                uint64_t _hi;
                uint64_t _lo;
            };
            unsigned char  _bin[16] = {0};
        };
    };

}
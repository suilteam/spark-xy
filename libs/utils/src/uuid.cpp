/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/uuid.hpp"
#include "suil/utils/utils.hpp"

#include <uuid/uuid.h>

namespace suil {

    Uuid::Uuid(unsigned char val[16])
    {
        SUIL_ASSERT(val != nullptr);
        uuid_copy(_bin, val);
    }

    Uuid::Uuid(const std::string_view &str)
    {
        if (str.empty()) {
            if (!uuid_generate_time_safe(_bin)) {
                uuid_clear(_bin);
            }
        }
        else {
            if (!uuid_parse(str.data(), _bin)) {
                uuid_clear(_bin);
            }
        }
    }

    std::string Uuid::toString(bool lower) const
    {
        std::string str;
        str.resize(36);
        if (lower) {
            uuid_unparse_lower(reinterpret_cast<const unsigned char *>(_bin), &str[0]);
        }
        else {
            uuid_unparse_upper(reinterpret_cast<const unsigned char *>(_bin), &str[0]);
        }
        return str;
    }

    bool Uuid::operator==(const Uuid &other) const
    {
        return uuid_compare(reinterpret_cast<const unsigned char *>(_bin),
                            reinterpret_cast<const unsigned char *>(other._bin)) == 0;
    }

    bool Uuid::operator!=(const Uuid &other) const
    {
        return !(*this == other);
    }

    bool Uuid::empty() const
    {
        return uuid_is_null(_bin);
    }
}
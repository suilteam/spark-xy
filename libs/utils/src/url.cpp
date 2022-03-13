/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/url.hpp"
#include "suil/utils/buffer.hpp"

namespace {

    void URLDecode(const char *src, size_t len, std::ostream& os)
    {
#define IS_HEX_CHAR(ch) \
        (((ch) >= '0' && (ch) <= '9') || \
         ((ch) >= 'a' && (ch) <= 'f') || \
         ((ch) >= 'A' && (ch) <= 'F'))

#define HEX_VALUE(ch, value) \
        if ((ch) >= '0' && (ch) <= '9') \
        { \
            (value) = (ch) - '0'; \
        } \
        else if ((ch) >= 'a' && (ch) <= 'f') \
        { \
            (value) = (ch) - 'a' + 10; \
        } \
        else \
        { \
            (value) = (ch) - 'A' + 10; \
        }

        const unsigned char *start;
        const unsigned char *end;

        unsigned char c_high;
        unsigned char c_low;
        int v_high;
        int v_low;

        start = (unsigned char *)src;
        end = (unsigned char *)src + len;
        while (start < end)
        {
            if (*start == '%' && start + 2 < end)
            {
                c_high = *(start + 1);
                c_low  = *(start + 2);

                if (IS_HEX_CHAR(c_high) && IS_HEX_CHAR(c_low))
                {
                    HEX_VALUE(c_high, v_high);
                    HEX_VALUE(c_low, v_low);
                    os.put(char((v_high << 4) | v_low));
                    start += 3;
                }
                else
                {
                    os.put(char(*start));
                    start++;
                }
            }
            else if (*start == '+')
            {
               os.put(' ');
                start++;
            }
            else
            {
                os.put(char(*start));
                start++;
            }
        }

#undef IS_HEX_CHAR
#undef HEX_VALUE
    }
}

namespace suil::URL {

    std::string encode(const void* data, size_t len)
    {
        StringBuffer mb{(len*3)};
        auto& os = mb();
        const char *src = static_cast<const char *>(data), *end = src + len;
        char c;
        while (src != end) {
            c = *src++;
            if (!isalnum(c) && strchr("-_.~", c) == nullptr) {
                static const char HexChars[] = "0123456789ABCDEF";
                os.put('%')
                  .put(HexChars[(c&0xF0)>>4])
                  .put(HexChars[(c&0x0F)]);
            }
            else {
                os.put(c);
            }
        }

        return mb.str();
    }

    std::string decode(const void* data, size_t len)
    {
        StringBuffer mb{len};

        URLDecode(static_cast<const char*>(data), int(len), mb());

        return mb.str();
    }

}
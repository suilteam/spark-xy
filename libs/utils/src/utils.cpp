/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-01-14
 */

#include "suil/utils/utils.hpp"
#include "suil/utils/exception.hpp"

#include <fcntl.h>
#include <openssl/rand.h>

namespace {

    inline bool isCharIn(const char* str, char c) {
        return  strchr(str, c) != nullptr;
    }
}
namespace suil {

    uint8_t c2i(char c) {
        if (c >= '0' && c <= '9') {
            return (uint8_t) (c - '0');
        } else if (c >= 'a' && c <= 'f') {
            return (uint8_t) (c - 'W');
        } else if (c >= 'A' && c <= 'F') {
            return (uint8_t) (c - '7');
        }
        throw OutOfRange("utils::c2i - character out range");
    };

    char i2c(uint8_t c, bool caps) {

        if (c <= 0x9) {
            return char(c) + '0';
        }
        else if (c <= 0xF) {
            if (caps)
                return char(c) + '7';
            else
                return char(c) + 'W';
        }
        throw OutOfRange(AnT(), "byte out of range");
    };

    void randbytes(void *out, size_t size)
    {
        RAND_bytes(static_cast<uint8_t *>(out), (int) size);
    }

    size_t hexstr(const void *in, size_t iLen, char *out, size_t oLen)
    {
        if ((in == nullptr) || (out == nullptr) || (oLen < (iLen<<1)))
            return 0;

        size_t rc = 0;
        for (size_t i = 0; i < iLen; i++) {
            out[rc++] = i2c((uint8_t) (0x0F&(static_cast<const uint8_t *>(in)[i]>>4)), false);
            out[rc++] = i2c((uint8_t) (0x0F&static_cast<const uint8_t *>(in)[i]), false);
        }
        return rc;
    }

    void reverse(void *data, size_t len)
    {
        auto bytes = static_cast<uint8_t *>(data);
        size_t i;
        const size_t stop = len >> 1;
        for (i = 0; i < stop; ++i) {
            uint8_t *left = bytes + i;
            uint8_t *right = bytes + len - i - 1;
            const uint8_t tmp = *left;
            *left = *right;
            *right = tmp;
        }
    }

    void bytes(const std::string_view& str, uint8_t *out, size_t olen) {
        size_t size = str.size()>>1;
        if (out == nullptr || olen < size)
            throw InvalidArguments("suil::bytes - output buffer invalid");

        for (int i = 0; i < size; i++) {
            out[i] = (uint8_t) (c2i(str[i]) << 4 | c2i(str[i]));
        }
    }

    int nonblocking(int fd, bool on)
    {
        int opt = fcntl(fd, F_GETFL, 0);
        if (opt == -1)
            opt = 0;
        opt = on? opt | O_NONBLOCK : opt & ~O_NONBLOCK;
        return fcntl(fd, F_SETFL, opt);
    }

    void closepipe(int p[2])
    {
        close(p[0]);
        close(p[1]);
    }

    int64_t strtonum(const std::string_view &str, int base, long long int min, long long int max) {
        long long l;
        char *ep;
        if (str.size() > 19) {
            throw OutOfRange("suil::strtonum - to many characters in string number");
        }
        if (min > max) {
            throw OutOfRange("suil::strtonum - min value specified is greater than max");
        }

        errno = 0;
        char cstr[32] = {0};
        memcpy(cstr, &str[0], str.size());
        cstr[str.size()] = '\0';

        l = strtoll(cstr, &ep, base);
        if (errno != 0 || str.data() == ep || !matchany(*ep, '.', '\0')) {
            throw InvalidArguments("utils::strtonum - failed, errno = ", errno);
        }

        if (l < min)
            throw OutOfRange("utils::strtonum -converted value is less than min value");

        if (l > max)
            throw OutOfRange("utils::strtonum -converted value is greator than max value");

        return (l);
    }

    std::string_view trim(const std::string_view& str, const char *what)
    {
        if (what  == nullptr) return  str;

        auto s = 0;
        while ((s < str.size()) && isCharIn(what, str[s]))
            s++;

        int e = int(str.size()) - 1;
        while ((e >= s) && isCharIn(what, str[e]))
            e--;
        return str.substr(s, 1+e-s);
    }

    std::string strip(const std::string_view& str, const char* what)
    {
        int s = 0, e = int(str.size()) - 1;

        std::string out{};
        out.resize(str.size());

        while ((s < str.size()) && isCharIn(what, str[s])) s++;
        while ((e >= s) && isCharIn(what, str[e])) e--;

        int j = 0;
        for (auto i = s; i <= e; i++) {
            if (isCharIn(what, str[i])) {
                continue;
            }
            out[j] = str[i];
            j++;
        }

        out.resize(j);
        return out;
    }

    std::vector<std::string> split(const std::string_view& str, const char *delim)
    {
        const char *ap = nullptr, *ptr = str.data(), *eptr = ptr + str.size();
        std::vector<std::string> out;
        auto cmp = [](const char* p, const char* d) -> const char* {
            if (d[0] == '\0') {
                auto s = p;
                while((s != nullptr) and (*s != '\0')) s++;
                return s;
            }
            else {
                return strstr(p, d);
            }
        };

        auto len = (delim == nullptr)? 0: (delim[0] == '\0'? 1: strlen(delim));
        if (len > 0) {
            for (ap = cmp(ptr, delim); ap != nullptr; ap = cmp(ap, delim)) {
                if (ap != eptr) {
                    out.emplace_back(ptr, size_t(ap - ptr));
                    ap += len;
                    ptr = ap;
                }
            }
        }

        if (ptr != eptr) {
            out.emplace_back(ptr, size_t(eptr-ptr));
        }

        return out;
    }

    std::vector<std::string_view> parts(const std::string_view& str, const char *delim)
    {
        const char *ap, *ptr = str.data(), *eptr = ptr + str.size();
        std::vector<std::string_view> out;
        auto cmp = [](const char* p, const char* d) -> const char* {
            if (d[0] == '\0') {
                auto s = p;
                while((s != nullptr) and (*s != '\0')) s++;
                return s;
            }
            else {
                return strstr(p, d);
            }
        };

        auto len = (delim == nullptr)? 0: (delim[0] == '\0'? 1: strlen(delim));
        if (len > 0) {
            for (ap = cmp(ptr, delim); (ap != nullptr and ap < eptr); ap = cmp(ap, delim)) {
                if (ap < eptr) {
                    out.emplace_back(ptr, size_t(ap - ptr));
                    ap += len;
                    ptr = ap;
                }
            }
        }

        if (ptr < eptr) {
            out.emplace_back(ptr, size_t(eptr-ptr));
        }

        return out;
    }

}

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"

TEST_CASE("Using defer macros", "[defer][vdefer]") {
    WHEN("Using defer macro") {
        int num{0};
        {
            defer({
                num = -1;
            });
            REQUIRE(num == 0);
            num = 100;
        }
        REQUIRE(num == -1);

        bool thrown{false};
        auto onThrow = [&thrown] {
            defer({
                thrown = true;
            });
            throw suil::Exception{""};
        };
        REQUIRE_THROWS(onThrow());
        REQUIRE(thrown);
    }

    WHEN("Using vdefer macro") {
        int num{0};
        {
            vdefer(num, {
                num = -1;
            });
            REQUIRE(num == 0);
            num = 100;
        }
        REQUIRE(num == -1);
    }
}

TEST_CASE("Using scoped macro", "[scoped]") {
    struct Scope {
        bool open{true};
        void close() {
            open = false;
        }
    };

    Scope sp;
    REQUIRE(sp.open);
    {
        scoped(_sp, sp);
        REQUIRE(_sp.open);
    }
    REQUIRE_FALSE(sp.open);

    Scope sp2;
    REQUIRE(sp2.open);
    auto onThrow = [&sp2] {
        scope(sp2);
        throw suil::Exception{""};
    };
    REQUIRE_THROWS(onThrow());
    REQUIRE_FALSE(sp2.open);
}

TEST_CASE("Using c2i/i2c api's", "[c2i][i2c]") {
    const std::size_t COUNT{sizeofcstr("0123456789abcdef")};
    const std::array<char, COUNT> HC = {
            '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'
    };
    const std::array<char, COUNT> HS = {
            '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'
    };

    WHEN("Converting character to byte") {
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(suil::c2i(HC[i]) == i);
            REQUIRE(suil::c2i(HS[i]) == i);
        }
    }

    WHEN("Converting a byte to a character") {
        for (int i = 0; i < COUNT; i++) {
            REQUIRE(suil::i2c(i, true) == HC[i]);
            REQUIRE(suil::i2c(i) == HS[i]);
        }
    }
}

TEST_CASE("String API's on std::string/std::string_view", "[trim][parts][split]")
{
    SECTION("string split", "[split]") {
        // tests String::split
        std::string str{"One,two,three,four,five||one,,two,three||,,,,"}; // 44

        {
            auto p1 = suil::split(str, "/");
            REQUIRE(p1.size() == 1);
            REQUIRE(p1[0].data() != str.data());
            REQUIRE(p1[0] == str);
        }

        auto p2 = suil::split(str, "||");
        REQUIRE(p2.size() == 3);
        REQUIRE(p2[0].size() == 23);
        REQUIRE(p2[0].data() != str.data());
        REQUIRE(p2[1].size() == 14);
        REQUIRE(p2[1].data() != &str[25]);
        REQUIRE(p2[2].size() == 4);
        REQUIRE(p2[2].data() != &str[41]);

        auto p3 = suil::split(p2[0], ",");
        REQUIRE(p3.size() == 5);
        REQUIRE(p3[0] == "One");
        REQUIRE(p3[1] == "two");
        REQUIRE(p3[2] == "three");
        REQUIRE(p3[3] == "four");
        REQUIRE(p3[4] == "five");

        auto p4 = suil::split(p2[1], ",");
        REQUIRE(p4.size() == 4);
        REQUIRE(p4[0] == "one");
        REQUIRE(p4[1].empty());
        REQUIRE(p4[2] == "two");
        REQUIRE(p4[3] == "three");

        auto p5 = suil::split(p2[2], ",");
        REQUIRE(!p5.empty());
    }

    SECTION("String parts", "[parts]") {
        // tests String::split
        std::string str{"One,two,three,four,five||one,,two,three||,,,,"}; // 44

        auto p1 = suil::parts(str, "/");
        REQUIRE(p1.size() == 1);
        REQUIRE(p1[0].data() == str.data());
        REQUIRE(p1[0].size() == 45);

        auto p2 = suil::parts(str, "||");
        REQUIRE(p2.size() == 3);
        REQUIRE(p2[0].size() == 23);
        REQUIRE(p2[0].data() == &str[0]);
        REQUIRE(p2[1].size() == 14);
        REQUIRE(p2[1].data() == &str[25]);
        REQUIRE(p2[2].size() == 4);
        REQUIRE(p2[2].data() == &str[41]);

        auto p3 = suil::parts(p2[0], ",");
        REQUIRE(p3.size() == 5);
        REQUIRE(p3[0] == "One");
        REQUIRE(p3[1] == "two");
        REQUIRE(p3[2] == "three");
        REQUIRE(p3[3] == "four");
        REQUIRE(p3[4] == "five");

        auto p4 = suil::parts(p2[1], ",");
        REQUIRE(p4.size() == 4);
        REQUIRE(p4[0] == "one");
        REQUIRE(p4[1].empty());
        REQUIRE(p4[2] == "two");
        REQUIRE(p4[3] == "three");

        auto p5 = suil::parts(p2[2], ",");
        REQUIRE(!p5.empty());
    }

    SECTION("String strip/trim", "[strip][trim]") {
        // tests String::[strip/trim]
        std::string s1{"   String String Hello   "}; // 25

        auto s2 = suil::strip(s1, "q");                // nothing is stripped out if it not in the string
        REQUIRE(s2.size() == 25);
        REQUIRE(s1 == s2);

        s2 = suil::strip(s1);                   // by default spaces are stripped out
        REQUIRE(s2.size() == 17);
        REQUIRE(s2 == "StringStringHello");

        s2 = suil::strip(s1, "S");                   // any character can be stripped out
        REQUIRE(s2.size() == 23);
        REQUIRE(s2 == "   tring tring Hello   ");

        s2 = suil::strip(s1, "Slg");                   // multiple characters can striped out
        REQUIRE(s2.size() == 19);
        REQUIRE(s2 == "   trin trin Heo   ");

        auto s3 = suil::trim(s1);                        // trim just calls strip with option to strip only the ends
        REQUIRE(s3.size() == 19);
        REQUIRE(s3 == "String String Hello");
        REQUIRE(s3.data() == &s1[3]);

        s3 = suil::trim(s1, "o");                    // nothing to trim since 'o' is not at the end or beginning
        REQUIRE(s3.size() == 25);
        REQUIRE(s3 == s1);
        REQUIRE(s3.data() == s1.data());

        s3 = suil::trim(s1, "So ");                  // trims the space, H and o at bot ends
        REQUIRE(s3.size() == 17);
        REQUIRE(s3 == "tring String Hell");
        REQUIRE(s3.data() == &s1[4]);
    }
}

#endif

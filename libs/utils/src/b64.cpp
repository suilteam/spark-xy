/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-12
 */

#include "suil/utils/b64.hpp"
#include "suil/utils/exception.hpp"
#include "suil/utils/buffer.hpp"

namespace suil::B64 {

    std::string encode(const uint8_t *data, size_t sz) {
        StringBuffer mb{};
        encode(mb(), data, sz);
        return mb.str();
    }

    void encode(std::ostream& os, const uint8_t *data, size_t sz) {
        static const char b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        while (sz >= 3) {
            // |X|X|X|X|X|X|-|-|
            os.put(b64table[((*data & 0xFC) >> 2)]);
            // |-|-|-|-|-|-|X|X|
            uint8_t h = (uint8_t) (*data++ & 0x03) << 4;
            // |-|-|-|-|-|-|X|X|_|X|X|X|X|-|-|-|-|
            os.put(b64table[h | ((*data & 0xF0) >> 4)]);
            // |-|-|-|-|X|X|X|X|
            h = (uint8_t) (*data++ & 0x0F) << 2;
            // |-|-|-|-|X|X|X|X|_|X|X|-|-|-|-|-|-|
            os.put(b64table[h | ((*data & 0xC0) >> 6)]);
            // |-|-|X|X|X|X|X|X|
            os.put(b64table[(*data++ & 0x3F)]);
            sz -= 3;
        }

        if (sz == 1) {
            // pad with ==
            // |X|X|X|X|X|X|-|-|
            os.put(b64table[((*data & 0xFC) >> 2)]);
            // |-|-|-|-|-|-|X|X|
            uint8_t h = (uint8_t) (*data++ & 0x03) << 4;
            os.put(b64table[h]);
            os.put('=');
            os.put('=');
        } else if (sz == 2) {
            // pad with =
            // |X|X|X|X|X|X|-|-|
            os.put(b64table[((*data & 0xFC) >> 2)]);
            // |-|-|-|-|-|-|X|X|
            uint8_t h = (uint8_t) (*data++ & 0x03) << 4;
            // |-|-|-|-|-|-|X|X|_|X|X|X|X|-|-|-|-|
            os.put(b64table[h | ((*data & 0xF0) >> 4)]);
            // |-|-|-|-|X|X|X|X|
            h = (uint8_t) (*data++ & 0x0F) << 2;
            os.put(b64table[h]);
            os.put('=');
        }
    }

    std::string decode(const uint8_t *in, size_t size) {
        StringBuffer mb{(uint32_t) (size / 4) * 3};
        decode(mb(), in, size);
        return mb.str();
    }

    void decode(std::ostream& os, const uint8_t *in, size_t size) {
        static const unsigned char ASCII_LOOKUP[256] =
                {
                        /* ASCII table */
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
                        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
                        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
                        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
                        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
                        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
                };

        size_t sz = size, pos{0};
        const uint8_t *it = in;

        while (sz > 4) {
            if (ASCII_LOOKUP[it[0]] == 64 ||
                ASCII_LOOKUP[it[1]] == 64 ||
                ASCII_LOOKUP[it[2]] == 64 ||
                ASCII_LOOKUP[it[3]] == 64)
            {
                // invalid B64 character
                throw InvalidArguments("utils64::decode - invalid B64 encoded string passed");
            }

            os.put(char(ASCII_LOOKUP[it[0]] << 2 | ASCII_LOOKUP[it[1]] >> 4));
            os.put(char(ASCII_LOOKUP[it[1]] << 4 | ASCII_LOOKUP[it[2]] >> 2));
            os.put(char(ASCII_LOOKUP[it[2]] << 6 | ASCII_LOOKUP[it[3]]));
            sz -= 4;
            it += 4;
        }
        int i = 0;
        while ((it[i] != '=') && (ASCII_LOOKUP[it[i]] != 64) && (i++ < 4));
        if ((sz-i) && (it[i] != '=')) {
            // invalid B64 character
            throw InvalidArguments("utils64::decode - invalid B64 encoded string passed");
        }
        sz -= 4-i;

        if (sz > 1) {
            os.put(char(ASCII_LOOKUP[it[0]] << 2 | ASCII_LOOKUP[it[1]] >> 4));
        }
        if (sz > 2) {
            os.put(char(ASCII_LOOKUP[it[1]] << 4 | ASCII_LOOKUP[it[2]] >> 2));
        }
        if (sz > 3) {
            os.put(char(ASCII_LOOKUP[it[2]] << 6 | ASCII_LOOKUP[it[3]]));
        }
    }
}

#ifdef SUIL_UNITTEST
#include "catch2/catch.hpp"

namespace sb = suil;
namespace B64 = suil::B64;

TEST_CASE("B64 encoding", "[common][utils][base64]")
{
    std::string raw{"Hello World!"}; // SGVsbG8gV29ybGQh
    std::string b64{};

    SECTION("base encode decode") {
        b64 = B64::encode(raw);
        CHECK(b64 == "SGVsbG8gV29ybGQh");
        REQUIRE(raw == B64::decode(b64));

        raw = "12345678";  // MTIzNDU2Nzg=
        b64 = B64::encode(raw);
        CHECK(b64 == "MTIzNDU2Nzg=");
        REQUIRE(raw == B64::decode(b64));
    }

    SECTION("different character lengths") {
        raw = "1234567";  // MTIzNDU2Nw==
        b64 = B64::encode(raw);
        CHECK(b64 == "MTIzNDU2Nw==");
        REQUIRE(raw == B64::decode(b64));

        raw = "123456";  // MTIzNDU2
        b64 = B64::encode(raw);
        CHECK(b64 == "MTIzNDU2");
        REQUIRE(raw == B64::decode(b64));

        raw = "12345";  // MTIzNDU=
        b64 = B64::encode(raw);
        CHECK(b64 == "MTIzNDU=");
        REQUIRE(raw == B64::decode(b64));
    }

    SECTION("data with spaces") {
        raw = "Sentence with spaces";  // U2VudGVuY2Ugd2l0aCBzcGFjZXM=
        b64 = B64::encode(raw);
        CHECK(b64 == "U2VudGVuY2Ugd2l0aCBzcGFjZXM=");
        REQUIRE(raw == B64::decode(b64));
    }

    SECTION("punctuation in data") {
        // U2VudGVuY2UsIHdpdGghJCYjQCpefmAvPyIie1soKV19fFwrPS1fLC47Og==
        raw = R"(Sentence, with!$&#@*^~`/?""{[()]}|\+=-_,.;:)";
        b64 = B64::encode(raw);
        CHECK(b64 == "U2VudGVuY2UsIHdpdGghJCYjQCpefmAvPyIie1soKV19fFwrPS1fLC47Og==");
        REQUIRE(raw == B64::decode(b64));
    }

    SECTION("binary data") {
        struct binary { uint64_t a; uint8_t  b[4]; };
        binary b{0xFFEEDDCCBBAA0011, {0x01, 0x02, 0x03, 0x04}};
        std::string_view ob{(char *)&b, sizeof(binary)};
        b64 = B64::encode((const uint8_t *) &b, sizeof(binary));
        std::string db(B64::decode(b64));
        REQUIRE(ob == db);
    }
}

#endif
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

#include <string>

namespace suil::URL {

    /**
     * @brief Uses URL encoding scheme to encode the given buffer
     * @param data the data to encode with URL encoding scheme
     * @param len the byte size of the buffer (\param data)
     *
     * @return the URL encode string
     */
    std::string encode(const void* data, size_t len);

    /**
     * @brief A wrapper function around the URL encoding, taking
     * types that have the data() and size() member function
     *
     * @tparam T The type of the data to encode
     * @param d the data to encode
     *
     * @return the URL encode string
     */
    template <DataBuf T>
    inline std::string encode(const T& d) {
        return encode(d.data(), d.size());
    }

    /**
     * @brief Decodes the given URL encoded string to it's ascii form
     *
     * @param data the data to decode
     * @param len the size of the data to decode
     *
     * @return URL decoded representation of the input data
     */
    std::string decode(const void* data, size_t len);

    /**
     * A wrapper function around the URL::decode function
     *
     * @tparam T the type of data to decode (must have data() and size() methods)
     * @param d the data to decode
     *
     * @return URL decoded representation of the input data
     */
    template <DataBuf T>
    inline std::string decode(const T& d) {
        return decode(d.data(), d.size());
    }

}
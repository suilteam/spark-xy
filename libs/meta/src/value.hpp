/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-01
 */

#pragma once

#include <string>
#include <variant>

namespace suil {

    struct Value {
        using V = std::variant<std::monostate, std::string, std::int64_t, double, bool, char>;
        /**
         * @brief a list of value types held in the variant, this
         * might not be necessary but useful in declaring specs
         */
        typedef enum {
            vtNone,
            vtString,
            vtInteger,
            vtFloat,
            vtBool,
            vtChar
        } Kind;

        Value() = default;

        Value(std::string str)
                : _kind{vtString},
                  _value{std::move(str)}
        {}

        Value(std::int64_t integer)
                : _kind{vtInteger},
                  _value{integer}
        {}

        Value(double fp)
                : _kind{vtFloat},
                  _value{fp}
        {}

        Value(bool b)
                : _kind{vtBool},
                  _value{b}
        {}

        Value(char c)
                : _kind{vtChar},
                  _value{c}
        {}

        Value(Value&&) = default;
        Value& operator=(Value&) = default;

        Value(const Value&) = default;
        Value& operator=(const Value&) = default;

        ~Value() {
            _kind = vtNone;
            _value = {};
        }

        operator bool() const { return !std::holds_alternative<std::monostate>(_value); }

        template <typename T>
        const T& get() const {
            return std::get<T>(_value);
        }

        Kind kind() const { return _kind; }

        const V& raw() const { return _value; }
    private:
        Kind _kind{vtNone};
        V _value{};
    };

}
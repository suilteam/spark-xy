/**
 * Copyright (c) 2022 suilteam, Carter 
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Mpho Mbotho
 * @date 2022-01-10
 */

#pragma once


#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <chrono>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <thread>

#include <suil/version.hpp>

/**
 * Some personal ego, wraps around de-referencing
 * `this`
 */
#define Ego (*this)

/**
 * Macro to to retrieve current error in string representation
 */
#define errno_s strerror(errno)

/**
 * Symbol variable with name v
 * @param v the name of the symbol
 */
#define var(NAME) s::NAME

/**
 * Access a symbol, \see sym
 */
#define sym(NAME) var(NAME)

/**
 * Access symbol type
 */
#define Sym(NAME) s::NAME##_t

/**
 * Define a symbol variable
 */

#define Var(NAME, TYPE) s::NAME##_t::variable_t<TYPE>

/**
 * Set an option, assign value \param v to variable \param o
 * @param o the variable to assign value to
 * @param v the value to assign to the variable
 */
#define opt(o, v) li::make_variable(var(o), v)

/**
 * Create a symbol property
 */
#define prop(name, tp) var(name)  = tp()

#if defined __GNUC__ || defined __llvm__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#define suil_no_inline __attribute__((noinline))
#define suil_force_inline __attribute__((always_inline))
#else
#define likely(x) (x)
#define unlikely(x) (x)
#define suil_no_inline
#define suil_force_inline
#endif

#ifdef SUIL_ASSERT
#undef
#endif

#ifndef suil_ut
#define suil_ut
#endif

#define SUIL_XPASTE(x, y) x##y

/**
 * utility macro to return the size of literal c-string
 * @param ch the literal c-string
 *
 * \example sizeof("Hello") == 5
 */
#define sizeofcstr(ch)   (sizeof(ch)-1)

/**
 * @def A macro to declare copy constructor of a class. This
 * macro should be used inside a class
 */
#define COPY_CTOR(Type) Type(const Type&)
/**
 * @def A macro to declare move constructor of a class. This
 * macro should be used inside a class
 */
#define MOVE_CTOR(Type) Type(Type&&)
/**
 * @def A macro to declare copy assignment operator of a class. This
 * macro should be used inside a class
 */
#define COPY_ASSIGN(Type) Type& operator=(const Type&)
/**
 * @def A macro to declare copy assignment operator of a class. This
 * macro should be used inside a class
 */
#define MOVE_ASSIGN(Type) Type& operator=(Type&&)
/**
 * @def A macro to disable copy construction of a class. This
 * macro should be used inside a class
 */
#define DISABLE_COPY(Type) COPY_CTOR(Type) = delete; COPY_ASSIGN(Type) = delete
/**
 * @def A macro to disable move assignment of a class. This
 * macro should be used inside a class
 */
#define DISABLE_MOVE(Type) MOVE_CTOR(Type) = delete; MOVE_ASSIGN(Type) = delete

#define SUIL_ASSERT(cond) do {                                                           \
        if (unlikely(!(cond))) {                                                       \
            fprintf(stderr, "%s:%d assertion failed: " #cond "\n", __FILE__, __LINE__); \
            fflush(stderr);                                                             \
            abort();                                                                    \
        }                                                                               \
    } while (false)

using int8   = int8_t ;
using int16  = int16_t;
using int32  = int32_t;
using int64  = int64_t;
using uint8  = uint8_t ;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;

/**
 * macro to add utility function's to a class/struct
 * for C++ smart pointers
 */
#define sptr(type)                          \
public:                                     \
    using Ptr = std::shared_ptr< type >;    \
    using WPtr = std::weak_ptr< type >;     \
    using UPtr = std::unique_ptr< type >;   \
    template <typename... Args>             \
    inline static Ptr mkshared(Args... args) {          \
        return std::make_shared< type >(    \
            std::forward<Args>(args)...);   \
    }                                       \
    template <typename... Args>             \
    inline static UPtr mkunique(Args... args) { \
        return std::make_unique< type >(    \
           std::forward<Args>(args)...);    \
    }

namespace suil {

    using namespace std::chrono_literals;

    using steady_clock = std::chrono::steady_clock;
    using system_clock = std::chrono::system_clock;
    using nanoseconds  = std::chrono::nanoseconds;
    using microseconds = std::chrono::microseconds;
    using milliseconds = std::chrono::milliseconds;
    using seconds = std::chrono::seconds;
    using minutes = std::chrono::minutes;
    using std::chrono::duration_cast;

    constexpr milliseconds DELAY_INF = -1ms;

    inline auto after(milliseconds timeout = DELAY_INF) -> steady_clock::time_point {
        return timeout <= 0ms? steady_clock::time_point{} : steady_clock::now() + timeout;
    }

    int64_t afterd(milliseconds timeout = DELAY_INF);

    int64_t fastnow();

    constexpr int INVALID_FD{-1};

    template<typename T>
    struct remove_rvalue_reference {
        using type = T;
    };

    template<typename T>
    struct remove_rvalue_reference<T &&> {
        using type = T;
    };

    template<typename T>
    using remove_rvalue_reference_t = typename remove_rvalue_reference<T>::type;

    template <typename T>
    concept arithmetic = std::is_arithmetic_v<T>;

    /**
     * change the given file descriptor to either non-blocking or
     * blocking
     * @param fd the file descriptor whose properties must be changed
     * @param on true when non-blocking property is on, false otherwise
     */
    int nonblocking(int fd, bool on = true);

    inline int clz(uint64_t num) {
        if (num) {
            return  __builtin_clz(num);
        }
        return 0;
    }

    inline int ctz(uint64_t num) {
        if (num) {
            return  __builtin_ctz(num);
        }
        return sizeof(uint64_t);
    }

    inline uint64_t np2(uint64_t num) {
        return (1U<<(sizeof(num) - suil::ctz(num)));
    }

    /**
     * @brief Generate random bytes
     *
     * @param buf a buffer in which to write the random bytes to
     * @param size the number of bytes to allocate
     */
    void randbytes(void *buf, size_t size);

    /**
     * @brief Convert the given input buffer to a hex string
     *
     * @param buf the input buffer to convert to hex
     * @param iLen the length of the input buffer
     * @param out the output buffer to write the hex string into
     * @param len the size of the output buffer (MUST be twice the size
     *  of the input buffer)
     * @return the number of bytes writen into the output buffer
     */
    size_t hexstr(const void *buf, size_t iLen, char *out, size_t len);

    /**
     * Reverse the bytes in the given buffer byte by byte
     * @param data the buffer to reverse
     * @param len the size of the buffer
     */
    void reverse(void *data, size_t len);

    /**
     * A utility function to close a pipe
     *
     * @param p a pipe descriptor
     */
    void closepipe(int p[2]);


    /**
     * @brief Hex representation to binary representation for a character
     *
     * @param c hex representation
     * @return binary representation
     *
     * @throws OutOfRangeError
     */
    uint8_t c2i(char c);

    /**
     * binary representation to hex representation
     * @param c binary representation
     * @param caps true if hex representation should be in CAP's
     * @return hex representation
     */
    char i2c(uint8_t c, bool caps = false);

    /**
     * @brief Converts the given hex string to its binary representation
     *
     * @param str the hex string to load into a byte buffer
     * @param out the byte buffer to load string into
     * @param olen the size of the output buffer (MUST be at least str.size()/2)
     */
    void bytes(const std::string_view&str, uint8_t *out, size_t olen);

    /**
     * @brief Converts the given hex string to its binary representation
     *
     * @param str the hex string to load into a byte buffer
     * @param buf the byte buffer to hold the binary bytes
     */
    inline void bytes(const std::string_view &str, std::span<uint8> buf) {
        bytes(str, buf.data(), buf.size());
    }

    namespace internal {

        /**
         * A scoped resource is a resource that implements a `close` method
         * which is meant to clean up any resources.
         * For a example a database connection can be wrapped around a `scoped_res`
         * class and the `close` method of the connection will be invoked at the
         * end of a block
         * @tparam R the type of resource to scope
         */
        template<typename R>
        struct scoped_res {
            explicit scoped_res(R &res) : res(res) {}

            scoped_res(const scoped_res &) = delete;

            scoped_res(scoped_res &&) = delete;

            scoped_res &operator=(scoped_res &) = delete;

            scoped_res &operator=(const scoped_res &) = delete;

            ~scoped_res() {
                res.close();
            }

        private suil_ut:
            R &res;
        };

        struct _defer final {
            _defer(std::function<void(void)> func)
                    : func(std::move(func))
            {}

            _defer(const _defer&) = delete;
            _defer& operator=(const _defer&) = delete;
            _defer(_defer&&) = delete;
            _defer& operator=(_defer&) = delete;

            ~_defer() {
                if (func)
                    func();
            }
        private:
            std::function<void(void)> func;
        };
    }

    template <typename T>
    concept DataBuf =
    requires(T t) {
        { t.data() } -> std::convertible_to<void *>;
        { t.size() } -> std::convertible_to<std::size_t>;
    };

    /**
     * Works with std::hash to allow types that have a data() and size() method
     * @tparam T
     */
    template <DataBuf T>
    struct Hasher {
        inline size_t operator()(const T& key) const {
            return std::hash<std::string_view>()(std::string_view{key.data(), key.size()});
        }
    };

    /**
    * Duplicate the given buffer to a new buffer. This is similar to
    * strndup but uses C++ new operator to allocate the new buffer
    *
    * @tparam T the type of buffer to duplicate
    * @param s the buffer that will be duplicated
    * @param sz the size of the buffer to duplicate
    * @return a newly allocated copy of the given data that should
    * be deleted with delete[]
    */
    template <typename T = char>
    auto duplicate(const T* s, size_t sz) -> T* {
        if (s == nullptr || sz == 0) return nullptr;
        auto size{sz};
        if constexpr (std::is_same_v<T, char>)
            size += 1;
        auto* d = new T[size];
        memcpy(d, s, sizeof(T)*sz);
        if constexpr (std::is_same_v<T, char>)
            d[sz] = '\0';
        return d;
    }

    /**
     * Reallocates the given buffer to a new memory segment with the given
     * size. If \param os is greater than \param ns then there will be no
     * allocation and source buffer will be returned.
     *
     * @tparam T the type of objects held in the buffer
     * @param s the buffer to reallocate. If valid, this buffer will be deleted on return
     * @param os the number of objects in the buffer to reallocate
     * @param ns the size of the new buffer
     * @return a new buffer of size \param ns which has objects previously held
     * in \s.
     */
    template <typename T = char>
    auto reallocate(T* s, size_t os, size_t ns) -> T* {
        if (ns == 0)
            return nullptr;
        if (os > ns) {
            return s;
        }
        return static_cast<T *>(srealloc(s, ns));
    }

    /**
     * Find the occurrence of a group of bytes \param needle in the given
     * source buffer
     * @param src the source buffer on which to do the lookup
     * @param slen the size of the source buffer
     * @param needle a group of bytes to lookup on the source buffer
     * @param len the size of the needle
     * @return a pointer to the first occurrence of the \param needle bytes
     */
    void * memfind(void *src, size_t slen, const void *needle, size_t len);

    /**
     * Copy bytes from buffer into an instance of type T and return
     * the instance
     * @tparam T the type whose bytes will be read from the buffer
     * @param buf the buffer to read from (the size of the buffer must be
     *  greater than sizeof(T))
     * @return an instance of \tparam T whose content was read from \param buf
     */
    template <typename T>
    inline T rd(void *buf) {
        T t;
        memcpy(&t, buf, sizeof(T));
        return t;
    }

    /**
     * Copy the byte content of the given instance into the given buffer
     * @tparam T the type of the instance
     * @param buf the buffer to copy into (the size of the buffer must be
     *  greater than sizeof(T))
     * @param v an instance to copy into the buffer
     */
    template <typename T>
    inline void wr(void *buf, const T& v) {
        memcpy(buf, &v, sizeof(T));
    }

    /**
     * @brief A status is a lightweight struct that can be used as a return
     * type for API's that return error code in with their return type
     *
     * @tparam T The type of the value returned by a function
     * @tparam E The type of error code, can be an integer or an enum
     * @tparam e the value indicating success on the error code
     */
    template <typename T, typename E = int, E e = 0>
    requires (std::is_default_constructible_v<T> &&
              std::is_default_constructible_v<E> &&
              (std::is_enum_v<E> || std::is_integral_v<E>))
    struct Status {
        Status() = default;
        Status(T res) : result{std::move(res)} {}
        Status(E err) : error{std::move(err)} {}
        operator bool() const { return error == e; }
        T result{};
        E error{e};
    };

    /**
     * @brief A wrapper function around creating a truth status code
     *
     * @tparam T the type of the result held in a status object
     * @tparam E the type of the error codes on the status object
     * @tparam ok the value of the error code corresponding to success status
     * @param t the value to return with status code
     *
     * @return an instance of the status code of type \tparam T
     */
    template <typename T, typename E = int, E ok = 0>
    Status<T, E> Ok(T t) { return { std::move(t) }; }

    /**
     * @brief A function to return the hash of the currently executing
     * thread's ID
     *
     * @return the hash of the current thread's ID
     */
    inline std::size_t tid() noexcept { return std::hash<std::thread::id>{}(std::this_thread::get_id()); }

    /**
     * @brief Compares \param orig string against the other string \param o
     *
     * @param orig the original string to match
     * @param o the string to compare to
     *
     * @return true if the given strings are equal
     */
    inline bool strmatchany(const char *orig, const char *o) {
        return strcmp(orig, o) == 0;
    }

    /**
     * @brief Match any of the given strings to the given string \param l
     *
     * @tparam A always const char*
     * @param l the string to match others against
     * @param r the first string to match against
     * @param args comma separated list of other string to match
     * @return true if any ot the given string is equal to \param l
     */
    template<typename... A>
    inline bool strmatchany(const char *l, const char *r, A... args) {
        return strmatchany(l, r) || strmatchany(l, std::forward<A>(args)...);
    }

    /**
     * @brief Compare the first value \p l against all the other values (smilar to ||)
     *
     * @tparam T the type of the values
     * @tparam A type of other values to match against
     * @param l the value to match against
     * @param args comma separated list of values to match
     * @return true if any
     */
    template<typename T, typename... A>
    inline bool matchany(const T l, A... args) {
        static_assert(sizeof...(args), "at least 1 argument is required");
        return ((args == l) || ...);
    }

    /**
     * @brief Converts a string to a decimal number. Floating point
     * numbers are not supported
     *
     * @param str the string to convert to a number
     * @param base the numbering base to the string is in
     * @param min expected minimum value
     * @param max expected maximum value
     * @return a number converted from given string
     */
    int64_t strtonum(const std::string_view& str, int base, long long int min, long long int max);

    /**
     * @brief Converts the given string to a number
     *
    * @tparam T the type of number to convert to
    * @param str the string to convert to a number
    * @return the converted number
    *
    * @throws \class Exception when the string is not a valid number
    */
    template<std::integral T>
    auto to_number(const std::string_view& str) -> T {
        return T(suil::strtonum(str, 10, INT64_MIN, INT64_MAX));
    }

    /**
     * @brief converts the given string to a number
     *
     * @tparam T the type of number to convert to
     * @param str the string to convert to a number
     * @return the converted number
     *
     * @throws runtime_error when the string is not a valid number
     */
    template<std::floating_point T>
    auto to_number(const std::string_view& str) -> T {
        double f;
        char *end;
        f = strtod(str.data(), &end);
        if (errno || *end != '\0')  {
            throw std::runtime_error(errno_s);
        }
        return (T) f;
    }

    /**
     * @brief convert given number to string a string
     *
     * @tparam T the type of number to convert
     * @param v the number to convert
     * @return converts the number to string using std::to_string
     */
    template<arithmetic T>
    inline auto tostr(T v) -> std::string {
        return std::to_string(v);
    }

    /**
     * @brief converts given string to string
     *
     * @param str
     * @return a peek of the given string
     */
    inline std::string tostr(const std::string_view& str) {
        return std::string{str};
    }

    /**
     * @brief Converts the given c-style string to a \class std::string
     *
     * @param str the c-style string to convert
     * @return a \class String which is a duplicate of the
     * given c-style string
     */
    inline std::string tostr(const char *str) {
        return std::string{str};
    }

    /**
     * @brief Cast \class std::string_view to number of given type
     *
     * @tparam T the type of number to cast to
     * @param data the string to cast
     * @param to the reference that will hold the result
     */
    template <arithmetic T>
    inline void cast(const std::string_view& data, T& to) {
        to = suil::to_number<T>(data);
    }

    /**
     * @brief Cast the given \class std::string_view to a boolean
     *
     * @param data the string to cast
     * @param to reference to hold the result. Will be true if the string is
     * 'true' or '1'
     */
    inline void cast(const std::string_view& data, bool& to) {
        to = ((strcasecmp(data.data(), "1") == 0) ||
              (strcasecmp(data.data(), "true") == 0));
    }

    /**
     * @brief Trims the given \class std::string_view, removing whitespaces at both ends.
     *
     * @note the returned string is string view referencing the part of the
     * input string view
     *
     * @param str the string to trim
     * @param what a list of characters given as a string to trim
     *
     * @return the trimmed string view
     */
    std::string_view trim(const std::string_view& str, const char *what = " ");

    /**
     * @brief Removes the given \param c from the string \param str
     * returning a new copy
     *
     * @param str the string to strip
     * @param what a list characters to strip out of the string
     *
     * @return a string with the specified character removed
     */
    std::string strip(const std::string_view& str, const char* what = " ");

    /**
     * @brief Splits a given string at every occurrence of the given
     * characters
     *
     * @param str the string to split
     * @param delim the delimiter to use when splitting the string
     *
     * @return A vector containing the split string parts. The parts are
     * copied from the original string
     */
    std::vector<std::string> split(const std::string_view& str, const char *delim = " ");

    /**
     * Splits the given \param str using the given \param delim and returns
     * parts as views into the string.
     *
     * @note the parts returned must be used before \param str goes out of scope in
     * the calling context
     *
     * @param str the string to split
     * @param delim the delimiter to use when splitting the string
     *
     * @return parts of the split string (just views into the original string)
     */
    std::vector<std::string_view> parts(const std::string_view& str, const char *delim = " ");

    /**
     * @brief \class std::string \class std::string_view case sensitive comparator
     */
    struct std_string_eq {
        inline bool operator()(const std::string& l, const std::string& r) const
        {
            return std::equal(l.begin(), l.end(), r.begin(), r.end());
        }
    };

    /**
     * @brief \class std::string \class std::string_view case insensitive comparator
     */
    struct std_string_case_eq {
        bool operator()(const std::string& s1, const std::string& s2) const {
            return operator()(std::string_view{s1}, std::string_view{s2});
        }

        bool operator()(const std::string_view& s1, const std::string_view& s2) const {
            return ((s1.size() == s2.size()) &&
                    (strncasecmp(s1.data(), s2.data(), s1.size()) == 0));
        }
    };


}

/**
* declare a scoped resource, creating variable \param n and assigning it to \param x
* The scoped resource must have a method close() which will be invoked at the end
* of the scope
*/
#define scoped(N, X) auto& N = X ; suil::internal::scoped_res<decltype( N )> _SUIL_SCOPE_##N { N }

#define scope(X) suil::internal::scoped_res<decltype( X )> SUIL_XPASTE(_SUIL_SCOPE, __LINE__) { X }

/**
 * Defers execution of the given code block to the end of the block in which it is defined
 * @param N the name of the defer block
 * @param F the code block to execute at the end of the block
 */
#define vdefer(N, F) suil::internal::_defer _SUIL_DEFER_##N {[&]() F }

/**
 * Defers execution of the given code block to the end of the block in which it is defined
 * @param F the code block to execute at the end of the block
 */
#define defer(F) suil::internal::_defer SUIL_XPASTE(_SUIL_DEFER, __LINE__) {[&]() F }
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

#include <cstdarg>
#include <cinttypes>

#include <string>

namespace suil::Console {

    /**
     * @brief A list of console print colors
     */
    enum : uint8_t {
        WHITE = 0,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        DEFAULT
    };

    /**
     * @brief color formatted print to console
     * @param color the color that will be used to print
     * @param bold true if characters are to be printed in bold, false otherwise
     * @param str the string to print
     */
    void cprint(uint8_t color, int bold, const char* str);

    /**
     * @brief color formatted print to console
     * @param color the color that will be used to print
     * @param bold 1 if the characters are to be bolded, false otherwise
     * @param fmt printf-style format string
     * @param args variable arguments list
     */
    void cprintv(uint8_t color, int bold, const char *fmt, va_list args);

    /**
     * @brief color formatted print to console
     * @param color the color that will be used to print
     * @param bold 1 if the characters are to be bolded, false otherwise
     * @param fmt printf-style format string
     * @param ... comma separated list of arguments
     */
    void cprintf(uint8_t color, int bold, const char *fmt, ...);

    /**
     * @brief prints to console/terminal appending a new line character at the end
     * @param fmt format string
     * @param ... format arguments
     */
    void println(const char *fmt, ...);

    /**
     * @brief prints to console in red
     * @param fmt format string
     * @param ... format arguments
     */
    void red(const char *fmt, ...);

    /**
     * @brief prints to console in blue
     * @param fmt format string
     * @param ... format arguments
     */
    void blue(const char *fmt, ...);

    /**
     * @brief prints to console in green
     * @param fmt format string
     * @param ... format arguments
     */
    void green(const char *fmt, ...);

    /**
     * @brief prints to console in yellow
     * @param fmt format string
     * @param ... format arguments
     */
    void yellow(const char *fmt, ...);

    /**
     * @brief prints to console in magenta
     * @param fmt format string
     * @param ... format arguments
     */
    void magenta(const char *fmt, ...);

    /**
     * @brief prints to console in cyan
     * @param fmt format string
     * @param ... format arguments
     */
    void cyan(const char *fmt, ...);

    /**
     * @brief Print a message to console using default console colors
     * @param fmt the format string
     * @param ... format arguments
     */
    void log(const char *fmt, ...);

    /**
     * @brief Print a message to console using a bold while console color
     * @param fmt the format string
     * @param ... format arguments
     */
    void info(const char *fmt, ...);

    /**
     * @brief Print a message to console using red console color
     * @param fmt the format string
     * @param ... format arguments
     */
    void error(const char *fmt, ...);

    /**
     * @brief Print a message to console using a yellow console color
     * @param fmt the format string
     * @param ... format arguments
     */
    void warn(const char *fmt, ...);
}
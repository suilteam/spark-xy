/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 * 
 * @author Carter
 * @date 2022-03-11
 */

#include <cppast/libclang_parser.hpp>
#include <iostream>

int main(int argc, char *argv[])
{
    cppast::libclang_compilation_database database(argv[1]); // the compilation database

    // simple_file_parser allows parsing multiple files and stores the results for us
    cppast::simple_file_parser<cppast::libclang_parser> parser(type_safe::ref(index));
    try
    {
        cppast::parse_database(parser, database); // parse all files in the database
    }
    catch (cppast::libclang_error& ex)
    {
        std::cerr << "fatal libClang error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    if (parser.error())
        // a non-fatal parse error
        // error has been logged to stderr
        return 1;

    for (auto& file : parser.files())
        cb(file);
    return 0;
}
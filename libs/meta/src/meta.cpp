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

#include <suil/utils/args.hpp>
#include <suil/utils/exception.hpp>
#include <suil/utils/logging.hpp>

#include <iostream>

using namespace suil::args;
using namespace std::string_view_literals;

using suil::Exception;

static void build(const Command& cmd);

int main(int argc, char *argv[])
{
    suil::args::Parser parser("suil-meta", "1.0", "Parses C++ header files and generates static meta types");
    try {
        parser(
                Command("build", "Processes the given C++ header files and generates meta types")
                        ({"out-dir", "The directory to generate the sources into", 'D', false})
                        ({"dot-fix", "The postfix string to add to the generated files", 'P', false})
                        (build)
                        ()
        );
        parser.parse(argc, argv);
        parser.handle();
    }
    catch (...) {
        auto ex = Exception::fromCurrent();
        serror("unhandled error: %s", ex.what());
        return EXIT_FAILURE;
    }
}

void build(const Command& cmd)
{
    auto outputDir = cmd.value("out-dir", ""sv);
    auto fnameDotFix = cmd.value("dot-fix", ""sv);

}
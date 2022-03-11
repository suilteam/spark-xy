cmake_minimum_required(VERSION 3.16)

# The version of the library, configurable from command line
if (NOT SUIL_VERSION)
    set(SUIL_META_VERSION "0.1.0" CACHE STRING "The version of suil-Meta library")
else()
    set(SUIL_META_VERSION ${SUIL_VERSION})
endif()

project(Meta
        VERSION ${SUIL_META_VERSION}
        LANGUAGES C CXX)

if (LINUX)
    set(SUIL_PLATFORM_SOURCES)
elseif(UNIX)
    set(SUIL_PLATFORM_SOURCES)
endif()

set(SUIL_META_SOURCES
        ${SUIL_PLATFORM_SOURCES}
        src/meta.cpp)

# Add and configure Meta shared library
SuilAddLibrary(Meta
        KIND SHARED
        SOURCES  ${SUIL_META_SOURCES}
        OPTIONS  PRIVATE -Wall -Wextra -pedantic -Werror -fno-rtti -g -O3
        RENAME   SuilMeta
        LIBS     Suil::Utils)

if (ENABLE_UNIT_TESTS)
    include_directories(test)
    SuilUnitTest(Meta-UnitTest
            SOURCES
                ${SUIL_META_SOURCES}
                test/main.cpp
            INCLUDES include ${SUIL_GCC_PLUGINS_DIR}/include
            RENAME   SuilMetaUt
            LIBS     OpenSSL::SSL Suil::Utils)
endif()
cmake_minimum_required(VERSION 3.10)

project(lapser VERSION 0.001 LANGUAGES C CXX)
set(CMAKE_DEBUG_POSTFIX -dbg)
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(LAPSER_COMM "LOCAL_COMM" CACHE STRING "Pattern of communication to use")
set_property(CACHE LAPSER_COMM PROPERTY STRINGS
    LOCAL_COMM
    PULL_COMM
    PUSH_COMM)

option(BUILD_SHARED_LIBS "Build using shared libraries"   ON)
option(ENABLE_ASAN       "Use AddressSanitizer"           OFF)
option(ENABLE_UBSAN      "Use UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TESTS      "Enable testing"                 OFF)
option(ENABLE_APPS       "Build sample applications"      OFF)

set(LIBDIR_STRAT "ENV_LD" CACHE STRING "Strategy to link with GASPI")
set_property(CACHE LIBDIR_STRAT PROPERTY STRINGS
    ENV_LD
    STATIC_LINK
    CMAKE_RUNPATH
    SYSTEM_LD)


if(ENABLE_ASAN)
    #add_compile_options(-fsanitize=address)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    message(STATUS "Sanitizer enabled: AddressSanitizer")
endif()

if(ENABLE_UBSAN)
    #add_compile_options(-fsanitize=undefined)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    message(STATUS "Sanitizer enabled: UndefinedSanitizer")
endif()

include(GNUInstallDirs)

add_subdirectory(src)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()

if(ENABLE_APPS)
    add_subdirectory(apps)
endif()

install(FILES include/lapser.h include/multilapser.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

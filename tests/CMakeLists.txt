set(GTEST_CMAKE_PATH "" CACHE STRING "Path to local install of googletest")

if(GTEST_CMAKE_PATH STREQUAL "")
  message(FATAL_ERROR "Tests were enabled, but GTEST_CMAKE_PATH was not specified.")
endif()

if(FALSE)
set(GTEST_INCLUDE_DIR "${GTEST_INSTALL_PATH}/include")
set(GTEST_LIBRARY "${GTEST_INSTALL_PATH}/lib64/libgtest.a")
set(GTEST_MAIN_LIBRARY "${GTEST_INSTALL_PATH}/lib64/libgtest_main.a")
endif()

set(BUILD_GMOCK OFF CACHE BOOL "Builds the googlemock subproject")
set(INSTALL_GTEST OFF CACHE BOOL "Enable installation of googletest")
add_subdirectory(${GTEST_CMAKE_PATH} ${CMAKE_BINARY_DIR}/gtests EXCLUDE_FROM_ALL)


add_definitions(-DRANK_TO_REPORT=0)


set(LIBDIR_STRAT_OLD "" CACHE STRING "Utility variable (do not change)" INTERNAL)
if(NOT LIBDIR_STRAT_OLD STREQUAL LIBDIR_STRAT)
    unset(GASPI_DBG_LIB CACHE)
    set(LIBDIR_STRAT_OLD ${LIBDIR_STRAT} CACHE STRING "Utility variable (do not change)" FORCE INTERNAL)
endif()

find_library(GASPI_DBG_LIB GPI2-dbg PATHS ENV LD_LIBRARY_PATH ENV LIBRARY_PATH
                                    PATH_SUFFIXES lib lib64)
get_filename_component(GASPI_LOCATION ${GASPI_DBG_LIB} DIRECTORY)

if(GASPI_LOCATION STREQUAL "" AND NOT LIBDIR_STRAT STREQUAL SYSTEM_LD)
    message(FATAL_ERROR "GASPI library location not found, and LIBDIR_STRAT '${LIBDIR_STRAT}' requires to know")
endif()

if(LIBDIR_STRAT STREQUAL STATIC_LINK)
    unset(GASPI_DBG_LIB CACHE)
    find_library(GASPI_DBG_LIB libGPI2-dbg.a PATHS ENV LD_LIBRARY_PATH
                                                   ENV LIBRARY_PATH
                                             PATH_SUFFIXES lib lib64)
endif()

if(LIBDIR_STRAT STREQUAL CMAKE_RUNPATH)
    set(CMAKE_BUILD_RPATH "${GASPI_LOCATION};${CMAKE_BUILD_RPATH}")
endif()


add_executable(test_with_gaspi
               lapser_test_main.cpp
               ctx_tests.cpp
               no_lapser_instance.cpp
               simple_instance.cpp
               leftright_instance.cpp
               full_instance.cpp
               simple_multilapser.cpp
               checksum_works.cpp
               slack_tests.cpp
)
target_link_libraries(test_with_gaspi PUBLIC gtest lapser ${GASPI_DBG_LIB} -pthread)

#give access to internals (src) and public interface (include)
target_include_directories(test_with_gaspi PRIVATE ${PROJECT_SOURCE_DIR}/src
                                                   ${PROJECT_SOURCE_DIR}/include)


set(CHECK_COMMAND ./test_with_gaspi)
if(LIBDIR_STRAT STREQUAL ENV_LD)
    configure_file(env_script.sh.in env_script.sh @ONLY)
    set(CHECK_COMMAND ./env_script.sh "${CMAKE_CURRENT_BINARY_DIR}/test_with_gaspi")

endif()
add_custom_target(check
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt || :
                  COMMAND gaspi_run     -m ${PROJECT_SOURCE_DIR}/hosts.txt ${CHECK_COMMAND} VERBATIM
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt
                  DEPENDS test_with_gaspi
                  COMMENT "Running tests with gaspi, mode ${LAPSER_COMM}")

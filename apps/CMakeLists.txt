


set(LIBDIR_STRAT_OLD "" CACHE STRING "Utility variable (do not change)")
if(NOT LIBDIR_STRAT_OLD STREQUAL LIBDIR_STRAT)
    unset(GASPI_DBG_LIB CACHE)
    unset(GASPI_LIB CACHE)
    set(LIBDIR_STRAT_OLD ${LIBDIR_STRAT} CACHE STRING "Utility variable" FORCE)
endif()

find_library(GASPI_DBG_LIB GPI2-dbg PATHS ENV LD_LIBRARY_PATH ENV LIBRARY_PATH
                                    PATH_SUFFIXES lib lib64)
find_library(GASPI_LIB GPI2 PATHS ENV LD_LIBRARY_PATH ENV LIBRARY_PATH
                                    PATH_SUFFIXES lib lib64)
get_filename_component(GASPI_LOCATION ${GASPI_DBG_LIB} DIRECTORY)

if(GASPI_LOCATION STREQUAL "" AND NOT LIBDIR_STRAT STREQUAL SYSTEM_LD)
    message(FATAL_ERROR "GASPI library location not found, and LIBDIR_STRAT '${LIBDIR_STRAT}' requires to know")
endif()

if(LIBDIR_STRAT MATCHES _LD$)
    set(GASPI_DBG_LIB GPI2-dbg)
    set(GASPI_LIB GPI2)
elseif(LIBDIR_STRAT STREQUAL STATIC_LINK)
    unset(GASPI_DBG_LIB CACHE)
    unset(GASPI_LIB CACHE)
    find_library(GASPI_DBG_LIB libGPI2-dbg.a PATHS ENV LD_LIBRARY_PATH
                                                   ENV LIBRARY_PATH
                                             PATH_SUFFIXES lib lib64)
    find_library(GASPI_LIB libGPI2.a PATHS ENV LD_LIBRARY_PATH
                                           ENV LIBRARY_PATH
                                     PATH_SUFFIXES lib lib64)
elseif(LIBDIR_STRAT STREQUAL CMAKE_RUNPATH)
    set(GASPI_DBG_LIB GPI2-dbg)
    set(GASPI_LIB GPI2)
    set(CMAKE_BUILD_RPATH "${GASPI_LOCATION};${CMAKE_BUILD_RPATH}")
    set(BUILD_RPATH "${GASPI_LOCATION};${BUILD_RPATH}")
endif()


add_executable(exp exp.c)

target_link_libraries(exp gssp debug ${GASPI_DBG_LIB} optimized ${GASPI_LIB})
target_link_libraries(exp -pthread)

#give access to auxiliary GASPI functions
target_include_directories(exp PRIVATE ${PROJECT_SOURCE_DIR}/src/aux/)


set(APP_COMMAND ./exp)

if(LIBDIR_STRAT STREQUAL ENV_LD)
    configure_file(env_script.sh.in env_script.sh @ONLY)
    set(APP_COMMAND ./env_script.sh "${CMAKE_CURRENT_BINARY_DIR}/exp")
endif()

add_custom_target(run_app
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt || :
                  COMMAND gaspi_run     -m ${PROJECT_SOURCE_DIR}/hosts.txt ${APP_COMMAND} VERBATIM
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt
                  DEPENDS exp
                  COMMENT "Running app exp, mode ${GSSP_COMM}")

add_custom_target(run_app_d
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt || :
                  COMMAND gaspi_run     -m ${PROJECT_SOURCE_DIR}/hosts.txt -d ${APP_COMMAND} VERBATIM
                  COMMAND gaspi_cleanup -m ${PROJECT_SOURCE_DIR}/hosts.txt
                  DEPENDS exp
                  COMMENT "Running app exp, mode ${GSSP_COMM}")

add_compile_options(-Wall -Wextra -pedantic)

# auxiliary gaspi functions
add_subdirectory(aux)

# Deal with the top-level options
if(LAPSER_COMM MATCHES ^LOCAL)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/local.c")
endif()

if(LAPSER_COMM MATCHES ^PULL)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/pull.c")
endif()

if(LAPSER_COMM MATCHES ^PUSH)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/push.c")
endif()
add_definitions(-D${LAPSER_COMM})
message(STATUS "Communication method: ${LAPSER_COMM}")


add_library(lapser lapser.c $<TARGET_OBJECTS:gaspi_aux_functions> ${impl-files})

target_include_directories(lapser PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(lapser PRIVATE c_std_99)

# If I am building a shared library, it needs to be -fPIC
set_target_properties(lapser PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})


# TODO verify that the EXPORT stuff is correctly done
install(TARGETS lapser DESTINATION ${CMAKE_INSTALL_LIBDIR} EXPORT lapserconfig)
install(EXPORT lapserconfig FILE lapserconfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lapser)

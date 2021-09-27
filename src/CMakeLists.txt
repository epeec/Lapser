add_compile_options(-Wall -Wextra -pedantic)

# auxiliary gaspi functions
add_subdirectory(aux)

# Deal with the top-level options
if(GSSP_COMM MATCHES ^LOCAL)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/local.c")
endif()

if(GSSP_COMM MATCHES ^PULL)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/pull.c")
endif()

if(GSSP_COMM MATCHES ^PUSH)
    set(impl-files "${CMAKE_CURRENT_SOURCE_DIR}/push.c")
endif()
add_definitions(-D${GSSP_COMM})
message(STATUS "Communication method: ${GSSP_COMM}")


add_library(gssp gssp.c $<TARGET_OBJECTS:gaspi_aux_functions> ${impl-files})

target_include_directories(gssp PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(gssp PRIVATE c_std_99)

# If I am building a shared library, it needs to be -fPIC
set_target_properties(gssp PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})


# TODO verify that the EXPORT stuff is correctly done
install(TARGETS gssp DESTINATION ${CMAKE_INSTALL_LIBDIR} EXPORT gsspconfig)
install(EXPORT gsspconfig FILE gsspconfig.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/gssp)

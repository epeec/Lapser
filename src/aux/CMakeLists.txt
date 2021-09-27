# GASPI auxiliary files

add_library(gaspi_aux_functions OBJECT
                                now.c
                                now.h
                                queue.c
                                queue.h
                                testsome.c
                                testsome.h
                                waitsome.c
                                waitsome.h)

set_target_properties(gaspi_aux_functions PROPERTIES POSITION_INDEPENDENT_CODE ${BUILD_SHARED_LIBS})

#ifndef WAITSOME_H
#define WAITSOME_H

#include <GASPI.h>

void wait_or_die ( gaspi_segment_id_t
                 , gaspi_notification_id_t
                 , gaspi_notification_t expected
                 );

int wait_or_die_limited ( gaspi_segment_id_t
                        , gaspi_notification_id_t
                        , gaspi_notification_t expected
                        , gaspi_timeout_t timeout
                        );
#endif

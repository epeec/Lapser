#ifndef QUEUE_H
#define QUEUE_H

#include <GASPI.h>

void
notify_and_wait 
	( gaspi_segment_id_t const segment_id_remote
	, gaspi_rank_t const rank
	, gaspi_notification_id_t const notification_id
	, gaspi_notification_t const notification_value
	, gaspi_queue_id_t const queue
	);

void
write_and_wait 
	( gaspi_segment_id_t const segment_id_local
        , gaspi_offset_t const offset_local
       	, gaspi_rank_t const rank
	, gaspi_segment_id_t const segment_id_remote
	, gaspi_offset_t const offset_remote
	, gaspi_size_t const size
	, gaspi_queue_id_t const queue
	);

void
write_and_cycle
        ( gaspi_segment_id_t const segment_id_local
        , gaspi_offset_t const offset_local
        , gaspi_rank_t const rank
        , gaspi_segment_id_t const segment_id_remote
        , gaspi_offset_t const offset_remote
        , gaspi_size_t const size
        );

void
write_notify_and_wait
	( gaspi_segment_id_t const segment_id_local
        , gaspi_offset_t const offset_local
        , gaspi_rank_t const rank
        , gaspi_segment_id_t const segment_id_remote
        , gaspi_offset_t const offset_remote
        , gaspi_size_t const size
        , gaspi_notification_id_t const notification_id
        , gaspi_notification_t const notification_value
	, gaspi_queue_id_t const queue
	);

void
write_notify_and_cycle 
	( gaspi_segment_id_t const segment_id_local
        , gaspi_offset_t const offset_local
        , gaspi_rank_t const rank
        , gaspi_segment_id_t const segment_id_remote
        , gaspi_offset_t const offset_remote
        , gaspi_size_t const size
        , gaspi_notification_id_t const notification_id
        , gaspi_notification_t const notification_value
	);

void 
wait_for_flush_queue
        ( gaspi_queue_id_t const queue );

void wait_for_flush_queues();

#endif

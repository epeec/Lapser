#include "gssp_internal.h"
#include "aux/success_or_die.h"
#include "aux/queue.h"
#include "aux/waitsome.h"

#include <string.h>

int gssp_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version) {

    item_metadata * meta_write = meta_lookup[item_id];
    if(meta_write->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to set item I am not the producer off\n"
                         "Arguments - producer %d, my rank %d\n",
                         meta_write->producer, _gssp_rank);
        return -1;
    }

    item * to_write = lookup[item_id];

    to_write->version = version;
    to_write->checksum = gssp_item_hash(new_value, value_size, version);
    memcpy(to_write->value, new_value, value_size);

    uint64_t consumers = meta_write->consumers;
    for(gaspi_rank_t rem_rank=0; consumers && rem_rank<_gssp_num; ++rem_rank, consumers >>= 1) {
        if( !(consumers & 0x1) ) { continue; }
        gaspi_offset_t const rem_off = meta_write->consumers_offset[rem_rank];
        gaspi_offset_t const loc_off = meta_write->offset;

        write_notify_and_wait(GSSP_DATA_SEGMENT, loc_off,
                              rem_rank, GSSP_DATA_SEGMENT, rem_off, item_slot_size,
                              _gssp_rank, ITEM_NOT_OFFSET, GSSP_DATA_QUEUE);

    }

    return 0;
}

size_t gssp_get(Key    item_id,
                void * recv_buf,
                size_t buf_size,
                Clock  base_version,
                Clock  slack) {


    item_metadata * meta_read = meta_lookup[item_id];
    if( !(meta_read->consumers & (UINT64_C(1) << _gssp_rank))
       && meta_read->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to read item I am not declared as a consumer nor producer\n"
                         "Arguments - my rank %d, producer %d, consumer bitset %ld\n",
                         _gssp_rank, meta_read->producer, meta_read->consumers);
        return -1;
    }

    item * to_read = lookup[item_id];

    int retries = 5;
    Hash local_checksum = 0xdeadbeef;

wait_for_incoming:

    retries -= 1;
    if(retries <= 0) {
        gssp_log_fprintf("Too many retries due to failing checksums for the get request "
                         "with args: item=%ld, clock=%ld, slack=%ld\n"
                         "Stored hash: %lx ; Local hash %lx\n",
                         item_id, base_version, slack, to_read->checksum, local_checksum);
        return -1;
    }

    Clock time_read = to_read->version;
    // TODO finer control on how we wait, and how much
    if( _gssp_rank != meta_read->producer) {
        while(base_version > time_read + slack) {
            wait_or_die_limited(GSSP_DATA_SEGMENT, meta_read->producer, ITEM_NOT_OFFSET, 500 /*miliseconds*/);
	    time_read=to_read->version;
        }
    }


    if(base_version > time_read + slack) {
        gssp_log_fprintf("Stored clock version on item %ld is too old for the get request\n"
                         "Arguments - base_version %ld, slack %ld, stored version %ld\n",
                         item_id, base_version, slack, to_read->version);
        return -1;
    }


    Hash before = to_read->checksum;
    memcpy(recv_buf, to_read->value, buf_size);
    local_checksum = gssp_item_hash(recv_buf, buf_size, time_read);

    if(local_checksum != before) {
        // Hoping it is an ongoing write
        int res = wait_or_die_limited(GSSP_DATA_SEGMENT, meta_read->producer, ITEM_NOT_OFFSET, 500 /*miliseconds*/);
	gssp_log_fprintf("Different checksum in item %ld clock %ld item clock prev %ld now %ld, wait got %ld (before %lx now %lx calc %lx)\n", item_id, base_version, time_read, to_read->version, res, before, to_read->checksum, local_checksum);
        goto wait_for_incoming;
    }

    return 0;
}

#include "gssp_internal.h"

#include <string.h>
#include "aux/success_or_die.h"
#include "aux/waitsome.h"
#include "aux/queue.h"


int gssp_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version) {

    // Alternative to this metadata lookup - verify item location;
    // because the produced slots are contiguous and at the beginning
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
    return 0;
}


size_t gssp_get(Key    item_id,
                void * recv_buf,
                size_t buf_size,
                Clock  base_version,
                Clock  slack) {

    item_metadata * meta_read = meta_lookup[item_id];
    if(!(meta_read->consumers & (UINT64_C(1) << _gssp_rank)) && meta_read->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to read item I am not declared as a consumer nor producer\n"
                         "Arguments - my rank %d, producer %d, consumer bitset %ld\n",
                         _gssp_rank, meta_read->producer, meta_read->consumers);
        return -1;
    }

    item * to_read = lookup[item_id];

    Byte* base_item_pointer;
    SUCCESS_OR_DIE( gaspi_segment_ptr(GSSP_DATA_SEGMENT, (gaspi_pointer_t *) &base_item_pointer) );

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

    // TODO to think what about getting notifications about the status of the thing
    //      (like "last clock produced in any item")?
    if(meta_read->producer != _gssp_rank) {
        gaspi_offset_t const local_item_offset = (Byte*) to_read - base_item_pointer;
        while(base_version > to_read->version + slack) {

            gaspi_return_t ret;
            while( (ret = (gaspi_read_notify(GSSP_DATA_SEGMENT, local_item_offset,
                                             meta_read->producer, GSSP_DATA_SEGMENT,
                                             meta_read->offset, item_slot_size,
                                             ITEM_NOT_OFFSET, GSSP_DATA_QUEUE,
                                             GASPI_BLOCK)) ) == GASPI_QUEUE_FULL) {
                wait_for_flush_queue(GSSP_DATA_QUEUE);
            }

            wait_or_die(GSSP_DATA_SEGMENT, ITEM_NOT_OFFSET, 1);
        }
    }

    if(base_version > to_read->version + slack) {
        gssp_log_fprintf("Stored clock version on item %ld is too old for the get request\n"
                         "Arguments - base_version %ld, slack %ld, stored version %ld\n",
                          item_id, base_version, slack, to_read->version);
        return -1;
    }

    memcpy(recv_buf, to_read->value, buf_size);
    local_checksum = gssp_item_hash(recv_buf, buf_size, to_read->version);

    if(local_checksum != to_read->checksum) {
        // FIXME this is probably wrong in this case, I should have the other guy notifying me
        wait_or_die_limited(GSSP_DATA_SEGMENT, ITEM_NOT_OFFSET, 1, 500 /*miliseconds*/);
        goto wait_for_incoming;
    }

    return 0;
}

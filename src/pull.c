#include "gssp_internal.h"
#include "aux/success_or_die.h"
#include "aux/waitsome.h"
#include "aux/queue.h"

#include <string.h>
#include <inttypes.h>

int gssp_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version) {

    // Alternative to this metadata lookup - verify item location;
    // because the produced slots are contiguous and at the beginning
    item_metadata * meta_write = meta_lookup[item_id];
    if(meta_write->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to set item %"PRIu64", when the producer rank is %d\n",
                         item_id, meta_write->producer);
        return -1;
    }

    item * to_write = lookup[item_id];

    memcpy(to_write->value, new_value, value_size);
    to_write->checksum = gssp_item_hash(new_value, value_size, version);
    to_write->version = version;

    return 0;
}


size_t gssp_get(Key    item_id,
                void * recv_buf,
                size_t buf_size,
                Clock  base_version,
                Clock  slack) {

    item_metadata * meta_read = meta_lookup[item_id];

    item * to_read = lookup[item_id];

    Byte* base_item_pointer;
    SUCCESS_OR_DIE( gaspi_segment_ptr(GSSP_DATA_SEGMENT, (gaspi_pointer_t *) &base_item_pointer) );

    int retries = 5;
    Hash local_checksum = 0xdeadbeef;
    Hash before = 0xdeadbeef;
    Clock time_read = -1;
    gaspi_notification_id_t const item_notify_id = (item_id << 1) | 0x1;
    gaspi_offset_t const local_item_offset = (Byte*) to_read - base_item_pointer;

wait_for_incoming:

    retries -= 1;
    if(retries <= 0) {
        gssp_log_fprintf("Too many retries due to failing checksums for the get request "
                         "with args: item=%"PRIu64", clock=%"PRIu64", slack=%"PRIu64"; "
                         "stored hash: %"PRIx64"; local hash %"PRIx64"; stored clock %"PRIu64"; last read clock %"PRIu64"\n",
                         item_id, base_version, slack, to_read->checksum, local_checksum, to_read->version, time_read);
        return -1;
    }

    time_read = to_read->version;
    before = to_read->checksum;
    // TODO to think what about getting notifications about the status of the thing
    //      (like "last clock produced in any item")?
    if(meta_read->producer != _gssp_rank) {
        while(base_version > time_read + slack || \
              before != gssp_item_hash(to_read->value, buf_size, time_read)) {
            gaspi_return_t ret;
            while( (ret = (gaspi_read_notify(GSSP_DATA_SEGMENT, local_item_offset,
                                             meta_read->producer, GSSP_DATA_SEGMENT,
                                             meta_read->offset, item_slot_size,
                                             item_notify_id, GSSP_DATA_QUEUE,
                                             GASPI_BLOCK)) ) == GASPI_QUEUE_FULL) {
                wait_for_flush_queue(GSSP_DATA_QUEUE);
            }

            wait_or_die(GSSP_DATA_SEGMENT, item_notify_id, 1);
            time_read = to_read->version;
            before = to_read->checksum;
        }
    }

    if(base_version > time_read + slack) {
        gssp_log_fprintf("Stored clock version on item %"PRIu64" is too old for the get request "
                         "with args: base_version %"PRIu64", slack %"PRIu64", stored version %"PRIu64"\n",
                         item_id, base_version, slack, to_read->version);
        return -1;
    }

    memcpy(recv_buf, to_read->value, buf_size);
    local_checksum = gssp_item_hash(recv_buf, buf_size, time_read);

    if(local_checksum != to_read->checksum) {
        // FIXME this is probably wrong in this case, I should have the other guy notifying me
        int res = wait_or_die_limited(GSSP_DATA_SEGMENT, item_notify_id, 1, 500 /*miliseconds*/);
        gssp_log_fprintf("Different checksum in item %"PRIu64", clock %"PRIu64" item clock prev %"PRIu64" now %"PRIu64", "
                         "wait got %ld (before %"PRIx64" now %"PRIx64" calc %"PRIx64")\n",
                        item_id, base_version, time_read, to_read->version, res, before, to_read->checksum, local_checksum);
        goto wait_for_incoming;
    }

    return 0;
}

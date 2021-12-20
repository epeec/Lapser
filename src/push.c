#include "lapser_internal.h"
#include "aux/success_or_die.h"
#include "aux/queue.h"
#include "aux/waitsome.h"

#include <string.h>
#include <inttypes.h>

int lapser_set(Key        item_id,
               void *     new_value,
               size_t     value_size,
               Clock      version,
               lapser_ctx *ctx) {

    item_metadata * meta_write = meta_lookup[item_id];
    if(meta_write->producer != _lapser_rank) {
        lapser_log_fprintf("Trying to set item %"PRIu64", when the producer rank is %d\n",
                         item_id, meta_write->producer);
        return -1;
    }

    item * to_write = lookup[item_id];

    memcpy(to_write->value, new_value, value_size);
    to_write->checksum = lapser_item_hash(new_value, value_size, version);
    to_write->version = version;


    uint64_t *consumers_ptr = meta_write->consumers;
    uint64_t consumers = 0xdeadbeef;
    gaspi_notification_id_t const item_notify_id = (item_id << 1) | 0x1;
    for(gaspi_rank_t rem_rank=0; rem_rank<_lapser_num; ++rem_rank, consumers >>= 1) {

        if(rem_rank % 64 == 0) { consumers = consumers_ptr[rem_rank>>6]; }
        if( !(consumers & 0x1) ) { continue; }

        gaspi_offset_t const rem_off = meta_write->consumers_offset[rem_rank];
        gaspi_offset_t const loc_off = meta_write->offset;

        write_notify_and_wait(ctx->data_segment, loc_off,
                              rem_rank, ctx->data_segment, rem_off, item_slot_size,
                              item_notify_id, 1, ctx->data_queue);


    }

    return 0;
}


size_t lapser_get(Key        item_id,
                  void *     recv_buf,
                  size_t     buf_size,
                  Clock      base_version,
                  Clock      slack,
                  lapser_ctx *ctx) {

    item_metadata * meta_read = meta_lookup[item_id];
    if( !(meta_read->consumers[_lapser_rank>>6] & (UINT64_C(1) << (_lapser_rank%64)))
       && meta_read->producer != _lapser_rank) {
        lapser_log_fprintf("Trying to read item %"PRIu64", when I am not producer (which is %d) nor consumer (bitset: %"PRIu64")\n",
                         item_id, meta_read->producer, meta_read->consumers[_lapser_rank>>6]);
        return -1;
    }

    item * to_read = lookup[item_id];

    int retries = 5;
    Hash local_checksum = 0xdeadbeef;
    Clock time_read = -1;
    gaspi_notification_id_t const item_notify_id = (item_id << 1) | 0x1;

wait_for_incoming:

    retries -= 1;
    if(retries <= 0) {
        lapser_log_fprintf("Too many retries due to failing checksums for the get request "
                         "with args: item=%"PRIu64", clock=%"PRIu64", slack=%"PRIu64"; "
                         "stored hash: %"PRIx64"; local hash %"PRIx64"; stored clock %"PRIu64"; last read clock %"PRIu64"\n",
                         item_id, base_version, slack, to_read->checksum, local_checksum, to_read->version, time_read);
        return -1;
    }

    time_read = to_read->version;
    // TODO finer control on how we wait, and how much
    if( _lapser_rank != meta_read->producer) {
        while(base_version > time_read + slack) {
            wait_or_die_limited(ctx->data_segment, item_notify_id, 1, 500 /*miliseconds*/);
            time_read = to_read->version;
        }
    }


    if(base_version > time_read + slack) {
        lapser_log_fprintf("Stored clock version on item %"PRIu64" is too old for the get request "
                         "with args: base_version %"PRIu64", slack %"PRIu64", stored version %"PRIu64"\n",
                         item_id, base_version, slack, to_read->version);
        return -1;
    }


    Hash before = to_read->checksum;
    memcpy(recv_buf, to_read->value, buf_size);
    local_checksum = lapser_item_hash(recv_buf, buf_size, time_read);

    if(local_checksum != before) {
        // Hoping it is an ongoing write
        int res = wait_or_die_limited(ctx->data_segment, item_notify_id, 1, 500 /*miliseconds*/);
        lapser_log_fprintf("Different checksum in item %"PRIu64", clock %"PRIu64" item clock prev %"PRIu64" now %"PRIu64", "
                         "wait got %ld (before %"PRIx64" now %"PRIx64" calc %"PRIx64")\n",
                        item_id, base_version, time_read, to_read->version, res, before, to_read->checksum, local_checksum);
        goto wait_for_incoming;
    }

    return 0;
}

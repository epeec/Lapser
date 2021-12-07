#include "lapser_internal.h"

#include <string.h>
#include <inttypes.h>

int lapser_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version) {

    // Alternative to this metadata lookup - verify item location;
    // because the produced slots are contiguous and at the beginning
    item_metadata * meta_write = meta_lookup[item_id];
    if(meta_write->producer != _lapser_rank) {
        lapser_log_fprintf("Trying to set item %"PRIu64", when the producer rank is %d\n",
                         item_id, meta_write->producer);
        return -1;
    }

    item * to_write = lookup[item_id];

    to_write->version = version;
    to_write->checksum = lapser_item_hash(new_value, value_size, version);
    memcpy(to_write->value, new_value, value_size);
    return 0;
}

size_t lapser_get(Key    item_id,
                void * recv_buf,
                size_t buf_size,
                Clock  base_version,
                Clock  slack) {

    item * to_read = lookup[item_id];

    if(base_version > to_read->version + slack) {
        lapser_log_fprintf("Stored clock version on item %"PRIu64" is too old for the get request "
                         "with args: base_version %"PRIu64", slack %"PRIu64", stored version %"PRIu64"\n",
                         item_id, base_version, slack, to_read->version);
        return -1;
    }


    Hash calculated_checksum = lapser_item_hash(to_read->value, buf_size, to_read->version);
    if(to_read->checksum != calculated_checksum) {
        lapser_log_fprintf("Detected different hash values for the get request of item %"PRIu64" with "
                         "stored hash: %"PRIx64"and calculated hash %"PRIx64"\n",
                         item_id, to_read->checksum, calculated_checksum);
        return -1;
    }

    memcpy(recv_buf, to_read->value, buf_size);
    return 0;
}

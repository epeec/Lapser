#include "gssp_internal.h"

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
    if( !( meta_read->consumers & (UINT64_C(1) << _gssp_rank)) && meta_read->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to read item %"PRIu64", when I am not producer (which is %d) nor consumer (relevant bitset: %"PRIu64")\n",
                         item_id, meta_read->producer, meta_read->consumers);
        return -1;
    }

    item * to_read = lookup[item_id];

    if(base_version > to_read->version + slack) {
        gssp_log_fprintf("Stored clock version on item %"PRIu64" is too old for the get request "
                         "with args: base_version %"PRIu64", slack %"PRIu64", stored version %"PRIu64"\n",
                         item_id, base_version, slack, to_read->version);
        return -1;
    }


    Hash calculated_checksum = gssp_item_hash(to_read->value, buf_size, to_read->version);
    if(to_read->checksum != calculated_checksum) {
        gssp_log_fprintf("Detected different hash values for the get request of item %"PRIu64" with "
                         "stored hash: %"PRIx64"and calculated hash %"PRIx64"\n",
                         item_id, to_read->checksum, calculated_checksum);
        return -1;
    }

    memcpy(recv_buf, to_read->value, buf_size);
    return 0;
}

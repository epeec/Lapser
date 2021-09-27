#include "gssp_internal.h"

#include <string.h>


int gssp_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version) {

    // Alternative to this metadata lookup - verify item location;
    // because the produced slots are contiguous and at the beginning
    item_metadata * meta_write = meta_lookup[item_id];
    if(meta_write->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to set item I am not the producer off\n");
        gssp_log_fprintf("Arguments - producer %d, my rank %d\n",meta_write->producer
                                                                ,_gssp_rank);
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
    if( !( meta_read->consumers & (1 << _gssp_rank))
       && meta_read->producer != _gssp_rank) {
        gssp_log_fprintf("Trying to read item I am not declared as a "
                         "consumer nor producer\n");
        gssp_log_fprintf("Arguments - my rank %d, producer %d, consumer bitset %ld\n"
                         , _gssp_rank, meta_read->producer, meta_read->consumers);
        return -1;
    }

    item * to_read = lookup[item_id];

    if(base_version > to_read->version + slack) {
        gssp_log_fprintf("Stored clock version on item %ld is too old "
                         "for the get request\n", item_id);
        gssp_log_fprintf("Arguments - base_version %ld, slack %ld, stored "
                         "version %ld\n", base_version, slack, to_read->version);
        return -1;
    }


    Hash calculated_checksum = gssp_item_hash(to_read->value, buf_size, to_read->version);
    if(to_read->checksum != calculated_checksum) {
        gssp_log_fprintf("Detected different hash values for the get request"
                         "of item %ld\n", item_id);
        gssp_log_fprintf("Stored hash: %lx ; Calculated hash %lx\n",
                         to_read->checksum, calculated_checksum);
        return -1;
    }

    memcpy(recv_buf, to_read->value, buf_size);
    return 0;
}

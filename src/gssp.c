#include "gssp_internal.h"

#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <GASPI.h>
#include "aux/success_or_die.h"
#include "aux/queue.h"
#include "aux/waitsome.h"

// Memory location for these variables.
// For documentation, see gssp_internal.h
item** lookup;
item_metadata** meta_lookup;
gaspi_offset_t* consumers_offsets;
size_t item_slot_size;
gaspi_rank_t _gssp_rank, _gssp_num;


// DJB hash, adapted from http://www.cse.yorku.ca/~oz/hash.html
static Hash djb_hash(Byte const data[], size_t len)
{
    Hash res = 5381;

    for(size_t i=0; i<len; ++i)
        res = ((res << 5) + res) ^ data[i]; /* hash * 33 ^ c */

    return res;
}

// FIXME use a more robust way to change the checksum
// Doing this so that checksum also depends on version
Hash gssp_item_hash(Byte const data[], size_t len, Clock age) {
    return djb_hash(data, len) + age;
}


// TODO: declare inline (?)
// TODO: use this function instead of others
// TODO: think to do the same for metadata
item * gssp_get_item_structure(Key item_id) {
    return lookup[item_id];
}


// TODO Better logging facilities
void gssp_log_printf(const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    printf("%ld:%05.2f, %02d: ", t.tv_sec, (t.tv_nsec)/1e6, _gssp_rank);

    vprintf(fmt, args);
    va_end(args);
}

static FILE *log_out;
void gssp_log_fprintf(const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    fprintf(log_out,"%ld:%05.2f, %02d: ", t.tv_sec, (t.tv_nsec)/1e6, _gssp_rank);

    vfprintf(log_out, fmt, args);
    fflush(log_out);
    va_end(args);
}

static size_t filter_lists(Key a[], size_t alen,
                           Key b[], size_t blen) {

    size_t i = 0, j = 0, k = 0;

    while(j < blen) {
        if(i == alen || b[j] < a[i]) {
            b[k++] = b[j++];
        } else if (b[j] > a[i]) {
            i++;
        } else { //b[j] == a[i]
            i++ ; j++;
        }
    }

    return k;
}

static size_t remove_duplicates(Key list[], size_t len) {

    size_t i = 0, j = 0;

    if(len == 0) { return 0; }

    while(i < len) {
        if(list[i] == list[j]) { i++; }
        else { list[++j] = list[i++]; }
    }

    return j+1;
}

static int compare_keys(const void *a, const void *b) {
    return (*(Key *) a) - (*(Key *) b);
}


// Sort to increasing order, remove duplicates, and then remove
// all the keys I produce myself from the list of keys to consume
static inline void sanitize_key_lists(Key to_produce[], size_t *plen,
                                      Key to_consume[], size_t *clen) {

    if(*plen > 0) {
        qsort(to_produce, *plen, sizeof(Key), compare_keys);
        *plen = remove_duplicates(to_produce, *plen);
    } 

    if(*clen > 0) {
        qsort(to_consume, *clen, sizeof(Key), compare_keys);
        *clen = remove_duplicates(to_consume, *clen);
    }

    *clen = filter_lists(to_produce, *plen, to_consume, *clen);
    return;
}




int gssp_init(int      total_items,
              size_t   size_of_item,
              Key      orig_keys_to_produce[],
              size_t   num_keys_to_produce,
              Key      orig_keys_to_consume[],
              size_t   num_keys_to_consume,
              uint16_t slack)

{
    // -1) Maybe create separate groups?
    // 0) Sanitize arguments and key lists
    // 1) Allocate control segment
    // 2) fetch & add to CONTROL_RANK, start to fill entries according to keys
    // 3) everybody communicates entries (allgather)
    //if push mode:
    // 4) set the consumers field, and push it to the producer.
    //at the end:
    // Y) allocate data segment
    // Z) parse the item metadata to allow faster lookups to data we are interested


    SUCCESS_OR_DIE(gaspi_proc_rank(&_gssp_rank));
    SUCCESS_OR_DIE(gaspi_proc_num(&_gssp_num));
    char log_path[FILENAME_MAX] = ".";
    char log_name[FILENAME_MAX];
    // TODO propagate the CWD info from root to other processes (they are writing in $HOME)
    //getcwd(log_path, FILENAME_MAX);
    snprintf(log_name, FILENAME_MAX, "/gssp-%d.out", _gssp_rank);
    strncat(log_path, log_name, FILENAME_MAX);
    log_out = fopen(log_path, "a");
    gssp_log_fprintf("After proc init\n");


    // 0)
    size_t total_prod_size = sizeof(Key) * num_keys_to_produce;
    Key * keys_to_produce = malloc(total_prod_size);
    // TODO check for failed malloc
    if(orig_keys_to_produce) {
        memcpy(keys_to_produce, orig_keys_to_produce, total_prod_size);
    }

    size_t total_cons_size = sizeof(Key) * num_keys_to_consume;
    Key * keys_to_consume = malloc(total_cons_size);
    // TODO check for failed malloc
    if(orig_keys_to_consume) {
        memcpy(keys_to_consume, orig_keys_to_consume, total_cons_size);
    }

    sanitize_key_lists(keys_to_produce, &num_keys_to_produce,
                       keys_to_consume, &num_keys_to_consume);

    // 1)
    gssp_log_fprintf("Starting to create control segment\n");
    gaspi_size_t const control_segment_size = sizeof(gaspi_atomic_value_t) + total_items * sizeof(item_metadata);
    SUCCESS_OR_DIE(
    gaspi_segment_create(GSSP_CONTROL_SEGMENT, control_segment_size,
                         GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED));
    gssp_log_fprintf("Created control segment\n");


    // 2)
    gssp_log_fprintf("Reserve space in control segment\n");
    gaspi_offset_t start_of_range;

    SUCCESS_OR_DIE(
    gaspi_atomic_fetch_add(GSSP_CONTROL_SEGMENT,
                           /*offset*/ 0,
                           GSSP_CONTROL_RANK,
                           num_keys_to_produce, &start_of_range, GASPI_BLOCK));

    gssp_log_fprintf("Reserved space in control segment: [%ld, %ld[\n", start_of_range, start_of_range+num_keys_to_produce);


    gaspi_pointer_t base_seg;
    item_metadata *meta;
    SUCCESS_OR_DIE( gaspi_segment_ptr(GSSP_CONTROL_SEGMENT, &base_seg) );
    // Offset because atomic counter comes first
    meta = (item_metadata*) ((char*) base_seg +  sizeof(gaspi_atomic_value_t));

    // Round up slot size to 8 bytes (because of alignment)
    item_slot_size = (sizeof(item) + sizeof(Byte[size_of_item]) + 7) & ~(7);

    gssp_log_fprintf("Start filling control segment\n", start_of_range);
    for(size_t i=start_of_range,j=0;i<start_of_range+num_keys_to_produce; ++i, ++j) {
        meta[i].item_id = keys_to_produce[j];
        meta[i].producer = _gssp_rank;
        meta[i].offset = j*item_slot_size;
    }

    // 3)
    gaspi_notification_id_t const metadata_write      = _gssp_rank;
    gaspi_offset_t          const metadata_offset     = start_of_range * sizeof(item_metadata)
                                                        + sizeof(gaspi_atomic_value_t);
    gaspi_size_t            const metadata_size       = num_keys_to_produce * sizeof(item_metadata);


    for(gaspi_rank_t remote_rank=0; remote_rank<_gssp_num; ++remote_rank) {
        if(remote_rank == _gssp_rank) { continue ; }
        gssp_log_fprintf("Sending full info to rank %d\n", remote_rank);
        write_notify_and_wait(GSSP_CONTROL_SEGMENT, metadata_offset,
                              remote_rank, GSSP_CONTROL_SEGMENT, metadata_offset,
                              metadata_size,
                              metadata_write, PRODUCE_WRITTEN, GSSP_CONTROL_QUEUE);
    }

    gssp_log_fprintf("Waiting for notifications\n");
    for(gaspi_rank_t remote_rank=0; remote_rank<_gssp_num; ++remote_rank) {
        if(remote_rank == _gssp_rank) { continue ; }
        wait_or_die(GSSP_CONTROL_SEGMENT, remote_rank, PRODUCE_WRITTEN);
    }

    // 4) registering interest
    gssp_log_fprintf("Start signaling consumers\n");
    for(int i=0; i<total_items; ++i) {
        for(size_t j=0; j<num_keys_to_consume; ++j) {
            if(meta[i].item_id != keys_to_consume[j]) { continue ; }
#if defined(PUSH_COMM)
            // try to register right away
            // well, where am *I* storing this...
            meta[i].offset = (num_keys_to_produce + j)*item_slot_size;

            gaspi_offset_t c_loc_offset =
                (Byte*)&meta[i].offset-(Byte*)base_seg;

            gaspi_offset_t c_rem_offset =
                (Byte*)&meta[i].consumers_offset[_gssp_rank]-(Byte*)base_seg;

            write_and_wait(GSSP_CONTROL_SEGMENT, c_loc_offset,
                           meta[i].producer, GSSP_CONTROL_SEGMENT, c_rem_offset,
                           sizeof(gaspi_offset_t), GSSP_CONTROL_QUEUE);

            // TODO maybe use an atomic (fetch) add? I don't care about other's bits,
            //      and if there are no bugs/no retrying to register, + <=> |
            gaspi_offset_t consumer_offset=(Byte*)&meta[i].consumers-(Byte*)base_seg;
            uint64_t *prev_value = &meta[i].consumers;
            uint64_t comparator, desired;
            do {
                comparator = *prev_value;
                desired = comparator | 1 << _gssp_rank;
                SUCCESS_OR_DIE(
                gaspi_atomic_compare_swap(GSSP_CONTROL_SEGMENT,
                                          consumer_offset,
                                          meta[i].producer,
                                          comparator, desired, prev_value,
                                          GASPI_BLOCK));
            } while(*prev_value != comparator);
#endif
            meta[i].consumers |= 1 << _gssp_rank;
            // This key to consume is already done, go to next item in outer loop
            break;
        }
    }
    gssp_log_fprintf("End signaling consumers\n");


    // Y)
    gaspi_size_t const data_segment_size = (num_keys_to_produce + num_keys_to_consume) * item_slot_size;
    SUCCESS_OR_DIE(
    gaspi_segment_create(GSSP_DATA_SEGMENT, data_segment_size,
                         GASPI_GROUP_ALL, GASPI_BLOCK, GASPI_MEM_INITIALIZED));

    // Z) Do stuff with the new metadata
    lookup = calloc(total_items, sizeof *lookup);
    assert(lookup != NULL);

    Byte* base_item_pointer;
    SUCCESS_OR_DIE( gaspi_segment_ptr(GSSP_DATA_SEGMENT, (gaspi_pointer_t *) &base_item_pointer) );

    int item_order_number = 0;

    for(size_t i=0; i<num_keys_to_produce; ++i) {
        Key item_id = keys_to_produce[i];
        item * to_produce = (item*)( base_item_pointer
                                     + (item_order_number++)
                                     * item_slot_size);

        to_produce->checksum = gssp_item_hash(to_produce->value, size_of_item, to_produce->version);
        lookup[item_id] = to_produce;
    }
    for(size_t i=0; i<num_keys_to_consume; ++i) {
        Key item_id = keys_to_consume[i];
        item * to_consume = (item*)( base_item_pointer
                                     + (item_order_number++)
                                     * item_slot_size);

        to_consume->checksum = gssp_item_hash(to_consume->value, size_of_item, to_consume->version);
        lookup[item_id] = to_consume;
    }

    meta_lookup = malloc(sizeof *meta_lookup * (total_items));
    assert(meta_lookup != NULL);
    for(int i=0; i<total_items; ++i) {
        meta_lookup[meta[i].item_id] = &meta[i];
    }

    // Just to check the consistency of everything
    for(size_t i=0; i<num_keys_to_produce; ++i) {
        int id = keys_to_produce[i];
        assert( meta_lookup[id]->offset ==
                (gaspi_offset_t) ((Byte*) lookup[id] - base_item_pointer));
    }

#if defined(PUSH_COMM)
    for(size_t i=0; i<num_keys_to_consume; ++i) {
        int id = keys_to_consume[i];
        assert( meta_lookup[id]->offset ==
                (gaspi_offset_t) ((Byte*) lookup[id] - base_item_pointer));
    }
#endif

    free(keys_to_produce);
    free(keys_to_consume);
    return 0;
}


int gssp_finish() {

    gssp_log_fprintf("Deleting auxiliary data structures\n");
    free(lookup);
    free(meta_lookup);

    wait_for_flush_queue(GSSP_CONTROL_QUEUE);
    wait_for_flush_queue(GSSP_DATA_QUEUE);

    gssp_log_fprintf("Waiting at barrier to delete segments\n");
    SUCCESS_OR_DIE( gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK) );

    gssp_log_fprintf("Deleting segments\n");
    SUCCESS_OR_DIE( gaspi_segment_delete(GSSP_DATA_SEGMENT) );
    SUCCESS_OR_DIE( gaspi_segment_delete(GSSP_CONTROL_SEGMENT) );

    gssp_log_fprintf("Done\n");

    return 0;
}

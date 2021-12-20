#include "lapser_internal.h"

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
// For documentation, see lapser_internal.h

struct _lapser_ctx _lapser_context_array[LAPSER_MAX_CONCURRENT] =
    {
        // Pre-initialized, for when we only use 1 context
      [0] = { .control_segment = 0, .control_queue = 0, .control_rank = 0,
          .data_segment = 1, .data_queue = 1, .gpi_group = GASPI_GROUP_ALL }
    };

lapser_ctx *_lapser_global_ctx = &_lapser_context_array[0];
item** lookup;
item_metadata** meta_lookup;
gaspi_offset_t* consumers_offsets;
size_t item_slot_size;
gaspi_rank_t _lapser_rank, _lapser_num;
lapser_ctx* _lapser_global_ctx;


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
Hash lapser_item_hash(Byte const data[], size_t len, Clock age) {
    return djb_hash(data, len) + age;
}


// TODO: declare inline (?)
// TODO: use this function instead of others
// TODO: think to do the same for metadata
item * lapser_get_item_structure(Key item_id) {
    return lookup[item_id];
}


// TODO Better logging facilities
void lapser_log_printf(const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    printf("%ld:%05.2f, %02d: ", t.tv_sec, (t.tv_nsec)/1e6, _lapser_rank);

    vprintf(fmt, args);
    va_end(args);
}

static FILE *log_out;
void lapser_log_fprintf(const char *fmt, ...) {

    va_list args;
    va_start(args, fmt);

    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    fprintf(log_out,"%ld:%05.2f, %02d: ", t.tv_sec, (t.tv_nsec)/1e6, _lapser_rank);

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




int lapser_init(int        total_items,
                size_t     size_of_item,
                Key        orig_keys_to_produce[],
                size_t     num_keys_to_produce,
                Key        orig_keys_to_consume[],
                size_t     num_keys_to_consume,
                uint16_t   slack,
                lapser_ctx *ctx)

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


    SUCCESS_OR_DIE(gaspi_proc_rank(&_lapser_rank));
    SUCCESS_OR_DIE(gaspi_proc_num(&_lapser_num));
    char log_path[FILENAME_MAX] = ".";
    char log_name[FILENAME_MAX];
    // TODO propagate the CWD info from root to other processes (they are writing in $HOME)
    //getcwd(log_path, FILENAME_MAX);
    snprintf(log_name, FILENAME_MAX, "/lapser-%d.out", _lapser_rank);
    strncat(log_path, log_name, FILENAME_MAX);
    log_out = fopen(log_path, "a");
    lapser_log_fprintf("After proc init\n");


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
    lapser_log_fprintf("Starting to create control segment\n");
    gaspi_size_t const control_segment_size = sizeof(gaspi_atomic_value_t) + total_items * sizeof(item_metadata);
    SUCCESS_OR_DIE(
    gaspi_segment_create(ctx->control_segment, control_segment_size,
                         ctx->gpi_group, GASPI_BLOCK, GASPI_MEM_INITIALIZED));
    lapser_log_fprintf("Created control segment\n");


    // 2)
    lapser_log_fprintf("Reserve space in control segment\n");
    gaspi_offset_t start_of_range;

    SUCCESS_OR_DIE(
    gaspi_atomic_fetch_add(ctx->control_segment,
                           /*offset*/ 0,
                           ctx->control_rank,
                           num_keys_to_produce, &start_of_range, GASPI_BLOCK));

    lapser_log_fprintf("Reserved space in control segment: [%ld, %ld[\n", start_of_range, start_of_range+num_keys_to_produce);


    gaspi_pointer_t base_seg;
    item_metadata *meta;
    SUCCESS_OR_DIE( gaspi_segment_ptr(ctx->control_segment, &base_seg) );
    // Offset because atomic counter comes first
    meta = (item_metadata*) ((char*) base_seg +  sizeof(gaspi_atomic_value_t));

    // Round up slot size to 8 bytes (because of alignment)
    item_slot_size = (sizeof(item) + sizeof(Byte[size_of_item]) + 7) & ~(7);
    ctx->item_slot_size = item_slot_size;

    lapser_log_fprintf("Start filling control segment\n", start_of_range);
    for(size_t i=start_of_range,j=0;i<start_of_range+num_keys_to_produce; ++i, ++j) {
        meta[i].item_id = keys_to_produce[j];
        meta[i].producer = _lapser_rank;
        meta[i].offset = j*item_slot_size;
    }

    // 3)
    gaspi_notification_id_t const metadata_write      = _lapser_rank;
    gaspi_offset_t          const metadata_offset     = start_of_range * sizeof(item_metadata)
                                                        + sizeof(gaspi_atomic_value_t);
    gaspi_size_t            const metadata_size       = num_keys_to_produce * sizeof(item_metadata);


    for(gaspi_rank_t remote_rank=0; remote_rank<_lapser_num; ++remote_rank) {
        if(remote_rank == _lapser_rank) { continue ; }
        lapser_log_fprintf("Sending full info to rank %d\n", remote_rank);
        write_notify_and_wait(ctx->control_segment, metadata_offset,
                              remote_rank, ctx->control_segment, metadata_offset,
                              metadata_size,
                              metadata_write, PRODUCE_WRITTEN, ctx->control_queue);
    }

    lapser_log_fprintf("Waiting for notifications\n");
    for(gaspi_rank_t remote_rank=0; remote_rank<_lapser_num; ++remote_rank) {
        if(remote_rank == _lapser_rank) { continue ; }
        wait_or_die(ctx->control_segment, remote_rank, PRODUCE_WRITTEN);
    }

#if defined(PUSH_COMM)
    // 4) registering interest
    lapser_log_fprintf("Start signaling consumers\n");
    size_t const cons_index = _lapser_rank >> 6;
    size_t const cons_off = _lapser_rank % 64;
    for(int i=0; i<total_items; ++i) {

        Key *k = bsearch(&meta[i].item_id, keys_to_consume, num_keys_to_consume,
                         sizeof(Key), compare_keys);
        if(k == NULL) {
            continue;
        }
        // try to register right away
        // well, where am *I* storing this...
        size_t j = k - keys_to_consume;
        meta[i].offset = (num_keys_to_produce + j)*item_slot_size;

        gaspi_offset_t c_loc_offset =
            (Byte*)&meta[i].offset-(Byte*)base_seg;

        gaspi_offset_t c_rem_offset =
            (Byte*)&meta[i].consumers_offset[_lapser_rank]-(Byte*)base_seg;

        write_and_wait(ctx->control_segment, c_loc_offset,
                       meta[i].producer, ctx->control_segment, c_rem_offset,
                       sizeof(gaspi_offset_t), ctx->control_queue);

        // TODO maybe use an atomic (fetch) add? I don't care about other's bits,
        //      and if there are no bugs/no retrying to register, + <=> |
        gaspi_offset_t consumer_offset=(Byte*)&meta[i].consumers[cons_index] -(Byte*)base_seg;
        uint64_t *prev_value = &meta[i].consumers[cons_index];
        uint64_t comparator, desired;
        do {
            comparator = *prev_value;
            desired = comparator | UINT64_C(1) << (cons_off);
            SUCCESS_OR_DIE(
            gaspi_atomic_compare_swap(ctx->control_segment,
                                      consumer_offset,
                                      meta[i].producer,
                                      comparator, desired, prev_value,
                                      GASPI_BLOCK));
        } while(*prev_value != comparator);
        meta[i].consumers[cons_index] |= UINT64_C(1) << cons_off;
    }
    lapser_log_fprintf("End signaling consumers\n");
#endif


    // Y)
    lapser_log_fprintf("Start allocating data segment & auxiliary data structures\n");
    gaspi_size_t const data_segment_size = (num_keys_to_produce + num_keys_to_consume) * item_slot_size;
    SUCCESS_OR_DIE(
    gaspi_segment_create(ctx->data_segment, data_segment_size,
                         ctx->gpi_group, GASPI_BLOCK, GASPI_MEM_INITIALIZED));

    // Z) Do stuff with the new metadata
    lookup = calloc(total_items, sizeof *lookup);
    assert(lookup != NULL);

    Byte* base_item_pointer;
    SUCCESS_OR_DIE( gaspi_segment_ptr(ctx->data_segment, (gaspi_pointer_t *) &base_item_pointer) );

    int item_order_number = 0;

    for(size_t i=0; i<num_keys_to_produce; ++i) {
        Key item_id = keys_to_produce[i];
        item * to_produce = (item*)( base_item_pointer
                                     + (item_order_number++)
                                     * item_slot_size);

        to_produce->checksum = lapser_item_hash(to_produce->value, size_of_item, to_produce->version);
        lookup[item_id] = to_produce;
    }
    for(size_t i=0; i<num_keys_to_consume; ++i) {
        Key item_id = keys_to_consume[i];
        item * to_consume = (item*)( base_item_pointer
                                     + (item_order_number++)
                                     * item_slot_size);

        to_consume->checksum = lapser_item_hash(to_consume->value, size_of_item, to_consume->version);
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

    ctx->slack = slack;
    free(keys_to_produce);
    free(keys_to_consume);
    lapser_log_fprintf("Finished initialization\n");
    return 0;
}


int lapser_finish(lapser_ctx *ctx) {

    lapser_log_fprintf("Deleting auxiliary data structures\n");
    free(lookup);
    free(meta_lookup);

    wait_for_flush_queue(ctx->control_queue);
    wait_for_flush_queue(ctx->data_queue);

    lapser_log_fprintf("Waiting at barrier to delete segments\n");
    SUCCESS_OR_DIE( gaspi_barrier(ctx->gpi_group, GASPI_BLOCK) );

    lapser_log_fprintf("Deleting segments\n");
    SUCCESS_OR_DIE( gaspi_segment_delete(ctx->data_segment) );
    SUCCESS_OR_DIE( gaspi_segment_delete(ctx->control_segment) );

    if(ctx != _lapser_global_ctx) {
        lapser_log_fprintf("Zeroing out ctx, to prevent use-after-free\n");
        memset(ctx, '\0', sizeof(lapser_ctx));
    }

    lapser_log_fprintf("Done\n");

    return 0;
}

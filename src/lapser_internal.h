#pragma once

#include <GASPI.h>
#include "multilapser.h"

// TODO check limit at lapser_init
#define LAPSER_MAX_NPROCS 256

typedef struct {
    Key             item_id;
    gaspi_rank_t    producer;
    gaspi_offset_t  offset;
#if defined(PUSH_COMM)
    uint64_t        consumers[LAPSER_MAX_NPROCS >> 6];
    gaspi_offset_t  consumers_offset[LAPSER_MAX_NPROCS];
#endif
} item_metadata;

typedef struct {
    Clock version;
    Hash checksum;
    Byte  value[];
} item;

struct _lapser_ctx {
    uint16_t           slack;
    // Auxiliary datastructure: looking up the item location
    // using the key as indexer: Key -> item*
    item**             lookup;
    // Auxiliary datastructure: looking up item metadata
    // using the key as indexer: Key -> item_metadata*
    item_metadata**    meta_lookup;
    gaspi_segment_id_t data_segment;
    gaspi_queue_id_t   data_queue;
    size_t             item_slot_size;
    gaspi_rank_t       control_rank;
    gaspi_segment_id_t control_segment;
    gaspi_queue_id_t   control_queue;
    gaspi_group_t      gpi_group;
};

#define LAPSER_MAX_CONCURRENT 8
extern struct _lapser_ctx _lapser_context_array[LAPSER_MAX_CONCURRENT];

// Getter for the whole structure
item * lapser_get_item_structure(Key item_id, lapser_ctx *c);


// TODO find a way to manage the read_notify notification id values -
//      I cannot use directly the item id (too large), and have to use
//      different ids (otherwise they overwrite each other)
// /!\ I can only use 16 bit identifiers, this means max of 65536
enum notify_situations {
    PRODUCE_READY = 1024,
    PRODUCE_WRITTEN,
    CONSUME_WRITTEN,
    ITEM_NOT_OFFSET = 2048
};


// Just to not call the proc_rank/num over and over
extern gaspi_rank_t _lapser_rank, _lapser_num;

// Hash function
Hash lapser_item_hash(Byte const data[], size_t len, Clock age);

// logging functions
void lapser_log_printf(const char *fmt, ...);
void lapser_log_fprintf(const char *fmt, ...);


// TODO define error codes
// TODO Key bounds checking get/set
// TODO better handling of variable size: if last set only used K bytes, get only returns those
//      bytes
// TODO think if I should really include consumers when not needed (like in PULL)

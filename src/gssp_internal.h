#pragma once

#include "gssp.h"
#include <GASPI.h>

// TODO: instead of starting with 0, start with highest possible segment/queue
//        (to not clobber lower numbers,
//         for apps already using gaspi for something else)
#define GSSP_CONTROL_RANK 0
#define GSSP_CONTROL_SEGMENT 0
#define GSSP_CONTROL_QUEUE 0
#define GSSP_DATA_SEGMENT 1
#define GSSP_DATA_QUEUE 1


// TODO expand above 64 processes
typedef struct {
    Key             item_id;
    gaspi_rank_t    producer;
    //Byte            consumers[128/8];
    uint64_t        consumers;
    gaspi_offset_t  offset;
#if defined(PUSH_COMM)
    gaspi_offset_t  consumers_offset[64];
#endif
} item_metadata;

typedef struct {
    Clock version;
    Hash checksum;
    Byte  value[];
} item;
// Getter for the whole structure
item * gssp_get_item_structure(Key item_id);

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

// Auxiliary datastructure: looking up the item location
// using the key as indexer: Key -> item*
extern item** lookup;

// Auxiliary datastructure: looking up item metadata
// using the key as indexer: Key -> item_metadata*
// TODO maybe try to avoid the meta_lookup if it is a get of local memory?
//      some kind of caching/flag in item?
extern item_metadata** meta_lookup;

// Size each item occupies in the data segment
extern size_t item_slot_size;

// Just to not call the proc_rank/num over and over
extern gaspi_rank_t _gssp_rank, _gssp_num;

// Hash function
Hash gssp_item_hash(Byte const data[], size_t len, Clock age);

// logging functions
void gssp_log_printf(const char *fmt, ...);
void gssp_log_fprintf(const char *fmt, ...);


// TODO define error codes
// TODO Key bounds checking get/set
// TODO better handling of variable size: if last set only used K bytes, get only returns those
//      bytes
// TODO think if I should really include consumers when not needed (like in PULL)


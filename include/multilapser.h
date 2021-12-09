#pragma once

#include <stdint.h>
#include <stddef.h>

typedef  unsigned char     Byte;
typedef  uint64_t          Key;
typedef  uint64_t          Clock;
typedef  uint64_t          Hash;

typedef struct _lapser_ctx lapser_ctx;

int lapser_init(int         total_items,
                size_t      size_of_item,
                Key         keys_to_produce[],
                size_t      num_keys_to_produce,
                Key         keys_to_consume[],
                size_t      num_keys_to_consume,
                uint16_t    slack,
                lapser_ctx *ctx);

int lapser_set(Key        item_id,
               void *     new_value,
               size_t     value_size,
               Clock      version,
               lapser_ctx *ctx);

size_t lapser_get(Key        item_id,
                  void *     recv_buf,
                  size_t     buf_size,
                  Clock      base_version,
                  Clock      slack,
                  lapser_ctx *ctx);

int lapser_finish(lapser_ctx *ctx);

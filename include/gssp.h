#pragma once

#include <stdint.h>
#include <stddef.h>

typedef  unsigned char     Byte;
typedef  uint64_t          Key;
typedef  uint64_t          Clock;
typedef  uint64_t          Hash;

int gssp_init(int      total_items,
              size_t   size_of_item,
              Key      keys_to_produce[],
              size_t   num_keys_to_produce,
              Key      keys_to_consume[],
              size_t   num_keys_to_consume,
              uint16_t slack);

int gssp_set(Key    item_id,
             void * new_value,
             size_t value_size,
             Clock  version);

size_t gssp_get(Key    item_id,
                void * recv_buf,
                size_t buf_size,
                Clock  base_version,
                Clock  slack);


int gssp_finish();

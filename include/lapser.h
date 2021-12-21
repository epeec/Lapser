#pragma once

#include <stdint.h>
#include <stddef.h>

#include "multilapser.h"


extern lapser_ctx *_lapser_global_ctx;

#define lapser_init(total_items, size_of_item, keys_to_produce, num_keys_to_produce, keys_to_consume, num_keys_to_consume, slack) \
    (lapser_init)(total_items, size_of_item, keys_to_produce, num_keys_to_produce, keys_to_consume, num_keys_to_consume, slack, &_lapser_global_ctx)

#define lapser_set(item_id, new_value, value_size, version) \
    (lapser_set)(item_id, new_value, value_size, version, _lapser_global_ctx)

#define lapser_get(item_id, recv_buf, buf_size, base_version, slack) \
    (lapser_get)(item_id, recv_buf, buf_size, base_version, slack, _lapser_global_ctx)

#define lapser_finish() (lapser_finish)(_lapser_global_ctx)

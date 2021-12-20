#include "lapser_internal.h"

#include <GASPI.h>
#include "aux/success_or_die.h"
#include <string.h>
#include <stdbool.h>

// FIXME turn this into thread-safe code

static bool is_used[LAPSER_MAX_CONCURRENT] = { [0] = true };


static gaspi_return_t lapser_new_ctx(lapser_ctx ** res) {

    size_t i=0;
    while(i < LAPSER_MAX_CONCURRENT && is_used[i]) { i++; };

    if(i >= LAPSER_MAX_CONCURRENT) return GASPI_ERROR;

    is_used[i] = 1;
    *res = &_lapser_context_array[i];

    return GASPI_SUCCESS;
}

gaspi_return_t lapser_free_ctx(lapser_ctx * ctx) {

    ptrdiff_t i = ctx - _lapser_context_array;

    if(i >= LAPSER_MAX_CONCURRENT) return GASPI_ERROR;

    is_used[i] = false;
    SUCCESS_OR_DIE( gaspi_queue_delete(ctx->control_queue) );
    SUCCESS_OR_DIE( gaspi_queue_delete(ctx->data_queue) );

    // To prevent use-after-free
    memset(ctx, '\0', sizeof(lapser_ctx));

    return GASPI_SUCCESS;
}


gaspi_return_t lapser_init_config_ctx(lapser_ctx ** res) {

    // Search for appropriate parameters

    // Find max total of queues and segments
    gaspi_number_t num_segs, num_queues;
    gaspi_number_t max_segs, max_queues;

    SUCCESS_OR_DIE( gaspi_segment_num(&num_segs) );
    SUCCESS_OR_DIE( gaspi_queue_num(&num_queues) );

    SUCCESS_OR_DIE( gaspi_segment_max(&max_segs) );
    SUCCESS_OR_DIE( gaspi_queue_max(&max_queues) );

    if(max_segs - num_segs < 2) {
        lapser_log_fprintf("Could not get enough segments: max %d, allocated %d\n",
                           max_segs, num_segs);
        return GASPI_ERROR;
    }

    if(max_queues - num_queues < 2) {
        lapser_log_fprintf("Could not get enough queues: max %d, allocated %d\n",
                           max_queues, num_queues);
        return GASPI_ERROR;
    }

    // Now, get some nice queues and segments, and in descending order
    gaspi_queue_id_t control_q, data_q;

    SUCCESS_OR_DIE( gaspi_queue_create(&control_q, GASPI_BLOCK) );
    SUCCESS_OR_DIE( gaspi_queue_create(&data_q, GASPI_BLOCK) );


    gaspi_segment_id_t control_s, data_s;
    gaspi_segment_id_t used_s[num_segs];
    SUCCESS_OR_DIE( gaspi_segment_list(num_segs, used_s) );

    control_s = max_segs;
    ssize_t j;
    do {
        control_s--;
        j = num_segs;
        while(j-- && used_s[j] != control_s) { continue; }
    } while(j != -1 && control_s != max_segs); // If not 0, then I got

    if(control_s == max_segs) {
        lapser_log_fprintf("Could not find an unused id for the control segment\n");
        return GASPI_ERROR;
    }

    data_s = control_s;
    do {
        data_s--;
        j = num_segs;
        while(j-- && used_s[j] != data_s) { continue; }
    } while(j != -1); // If not 0, then I got

    if(data_s == max_segs) {
        lapser_log_fprintf("Could not find an unused id for the data segment\n");
        return GASPI_ERROR;
    }

    // Fill structure and return
    SUCCESS_OR_DIE( lapser_new_ctx(res) );

    (*res)->data_segment = data_s;
    (*res)->data_queue = data_q;
    (*res)->control_rank = 0; // TODO randomize
    (*res)->control_segment = control_s;
    (*res)->control_queue = control_q;
    (*res)->gpi_group = GASPI_GROUP_ALL;

    return GASPI_SUCCESS;
}

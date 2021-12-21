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


static inline gaspi_return_t find_next_segment_id(gaspi_segment_id_t *r1,
                                                  gaspi_segment_id_t *r2) {

    bool assigning_second=false;
    gaspi_segment_id_t res;

    gaspi_number_t num_segs, max_segs;
    SUCCESS_OR_DIE( gaspi_segment_num(&num_segs) );
    SUCCESS_OR_DIE( gaspi_segment_max(&max_segs) );

    if(max_segs - num_segs < 2) {
        lapser_log_fprintf("Could not get enough segments: max %d, allocated %d\n",
                           max_segs, num_segs);
        return GASPI_ERROR;
    }

    gaspi_segment_id_t used_s[num_segs+1];
    SUCCESS_OR_DIE( gaspi_segment_list(num_segs, used_s) );


    res = max_segs;
    ssize_t j;


repeat:
    // First check active, allocated segments
    do {
        res--;
        j = num_segs;
        while(j-- && used_s[j] != res) { continue; }
    } while(j != -1 && res != max_segs);

    // Ok, now res has got a possible segment id
    // Then, need to check other configured contexts
    for(size_t i=0; i<LAPSER_MAX_CONCURRENT; ++i) {
        if(!is_used[i]) continue;

        if(res == _lapser_context_array[i].control_segment ||
           res == _lapser_context_array[i].data_segment) {
            goto repeat;
        }
    }

    if(res == max_segs) {
        lapser_log_fprintf("Could not find an unused id for segment\n");
        return GASPI_ERROR;
    }

    if(!assigning_second) {
        *r1 = res;
        assigning_second = true;
        goto repeat; //Next one is to r2
    }

    *r2 = res;
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


// Need to get 2 queues, and 2 segment ids
gaspi_return_t lapser_init_config_ctx(lapser_ctx ** res) {


    gaspi_number_t num_queues, max_queues;
    SUCCESS_OR_DIE( gaspi_queue_num(&num_queues) );
    SUCCESS_OR_DIE( gaspi_queue_max(&max_queues) );

    if(max_queues - num_queues < 2) {
        lapser_log_fprintf("Could not get enough queues: max %d, allocated %d\n",
                           max_queues, num_queues);
        return GASPI_ERROR;
    }

    // Now, get some nice queues
    gaspi_queue_id_t control_q, data_q;

    SUCCESS_OR_DIE( gaspi_queue_create(&control_q, GASPI_BLOCK) );
    SUCCESS_OR_DIE( gaspi_queue_create(&data_q, GASPI_BLOCK) );

    gaspi_segment_id_t control_s, data_s;
    SUCCESS_OR_DIE( find_next_segment_id(&control_s, &data_s) );

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

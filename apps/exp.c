#include <string.h>
#include <GASPI.h>
#include <lapser.h>
#include "success_or_die.h"
#include "assert.h"

// This is just a skeleton app to try out simple things
// Useful to try and run failing tests

#define NUM_ITER 10000
#define SLACK 5

int main() {

    gaspi_rank_t rank, num;

    SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );
    gaspi_proc_rank(&rank);
    gaspi_proc_num(&num);

    Key a[1] = { rank };
    Key b[num];
    for(size_t i=0; i<num; ++i) { b[i] = i; }

    int input[num];
    int output[num][num];

    memset(input, '\0', sizeof input);
    memset(output, '\0', sizeof output);
    ASSERT(sizeof input == sizeof output[0]);


    lapser_init(num, sizeof input, a, 1, b, num, SLACK);


    for(int i=1; i < NUM_ITER+1; ++i) {
        input[rank] = i;
        lapser_set(rank, input, sizeof input, i);

        for(int j=0; j < num; ++j) {
            int res = lapser_get(j, output[j], sizeof output[j], i, SLACK);
            if(res != 0) printf("%d: lapser_get failed with %d in iteration %d for item %d\n", rank, res, i, j);
            input[j] = output[j][j];
            if(output[j][j] < i - SLACK) {
                printf("%d: Error, too old from %d: %d %d\n%1$d: Iteration: %d, Slack: %d, Full values:\n", rank, j, i-SLACK, output[j][j], i, SLACK);
                for(int k=0; k<num; ++k) {
                    printf("%d ", output[j][k]);
                    if(k!=rank) SUCCESS_OR_DIE(gaspi_proc_kill(k, GASPI_BLOCK));
                }
                putchar('\n');
                return -1;
            }
        }
    }


    lapser_finish();

    SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );
    return 0;
}




const char *__asan_default_options() { return "detect_leaks=false:"; }

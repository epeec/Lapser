#include <gtest/gtest.h>

extern "C" {
#include <GASPI.h>
#include "aux/success_or_die.h"

// Sanitizer failures are test failures
void __ubsan_on_report() { FAIL() << "Encountered an undefined behavior sanitizer error"; }
void __asan_on_error()   { FAIL() << "Encountered an address sanitizer error"; }
void __tsan_on_report()  { FAIL() << "Encountered a thread sanitizer error"; }

const char *__asan_default_options() { return "detect_leaks=false:"; }
}

// Terse printer: show small summary from each process
class SummaryPrinter : public testing::EmptyTestEventListener {

    virtual void OnTestProgramEnd(const testing::UnitTest& unit_test) override {
        gaspi_rank_t rank;
        gaspi_proc_rank(&rank);
        printf("Rank %2ld: Success: %d / Failed %d / Total %d\n", rank,
                                                                  unit_test.successful_test_count(),
                                                                  unit_test.failed_test_count(),
                                                                  unit_test.total_test_count());
        if(unit_test.failed_test_count() != 0) {
            for(int i=unit_test.total_test_case_count() - 1; i >= 0; --i) {
                auto suite = unit_test.GetTestCase(i);
                for(int j=suite->total_test_count() - 1; j >= 0; --j) {
                    auto info = suite->GetTestInfo(j);
                    if(info->result()->Failed() == true) {
                        printf("Rank %2ld: Failed %s.%s\n", rank, info->test_case_name(), info->name());
                    }
                }

            }
        }

    }
};

// global
gaspi_rank_t rank, num;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    SUCCESS_OR_DIE( gaspi_proc_init(GASPI_BLOCK) );
    SUCCESS_OR_DIE( gaspi_proc_rank(&rank) );
    SUCCESS_OR_DIE( gaspi_proc_num(&num) );

    ::testing::TestEventListeners& listeners = ::testing::UnitTest::GetInstance()->listeners();

#if defined(RANK_TO_REPORT) //configured in CMake, default 0
    if (rank != RANK_TO_REPORT) {
      delete listeners.Release(listeners.default_result_printer());
      listeners.Append(new SummaryPrinter);
    }
#else
    delete listeners.Release(listeners.default_result_printer());
    listeners.Append(new SummaryPrinter);
#endif

    auto res = RUN_ALL_TESTS();

    SUCCESS_OR_DIE( gaspi_proc_term(GASPI_BLOCK) );
    return res;
}

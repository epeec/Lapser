
#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
}

extern gaspi_rank_t rank, num;
static const int NUM_ITER = 10;


class SlackLapserTest : public ::testing::TestWithParam<Clock> {
  protected:
    void SetUp() override {
        Key a[] = { rank };
        Key b[num];
        int i = 0, j=0;
        for(int i=0; i<num; ++i) { if(i != rank) { b[j++] = i;}  }
        ASSERT_EQ(num-1, j);
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), a, 1, b, num-1, GetParam()));
    }

    void TearDown() override {
        ASSERT_EQ(0, lapser_finish());
    }
};

INSTANTIATE_TEST_CASE_P(SlackLowPow2, SlackLapserTest, testing::Values(1,2,4,8));

TEST_P(SlackLapserTest, ReadMyWrites) {
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, lapser_get(rank, &res, sizeof res, 1, GetParam()));
    EXPECT_EQ(buf, res);
}


TEST_P(SlackLapserTest, ReadWithSlack) {
    Byte buf = 'A';
    Byte res;

    for(int i=1; i<=NUM_ITER; ++i) {
        buf = buf + 1;
        ASSERT_EQ(0, lapser_set(rank, &buf, 1, i));
    }

    for(int i=0; i<num; ++i) {
        ASSERT_EQ(0, lapser_get(i, &res, sizeof res, NUM_ITER, GetParam()));
        EXPECT_TRUE((buf-res) <= GetParam()) << "Read is not within specified slack";
    }
}

TEST_P(SlackLapserTest, StrictReads) {
    Byte buf = 'A';
    Byte res;

    for(int i=1; i<=NUM_ITER; ++i) {
        buf = buf + 1;
        ASSERT_EQ(0, lapser_set(rank, &buf, 1, i));
    }

    for(int i=0; i<num; ++i) {
        ASSERT_EQ(0, lapser_get(i, &res, sizeof res, NUM_ITER, 0));
        EXPECT_EQ('A'+NUM_ITER, res);
    }
}

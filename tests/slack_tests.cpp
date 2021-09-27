#include "gtest/gtest.h"

extern "C" {
#include "gssp_internal.h"
}

extern gaspi_rank_t rank, num;
static const int NUM_ITER = 10;


class SlackGSSPTest : public ::testing::TestWithParam<Clock> {
  protected:
    void SetUp() override {
        Key a[] = { rank };
        Key b[num-1];
        int i = 0, j=0;
        for(int i=0; i<num; ++i) { if(i != rank) { b[j++] = i;}  }
        ASSERT_EQ(num-1, j);
        ASSERT_EQ(0, gssp_init(num, sizeof(Byte), a, 1, b, num-1, GetParam()));
    }

    void TearDown() override {
        ASSERT_EQ(0, gssp_finish());
    }
};

INSTANTIATE_TEST_CASE_P(SlackLowPow2, SlackGSSPTest, testing::Values(1,2,4,8));

TEST_P(SlackGSSPTest, ReadMyWrites) {
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, gssp_get(rank, &res, sizeof res, 1, GetParam()));
    EXPECT_EQ(buf, res);
}


TEST_P(SlackGSSPTest, ReadWithSlack) {
    Byte buf = 'A';
    Byte res;

    for(int i=1; i<=NUM_ITER; ++i) {
        buf = buf + 1;
        ASSERT_EQ(0, gssp_set(rank, &buf, 1, i));
    }

    for(int i=0; i<num; ++i) {
        ASSERT_EQ(0, gssp_get(i, &res, sizeof res, NUM_ITER, GetParam()));
        EXPECT_TRUE((buf-res) <= GetParam()) << "Read is not within specified slack";
    }
}

TEST_P(SlackGSSPTest, StrictReads) {
    Byte buf = 'A';
    Byte res;

    for(int i=1; i<=NUM_ITER; ++i) {
        buf = buf + 1;
        ASSERT_EQ(0, gssp_set(rank, &buf, 1, i));
    }

    for(int i=0; i<num; ++i) {
        ASSERT_EQ(0, gssp_get(i, &res, sizeof res, NUM_ITER, 0));
        EXPECT_EQ('A'+NUM_ITER, res);
    }
}


#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
}

extern gaspi_rank_t rank, num;

// Full because we are going to do all-to-all communication
class FullLapserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Key a = rank;
        Key b[num];
        for(int i=0; i<num; ++i) { b[i] = i; }
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, b, num, 0));
    }

    void TearDown() override {
        ASSERT_EQ(0, lapser_finish());
    }
};


TEST_F(FullLapserTest, SetLocalItem) {
    Byte buf = 'A' + rank;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
}

TEST_F(FullLapserTest, SetNonLocalItem) {
    Byte buf = 'A' + rank;
    ASSERT_NE(0, lapser_set((rank+1)%num, &buf, sizeof buf, 1));
}

TEST_F(FullLapserTest, GetItemWithoutSet) {
    Byte res;
    ASSERT_NE(0, lapser_get(rank, &res, sizeof res, 1, 0));
}

TEST_F(FullLapserTest, SetGetLocalItem) {
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, lapser_get(rank, &res, sizeof res, 1, 0));
    EXPECT_EQ(buf, res);
}

TEST_F(FullLapserTest, SetGetRemoteItems) {
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));

    for(int rank=0; rank<num; ++rank) {
        ASSERT_EQ(0, lapser_get(rank, &res, sizeof res, 1, 0));
        EXPECT_EQ(res, 'A' + rank);
    }
}

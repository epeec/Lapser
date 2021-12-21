
#include "gtest/gtest.h"

extern "C" {
#include "lapser_internal.h"
#include "aux/left_right.h"
}

extern gaspi_rank_t rank, num;

// Simple because it only tries to read from 1 neighbour rank
class MultiLapserTest : public ::testing::Test {
  protected:
    lapser_ctx *c1=NULL, *c2=NULL;
    void SetUp() override {
        Key a = rank;
        Key b1 = RIGHT(rank, num);
        Key b2 = LEFT(rank, num);

        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, &b1, 1, 0, &c1));
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, &b2, 1, 0, &c2));
    }

    void TearDown() override {
        ASSERT_EQ(0, lapser_finish(c2));
        ASSERT_EQ(0, lapser_finish(c1));
    }
};


TEST_F(MultiLapserTest, SetLocalItems) {
    Byte buf1 = 'A' + rank;
    Byte buf2 = 'Z' - rank;
    ASSERT_EQ(0, lapser_set(rank, &buf1, sizeof buf1, 1, c1));
    ASSERT_EQ(0, lapser_set(rank, &buf2, sizeof buf2, 1, c2));
}

TEST_F(MultiLapserTest, SetNonLocalItem) {
    Byte buf1 = 'A' + rank;
    Byte buf2 = 'Z' - rank;
    ASSERT_NE(0, lapser_set(RIGHT(rank,num), &buf1, sizeof buf1, 1, c1));
    ASSERT_NE(0, lapser_set(LEFT(rank,num), &buf2, sizeof buf2, 1, c2));
}

TEST_F(MultiLapserTest, GetItemWithoutSet) {
    Byte res;
    ASSERT_NE(0, lapser_get(rank, &res, sizeof res, 1, 0, c1));
    ASSERT_NE(0, lapser_get(rank, &res, sizeof res, 1, 0, c2));
}

TEST_F(MultiLapserTest, SetGetLocalItem) {
    Byte buf1 = 'A' + rank;
    Byte buf2 = 'Z' - rank;
    Byte res1, res2;

    ASSERT_EQ(0, lapser_set(rank, &buf1, sizeof buf1, 1, c1));
    ASSERT_EQ(0, lapser_set(rank, &buf2, sizeof buf2, 1, c2));

    ASSERT_EQ(0, lapser_get(rank, &res1, sizeof res1, 1, 0, c1));
    ASSERT_EQ(0, lapser_get(rank, &res2, sizeof res2, 1, 0, c2));

    EXPECT_EQ(buf1, res1);
    EXPECT_EQ(buf2, res2);
}

TEST_F(MultiLapserTest, SetGetRemoteItem) {
    gaspi_rank_t right = RIGHT(rank, num);
    gaspi_rank_t left = LEFT(rank, num);

    Byte buf1 = 'A' + rank;
    Byte buf2 = 'Z' - rank;
    Byte res1, res2;


    ASSERT_EQ(0, lapser_set(rank, &buf1, sizeof buf1, 1, c1));
    ASSERT_EQ(0, lapser_set(rank, &buf2, sizeof buf2, 1, c2));

    ASSERT_EQ(0, lapser_get(right, &res1, sizeof res1, 1, 0, c1));
    ASSERT_EQ(0, lapser_get(left, &res2, sizeof res2, 1, 0, c2));

    EXPECT_EQ(res1, 'A' + right);
    EXPECT_EQ(res2, 'Z' - left);
}


#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
#include "aux/left_right.h"
}

extern gaspi_rank_t rank, num;

// Each rank produces 2 items which are read from the left and right
class LeftRightLapserTest : public ::testing::Test {
  protected:
    Key produce[2];
    Key consume[2];
    Byte buf[2];

    void SetUp() override {
        produce[0] = 2*rank;
        produce[1] = 2*rank+1;
        consume[0] = 2*LEFT(rank, num) + 1;
        consume[1] = 2*RIGHT(rank, num);
        buf[0] = 'A' + rank;
        buf[1] = 'Z' - rank;

        ASSERT_EQ(0, lapser_init(2*num, sizeof(Byte), produce, 2, consume, 2, 0));
    }

    void TearDown() override {
        ASSERT_EQ(0, lapser_finish());
    }
};


TEST_F(LeftRightLapserTest, SetLocalItem) {
    EXPECT_EQ(0, lapser_set(produce[0], buf, sizeof *buf, 1));
    EXPECT_EQ(0, lapser_set(produce[1], buf+1, sizeof *buf, 1));
}

TEST_F(LeftRightLapserTest, SetNonLocalItem) {
    EXPECT_NE(0, lapser_set(consume[0], buf, sizeof *buf, 1));
}

TEST_F(LeftRightLapserTest, GetItemWithoutSet) {
    Byte res;
    ASSERT_NE(0, lapser_get(produce[1], &res, sizeof res, 1, 0));
}

TEST_F(LeftRightLapserTest, SetGetLocalItem) {
    Byte res;
    ASSERT_EQ(0, lapser_set(produce[1], buf, sizeof *buf, 1));
    ASSERT_EQ(0, lapser_get(produce[1], &res, sizeof res, 1, 0));
    EXPECT_EQ(buf[0], res);
}

TEST_F(LeftRightLapserTest, SetGetRemoteItems) {
    Byte res[2];
    ASSERT_EQ(0, lapser_set(produce[0], buf, sizeof *buf, 1));
    ASSERT_EQ(0, lapser_set(produce[1], buf+1, sizeof *buf, 1));

    ASSERT_EQ(0, lapser_get(consume[0], res,   sizeof *res, 1, 0));
    ASSERT_EQ(0, lapser_get(consume[1], res+1, sizeof *res, 1, 0));

    EXPECT_EQ(res[0], 'Z' - LEFT(rank, num) );
    EXPECT_EQ(res[1], 'A' + RIGHT(rank, num) );
}

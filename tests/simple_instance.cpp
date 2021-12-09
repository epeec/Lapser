
#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
}

extern gaspi_rank_t rank, num;

// Simple because it only tries to read from 1 neighbour rank
class SimpleLapserTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Key a = rank;
        Key b = (rank + 1) % num;
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, &b, 1, 0));
    }

    void TearDown() override {
        ASSERT_EQ(0, lapser_finish());
    }
};


TEST_F(SimpleLapserTest, SetLocalItem) {
    Byte buf = 'A' + rank;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
}

TEST_F(SimpleLapserTest, SetNonLocalItem) {
    Byte buf = 'A' + rank;
    ASSERT_NE(0, lapser_set((rank+1)%num, &buf, sizeof buf, 1));
}

TEST_F(SimpleLapserTest, GetItemWithoutSet) {
    Byte res;
    ASSERT_NE(0, lapser_get(rank, &res, sizeof res, 1, 0));
}

TEST_F(SimpleLapserTest, SetGetLocalItem) {
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, lapser_get(rank, &res, sizeof res, 1, 0));
    EXPECT_EQ(buf, res);
}

TEST_F(SimpleLapserTest, SetGetRemoteItem) {
    gaspi_rank_t right = (rank + 1) % num;
    Byte buf = 'A' + rank;
    Byte res;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));

    ASSERT_EQ(0, lapser_get(right, &res, sizeof res, 1, 0));
    EXPECT_EQ(res, 'A' + right);
}

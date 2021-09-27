
#include "gtest/gtest.h"

extern "C" {
#include "gssp_internal.h"
}

extern gaspi_rank_t rank, num;

// Simple because it only tries to read from 1 neighbour rank
class SimpleGSSPTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Key a = rank;
        Key b = (rank + 1) % num;
        ASSERT_EQ(0, gssp_init(num, sizeof(char), &a, 1, &b, 1, 0));
    }

    void TearDown() override {
        ASSERT_EQ(0, gssp_finish());
    }
};


TEST_F(SimpleGSSPTest, SetLocalItem) {
    char buf = 'A' + rank;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));
}

TEST_F(SimpleGSSPTest, SetNonLocalItem) {
    char buf = 'A' + rank;
    ASSERT_NE(0, gssp_set((rank+1)%num, &buf, sizeof buf, 1));
}

TEST_F(SimpleGSSPTest, GetItemWithoutSet) {
    char res;
    ASSERT_NE(0, gssp_get(rank, &res, sizeof res, 1, 0));
}

TEST_F(SimpleGSSPTest, SetGetLocalItem) {
    char buf = 'A' + rank;
    char res;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, gssp_get(rank, &res, sizeof res, 1, 0));
    EXPECT_EQ(buf, res);
}

TEST_F(SimpleGSSPTest, SetGetRemoteItem) {
    gaspi_rank_t right = (rank + 1) % num;
    char buf = 'A' + rank;
    char res;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));

    ASSERT_EQ(0, gssp_get(right, &res, sizeof res, 1, 0));
    EXPECT_EQ(res, 'A' + right);
}

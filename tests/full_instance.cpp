
#include "gtest/gtest.h"

extern "C" {
#include "gssp_internal.h"
}

extern gaspi_rank_t rank, num;

// Full because we are going to do all-to-all communication
class FullGSSPTest : public ::testing::Test {
  protected:
    void SetUp() override {
        Key a = rank;
        Key b[num];
        for(int i=0; i<num; ++i) { b[i] = i; }
        ASSERT_EQ(0, gssp_init(num, sizeof(char), &a, 1, b, num, 0));
    }

    void TearDown() override {
        ASSERT_EQ(0, gssp_finish());
    }
};


TEST_F(FullGSSPTest, SetLocalItem) {
    char buf = 'A' + rank;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));
}

TEST_F(FullGSSPTest, SetNonLocalItem) {
    char buf = 'A' + rank;
    ASSERT_NE(0, gssp_set((rank+1)%num, &buf, sizeof buf, 1));
}

TEST_F(FullGSSPTest, GetItemWithoutSet) {
    char res;
    ASSERT_NE(0, gssp_get(rank, &res, sizeof res, 1, 0));
}

TEST_F(FullGSSPTest, SetGetLocalItem) {
    char buf = 'A' + rank;
    char res;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));
    ASSERT_EQ(0, gssp_get(rank, &res, sizeof res, 1, 0));
    EXPECT_EQ(buf, res);
}

TEST_F(FullGSSPTest, SetGetRemoteItems) {
    char buf = 'A' + rank;
    char res;
    ASSERT_EQ(0, gssp_set(rank, &buf, sizeof buf, 1));

    for(int rank=0; rank<num; ++rank) {
        ASSERT_EQ(0, gssp_get(rank, &res, sizeof res, 1, 0));
        EXPECT_EQ(res, 'A' + rank);
    }
}

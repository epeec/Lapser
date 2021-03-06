
#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
}

extern gaspi_rank_t rank, num;

// This is a really dumb set of tests
TEST(HashTest, DumbChecks) {

    Byte a[] = "ABCEDFGHIJKLMNOPQRSTUVWXYZ";
    Byte b[] = {0x41, 0x42};
    Byte c[100] = {'A'} ;

    Hash ha = lapser_item_hash(a, sizeof *a, 0);
    Hash hc = lapser_item_hash(c, 1, 0);

    EXPECT_EQ(lapser_item_hash(a, 2, 0), lapser_item_hash(b, 2, 0));
    EXPECT_EQ(ha, lapser_item_hash(a, sizeof *a, 0));
    EXPECT_NE(ha, lapser_item_hash(a+1, sizeof *a - 1, 0));
    for(int i=0; i<sizeof *a; ++i) {
        EXPECT_NE(ha, lapser_item_hash(a, i, 0)) << "just a random collision";
    }
    for(int i=2; i<sizeof *c; ++i) {
        EXPECT_NE(hc, lapser_item_hash(c, i, 0)) << "collision with empty padding" ;
    }

}


// The simple example again, but now we are going to check if the checksum works
class ChecksumTest : public ::testing::Test {
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

TEST_F(ChecksumTest, SetLocalItem) {
    item * to_test = lapser_get_item_structure(rank, _lapser_global_ctx);
    Byte buf = 'A' + rank;
    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
    EXPECT_EQ(lapser_item_hash((Byte*) &buf, sizeof buf, to_test->version), to_test->checksum);
}

TEST_F(ChecksumTest, CorrectInitialization) {
    item * to_test = lapser_get_item_structure(rank, _lapser_global_ctx);
    EXPECT_EQ(lapser_item_hash(to_test->value, sizeof(Byte), to_test->version), to_test->checksum);
}

TEST_F(ChecksumTest, GetLocalItemWithoutSetting) {
    Byte buf;
    item * to_test = lapser_get_item_structure(rank, _lapser_global_ctx);
    ASSERT_EQ(0, lapser_get(rank, &buf, sizeof buf, 1, 1));
    EXPECT_EQ(lapser_item_hash((Byte*) &buf, sizeof buf, to_test->version), to_test->checksum);
}

// TODO more granular error codes
TEST_F(ChecksumTest, GetLocalWithCorruptedHash) {
    item * to_test = lapser_get_item_structure(rank, _lapser_global_ctx);
    Byte buf = 'A' + rank;
    Byte res;

    ASSERT_EQ(0, lapser_set(rank, &buf, sizeof buf, 1));
    EXPECT_EQ(lapser_item_hash((Byte*) &buf, sizeof buf, to_test->version), to_test->checksum);

    to_test->checksum -= 1;

    EXPECT_NE(0, lapser_get(rank, &res, sizeof res, 1, 0));
}

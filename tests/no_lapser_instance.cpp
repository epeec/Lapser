
#include "gtest/gtest.h"

extern "C" {
#include "lapser.h"
#include "lapser_internal.h"
}

extern gaspi_rank_t rank, num;

// TODO defer data segment creation to after we get that information
TEST(Setup, DISABLED_NoProduceNoConsume) {
    ASSERT_EQ(0, lapser_init(num, sizeof(Byte), NULL, 0, NULL, 0, 0));
}


TEST(Setup, AllProduceNoConsume) {
    Key a = rank;
    ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, NULL, 0, 0));
    EXPECT_EQ(0, lapser_finish());
}

TEST(Setup, AllProduceAllConsume) {
    Key a = rank;
    Key b = (rank + 1) % num;
    ASSERT_EQ(0, lapser_init(num, sizeof(Byte), &a, 1, &b, 1, 0));
    EXPECT_EQ(0, lapser_finish());
}


TEST(Setup, AllProduceAllConsumeFromAll) {
    Key a[] = { rank };
    Key b[num];
    int i = 0, j=0;
    for(int i=0; i<num; ++i) { if(i != rank) { b[j++] = i;}  }
    ASSERT_EQ(num-1, j);

    ASSERT_EQ(0, lapser_init(num, sizeof(Byte), a, 1, b, num-1, 0));
    EXPECT_EQ(0, lapser_finish());
}

// Even ranks produce, odd ranks consume from rank-1
TEST(Setup, SomeProduceSomeConsume) {
    Key a[] = { rank };
    Key b[] = { ((Key) rank) & ~0x0001 };

    if(rank % 2) { //Odd
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), NULL, 0, b, 1, 0));
    } else { //Even
        ASSERT_EQ(0, lapser_init(num, sizeof(Byte), a, 1, NULL, 0, 0));
    }

    EXPECT_EQ(0, lapser_finish());
}

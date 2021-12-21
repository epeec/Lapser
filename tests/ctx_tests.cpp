
#include "gtest/gtest.h"

extern "C" {
#include "lapser_internal.h"
#include <GASPI.h>
}

TEST(Ctx, CreateDelete) {
    lapser_ctx *h;
    EXPECT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h));
    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h));
}

TEST(Ctx, DeleteFreesQueues) {
    gaspi_number_t before, during, after;

    ASSERT_EQ(GASPI_SUCCESS, gaspi_queue_num(&before));

    lapser_ctx *h;
    EXPECT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h));

    ASSERT_EQ(GASPI_SUCCESS, gaspi_queue_num(&during));

    EXPECT_EQ(GASPI_SUCCESS, lapser_free_ctx(h));

    ASSERT_EQ(GASPI_SUCCESS, gaspi_queue_num(&after));

    EXPECT_EQ(before, after);
    EXPECT_NE(before, during); EXPECT_NE(during, after);
}

TEST(Ctx, DifferentQueuesAndSegments) {
    lapser_ctx *h;
    EXPECT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h));

    EXPECT_NE(h->control_segment, h->data_segment);
    EXPECT_NE(h->control_queue, h->data_queue);

    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h));
}

TEST(Ctx, DifferentCtxs) {

    lapser_ctx *h1, *h2;
    ASSERT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h1));
    ASSERT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h2));

    EXPECT_NE(h1->control_segment, h1->data_segment);
    EXPECT_NE(h1->control_queue, h1->data_queue);

    EXPECT_NE(h2->control_segment, h2->data_segment);
    EXPECT_NE(h2->control_queue, h2->data_queue);

    EXPECT_NE(h1->control_segment, h2->control_segment);
    EXPECT_NE(h1->control_queue, h2->control_queue);
    EXPECT_NE(h1->data_segment, h2->data_segment);
    EXPECT_NE(h1->data_queue, h2->data_queue);

    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h2));
    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h1));
}


TEST(Ctx, StartFromHigherToLowerSegments) {

    lapser_ctx *h1, *h2;
    ASSERT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h1));
    ASSERT_EQ(GASPI_SUCCESS, lapser_init_config_ctx(&h2));

    EXPECT_GT(h1->control_segment, h2->control_segment);
    EXPECT_GT(h1->data_segment, h2->data_segment);

    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h2));
    ASSERT_EQ(GASPI_SUCCESS, lapser_free_ctx(h1));
}

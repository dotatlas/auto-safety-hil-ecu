/* Unity tests for e2e.c. The CRC values come from the crccheck package, not from e2e.c itself. */
#include "e2e.h"
#include "unity.h"

static const uint8_t DATA[8] = {0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0x00};
static e2e_counter_t counter;
static uint8_t lost;

/* setUp gives each test a counter that has not seen a frame. */
void setUp(void)
{
    e2e_counter_reset(&counter);
    lost = 0xFFU;
}

/* tearDown is required by Unity. The tests have nothing to release. */
void tearDown(void) {}

/* test_crc_check_value checks the standard check value of CRC-8 SAE J1850. */
static void test_crc_check_value(void)
{
    const uint8_t text[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    TEST_ASSERT_EQUAL_HEX8(0x4BU, e2e_crc8(text, sizeof(text)));
    TEST_ASSERT_EQUAL_HEX8(0x00U, e2e_crc8(text, 0U));
}

/* test_frame_crc_includes_the_id checks that the same data gives a different CRC per ID. */
static void test_frame_crc_includes_the_id(void)
{
    const uint8_t zeros[8] = {0};
    TEST_ASSERT_EQUAL_HEX8(0xA7U, e2e_frame_crc(0x100U, DATA));
    TEST_ASSERT_EQUAL_HEX8(0xC2U, e2e_frame_crc(0x101U, DATA));
    TEST_ASSERT_EQUAL_HEX8(0xFBU, e2e_frame_crc(0x7FFU, DATA));
    TEST_ASSERT_EQUAL_HEX8(0xC2U, e2e_frame_crc(0x200U, zeros));
}

/* test_frame_crc_ignores_byte_7 checks that the CRC byte itself is not in the CRC. */
static void test_frame_crc_ignores_byte_7(void)
{
    uint8_t data[8] = {0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0x99};
    TEST_ASSERT_EQUAL_HEX8(0xA7U, e2e_frame_crc(0x100U, data));
}

/* test_every_single_bit_flip_changes_the_crc checks all 56 data bits one by one. */
static void test_every_single_bit_flip_changes_the_crc(void)
{
    const uint8_t good = e2e_frame_crc(0x100U, DATA);
    for (uint8_t bit = 0U; bit < 56U; bit++) {
        uint8_t data[8] = {0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0x00};
        data[bit / 8U] = (uint8_t)(data[bit / 8U] ^ (1U << (bit % 8U)));
        TEST_ASSERT_NOT_EQUAL(good, e2e_frame_crc(0x100U, data));
    }
}

/* test_first_frame_is_accepted checks that any value syncs the counter. */
static void test_first_frame_is_accepted(void)
{
    TEST_ASSERT_EQUAL(E2E_COUNTER_FIRST, e2e_counter_check(&counter, 9U, &lost));
    TEST_ASSERT_EQUAL_UINT8(0U, lost);
}

/* test_next_value_is_ok checks the normal case, also across the wrap from 15 to 0. */
static void test_next_value_is_ok(void)
{
    (void)e2e_counter_check(&counter, 14U, &lost);
    TEST_ASSERT_EQUAL(E2E_COUNTER_OK, e2e_counter_check(&counter, 15U, &lost));
    TEST_ASSERT_EQUAL(E2E_COUNTER_OK, e2e_counter_check(&counter, 0U, &lost));
    TEST_ASSERT_EQUAL_UINT8(0U, lost);
}

/* test_same_value_is_a_repeat checks a frozen sender. */
static void test_same_value_is_a_repeat(void)
{
    (void)e2e_counter_check(&counter, 3U, &lost);
    TEST_ASSERT_EQUAL(E2E_COUNTER_REPEAT, e2e_counter_check(&counter, 3U, &lost));
    TEST_ASSERT_EQUAL(E2E_COUNTER_REPEAT, e2e_counter_check(&counter, 3U, &lost));
    TEST_ASSERT_EQUAL(E2E_COUNTER_OK, e2e_counter_check(&counter, 4U, &lost));
}

/* test_jump_counts_the_lost_frames checks a gap, also across the wrap. */
static void test_jump_counts_the_lost_frames(void)
{
    (void)e2e_counter_check(&counter, 3U, &lost);
    TEST_ASSERT_EQUAL(E2E_COUNTER_JUMP, e2e_counter_check(&counter, 6U, &lost));
    TEST_ASSERT_EQUAL_UINT8(2U, lost);
    TEST_ASSERT_EQUAL(E2E_COUNTER_OK, e2e_counter_check(&counter, 7U, &lost));
    (void)e2e_counter_check(&counter, 14U, &lost);
    TEST_ASSERT_EQUAL_UINT8(6U, lost);
    TEST_ASSERT_EQUAL(E2E_COUNTER_JUMP, e2e_counter_check(&counter, 1U, &lost));
    TEST_ASSERT_EQUAL_UINT8(2U, lost);
}

/* test_reset_makes_the_next_frame_first checks the resync after a timeout. */
static void test_reset_makes_the_next_frame_first(void)
{
    (void)e2e_counter_check(&counter, 3U, &lost);
    e2e_counter_reset(&counter);
    TEST_ASSERT_EQUAL(E2E_COUNTER_FIRST, e2e_counter_check(&counter, 3U, &lost));
}

/* main runs the tests for CTest. */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc_check_value);
    RUN_TEST(test_frame_crc_includes_the_id);
    RUN_TEST(test_frame_crc_ignores_byte_7);
    RUN_TEST(test_every_single_bit_flip_changes_the_crc);
    RUN_TEST(test_first_frame_is_accepted);
    RUN_TEST(test_next_value_is_ok);
    RUN_TEST(test_same_value_is_a_repeat);
    RUN_TEST(test_jump_counts_the_lost_frames);
    RUN_TEST(test_reset_makes_the_next_frame_first);
    return UNITY_END();
}

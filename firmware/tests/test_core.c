/* Unity tests for the echo behavior in core.c. */
#include "core.h"
#include "unity.h"

static const uint8_t DATA[CORE_FRAME_DATA_MAX] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
static core_output_t output;

/* setUp gives each test a new core with the 0x100 echo offset. */
void setUp(void)
{
    const core_config_t config = {.echo_id_offset = 0x100U};
    core_init(&config);
}

/* tearDown is required by Unity. The tests have nothing to release. */
void tearDown(void) {}

/* test_echo_adds_the_offset_and_copies_the_data checks the normal path. */
static void test_echo_adds_the_offset_and_copies_the_data(void)
{
    core_on_frame(0U, 0x100U, DATA, 8U);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(1U, output.tx_count);
    TEST_ASSERT_EQUAL_HEX16(0x200U, output.tx[0].id);
    TEST_ASSERT_EQUAL_UINT8(8U, output.tx[0].dlc);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(DATA, output.tx[0].data, 8U);
}

/* test_step_empties_the_queue checks that a frame is sent one time only. */
static void test_step_empties_the_queue(void)
{
    core_on_frame(0U, 0x100U, DATA, 8U);
    core_step(0U, &output);
    core_step(10U, &output);
    TEST_ASSERT_EQUAL_UINT8(0U, output.tx_count);
}

/* test_dlc_above_8_is_dropped checks the DLC limit at the trust boundary. */
static void test_dlc_above_8_is_dropped(void)
{
    core_on_frame(0U, 0x100U, DATA, 9U);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(0U, output.tx_count);
}

/* test_dlc_of_8_and_0_are_accepted checks both ends of the valid DLC range. */
static void test_dlc_of_8_and_0_are_accepted(void)
{
    core_on_frame(0U, 0x100U, DATA, 0U);
    core_on_frame(0U, 0x100U, DATA, 8U);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(2U, output.tx_count);
}

/* test_id_above_11_bits_is_dropped checks the received ID limit. */
static void test_id_above_11_bits_is_dropped(void)
{
    core_on_frame(0U, 0x800U, DATA, 8U);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(0U, output.tx_count);
}

/* test_echo_id_above_11_bits_is_dropped checks the ID after the offset. */
static void test_echo_id_above_11_bits_is_dropped(void)
{
    core_on_frame(0U, 0x700U, DATA, 8U);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(0U, output.tx_count);
}

/* test_full_queue_drops_the_newest_frame checks that the queue never overruns. */
static void test_full_queue_drops_the_newest_frame(void)
{
    for (uint16_t i = 0U; i < (CORE_TX_MAX + 1U); i++) {
        core_on_frame(0U, (uint16_t)(0x100U + i), DATA, 8U);
    }
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(CORE_TX_MAX, output.tx_count);
    TEST_ASSERT_EQUAL_HEX16(0x203U, output.tx[CORE_TX_MAX - 1U].id);
}

/* test_init_clears_the_queue checks that a new start sends no old frames. */
static void test_init_clears_the_queue(void)
{
    const core_config_t config = {.echo_id_offset = 0x100U};
    core_on_frame(0U, 0x100U, DATA, 8U);
    core_init(&config);
    core_step(0U, &output);
    TEST_ASSERT_EQUAL_UINT8(0U, output.tx_count);
}

/* main runs the tests for CTest. */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_echo_adds_the_offset_and_copies_the_data);
    RUN_TEST(test_step_empties_the_queue);
    RUN_TEST(test_dlc_above_8_is_dropped);
    RUN_TEST(test_dlc_of_8_and_0_are_accepted);
    RUN_TEST(test_id_above_11_bits_is_dropped);
    RUN_TEST(test_echo_id_above_11_bits_is_dropped);
    RUN_TEST(test_full_queue_drops_the_newest_frame);
    RUN_TEST(test_init_clears_the_queue);
    return UNITY_END();
}

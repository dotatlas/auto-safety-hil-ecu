/* Unity tests for the generated codec. tests/test_codec.py checks the same bytes in Python. */
#include "unity.h"
#include "vehicle_hil.h"

static const uint8_t VEHICLE_STATE_BYTES[VEHICLE_HIL_VEHICLE_STATE_LENGTH] = {
    0xFEU, 0x06U, 0x9CU, 0xFFU, 0xF1U, 0x3FU, 0x05U, 0xA5U};

/* setUp is required by Unity. The tests have no state. */
void setUp(void) {}

/* tearDown is required by Unity. The tests have nothing to release. */
void tearDown(void) {}

/* test_pack_gives_the_known_bytes packs raw values with the generated function. */
static void test_pack_gives_the_known_bytes(void)
{
    const struct vehicle_hil_vehicle_state_t signals = {
        .vehicle_speed_mps = 1790U,
        .longitudinal_accel_mps2 = -100,
        .steering_angle_deg = -15,
        .gear_state = 3U,
        .alive_counter = 5U,
        .crc8 = 0xA5U,
    };
    uint8_t packed[VEHICLE_HIL_VEHICLE_STATE_LENGTH];

    int size = vehicle_hil_vehicle_state_pack(packed, &signals, sizeof(packed));

    TEST_ASSERT_EQUAL_INT(VEHICLE_HIL_VEHICLE_STATE_LENGTH, size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(VEHICLE_STATE_BYTES, packed, VEHICLE_HIL_VEHICLE_STATE_LENGTH);
}

/* test_unpack_gives_the_known_values reads the same bytes back, with the signs kept. */
static void test_unpack_gives_the_known_values(void)
{
    struct vehicle_hil_vehicle_state_t signals;

    int result =
        vehicle_hil_vehicle_state_unpack(&signals, VEHICLE_STATE_BYTES, sizeof(VEHICLE_STATE_BYTES));

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT16(1790U, signals.vehicle_speed_mps);
    TEST_ASSERT_EQUAL_INT16(-100, signals.longitudinal_accel_mps2);
    TEST_ASSERT_EQUAL_INT16(-15, signals.steering_angle_deg);
    TEST_ASSERT_EQUAL_UINT8(3U, signals.gear_state);
    TEST_ASSERT_EQUAL_UINT8(5U, signals.alive_counter);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, signals.crc8);
}

/* main runs the tests for CTest. */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_pack_gives_the_known_bytes);
    RUN_TEST(test_unpack_gives_the_known_values);
    return UNITY_END();
}

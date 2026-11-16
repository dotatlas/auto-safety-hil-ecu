/* Unity tests for the generated codec. tests/test_codec.py checks the same bytes in Python. */
#include "unity.h"
#include "vehicle_hil.h"

#define FRAME_LENGTH 8U

/* ASSERT_FRAME packs the values, compares the bytes, then unpacks and packs them again. */
#define ASSERT_FRAME(name, values, expected)                                                       \
    do {                                                                                           \
        uint8_t packed[FRAME_LENGTH];                                                              \
        struct vehicle_hil_##name##_t back;                                                        \
        TEST_ASSERT_EQUAL_INT(FRAME_LENGTH, vehicle_hil_##name##_pack(packed, &(values), 8U));     \
        TEST_ASSERT_EQUAL_HEX8_ARRAY((expected), packed, FRAME_LENGTH);                            \
        TEST_ASSERT_EQUAL_INT(0, vehicle_hil_##name##_unpack(&back, (expected), 8U));              \
        TEST_ASSERT_EQUAL_INT(FRAME_LENGTH, vehicle_hil_##name##_pack(packed, &back, 8U));         \
        TEST_ASSERT_EQUAL_HEX8_ARRAY((expected), packed, FRAME_LENGTH);                            \
    } while (0)

static const uint8_t VEHICLE_STATE_BYTES[FRAME_LENGTH] = {0xFE, 0x06, 0x9C, 0xFF,
                                                          0xF1, 0x3F, 0x05, 0xA5};

/* setUp is required by Unity. The tests have no state. */
void setUp(void) {}

/* tearDown is required by Unity. The tests have nothing to release. */
void tearDown(void) {}

/* test_unpack_keeps_the_signs reads one frame field by field, with negative values. */
static void test_unpack_keeps_the_signs(void)
{
    struct vehicle_hil_vehicle_state_t signals;

    TEST_ASSERT_EQUAL_INT(0, vehicle_hil_vehicle_state_unpack(&signals, VEHICLE_STATE_BYTES, 8U));
    TEST_ASSERT_EQUAL_UINT16(1790U, signals.vehicle_speed_mps);
    TEST_ASSERT_EQUAL_INT16(-100, signals.longitudinal_accel_mps2);
    TEST_ASSERT_EQUAL_INT16(-15, signals.steering_angle_rad);
    TEST_ASSERT_EQUAL_UINT8(3U, signals.gear_state);
    TEST_ASSERT_EQUAL_UINT8(5U, signals.alive_counter);
    TEST_ASSERT_EQUAL_HEX8(0xA5U, signals.crc8);
}

/* test_sensor_frames checks the six messages that the simulator sends. */
static void test_sensor_frames(void)
{
    const struct vehicle_hil_vehicle_state_t vehicle = {1790, -100, -15, 3, 5, 0xA5};
    const struct vehicle_hil_wheel_speed_t wheels = {895, 900, 4095, 0, 15, 0x3C};
    const struct vehicle_hil_imu_data_t imu = {-5000, 5000, -2000, 1, 1, 0x01};
    const struct vehicle_hil_obstacle_data_t obstacle = {3500, 1790, 100, 1, 1, 7, 0x77};
    const struct vehicle_hil_sensor_health_t health = {1, 0, 1, 1, 0xBEEF, 2, 0x22};
    const struct vehicle_hil_drive_request_t request = {-1000, 2000, 2, 1, 9, 0x99};
    const uint8_t vehicle_bytes[] = {0xFE, 0x06, 0x9C, 0xFF, 0xF1, 0x3F, 0x05, 0xA5};
    const uint8_t wheel_bytes[] = {0x7F, 0x43, 0x38, 0xFF, 0x0F, 0x00, 0x0F, 0x3C};
    const uint8_t imu_bytes[] = {0x78, 0xEC, 0x88, 0x13, 0x30, 0x18, 0x01, 0x01};
    const uint8_t obstacle_bytes[] = {0xAC, 0x0D, 0xFE, 0x06, 0xE4, 0x01, 0x07, 0x77};
    const uint8_t health_bytes[] = {0x0D, 0xEF, 0xBE, 0x00, 0x00, 0x00, 0x02, 0x22};
    const uint8_t request_bytes[] = {0x18, 0xFC, 0xD0, 0xA7, 0x00, 0x00, 0x09, 0x99};

    ASSERT_FRAME(vehicle_state, vehicle, vehicle_bytes);
    ASSERT_FRAME(wheel_speed, wheels, wheel_bytes);
    ASSERT_FRAME(imu_data, imu, imu_bytes);
    ASSERT_FRAME(obstacle_data, obstacle, obstacle_bytes);
    ASSERT_FRAME(sensor_health, health, health_bytes);
    ASSERT_FRAME(drive_request, request, request_bytes);
}

/* test_ecu_frames checks the four messages that the ECU sends. */
static void test_ecu_frames(void)
{
    const struct vehicle_hil_safety_command_t command = {100, 3, 1, 0x1234, 0, 0x00};
    const struct vehicle_hil_ecu_status_t status = {0x80000001UL, 37, 100, 14, 0xEE};
    const struct vehicle_hil_actuator_command_t actuator = {-800, -500, 1, 2, 3, 0x33};
    const struct vehicle_hil_ecu_diagnostics_t diagnostics = {255, 1, 2, 3, 4, 15, 6, 8, 0x88};
    const uint8_t command_bytes[] = {0x64, 0x13, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00};
    const uint8_t status_bytes[] = {0x01, 0x00, 0x00, 0x80, 0x25, 0x32, 0x0E, 0xEE};
    const uint8_t actuator_bytes[] = {0xE0, 0xFC, 0x0C, 0x5E, 0x00, 0x00, 0x03, 0x33};
    const uint8_t diagnostics_bytes[] = {0xFF, 0x01, 0x02, 0x03, 0x04, 0x6F, 0x08, 0x88};

    ASSERT_FRAME(safety_command, command, command_bytes);
    ASSERT_FRAME(ecu_status, status, status_bytes);
    ASSERT_FRAME(actuator_command, actuator, actuator_bytes);
    ASSERT_FRAME(ecu_diagnostics, diagnostics, diagnostics_bytes);
}

/* main runs the tests for CTest. */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_unpack_keeps_the_signs);
    RUN_TEST(test_sensor_frames);
    RUN_TEST(test_ecu_frames);
    return UNITY_END();
}

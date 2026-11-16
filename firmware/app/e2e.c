/* End-to-end protection for CAN frames. The receive checks in the core call these functions. */
#include "e2e.h"

#define E2E_CRC_POLYNOMIAL 0x1DU
#define E2E_CRC_START 0xFFU
#define E2E_CRC_FINAL_XOR 0xFFU
#define E2E_CRC_DATA_BYTES 7U

/* e2e_crc_add adds length bytes to a running CRC, one bit at a time. The two CRC functions use it. */
static uint8_t e2e_crc_add(uint8_t crc, const uint8_t *bytes, uint32_t length)
{
    for (uint32_t i = 0U; i < length; i++) {
        crc = (uint8_t)(crc ^ bytes[i]);
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            crc = ((crc & 0x80U) != 0U) ? (uint8_t)((crc << 1U) ^ E2E_CRC_POLYNOMIAL)
                                        : (uint8_t)(crc << 1U);
        }
    }
    return crc;
}

/* e2e_crc8 returns the CRC-8 SAE J1850 (0x1D, start 0xFF, final XOR 0xFF) of length bytes. */
uint8_t e2e_crc8(const uint8_t *bytes, uint32_t length)
{
    return (uint8_t)(e2e_crc_add(E2E_CRC_START, bytes, length) ^ E2E_CRC_FINAL_XOR);
}

/* e2e_frame_crc returns the byte 7 CRC: the ID low byte, the ID high byte, then data bytes 0-6. */
uint8_t e2e_frame_crc(uint16_t id, const uint8_t *data)
{
    const uint8_t id_bytes[2] = {(uint8_t)(id & 0xFFU), (uint8_t)(id >> 8U)};
    uint8_t crc = e2e_crc_add(E2E_CRC_START, id_bytes, 2U);
    crc = e2e_crc_add(crc, data, E2E_CRC_DATA_BYTES);
    return (uint8_t)(crc ^ E2E_CRC_FINAL_XOR);
}

/* e2e_counter_check compares counter with the last one in state, and updates state.
 * It writes the number of lost frames to lost (0 when none were lost). */
e2e_counter_result_t e2e_counter_check(e2e_counter_t *state, uint8_t counter, uint8_t *lost)
{
    /* The subtraction wraps at 16 like the counter, so 1 after 15 is a step of 2. */
    const uint8_t step = (uint8_t)((counter - state->last) % E2E_COUNTER_MODULO);
    const bool synced = state->synced;

    *lost = 0U;
    if (synced && (step == 0U)) {
        return E2E_COUNTER_REPEAT;
    }
    state->last = counter;
    state->synced = true;
    if (!synced) {
        return E2E_COUNTER_FIRST;
    }
    if (step == 1U) {
        return E2E_COUNTER_OK;
    }
    *lost = (uint8_t)(step - 1U);
    return E2E_COUNTER_JUMP;
}

/* e2e_counter_reset makes the next frame count as the first, for example after a timeout. */
void e2e_counter_reset(e2e_counter_t *state)
{
    state->last = 0U;
    state->synced = false;
}

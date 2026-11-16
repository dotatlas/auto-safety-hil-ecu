/* End-to-end protection for CAN frames: the CRC-8 in byte 7 and the alive counter in byte 6. */
#ifndef E2E_H
#define E2E_H

#include <stdbool.h>
#include <stdint.h>

#define E2E_COUNTER_MODULO 16U

typedef enum {
    E2E_COUNTER_FIRST,  /* the first frame after start or after a timeout: accepted, synced */
    E2E_COUNTER_OK,     /* last + 1: accepted */
    E2E_COUNTER_REPEAT, /* the same value again: rejected */
    E2E_COUNTER_JUMP    /* frames were lost: accepted, synced again */
} e2e_counter_result_t;

typedef struct {
    uint8_t last;
    bool synced;
} e2e_counter_t;

/* e2e_crc8 returns the CRC-8 SAE J1850 (0x1D, start 0xFF, final XOR 0xFF) of length bytes. */
uint8_t e2e_crc8(const uint8_t *bytes, uint32_t length);

/* e2e_frame_crc returns the byte 7 CRC: the ID low byte, the ID high byte, then data bytes 0-6. */
uint8_t e2e_frame_crc(uint16_t id, const uint8_t *data);

/* e2e_counter_check compares counter with the last one in state, and updates state.
 * It writes the number of lost frames to lost (0 when none were lost). */
e2e_counter_result_t e2e_counter_check(e2e_counter_t *state, uint8_t counter, uint8_t *lost);

/* e2e_counter_reset makes the next frame count as the first, for example after a timeout. */
void e2e_counter_reset(e2e_counter_t *state);

#endif

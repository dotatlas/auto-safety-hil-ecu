/* The ECU core API. It has no HAL or FreeRTOS includes, so it builds for the target and the PC. */
#ifndef CORE_H
#define CORE_H

#include <stdint.h>

#define CORE_FRAME_DATA_MAX 8U
#define CORE_TX_MAX 4U

typedef struct {
    uint16_t id;
    uint8_t dlc;
    uint8_t data[CORE_FRAME_DATA_MAX];
} core_frame_t;

typedef struct {
    uint16_t echo_id_offset;
} core_config_t;

typedef struct {
    core_frame_t tx[CORE_TX_MAX];
    uint8_t tx_count;
} core_output_t;

/* core_init clears the core state and keeps the config for core_on_frame. */
void core_init(const core_config_t *config);

/* core_on_frame takes one received CAN frame and queues its echo for core_step. */
void core_on_frame(uint32_t now_us, uint16_t id, const uint8_t *data, uint8_t dlc);

/* core_step moves the queued frames to out for the caller to send. */
void core_step(uint32_t now_ms, core_output_t *out);

#endif

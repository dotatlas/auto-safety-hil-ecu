/* The ECU core. It echoes frames now. The safety logic replaces the echo in a later change. */
#include "core.h"

#include <string.h>

#define CORE_CAN_ID_MAX 0x7FFU

static core_config_t core_config;
static core_frame_t core_queue[CORE_TX_MAX];
static uint8_t core_queue_count;

/* core_init clears the core state and keeps the config for core_on_frame. */
void core_init(const core_config_t *config)
{
    core_config = *config;
    core_queue_count = 0U;
}

/* core_on_frame takes one received CAN frame and queues its echo for core_step.
 * shortcut: the echo does not check CRC or counter, add these when the full protocol lands. */
void core_on_frame(uint32_t now_us, uint16_t id, const uint8_t *data, uint8_t dlc)
{
    (void)now_us;
    uint32_t echo_id = (uint32_t)id + core_config.echo_id_offset;

    if ((echo_id > CORE_CAN_ID_MAX) || (dlc > CORE_FRAME_DATA_MAX) ||
        (core_queue_count >= CORE_TX_MAX)) {
        return;
    }
    core_frame_t *frame = &core_queue[core_queue_count];
    frame->id = (uint16_t)echo_id;
    frame->dlc = dlc;
    memcpy(frame->data, data, dlc);
    core_queue_count++;
}

/* core_step moves the queued frames to out for the caller to send. */
void core_step(uint32_t now_ms, core_output_t *out)
{
    (void)now_ms;
    memcpy(out->tx, core_queue, sizeof(core_queue));
    out->tx_count = core_queue_count;
    core_queue_count = 0U;
}

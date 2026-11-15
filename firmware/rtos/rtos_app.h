/* The start of the FreeRTOS part of the firmware. main() calls rtos_start once. */
#ifndef RTOS_APP_H
#define RTOS_APP_H

/* rtos_start makes the tasks and starts the scheduler. It does not return. */
void rtos_start(void);

#endif

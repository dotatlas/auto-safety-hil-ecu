/* FreeRTOS settings for the STM32G474 (Cortex-M4F, 170 MHz). FreeRTOS reads this file at build time. */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

extern uint32_t SystemCoreClock;
extern void rtos_runtime_counter_start(void);
extern uint32_t rtos_runtime_counter_get(void);

/* Scheduler */
#define configUSE_PREEMPTION 1
#define configCPU_CLOCK_HZ (SystemCoreClock)
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configMAX_PRIORITIES 8
#define configMINIMAL_STACK_SIZE ((uint16_t)128)
#define configMAX_TASK_NAME_LEN 16
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_TIMERS 0
#define configUSE_MUTEXES 1

/* Memory: static allocation only, no heap */
#define configSUPPORT_STATIC_ALLOCATION 1
#define configSUPPORT_DYNAMIC_ALLOCATION 0

/* Checks */
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configASSERT(x)                                                                            \
    if ((x) == 0) {                                                                                \
        __asm volatile("cpsid i");                                                                 \
        for (;;) {                                                                                 \
        }                                                                                          \
    }

/* Run-time statistics, counted by TIM2 at 1 MHz */
#define configGENERATE_RUN_TIME_STATS 1
#define configUSE_TRACE_FACILITY 1
#define portCONFIGURE_TIMER_FOR_RUN_TIME_STATS() rtos_runtime_counter_start()
#define portGET_RUN_TIME_COUNTER_VALUE() rtos_runtime_counter_get()

/* API functions to include */
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

/* Interrupt priorities. The STM32G4 has 4 priority bits: 0 is the highest, 15 the lowest. */
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY                                                            \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY                                                       \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

/* Give the FreeRTOS port functions the names of the vector table entries. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#endif

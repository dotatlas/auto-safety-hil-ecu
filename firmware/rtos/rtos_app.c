/* The first FreeRTOS tasks: a blink task and a text task. Both use static memory only. */
#include "rtos_app.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "main.h"
#include "task.h"
#include "tim.h"

#define BLINK_PERIOD_MS 500U
#define HELLO_PERIOD_MS 1000U
#define BLINK_STACK_WORDS 128U
#define HELLO_STACK_WORDS 512U

static StaticTask_t blink_tcb;
static StackType_t blink_stack[BLINK_STACK_WORDS];
static StaticTask_t hello_tcb;
static StackType_t hello_stack[HELLO_STACK_WORDS];
static StaticTask_t idle_tcb;
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

/* blink_task toggles the green LED every 500 ms, at fixed times that do not drift. */
static void blink_task(void *argument)
{
    (void)argument;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        BSP_LED_Toggle(LED_GREEN);
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(BLINK_PERIOD_MS));
    }
}

/* hello_task prints the tick count once a second on the ST-LINK virtual COM port.
 * shortcut: printf blocks on the UART, use the non-blocking Debug task from milestone 5. */
static void hello_task(void *argument)
{
    (void)argument;
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        printf("FreeRTOS tick %lu\r\n", (unsigned long)xTaskGetTickCount());
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(HELLO_PERIOD_MS));
    }
}

/* rtos_start makes the tasks and starts the scheduler. It does not return. */
void rtos_start(void)
{
    (void)xTaskCreateStatic(blink_task, "blink", BLINK_STACK_WORDS, NULL, 1U, blink_stack,
                            &blink_tcb);
    (void)xTaskCreateStatic(hello_task, "hello", HELLO_STACK_WORDS, NULL, 1U, hello_stack,
                            &hello_tcb);
    vTaskStartScheduler();
    for (;;) {
    }
}

/* rtos_runtime_counter_start starts TIM2 for the FreeRTOS run-time statistics. */
void rtos_runtime_counter_start(void)
{
    (void)HAL_TIM_Base_Start(&htim2);
}

/* rtos_runtime_counter_get returns the TIM2 count in microseconds for FreeRTOS. */
uint32_t rtos_runtime_counter_get(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}

/* vApplicationGetIdleTaskMemory gives FreeRTOS the static memory for its idle task. */
void vApplicationGetIdleTaskMemory(StaticTask_t **tcb, StackType_t **stack,
                                   configSTACK_DEPTH_TYPE *stack_words)
{
    *tcb = &idle_tcb;
    *stack = idle_stack;
    *stack_words = configMINIMAL_STACK_SIZE;
}

/* vApplicationStackOverflowHook stops the chip when FreeRTOS finds a task stack overflow. */
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{
    (void)task;
    (void)name;
    __disable_irq();
    for (;;) {
    }
}

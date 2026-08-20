#include "RTE_Components.h"
#include CMSIS_device_header

#include <stdio.h>
#include "Driver_IO.h"
#include "board_config.h"
#include "fault_handler.h"
#include <errno.h>

#if defined(RTE_CMSIS_RTOS2_FreeRTOS)
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#endif

#if defined(RTE_Drivers_LL_PINCONF)
#include "pinconf.h"
#endif

#define _GET_DRIVER_REF(ref, peri, chan) \
    extern ARM_DRIVER_##peri Driver_##peri##chan; \
    static ARM_DRIVER_##peri * ref = &Driver_##peri##chan;
#define GET_DRIVER_REF(ref, peri, chan) _GET_DRIVER_REF(ref, peri, chan)

GET_DRIVER_REF(gpio_b, GPIO, BOARD_LEDRGB0_B_GPIO_PORT);
GET_DRIVER_REF(gpio_g, GPIO, BOARD_LEDRGB0_G_GPIO_PORT);
GET_DRIVER_REF(gpio_r, GPIO, BOARD_LEDRGB0_R_GPIO_PORT);
#define BOARD_LEDRGB_B_GPIO_PIN BOARD_LEDRGB0_B_GPIO_PIN
#define BOARD_LEDRGB_G_GPIO_PIN BOARD_LEDRGB0_G_GPIO_PIN
#define BOARD_LEDRGB_R_GPIO_PIN BOARD_LEDRGB0_R_GPIO_PIN

static void board_led_init(void)
{
    gpio_b->Initialize(BOARD_LEDRGB_B_GPIO_PIN, NULL);
    gpio_b->PowerControl(BOARD_LEDRGB_B_GPIO_PIN, ARM_POWER_FULL);
    gpio_b->SetDirection(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);

    gpio_r->Initialize(BOARD_LEDRGB_R_GPIO_PIN, NULL);
    gpio_r->PowerControl(BOARD_LEDRGB_R_GPIO_PIN, ARM_POWER_FULL);
    gpio_r->SetDirection(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_DIRECTION_OUTPUT);
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_LOW);
}

#if defined(RTE_CMSIS_RTOS2_FreeRTOS)

#define IDLE_TASK_STACK_SIZE          configMINIMAL_STACK_SIZE
#define TIMER_SERVICE_TASK_STACK_SIZE configTIMER_TASK_STACK_DEPTH
#define LED_TASK_STACK_WORDS          256U

static StackType_t idle_stack[2U * IDLE_TASK_STACK_SIZE];
static StaticTask_t idle_tcb;
static StackType_t timer_stack[2U * TIMER_SERVICE_TASK_STACK_SIZE];
static StaticTask_t timer_tcb;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &idle_tcb;
    *ppxIdleTaskStackBuffer = idle_stack;
    *pulIdleTaskStackSize   = IDLE_TASK_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                    StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer   = &timer_tcb;
    *ppxTimerTaskStackBuffer = timer_stack;
    *pulTimerTaskStackSize   = TIMER_SERVICE_TASK_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    for (;;) {
    }
}

static void led_blink_task(void *pvParameters)
{
    (void)pvParameters;

    for (;;) {
#if defined(RTSS_HE)
        gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#else
        gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#endif
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#endif

int main (void)
{
    int32_t ret = board_pins_config();
    if (ret != 0) {
        while (1) {
        }
    }

    fault_dump_enable(true);

    printf("\r\nHello Blinky!\r\n");

#if defined(ENSEMBLE_SOC_E1C) && defined(RTE_Drivers_LL_PINCONF)
    pinconf_set(BOARD_LEDRGB0_B_GPIO_PORT, BOARD_LEDRGB_B_GPIO_PIN,
                PINMUX_ALTERNATE_FUNCTION_0, PADCTRL_OUTPUT_DRIVE_STRENGTH_4MA);
#endif

    board_led_init();

#if !defined(RTE_CMSIS_RTOS2_FreeRTOS)
#if defined(RTSS_HE)
    SysTick_Config(SystemCoreClock / 10);
#else
    SysTick_Config(SystemCoreClock / 25);
#endif
#else

    printf("\r\nStarting Blinky Task!\r\n");

    SystemCoreClockUpdate();

    if (xTaskCreate(led_blink_task, "led_blink", LED_TASK_STACK_WORDS, NULL,
                    configMAX_PRIORITIES - 1, NULL) != pdPASS) {
        while (1) {
        }
    }

    vTaskStartScheduler();
    while (1) {
        __WFI();
    }
#endif
}

#if !defined(RTE_CMSIS_RTOS2_FreeRTOS)
void SysTick_Handler (void)
{
#if defined(RTSS_HE)
    gpio_b->SetValue(BOARD_LEDRGB_B_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#else
    gpio_r->SetValue(BOARD_LEDRGB_R_GPIO_PIN, GPIO_PIN_OUTPUT_STATE_TOGGLE);
#endif
}
#endif

int _close(int val)
{
    (void)val;
    errno = ENOSYS;
    return -1;
}

int _lseek(int val0, int val1, int val2)
{
    (void)val0;
    (void)val1;
    (void)val2;
    errno = ENOSYS;
    return -1;
}

int _read(int val0, char * val1, int val2)
{
    (void)val0;
    (void)val1;
    (void)val2;
    errno = ENOSYS;
    return -1;
}

int _fstat(int val0, void * val1)
{
    (void)val0;
    (void)val1;
    errno = ENOSYS;
    return -1;
}

int _isatty(int val0)
{
    (void)val0;
    return 0;
}

int _getpid(void)
{
    return 1;
}

int _kill(int val0, int val1)
{
    (void)val0;
    (void)val1;
    errno = ENOSYS;
    return -1;
}

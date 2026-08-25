#ifndef FCYC_H
#define FCYC_H

#include <FreeRTOS.h>

#include "task.h"

#ifndef FCYC_putchar
#define UART_BASE 0x10000000
#define FCYC_putchar(x) FCYC_putchar(x)
#endif

void FCYC_init(void);

#endif

#ifdef FCYC_IMPLEMENTATION
static int FCYC_putchar(int c) {
  volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
  *uart = c;
  return 0;
}

static void FCYC_print(const char *s) {
  while (*s)
    FCYC_putchar(*s++);
}

static void FCYC_print_num(uint64_t n) {
  char buf[24];
  int i = 0;
  if (n == 0) {
    FCYC_putchar('0');
    return;
  }
  while (n) {
    buf[i++] = '0' + (n % 10);
    n /= 10;
  }
  while (i--)
    FCYC_putchar(buf[i]);

  return;
}

static uint64_t FCYC_get_mtime(void) {
  uint64_t ullNextTime = 0ULL;
  uint32_t ulCurrentTimeHigh, ulCurrentTimeLow;
  volatile uint32_t *const pulTimeHigh =
      (volatile uint32_t
           *const)((configMTIME_BASE_ADDRESS) +
                   4UL); /* 8-byte type so high 32-bit word is 4 bytes up. */
  volatile uint32_t *const pulTimeLow =
      (volatile uint32_t *const)(configMTIME_BASE_ADDRESS);
  ulCurrentTimeHigh = *pulTimeHigh;
  ulCurrentTimeLow = *pulTimeLow;
  ullNextTime = (uint64_t)ulCurrentTimeHigh;
  ullNextTime <<= 32ULL; /* High 4-byte word is 32-bits up. */
  ullNextTime |= (uint64_t)ulCurrentTimeLow;
  return ullNextTime;
}

static volatile struct {
  TickType_t count;
  uint64_t start;
  uint64_t isr;
  uint64_t task;
  uint64_t isr_max;
  uint64_t isr_min;
  uint64_t task_max;
  uint64_t task_min;
} FCYC_gxData;

static TickType_t FCYC_gxInterval;
static uint64_t FCYC_gxIntervalMTIME;
static TaskHandle_t FCYC_gxBenchTask;

static void vTaskFunction(void *pvParameters) {
  (void)pvParameters;

  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    FCYC_gxData.task = FCYC_get_mtime();
    uint64_t xTaskLatency = FCYC_gxData.task - FCYC_gxData.start;

    uint64_t xISRLatency = FCYC_gxData.isr - FCYC_gxData.start;

    if (xTaskLatency > FCYC_gxData.task_max)
      FCYC_gxData.task_max = xTaskLatency;
    else if (xTaskLatency < FCYC_gxData.task_min)
      FCYC_gxData.task_min = xTaskLatency;

    if (xISRLatency > FCYC_gxData.isr_max)
      FCYC_gxData.isr_max = xISRLatency;
    else if (xISRLatency < FCYC_gxData.isr_min)
      FCYC_gxData.isr_min = xISRLatency;

    FCYC_gxData.start += FCYC_gxIntervalMTIME;
  }
}

static void vSummaryFunction(void *pvParameters) {
  const TickType_t xDelay = pdMS_TO_TICKS(1000);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xDelay);
    FCYC_print("isr_max lat:");
    FCYC_print_num((FCYC_gxData.isr_max * 1000) /
                   (configCPU_CLOCK_HZ / (unsigned long)1000000));
    FCYC_print("ns isr_min lat:");
    FCYC_print_num((FCYC_gxData.isr_min * 1000) /
                   (configCPU_CLOCK_HZ / (unsigned long)1000000));
    FCYC_print("ns task_max lat:");
    FCYC_print_num((FCYC_gxData.task_max * 1000) /
                   (configCPU_CLOCK_HZ / (unsigned long)1000000));
    FCYC_print("ns task_min lat:");
    FCYC_print_num((FCYC_gxData.task_min * 1000) /
                   (configCPU_CLOCK_HZ / (unsigned long)1000000));
    FCYC_print("ns\n");
  }
}

void vApplicationTickHook(void) {
  FCYC_gxData.isr = FCYC_get_mtime();
  FCYC_gxData.count += 1;
  if (FCYC_gxData.count == FCYC_gxInterval) {
    FCYC_gxData.count = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(FCYC_gxBenchTask, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void FCYC_init(void) {
  static StaticTask_t xTaskBuffer[2];
  static StackType_t xStack[2][configMINIMAL_STACK_SIZE];

  FCYC_print("Starting main:\n");
  FCYC_gxInterval = pdMS_TO_TICKS(100);
  FCYC_gxIntervalMTIME =
      (configCPU_CLOCK_HZ / (uint64_t)configTICK_RATE_HZ) * FCYC_gxInterval;
  FCYC_print_num(FCYC_gxIntervalMTIME);
  FCYC_print("\n");
  FCYC_gxData.start = FCYC_get_mtime() + FCYC_gxIntervalMTIME;
  FCYC_gxData.isr_min = ~0;
  FCYC_gxData.task_min = ~0;

  FCYC_gxBenchTask =
      xTaskCreateStatic(vTaskFunction, "bench", configMINIMAL_STACK_SIZE, NULL,
                        configMAX_PRIORITIES - 1, xStack[0], &xTaskBuffer[0]);
  xTaskCreateStatic(vSummaryFunction, "summary", configMINIMAL_STACK_SIZE, NULL,
                    configMAX_PRIORITIES - 2, xStack[1], &xTaskBuffer[1]);
}
#endif

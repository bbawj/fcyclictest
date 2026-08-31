#ifndef FCYC_H
#define FCYC_H

#include <FreeRTOS.h>

#include "task.h"

#ifndef FCYC_putchar
#define FCYC_putchar(x) (x)
#endif

// ulIterations is the number of iterations the task is executed before stopping
// xInterval is the expected number of miliseconds between each task execution
void vFCYC_init(unsigned long ulIterations, unsigned long ulInterval,
                TaskHandle_t vDoneCallback);
void vFCYCTickHook(void);

#ifndef FCYC_get_mtime
#error "Please define FCYC_get_mtime"
#endif

#endif

#ifdef FCYC_IMPLEMENTATION
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

#ifndef MAX_HIST
#define MAX_HIST 1000
#endif
#ifndef HIST_BINS
#define HIST_BINS 5
#endif

static volatile struct {
  unsigned long count;
  unsigned long iterations;
  uint64_t start;
  uint64_t isr;
  uint64_t task;
  uint64_t isr_max;
  uint64_t isr_min;
  uint64_t task_max;
  uint64_t task_min;
  uint64_t hist[MAX_HIST]
} FCYC_gxData;

static TickType_t FCYC_gxInterval;
static uint64_t FCYC_gxIntervalMTIME;
static TaskHandle_t FCYC_gxBenchTask;
static BaseType_t xDone = pdFALSE;
static TaskHandle_t FCYC_vDoneCallback = NULL;

static void pvSort(uint64_t *arr, BaseType_t xLen) {
  configASSERT(xLen > 0);
  for (BaseType_t i = 1; i < xLen; ++i) {
    for (BaseType_t j = i; arr[j] < arr[j - 1] && j > 0; j--) {
      uint64_t temp = arr[j - 1];
      arr[j - 1] = arr[j];
      arr[j] = temp;
    }
  }
}

static void pvPrintSummary(void) {
  FCYC_print("Iterations: ");
  FCYC_print_num(FCYC_gxData.count);
  FCYC_print("\n");
  FCYC_print("isr_max lat:");
  FCYC_print_num(FCYC_gxData.isr_max);
  FCYC_print("ns isr_min lat:");
  FCYC_print_num(FCYC_gxData.isr_min);
  FCYC_print("ns task_max lat:");
  FCYC_print_num(FCYC_gxData.task_max);
  FCYC_print("ns task_min lat:");
  FCYC_print_num(FCYC_gxData.task_min);
  FCYC_print("ns\n");
}

static void pvPrintHist(void) {
  pvSort(FCYC_gxData.hist, FCYC_gxData.count);
  pvPrintSummary();
  unsigned long interval =
      (FCYC_gxData.task_max - FCYC_gxData.task_min + HIST_BINS - 1) / HIST_BINS;
  if (FCYC_gxData.count < HIST_BINS) {
    for (unsigned long i = 0; i < FCYC_gxData.count; ++i) {
      FCYC_print_num(i);
      FCYC_print(". ");
      FCYC_print_num(FCYC_gxData.hist[i]);
      FCYC_print("ns\n");
    }
  } else {
    unsigned long freq = 1;
    unsigned long min = FCYC_gxData.task_min;
    for (unsigned long i = 0; i < FCYC_gxData.count; ++i) {
      if (FCYC_gxData.hist[i] >= min + interval) {
        FCYC_print("[");
        FCYC_print_num(min);
        FCYC_print(",");
        FCYC_print_num(FCYC_gxData.hist[i - 1]);
        FCYC_print("]: ");
        for (unsigned long j = 0; j < freq; ++j) {
          FCYC_print("*");
        }
        FCYC_print("\n");
        freq = 1;
        min = FCYC_gxData.hist[i];
      } else
        freq++;
    }
    FCYC_print("[");
    FCYC_print_num(min);
    FCYC_print(",");
    FCYC_print_num(FCYC_gxData.task_max);
    FCYC_print("]: ");
    for (unsigned long j = 0; j < freq; ++j) {
      FCYC_print("*");
    }
    FCYC_print("\n");
  }
}

static void vTaskFunction(void *pvParameters) {
  (void)pvParameters;

  while (FCYC_gxData.iterations == 0 ||
         FCYC_gxData.count < FCYC_gxData.iterations) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    FCYC_gxData.task = FCYC_get_mtime();
    uint64_t xTaskLatency = (FCYC_gxData.task - FCYC_gxData.start) * 1000 /
                            (configCPU_CLOCK_HZ / (unsigned long)1000000);

    uint64_t xISRLatency = (FCYC_gxData.isr - FCYC_gxData.start) * 1000 /
                           (configCPU_CLOCK_HZ / (unsigned long)1000000);

    if (xTaskLatency > FCYC_gxData.task_max)
      FCYC_gxData.task_max = xTaskLatency;
    else if (xTaskLatency < FCYC_gxData.task_min)
      FCYC_gxData.task_min = xTaskLatency;

    if (xISRLatency > FCYC_gxData.isr_max)
      FCYC_gxData.isr_max = xISRLatency;
    else if (xISRLatency < FCYC_gxData.isr_min)
      FCYC_gxData.isr_min = xISRLatency;

    FCYC_gxData.hist[FCYC_gxData.count++] = xTaskLatency;

    FCYC_gxData.start += FCYC_gxIntervalMTIME;
  }

  xDone = pdTRUE;
  FCYC_print("All iterations done\n");
  pvPrintHist();

  if (FCYC_vDoneCallback != NULL)
    xTaskNotifyGive(FCYC_vDoneCallback);

  vTaskDelete(NULL);
}

static void vSummaryFunction(void *pvParameters) {
  const TickType_t xDelay = pdMS_TO_TICKS(1000);
  TickType_t xLastWakeTime = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&xLastWakeTime, xDelay);
    pvPrintSummary();
    if (xDone == pdTRUE)
      break;
  }
  vTaskDelete(NULL);
}

void vFCYCTickHook(void) {
  FCYC_gxData.isr = FCYC_get_mtime();
  static TickType_t count = 0;
  count += 1;
  if (count == FCYC_gxInterval) {
    count = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(FCYC_gxBenchTask, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void vFCYC_init(unsigned long ulIterations, unsigned long ulInterval,
                TaskHandle_t vDoneCallback) {
  static StaticTask_t xTaskBuffer[2];
  static StackType_t xStack[2][configMINIMAL_STACK_SIZE];

  FCYC_print("FCYC_init called\n");
  FCYC_gxData.iterations = ulIterations;
  FCYC_gxInterval = pdMS_TO_TICKS(ulInterval);
  FCYC_gxIntervalMTIME =
      (configCPU_CLOCK_HZ / (uint64_t)configTICK_RATE_HZ) * FCYC_gxInterval;
  FCYC_gxData.start = FCYC_get_mtime() + FCYC_gxIntervalMTIME;
  FCYC_gxData.isr_min = ~0;
  FCYC_gxData.task_min = ~0;

  FCYC_gxBenchTask =
      xTaskCreateStatic(vTaskFunction, "bench", configMINIMAL_STACK_SIZE, NULL,
                        configMAX_PRIORITIES - 1, xStack[0], &xTaskBuffer[0]);
  FCYC_vDoneCallback = vDoneCallback;
  xTaskCreateStatic(vSummaryFunction, "summary", configMINIMAL_STACK_SIZE, NULL,
                    configMAX_PRIORITIES - 2, xStack[1], &xTaskBuffer[1]);
}
#endif

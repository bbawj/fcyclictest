#include <FreeRTOS.h>

#include "task.h"

#define FCYC_IMPLEMENTATION
#define FCYC_putchar(x) putchar(x)
#define FCYC_get_mtime() get_mtime()
#include "fcyc.h"

void vAssertCalled(const char *pcFileName, uint32_t ulLine) {
  volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

  /* Called if an assertion passed to configASSERT() fails.  See
   * http://www.freertos.org/a00110.html#configASSERT for more information. */

  FCYC_print("ASSERT! Line ");
  FCYC_print_num((int)ulLine);
  FCYC_print(", file ");
  FCYC_print(pcFileName);
  FCYC_print("\n");

  taskENTER_CRITICAL();
  {
    /* You can step out of this function to debug the assertion by using
     * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
     * value. */
    while (ulSetToNonZeroInDebuggerToContinue == 0) {
      __asm volatile("NOP");
      __asm volatile("NOP");
    }
  }
  taskEXIT_CRITICAL();
}

void vApplicationTickHook(void) {
#ifdef FEATURE_TRACE
#endif

  vFCYCTickHook();
#ifdef FULL_DEMO
  vTimerPeriodicISRTests();
  vPeriodicEventGroupsProcessing();
  xNotifyTaskFromISR();
#endif
}

#ifdef FULL_DEMO

/* Standard demo application includes. */
#include "EventGroupsDemo.h"
#include "TaskNotify.h"
#include "TimerDemo.h"
#include "blocktim.h"
#include "dynamic.h"
#include "recmutex.h"

#define TIMER_TEST_PERIOD 50
#define mainNO_ERROR_CHECK_TASK_PERIOD pdMS_TO_TICKS(5000)

static void prvCheckTask(void *pvParameters) {
  tband_freertos_task_evtmarker_name(0, "Check Event");

  const TickType_t xDelayPeriod = mainNO_ERROR_CHECK_TASK_PERIOD;
  TickType_t xLastExecutionTime;
  const char *pcStatusMessage = ".";

  (void)pvParameters;

  /* Initialise xLastExecutionTime so the first call to vTaskDelayUntil()
  works correctly. */
  xLastExecutionTime = xTaskGetTickCount();

  /* Cycle for ever, delaying then checking all the other tasks are still
  operating without error.  The onboard LED is toggled on each iteration.
  If an error is detected then the delay period is decreased from
  mainNO_ERROR_CHECK_TASK_PERIOD to mainERROR_CHECK_TASK_PERIOD.  This has the
  effect of increasing the rate at which the onboard LED toggles, and in so
  doing gives visual feedback of the system status. */
  for (;;) {
    /* Delay until it is time to execute again. */
    vTaskDelayUntil(&xLastExecutionTime, xDelayPeriod);

    tband_freertos_task_evtmarker_begin(0, "");
    /* Check all the demo tasks (other than the flash tasks) to ensure
    that they are all still running, and that none have detected an error. */
    if (xAreDynamicPriorityTasksStillRunning() != pdTRUE) {
      pcStatusMessage = "ERROR: Dynamic priority demo/tests.\r\n";
    }

    if (xAreBlockTimeTestTasksStillRunning() != pdTRUE) {
      pcStatusMessage = "ERROR: Block time demo/tests.\r\n";
    }

    if (xAreRecursiveMutexTasksStillRunning() != pdTRUE) {
      pcStatusMessage = "ERROR: Recursive mutex demo/tests.\r\n";
    }

    if (xAreTimerDemoTasksStillRunning((TickType_t)xDelayPeriod) != pdPASS) {
      pcStatusMessage = "ERROR: Timer demo/tests.\r\n";
    }

    if (xAreEventGroupTasksStillRunning() != pdPASS) {
      pcStatusMessage = "ERROR: Event group demo/tests.\r\n";
    }

    if (xAreTaskNotificationTasksStillRunning() != pdPASS) {
      pcStatusMessage = "ERROR: Task notification demo/tests.\r\n";
    }

    /* Write the status message to the UART. */
    FCYC_print(pcStatusMessage);
    tband_freertos_task_evtmarker_end(0);
  }
}

#endif

#ifdef FEATURE_TRACE
static volatile uint8_t *pMetaBuf;
static const volatile uint8_t *pTraceBuf;
static size_t xMetaSize;
static size_t xTraceSize;

static void vLogTraceBuffers(void *pvParameters) {
  (void)pvParameters;

  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  FCYC_print("Dumping trace location\n");
  tband_stop_snapshot();
  while (!tband_tracing_finished()) {
    // Wait for all cores to finish
  }

  pMetaBuf = tband_get_metadata_buf(0);
  xMetaSize = tband_get_metadata_buf_amnt(0);
  FCYC_print("Metadata at: ");
  FCYC_print_num((uint64_t)pMetaBuf);
  FCYC_print(" size: ");
  FCYC_print_num(xMetaSize);
  FCYC_print("\n");

  pTraceBuf = tband_get_core_snapshot_buf(0);
  xTraceSize = tband_get_core_snapshot_buf_amnt(0);
  FCYC_print("Events at: ");
  FCYC_print_num((uint64_t)pTraceBuf);
  FCYC_print(" size: ");
  FCYC_print_num(xTraceSize);
  FCYC_print("\n");
  vTaskDelete(NULL);
}
#endif

int main(void) {
#ifndef FULL_DEMO
  FCYC_print("Running basic demo\n");
#else
  FCYC_print("Running full demo\n");
  /* Start all the other standard demo/test tasks.  They have no particular
        functionality, but do demonstrate how to use the FreeRTOS API and
     test the kernel port. */
  vStartDynamicPriorityTasks();
  vCreateBlockTimeTasks();
  vStartRecursiveMutexTasks();
  vStartTimerDemoTask(TIMER_TEST_PERIOD);
  vStartEventGroupTasks();
  vStartTaskNotifyTask();

  static StaticTask_t xCheckTaskBuffer;
  static StackType_t xCheckStack[configMINIMAL_STACK_SIZE];
  xTaskCreateStatic(prvCheckTask, "check", configMINIMAL_STACK_SIZE, NULL, 1,
                    xCheckStack, &xCheckTaskBuffer);
#endif

#ifdef FEATURE_TRACE
  FCYC_print("Enabling trace\n");
  configASSERT(configUSE_TRACE_FACILITY == 1);
  static TaskHandle_t xLogHandle;
  static StaticTask_t xTraceTaskBuffer;
  static StackType_t xTraceStack[configMINIMAL_STACK_SIZE];
  xLogHandle =
      xTaskCreateStatic(vLogTraceBuffers, "logtrace", configMINIMAL_STACK_SIZE,
                        NULL, 1, xTraceStack, &xTraceTaskBuffer);
  vFCYC_init(100, 100, xLogHandle);
  tband_gather_system_metadata();
  if (tband_trigger_snapshot() != 0) {
    configASSERT(0);
  }
#else
  vFCYC_init(100, 100, NULL);
#endif

  vTaskStartScheduler();
  // tband_freertos_scheduler_started();

  for (;;)
    ;

  return 0;
}

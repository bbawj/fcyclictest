#include <FreeRTOS.h>

#include "task.h"

#define FCYC_IMPLEMENTATION
#include "fcyc.h"

#ifndef FULL_DEMO
int main(void) {
  FCYC_print("Running basic demo\n");
  vFCYC_init();
  vTaskStartScheduler();

  for (;;)
    ;
  return 0;
}
#else

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
  }
}

void vApplicationTickHook(void) {
  vFCYCTickHook();
  vTimerPeriodicISRTests();
  vPeriodicEventGroupsProcessing();
  xNotifyTaskFromISR();
}

int main(void) {
  FCYC_print("Running full demo\n");
  /* Start all the other standard demo/test tasks.  They have no particular
        functionality, but do demonstrate how to use the FreeRTOS API and test
     the kernel port. */
  vStartDynamicPriorityTasks();
  vCreateBlockTimeTasks();
  vStartRecursiveMutexTasks();
  vStartTimerDemoTask(TIMER_TEST_PERIOD);
  vStartEventGroupTasks();
  vStartTaskNotifyTask();

  static StaticTask_t xTaskBuffer;
  static StackType_t xStack[configMINIMAL_STACK_SIZE];
  xTaskCreateStatic(prvCheckTask, "check", configMINIMAL_STACK_SIZE, NULL, 1,
                    xStack, &xTaskBuffer);

  vFCYC_init();
  vTaskStartScheduler();

  for (;;)
    ;

  return 0;
}

#endif

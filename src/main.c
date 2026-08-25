#include <FreeRTOS.h>

#include "task.h"

#define FCYC_IMPLEMENTATION
#include "fcyc.h"

int main(void) {
  FCYC_init();
  vTaskStartScheduler();

  for (;;)
    ;
  return 0;
}

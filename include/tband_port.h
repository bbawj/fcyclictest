#include <FreeRTOS.h>

#include "task.h"

#define tband_portTIMESTAMP() get_mtime()
#define tband_portTIMESTAMP_RESOLUTION_NS (100)

#define tband_portENTER_CRITICAL_FROM_ANY()                                    \
  bool tband_port_in_irq = xPortIsInsideInterrupt();                           \
  BaseType_t tband_port_key = 0;                                               \
  if (tband_port_in_irq) {                                                     \
    tband_port_key = taskENTER_CRITICAL_FROM_ISR();                            \
  } else {                                                                     \
    taskENTER_CRITICAL();                                                      \
    (void)tband_port_key;                                                      \
  }

#define tband_portEXIT_CRITICAL_FROM_ANY()                                     \
  if (tband_port_in_irq) {                                                     \
    taskEXIT_CRITICAL_FROM_ISR(tband_port_key);                                \
  } else {                                                                     \
    taskEXIT_CRITICAL();                                                       \
  }

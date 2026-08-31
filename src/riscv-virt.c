#include "riscv-virt.h"
#include <FreeRTOS.h>

int xGetCoreID(void) {
  int id;

  __asm("csrr %0, mhartid" : "=r"(id));

  return id;
}

uint64_t get_mtime(void) {
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

int putchar(int c) {
  volatile uint8_t *uart = (volatile uint8_t *)UART_BASE;
  *uart = c;
  return 0;
}

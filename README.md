# FreeRTOS cyclictest

Basic port of a [cyclictest-like](https://git.kernel.org/pub/scm/utils/rt-tests/rt-tests.git/tree/src/cyclictest) benchmark to FreeRTOS. Also inspired by Zephyr's [zyclictest](https://docs.zephyrproject.org/latest/services/debugging/zyclictest.html).

## Getting Started

The code is organised as a header-only library exposing vFCYC_init to create the benchmarking and summary tasks. Include it in your existing application to begin profiling. A basic main.c would look as follows:
```c
#define FCYC_IMPLEMENTATION
#include "fcyc.h"

int main(void) {
  // run for 100 iterations with 10ms between each task execution
  vFCYC_init(100, 10);
  vTaskStartScheduler();

  for (;;)
    ;
  return 0;
}
```

To test the provided application, compile with cmake giving a path to your toolchain and whether to use the FULL_DEMO option.
```bash
cmake -DFULL_DEMO=ON -DTOOLCHAIN_PATH="/opt/riscv/bin" -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -B build
```

Run with qemu:
```
cd build

make run

# Example output
Running full demo
FCYC_init called
Iterations: 100
isr_max lat:13400ns isr_min lat:12200ns task_max lat:18400ns task_min lat:14200ns
[14200,15000]: ***********************
[15300,16100]: *************************************************
[16400,17200]: **********
[17300,18100]: *****************
[18300,18400]: **
```

## Description

Benchmarks ISR service latency (time taken between interrupt trigger and ISR execution). The interrupt used here is tied to the FreeRTOS tick interrupt, and the ISR runs off the vApplicationTickHook.

Additionally, a task is notified from the ISR, giving the combined task latency (interrupt service + ISR execution + task switch).

The base application runs in QEMU with a specified `icount` `SHIFT` value to represent the time per instruction as `2^(SHIFT)` ns. This gives us a deterministic way to calculate latencies in the emulated hardware across different Host OS. The base application also does not include any additional tasks beyond the benchmark and summary tasks. Without the presence of load, the measurements are mostly of the scheduler overhead in performing context switches and does not include other sources such as external hardware interrupts and anything else that may occur on real systems.


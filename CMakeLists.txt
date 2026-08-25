cmake_minimum_required(VERSION 3.25)
project(freertos-cyclictest C ASM)
include(FetchContent)

FetchContent_Declare( freertos_kernel
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
  GIT_TAG        main #Note: Best practice to use specific git-hash or tagged version
)
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
  INTERFACE
  .
  include
)

target_compile_definitions(freertos_config
  INTERFACE
  projCOVERAGE_TEST=0
)

set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
# Select the native compile PORT
set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
# Select the cross-compile PORT
if (CMAKE_CROSSCOMPILING)
  set(FREERTOS_PORT "GCC_RISC_V_GENERIC" CACHE STRING "" FORCE)
  set(FREERTOS_RISCV_EXTENSION "RISCV_MTIME_CLINT_no_extensions" CACHE STRING "" FORCE)
endif()

FetchContent_MakeAvailable(freertos_kernel)

add_executable(${PROJECT_NAME} 
  src/main.c
)

target_sources(${PROJECT_NAME} PRIVATE
  startup.S
)

# 6. Link FreeRTOS kernel and configuration libraries to your app
target_link_libraries(${PROJECT_NAME} PRIVATE 
  freertos_kernel
  freertos_config
)

# 7. Add RISC-V specific compiler/linker flags
target_compile_options(${PROJECT_NAME} PUBLIC 
  -O3
)
target_link_options(${PROJECT_NAME} PRIVATE 
  -T "${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
  -nostartfiles
)

find_program(QEMU_EXECUTABLE NAMES qemu-system-riscv32)

set(SHIFT "2" CACHE STRING "Sets the qemu 'icount' shift value to N; each instruction will take 2^N ns to complete.")

add_custom_target(run
  COMMAND ${QEMU_EXECUTABLE} -icount shift=${SHIFT} -machine virt -nographic -bios none -kernel ${PROJECT_NAME}
  COMMENT "Running QEMU..."
  DEPENDS ${PROJECT_NAME}
  VERBATIM
)

option (FULL_DEMO "Bring in the full demo suite from FreeRTOS demo commons" OFF)

if (FULL_DEMO)
  FetchContent_Declare(freertos_demos
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES ""
  )
  FetchContent_MakeAvailable(freertos_demos)
  target_compile_definitions(${PROJECT_NAME} PUBLIC -DFULL_DEMO=1)
  target_include_directories(${PROJECT_NAME} PUBLIC
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/include
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/ThirdParty/Template
  )
  set(DEMO_SRCS 
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/TaskNotify.c
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/EventGroupsDemo.c
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/TimerDemo.c
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/blocktim.c
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/dynamic.c
    ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/Common/Minimal/recmutex.c
  )
  file(GLOB TP_SRCS ${freertos_demos_SOURCE_DIR}/FreeRTOS/Demo/ThirdParty/*.c)
  target_sources(${PROJECT_NAME} PUBLIC ${DEMO_SRCS} ${TP_SRCS})
endif()


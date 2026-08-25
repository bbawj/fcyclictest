.section .text.start
.global _start

_start:
    la sp, _stack_top
    la t0, freertos_risc_v_trap_handler
    csrw mtvec, t0
    call main
    call exit
1:  j 1b

.equ VIRT_TEST, 0x100000
.global exit
exit:
    li t0, VIRT_TEST
    li t1, 0x5555
    sw t1, 0(t0)
    ret

ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
    .text : { *(.text.start) *(.text*) } > RAM
    .rodata : { *(.rodata*) } > RAM
    .data : { *(.data*) } > RAM
    .bss : { __bss_start = .; *(.bss*) *(COMMON) _bss_end = .; } > RAM
    . = ALIGN(16);
    . = . + 0x4000;
    _stack_top = .;
}


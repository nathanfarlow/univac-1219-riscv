ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);  /* R-X */
    data PT_LOAD FLAGS(6);  /* RW- */
}

SECTIONS {
    . = 0x0;

    .text : { *(.text.start) *(.text*) } :text
    .data : { *(.rodata*) *(.srodata*) *(.data*) *(.sdata*) } :data

    . = ALIGN(4);
    __bss_start = .;
    .bss  : { *(.bss*) *(.sbss*) } :data
    . = ALIGN(4);
    __bss_end = .;

    /* Uninitialized data section - NOT zeroed at startup */
    .noinit __bss_end (NOLOAD) : AT(__bss_end) { *(.noinit*) } :data

    __heap_start = .;
    . = . + 0x4000;  /* 16KB heap */
    __heap_end = .;

    /DISCARD/ : { *(.interp) *(.dynsym) *(.dynstr) *(.gnu.hash) *(.dynamic) *(.comment) *(.note*) *(.eh_frame) }
}

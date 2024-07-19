_kernel_virt_start = 0xC0000000;
SECTIONS {
    .text _kernel_virt_start : AT (0) {
        *(.text)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }
    .rdata : {
        *(.rdata*)
    }
    . = ALIGN(4096); /* For paging */
    _kernel_virt_end = .;
}

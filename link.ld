_kernel_phys_start = 0x100000;
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
    . = ALIGN(4096);
    _kernel_virt_end = .;
    _kernel_phys_end = _kernel_phys_start + (_kernel_virt_end - _kernel_virt_start);
}

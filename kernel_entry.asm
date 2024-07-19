; This file exists solely so that the linker does not move the entry point of the kernel to a new location.
segment .text

extern _kmain
bits 32

jmp _kmain

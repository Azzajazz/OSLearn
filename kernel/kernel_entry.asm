bits 32

global start

%include "constants.inc"

extern _kernelMain
extern __bss_start
extern __bss_finish

section .entry class=CODE
start:
    ;Clear BSS section
    mov edi, __bss_start
    mov ecx, __bss_finish
    sub ecx, __bss_start
    xor al, al
    rep stosb

    ;Jump to C code
    push BOOT_INFO_ADDR
    call _kernelMain

    ;kernelMain should not return, but if it does then halt
hang:
    cli
    hlt
    jmp hang

%include "kernel/interrupts.asm"
%include "kernel/x86.asm"

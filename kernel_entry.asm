; This file exists primarily so that the linker does not move the entry point of the kernel to a new location.
; It also contains the default interrupt and trap handlers
segment .text

extern _kmain
extern _print_cstring
bits 32

jmp _kmain

; @Handler
; The default trap handler
global _default_trap_handler
_default_trap_handler:
    push TRAP_MESSAGE
    call _print_cstring
    sub esp, 4
    cli
    hlt

; @Handler
; The default interrupt handler
global _default_interrupt_handler
_default_interrupt_handler:
    push INTERRUPT_MESSAGE
    call _print_cstring
    sub esp, 4
    cli
    hlt

TRAP_MESSAGE: db "TRAP!", 0
INTERRUPT_MESSAGE: db "INTERRUPT!", 0

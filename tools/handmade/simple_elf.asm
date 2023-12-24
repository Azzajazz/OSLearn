global _start

section .text
_start:
    mov eax, 0
    mov edi, 1
    int 0x10

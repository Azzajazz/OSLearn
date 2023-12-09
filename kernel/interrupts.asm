bits 32

global _testHandler
global _pitHandler
global _keyboardHandler

extern _stringPrint
extern _printFmt
extern _keyboardInterrupt

CR equ 0x0d
LF equ 0x0a

PIC1_COMMAND equ 0x20
PIC1_DATA equ 0x21
PIC2_COMMAND equ 0xA0
PIC2_DATA equ 0xA1

PIC_EOI equ 0x20
PIC_READ_ISR equ 0x0B

KEYBOARD_INPUT equ 0x60
KEYBOARD_OUTPUT equ 0x60
KEYBOARD_COMMAND equ 0x64
KEYBOARD_STATUS equ 0x64

IRQ_KEYBOARD equ 0x1

section .text
;@DEBUG: This is temporary.
;%Handler
;A simple test handler.
_testHandler:
    push test_message
    call _stringPrint
    add esp, 4
    iret

;%Handler
;Hardware interrupt identification
_pitHandler:
    xor eax, eax
    call pic_send_eoi
    iret

;%Handler
;Handler for PS/2 keyboard port
_keyboardHandler:
    xor eax, eax
    in al, KEYBOARD_INPUT
    push eax
    call _keyboardInterrupt
    add esp, 4
    mov eax, IRQ_KEYBOARD
    call pic_send_eoi
    iret

;%Function
;A helper function to send the EOI signal to the relevant PIC controller.
;Parameters:
;  eax: interrupt request
;Returns:
;  None
pic_send_eoi:
    cmp eax, 8
    jl master
    mov al, PIC_EOI
    out PIC2_COMMAND, al
master:
    mov al, PIC_EOI
    out PIC1_COMMAND, al
    ret

;%Function
;Reads the ISR regsiter from the cascaded PICs
;Parameters:
;  None
;Returns:
;  eax: slaveISR:masterISR
pic_read_isr:
    mov al, PIC_READ_ISR
    out PIC2_COMMAND, al
    mov al, 0
    out 0x80, al
    in al, PIC2_COMMAND
    mov bh, al
    
    mov al, PIC_READ_ISR
    out PIC1_COMMAND, al
    mov al, 0
    out 0x80, al
    in al, PIC1_COMMAND
    mov bl, al
    
    mov eax, ebx
    ret
    
section .data
test_message: db "This is a test exception call!", CR, LF, 0
format_string_x32: db "%x32", CR, LF, 0
format_string_x8: db "%x8", CR, LF, 0

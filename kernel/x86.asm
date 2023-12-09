bits 32

;@TODO: These could all be inlined
global @x86Out8@8
global @x86Out16@8
global @x86Out32@8
global @x86In8@4
global @x86In16@4
global @x86In32@4
global @x86IoWait@0

global @ps2WaitForReadable@0
global @ps2WaitForWriteable@0

;@DEBUG: temporary
global @read_isr@0

section .text
;%Function
;A wrapper for the 8-bit x86 out instruction
;Parameters:
;  ecx: Port number
;  edx: Message to send
;Returns:
;  None
@x86Out8@8:
    mov al, cl
    mov dx, cx
    out dx, al
    ret

;%Function
;A wrapper for the 16-bit x86 out instruction
;Parameters:
;  ecx: Port number
;  edx: Message to send
;Returns:
;  None
@x86Out16@8:
    mov ax, cx
    mov dx, cx
    out dx, ax
    ret

;%Function
;A wrapper for the 32-bit x86 out instruction
;Parameters:
;  ecx: Port number
;  edx: Message to send
;Returns:
;  None
@x86Out32@8:
    mov eax, ecx
    mov dx, cx
    out dx, eax
    ret

;%Function
;A wrapper for the 8-bit x86 in instruction
;Parameters:
;  ecx: Port number
;Returns:
;  eax: Message that was received 
@x86In8@4:
    xor eax, eax
    mov dx, cx
    in al, dx
    ret

;%Function
;A wrapper for the 16-bit x86 in instruction
;Parameters:
;  ecx: Port number
;Returns:
;  eax: Message that was received 
@x86In16@4:
    xor eax, eax
    mov dx, cx
    in ax, dx
    ret

;%Function
;A wrapper for the 32-bit x86 in instruction
;Parameters:
;  ecx: Port number
;Returns:
;  eax: Message that was received 
@x86In32@4:
    mov dx, cx
    in eax, dx
    ret

;%Function
;I/O waiting function for x86
;Parameters:
;  None
;Returns:
;  None
@x86IoWait@0:
    mov al, 0
    out 0x80, al
    ret


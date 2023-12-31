bits 16

global start

%include "constants.inc"

section .entry class=CODE
start:
    mov ax, stage2_entry_message
    call puts_real_mode

;@TODO: Anything else that is required in real mode, e.g:
;  - Memory size query
;  - Target processor BIOS information

    lgdt [gdtr]
    call a20_enabled
    test ax, ax
    jne skip_a20_enable
    call enable_a20

skip_a20_enable:
    ;Load the kernel into high memory
    ;The A20 line must be enabled for this to work
    mov ax, kernel_load_message
    call puts_real_mode

    mov ax, kernel_name
    mov bx, KERNEL_OFFSET 
    mov cx, KERNEL_SECTOR
    call read_fat_file

    mov ax, success_message
    call puts_real_mode

    ;Enter protected mode
    mov ax, protected_mode_message
    call puts_real_mode

    mov ah, 0x00
    mov al, 0x02
    int 0x10

    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pmode

pmode:
    bits 32
    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ;Write to memory for kernel to use
    mov dword [BOOT_INFO_ADDR], IDT_ADDR
    mov dword [BOOT_INFO_ADDR + 4], PAGE_DIR_ADDR
    mov dword [BOOT_INFO_ADDR + 8], PAGE_TABLES_VIRT_ADDR
    mov dword [BOOT_INFO_ADDR + 12], KERNEL_DATA_VIRT_ADDR

    ;Set up paging for low 1MiB
    xor eax, eax
    xor ebx, ebx
    mov ecx, 0x100000
    call map_memory

    ;Set up paging for kernel memory
    mov eax, KERNEL_ADDR
    mov ebx, KERNEL_VIRT_ADDR
    mov ecx, 0x40000000
    call map_memory

    ;Enable paging
    mov eax, PAGE_DIR_ADDR
    mov cr3, eax

    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax    

    ;Set up interrupts
    mov ah, 0x20
    mov al, 0x28
    call initialize_pic
    call write_idt
    lidt [idtr]
    sti

    ;Jump to kernel code
    mov eax, KERNEL_VIRT_ADDR
    jmp eax

;%Function
;Checks whether the A20 line is enabled.
;This function compares the addresses 0x0000:0x7DFE and 0xffff:0x7E0E.
;If they are different, then the A20 line has been enabled. Otherwise, the address
;0x0000:0x7DFE is rotated left and compared again to ensure no false negatives.
;Parameters:
;  None
;Returns:
;  ax: 0 if disabled, 1 otherwise
a20_enabled:
    bits 16
    push si
    push di
    push ds
    
    mov ax, 0xffff
    mov ds, ax
    mov si, 0x7DFE
    mov di, 0x7E0E
    mov bx, word [es:si]
    mov cx, word [ds:si]
    cmp bx, cx
    jne enabled
    
    rcl word [es:si], 8
    mov bx, word [es:si]
    cmp bx, cx
    jne enabled

    mov ax, 0
    jmp a20_enabled_done
enabled:
    mov ax, 1
a20_enabled_done:
    pop ds
    pop di
    pop si
    ret

;%Function
;Attempts to enable the A20 port through the keyboard control port.
;Parameters:
;  None
;Returns:
;  None
enable_a20:
    bits 16

    call poll_keyboard
    mov al, 0xAD
    out 0x64, al

    call poll_keyboard
    mov al, 0xD0
    out 0x64, al

    call poll_keyboard2
    in al, 0x60
    push eax

    call poll_keyboard
    mov al, 0xD1
    out 0x64, al

    call poll_keyboard
    pop eax
    or al, 2
    out 0x60, al

    call poll_keyboard
    mov al, 0xEA
    out 0x64, al

    call poll_keyboard
    ret

poll_keyboard:
    in al, 0x64
    test al, 2
    jnz poll_keyboard
    ret

poll_keyboard2:
    in al, 0x64
    test al, 1
    jz poll_keyboard2
    ret

;%Function
;  Initialize the PIC controller
;Parameters:
;  ah: Master PIC offset
;  al: Slave PIC offset
;Returns:
;  None
initialize_pic:
    bits 32
    mov bx, ax 

    ;Save mask registers
    in al, 0x21
    mov cl, al
    in al, 0xA1
    mov ch, al

    ;0x11 = ICW1 + ICW4 needed 
    mov al, 0x11
    out 0x20, al
    mov al, 0
    out 0x80, al
    mov al, 0x11
    out 0xA0, al
    mov al, 0
    out 0x80, al

    ;Write offsets to PICs
    mov al, bh
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, bl
    out 0xA1, al
    mov al, 0
    out 0x80, al

    ;Notify PICs of master/slave
    mov al, 0x4
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 0x2
    out 0xA1, al
    mov al, 0
    out 0x80, al

    ;0x01 = 8086 mode
    mov al, 0x01
    out 0x21, al
    mov al, 0
    out 0x80, al
    mov al, 0x01
    out 0xA1, al
    mov al, 0
    out 0x80, al

    ;Restore masks
    mov al, cl
    out 0x21, al
    mov al, ch
    out 0xA1, al
    
    ret

;%Function
;Initializes the IDT at the address IDT_ADDR
;Parameters:
;  None
;Returns:
;  None
write_idt:
    bits 32
    push esi
    
    mov ebx, IDT_ADDR
    mov esi, 0x20
write_trap_handlers:
    test esi, esi
    je write_interrupt_handlers
    dec esi
    mov eax, default_trap_handler
    mov ecx, 0x08 ;Kernel code segment
    mov edx, 0x8F ;Trap gate, priority 0
    push ebx
    call write_idt_entry
    pop ebx
    add ebx, 8
    jmp write_trap_handlers
write_interrupt_handlers:
    mov esi, 0xE0
loop_interrupt_handlers:
    test esi, esi
    je .done
    dec esi
    mov eax, default_interrupt_handler
    mov ecx, 0x08 ;Kernel code segment
    mov edx, 0x8E ;Interrupt gate, priority 0
    push ebx
    call write_idt_entry
    pop ebx
    add ebx, 8
    jmp loop_interrupt_handlers

.done:
    pop esi
    ret
    
;%Function
;Writes an IDT entry at the given address, with given parameters
;Parameters:
;  eax: offset
;  ebx: address
;  ecx: selector
;  edx: type attributes
write_idt_entry:
    bits 32
    push esi

    movzx esi, cx
    shl esi, 16
    mov si, ax
    mov dword [ds:ebx], esi
    shr eax, 16
    mov esi, eax
    shl esi, 16
    mov si, dx
    shl si, 8
    mov dword [ds:ebx + 4], esi
    
    pop esi
    ret 

;%Function
;Maps the physical memory to the given virtual memory. Pages for the memory mapping are contiguously allocated.
;All parameters must be multiples of 0x1000.
;Parameters:
;  eax: Physical memory start
;  ebx: Virtual memory start
;  ecx: Size of memory to map
;Returns:
;  None
map_memory:
    bits 32

    push edi
    push esi

    ;bits 22-31 are the page directory index
    mov esi, ebx
    shr esi, 22
    shl esi, 2
    ;bits 12-21 are the page table index
    mov edi, ebx
    shr edi, 12
    and edi, 0x3FF
    shl edi, 2
    ;Also offset into the correct page table
    mov edx, esi
    shl edx, 10
    add edi, edx
    add edi, PAGE_TABLES_ADDR
    add esi, PAGE_DIR_ADDR
    
    ;The number of page table entries is ecx / 4096
    mov edx, ecx
    shr edx, 12
    push edx
    push edi
.fill_table:
    test edx, edx
    je .fill_done
    dec edx
    mov dword [ds:edi], eax
    or dword [ds:edi], 3
    add edi, 4    
    add eax, 0x1000
    jmp .fill_table
.fill_done:
    pop edi
    ;The number of page directory entries is ceil(# table entries / 1024)
    pop edx
    add edx, 1023
    shr edx, 10
.fill_directory:
    test edx, edx
    je .done
    dec edx
    mov dword [ds:esi], edi
    or dword [ds:esi], 3
    add esi, 4
    add edi, (1024 * 4)
    jmp .fill_directory
.done:
    pop esi
    pop edi
    ret

;%Function
;Fills a page table entry to point to contiguous physical memory
;Parameters:
;  eax: number of entries
;  ebx: page table address
;Returns:
;  None
fill_page_table:
    bits 32

    xor ecx, ecx
.loop:
    test eax, eax
    je fill_done 
    dec eax
    mov dword [ds:ebx], ecx ;Page block starts at relative address 0, read-write and present
    or dword [ds:ebx], 3
    add ebx, 4
    add ecx, 0x1000
    jmp .loop
fill_done:
    ret

%include "input_output.asm"

;%Handler
;Default trap handler (this just hangs for now)
default_trap_handler:
    nop
    jmp default_trap_handler

;%Handler
;Default interrupt handler (this just hangs for now)
default_interrupt_handler:
    nop
    jmp default_interrupt_handler

gdtr:
dw (GDT_SIZE - 1)
dd GDT_ADDR
idtr:
dw (IDT_SIZE - 1)
dd IDT_ADDR
stage2_entry_message: db "Executing stage2.bin", CR, LF, 0
kernel_load_message: db "Loading kernel.bin...  ", 0
protected_mode_message: db "Entering protected mode...  ", 0
success_message: db "Success!", CR, LF, 0
failure_message: db "Failure!", CR, LF, 0
kernel_name: db "kernel  bin"

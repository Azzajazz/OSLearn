org 0x7c00
bits 16

%include "disk_info.inc"

STAGE2_START equ 0x7e00
KERNEL_SEGMENT_REAL_MODE equ 0xffff
KERNEL_OFFSET_REAL_MODE equ 0x10
KERNEL_PHYS_ADDR equ KERNEL_SEGMENT_REAL_MODE * 16 + KERNEL_OFFSET_REAL_MODE

PAGE_DIR_PHYS_ADDR equ 0x1000
LOW_TABLE_PHYS_ADDR equ 0x2000
KERNEL_TABLE_PHYS_ADDR equ LOW_TABLE_PHYS_ADDR + 4 * 1024

KERNEL_VIRT_ADDR equ 0xC0000000

struc tfs_header_struc
    FAT_sector_offset:       resw 1
    TAT_sector_offset:       resw 1
    FAT_sector_count:        resb 1
    TAT_sector_count:        resb 1 ; TAT = Tag Allocation Table
    TFM_sector_offset:       resw 1 ; TFM = Tag File Map
    TFM_sector_count:        resb 1
    reserved_sector_count:   resb 1
    tag_data_sector_offset:  resw 1
    tag_data_sector_count:   resw 1
    file_data_sector_offset: resw 1
endstruc
; All remaining sectors not accounted for here are file data sectors, so the file data sector count is omitted.

jmp setup
; The jmp above is either 2 or 3 bytes. In order to preserve alignment, we align to the nearest 4 byte boundary
align 4
tfs_header:
istruc tfs_header_struc
    at FAT_sector_offset,       dw 0
    at TAT_sector_offset,       dw 0
    at FAT_sector_count,        db 0
    at TAT_sector_count,        db 0 ; TAT = Tag Allocation Table
    at TFM_sector_offset,       dw 0 ; TFM = Tag File Map
    at TFM_sector_count,        db 0
    at reserved_sector_count,   db 0
    at tag_data_sector_offset,  dw 0
    at tag_data_sector_count,   dw 0
    at file_data_sector_offset, dw 0
iend

setup:
    ; Set up segment registers
    xor ax, ax
    mov es, ax
    mov ds, ax
    mov fs, ax

    ; Set up the stack
    mov bp, 0x7c00 
    mov sp, 0x7c00 ; This may change later, since that's not a lot of room for the stack

    ; Load stage 2 of the bootloader
    mov ah, 0x02
    ; al = number of sectors to load
    mov al, [tfs_header + reserved_sector_count]
    sub al, 1 ; The first reserved sector is the boot sector, which is already loaded by the BIOS
    mov ch, 0 ; Cylinder number (bits 0-7)
    mov cl, 2 ; Sector number (bits 0-5)
    mov dh, 0 ; Head number
    mov dl, 0 ; Drive number
    mov bx, STAGE2_START ; Buffer to load the sectors to
    int 0x13

    ; Load kernel code from the file system
    ; The kernel code is assumed to start at the first sector in the file data region (and therefore the first entry in the FAT) and be allocated in contiguous sectors
    mov ax, [tfs_header + FAT_sector_offset]
    call linear_sector_address_to_chs
    mov ah, 0x02
    mov al, [tfs_header + FAT_sector_count]
    mov dl, 0
    mov bx, FAT
    int 0x13

    xor bx, bx
    mov ax, KERNEL_SEGMENT_REAL_MODE
    mov es, ax
load_kernel:
    mov ax, FAT
    call read_fat
    push ax ; Save the FAT entry for later

    mov ax, [tfs_header + file_data_sector_offset]
    add ax, bx
    call linear_sector_address_to_chs
    mov ah, 0x02
    mov al, 1
    mov dl, 0
    push bx ; Save the sector index for later
    shl bx, 9 ; Multiply by sector size (512)
    add bx, KERNEL_OFFSET_REAL_MODE
    int 0x13    
    pop bx ; Restore the sector index

    pop ax ; Restore the FAT entry
    cmp ax, 0xfff
    je enable_protected_mode
    inc bx
    
    jmp load_kernel
enable_protected_mode:
    cli

    call enable_a20

    lgdt [gdtr]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp KERNEL_CODE_SEGMENT:in_protected_mode
    
bits 32
in_protected_mode:
    mov ax, KERNEL_DATA_SEGMENT
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Clear out the page directory
    mov eax, 1024
    mov ebx, PAGE_DIR_PHYS_ADDR
clear_phys_table_loop:
    test eax, eax
    je fill_low_table
    dec eax
    mov dword [ebx], 0
    add ebx, 4
    jmp clear_phys_table_loop

    ; Fill the low table responsible for identity mapping the low MiB
fill_low_table:
    mov eax, 0
fill_low_table_loop:
    mov ebx, eax
    shr ebx, 12
    and ebx, 0x3ff
    shl ebx, 2 ; Page table entries are 4 bytes
    add ebx, LOW_TABLE_PHYS_ADDR
    mov dword [ebx], eax
    or dword [ebx], 0x3 ; Set present and read/write bits
    add eax, 0x1000
    cmp eax, 0x100000
    jge fill_kernel_table
    jmp fill_low_table_loop

    ; Fill the kernel table (maps 0x10000 - 0x410000 to 0xC0000000 - 0xC0400000)
    ; @HACK: This assumes that the kernel code will only take up 4 MiB 
fill_kernel_table:
    mov eax, KERNEL_PHYS_ADDR
fill_kernel_table_loop:
    mov ebx, KERNEL_VIRT_ADDR
    add ebx, eax
    sub ebx, KERNEL_PHYS_ADDR
    shr ebx, 12
    and ebx, 0x3ff
    shl ebx, 2 ; Page table entries are 4 bytes
    add ebx, KERNEL_TABLE_PHYS_ADDR
    mov dword [ebx], eax
    or dword [ebx], 0x3 ; Set present and read/write bits
    add eax, 0x1000
    cmp eax, KERNEL_PHYS_ADDR + 0x400000
    jge put_directory_entries
    jmp fill_kernel_table_loop

put_directory_entries:
    ; Low table entry
    mov dword [PAGE_DIR_PHYS_ADDR], LOW_TABLE_PHYS_ADDR
    or dword [PAGE_DIR_PHYS_ADDR], 0x3 ; Set present and read/write bits
    ; Kernel table entry
    mov ebx, KERNEL_VIRT_ADDR
    shr ebx, 22
    shl ebx, 2 ; Page directory entries are 4 bytes
    add ebx, PAGE_DIR_PHYS_ADDR
    mov dword [ebx], KERNEL_TABLE_PHYS_ADDR
    or dword [ebx], 0x3
    ; Map the page directory to itself in the last entry
    mov ebx, 1023
    shl ebx, 2
    add ebx, PAGE_DIR_PHYS_ADDR
    mov dword [ebx], PAGE_DIR_PHYS_ADDR
    or dword [ebx], 0x3

    ; Enable paging
    mov eax, PAGE_DIR_PHYS_ADDR
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax    
    
    jmp KERNEL_VIRT_ADDR

bits 16
HELLO: db "Hello, world!", 0

; @Function
; Prints a string using BIOS teletype output
; Params:
;   si = String address
; Returns:
;   None
print_string_real_mode:
    mov al, [si]
    test al, al
    je .done
    inc si 
    mov ah, 0x0e
    mov bh, 0
    int 0x10
    jmp print_string_real_mode
.done:
    ret

; @Function
; Converts a linear sector address to a chs addressing scheme. The information is put in the registers where int 0x13, ah = 0x02 expects it.
; Params:
;   ax = Linear sector address
; Returns:
;   ch = Cylinder number
;   cl = Sector number
;   dh = Head number
linear_sector_address_to_chs:
    push bx

    ; Sector number = linear sector address % sectors per cylinder + 1
    xor dx, dx
    mov bx, SECTORS_PER_CYLINDER
    div bx
    mov cx, dx
    inc cx

    ; Head number = (linear sector address / sectors per cylinder) % head count
    xor dx, dx
    mov bx, HEAD_COUNT
    div bx
    push dx ; Save this result for later, since we clobber dx in the next division

    ; Cylinder number = ((linear sector address / sectors per cylinder) / head count) % cylinder count
    xor dx, dx
    mov bx, CYLINDER_COUNT
    div bx
    mov ch, dl
    ; Restore the clobbered head number
    pop dx
    mov dh, dl

    pop bx
    ret

; @Function
; Reads a FAT entry
; Params:
;   ax = FAT base address
;   bx = entry index
read_fat:
    push ax
    mov ax, bx
    pop bx
    push ax
    push cx
    push dx ; Saved since the div affects dx

    push ax ; Save the original entry index
    shr ax, 1
    mov cx, 3
    mul cx 
    add bx, ax

    pop cx ; Restore entry index
    test cx, 1
    je .even

    ; Odd case
    mov al, [bx + 1]
    mov ah, [bx + 2]
    shr ax, 4
    jmp .done
.even:
    mov al, [bx]
    mov ah, [bx + 1]
    and ax, 0xfff
.done:
    pop dx
    pop cx
    pop bx
    ret

; @IMPORTANT
; Do not move this down! Doing so will cause the times expression to be negative.
; We cannot change this expression, since this defines the format of the boot sector.
times 510 - ($ - $$) db 0
dw 0xaa55

; @Function
; Enables the A20 line if it is disabled
; Params:
;   None
; Returns:
;   None
enable_a20:
    ; Check if the A20 line is enabled.
    ; This is done by comparing the boot signature at address 0x0000:0x7dfe (0x7dfe linear addressing) with the address 0xffff:0x7e0e (0x107cfe linear addressing).
    ; If the addresses are different, then A20 has been enabled. Otherwise, change the boot signature and check again. If the addresses are different after the change,
    ; then the A20 line is enabled. Otherwise, it is disabled
    push ax
    push bx
    push es

    mov bx, [ds:0x7dfe]
    mov ax, 0xffff
    mov es, ax
    mov ax, [es:0x7e0e]
    cmp ax, bx
    jne .done
    shl bx, 4    
    mov [ds:0x7dfe], bx
    mov ax, [es:0x7e0e]
    cmp ax, bx
    jne .done

    ; If we get here, then the A20 line is disabled. We need to enable it.
    ; Code copied from https://wiki.osdev.org/A20_Line#Keyboard_Controller_2
    call    a20wait
    mov     al,0xAD
    out     0x64,al
    call    a20wait
    mov     al,0xD0
    out     0x64,al
    call    a20wait2
    in      al,0x60
    push    eax
    call    a20wait
    mov     al,0xD1
    out     0x64,al
    call    a20wait
    pop     eax
    or      al,2
    out     0x60,al
    call    a20wait
    mov     al,0xAE
    out     0x64,al
    call    a20wait
.done:
    pop es
    pop bx
    pop ax
    ret

; @Helper for a20 enable
a20wait:
        in      al,0x64
        test    al,2
        jnz     a20wait
        ret

; @Helper for a20 enable
a20wait2:
        in      al,0x64
        test    al,1
        jz      a20wait2
        ret

gdt:
dq 0 ; GDT null entry

gdt_kernel_code:
; Kernel code segment
; Base = 0, Limit = 0xfffff,
; Present = 1, Privelege level = 0, Type = 1(code/data), Executable = 1, Conforming = 1, Readable = 1, Accessed = 0
; Granularity = 1(4KiB block granularity), Size = 1(32-bit segment), Long mode = 0
dw 0xffff
dw 0x0000
db 0x00
db 0b10011110
db 0b11001111
db 0x00

gdt_kernel_data:
; Kernel data segment
; Base = 0, Limit = 0xfffff,
; Present = 1, Privelege level = 0, Type = 1(code/data), Executable = 0, Direction = 0, Writeable = 1, Accessed = 0
; Granularity = 1(4KiB block granularity), Size = 1(32-bit segment), Long mode = 0
dw 0xffff
dw 0x0000
db 0x00
db 0b10010010
db 0b11001111
db 0x00

gdt_user_code:
; User code segment
; Base = 0, Limit = 0xfffff,
; Present = 1, Privelege level = 3, Type = 1(code/data), Executable = 1, Conforming = 1, Readable = 1, Accessed = 0
; Granularity = 1(4KiB block granularity), Size = 1(32-bit segment), Long mode = 0
dw 0xffff
dw 0x0000
db 0x00
db 0b11110110
db 0b11001111
db 0x00

gdt_user_data:
; User data segment
; Base = 0, Limit = 0xfffff,
; Present = 1, Privelege level = 3, Type = 1(code/data), Executable = 0, Direction = 0, Writeable = 1, Accessed = 0
; Granularity = 1(4KiB block granularity), Size = 1(32-bit segment), Long mode = 0
dw 0xffff
dw 0x0000
db 0x00
db 0b11110010
db 0b11001111
db 0x00
gdt_end:

gdtr:
dw gdt_end - gdt - 1
dd gdt

KERNEL_CODE_SEGMENT equ gdt_kernel_code - gdt
KERNEL_DATA_SEGMENT equ gdt_kernel_data - gdt

FAT: times 512 db 0

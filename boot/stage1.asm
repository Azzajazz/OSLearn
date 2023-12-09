bits 16

global start

%include "constants.inc"

%macro x86_set_video_mode 1
    mov ah, 0x00
    mov al, %1
    int 0x10
%endmacro

segment .text class=CODE
jmp start
;The jmp instruction above generates two bytes 0xeb 0x?? while the FAT12 file system specifies this must be 3 bytes ending in 0x90
db 0x90

;Allocate 59 bytes for the FAT12 BPB
times 59 db 0

%include "input_output.asm"

start:
    ;Disable interrupts
    cli
    ;Clear direction flag
    cld

    ;Set segment registers
    xor ax, ax
    mov ax, ds
    mov ax, es
    mov ax, ss

    ;Set stack registers
    mov sp, STACK_TOP
    mov bp, STACK_TOP

    ;Set graphics mode
    x86_set_video_mode 0x02

    mov ax, starting_message
    call puts_real_mode

load_gdt_and_fat_from_disk:
    x86_load_sectors_from_disk 1, 1, GDT_ADDR, 0
    x86_load_sectors_from_disk ROOT_DIR_SECTOR, 14, ROOT_DIR_ADDR, 0
    x86_load_sectors_from_disk FAT1_SECTOR, 9, FAT1_ADDR, 0
    x86_load_sectors_from_disk FAT2_SECTOR, 9, FAT2_ADDR, 0

    ;Search through the root directory for the right name
    mov ax, stage2_load_message
    call puts_real_mode

    mov ax, stage2_name
    mov bx, STAGE2_ADDR
    xor cx, cx
    call read_fat_file

stage2_loaded:
    mov ax, success_message
    call puts_real_mode

    mov ax, STAGE2_ADDR
    jmp ax

hang:
    hlt
    jmp hang

    
stage2_name: db "stage2  bin"
starting_message: db "Stage 1 is starting...", CR, LF, 0
stage2_load_message: db "Loading stage2.bin on disk...  ", 0
stage2_error_message: db "stage2.bin could not be loaded!", CR, LF, 0
success_message: db "Success!", CR, LF, 0
error_message: db "Failure!", CR, LF, 0


pad:
times (510 - ($ - $$)) db 0
dw 0xaa55

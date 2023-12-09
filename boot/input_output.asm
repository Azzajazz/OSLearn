%ifndef input_output_asm_
%define input_output_asm_

%macro x86_teletype_print 2
    mov ah, 0x0E
    mov al, %1
    mov bh, %2
    int 0x10
%endmacro

%macro x86_load_sectors_from_disk 4
    mov ax, %1
    call logical_address_to_chs
    mov ah, 0x02
    mov al, %2
    mov dl, %4
    mov bx, %3
    int 0x13
%endmacro

;%Function
;Writes a null-terminated string to the screen using BIOS teletype (real mode compatible only)
;Parameters:
;  ax: string address
;Returns:
;  None
puts_real_mode:
    bits 16

    push si
    mov si, ax
loop:
    mov al, byte [ds:si]
    inc si
    test al, al
    je .done
    x86_teletype_print al, 0
    jmp loop
.done:
    pop si
    ret

;%Function
;Reads an entire file from the FAT12 system
;Parameters:
;  ax: file name address
;  bx: memory base address
;  cx: memory segment
;Returns:
;  None
read_fat_file:
    bits 16
    push si
    push di

    push cx
    mov dx, ROOT_DIR_ADDR
search_for_file:
    mov si, dx
    mov di, ax
    mov cx, 11
    repe cmpsb
    je read_file
    add dx, 32
    jmp search_for_file
read_file:
    ;Read the file with the given name.
    ;Clusters are read until an 0xFFF entry is found in the FAT table
    mov si, bx
    mov bx, dx
    mov bx, [ds:bx + DIR_FIRST_CLUSTER_LO_OFFSET]
    pop cx
next_sector:
    mov ax, bx
    add ax, DATA_SECTOR
    push es
    mov es, cx
    push bx
    push cx
    x86_load_sectors_from_disk ax, 1, si, 0
    pop cx
    pop bx
    pop es
    mov ax, FAT1_ADDR
    push bx
    push cx
    call read_fat_cluster
    pop cx
    pop bx
    cmp ax, 0xfff
    je done
    add si, 512
    mov bx, ax
    jmp next_sector

done:
    pop di
    pop si
    ret
 
;%Function
;Converts a logical memory address to the chs addressing scheme.
;The return registers are unusual, but are designed to mirror the locations of the values used by BIOS int13 ah=0x02
;Params:
;  ax: logical address
;Returns:
;  ch: low 8 bits of cylinder number
;  cl: sector number (bits 0-5), high 2 bits of cylinder number (bits 6-7)
;  dh: head number
logical_address_to_chs:
    bits 16

    ;Get the sectors per track
    mov bx, word [BPB_ADDR + BPB_SECTORS_PER_TRACK_OFFSET]

    ;Calculate the sector number = <logical address> % <sectors per track> + 1
    xor dx, dx
    div bx
    mov cl, dl ;The remainder is at most 17 for this disk geometry and so fits in dl.
    inc cl
    
    ;Calculate the head number = <sector index> % <head count>
    mov bx, HEAD_COUNT
    xor dx, dx
    div bx
    mov dh, dl

    ;The remaining quotient is the cylinder number
    mov ch, al
    shr ax, 2
    and al, 11000000b
    or cl, al
    ret

;%Function
;Reads the cluster entry of the FAT table at the given address.
;Params:
;  ax: FAT table address
;  bx: FAT entry index
;Returns:
;  ax: Value of FAT entry index
read_fat_cluster:
    bits 16

    push si

    mov si, bx
    mov cx, si
    and cx, 1
    shr si, 1
    mov dx, si
    shl si, 1
    add si, dx
    add si, cx
    add si, 3
    add si, ax
    mov al, [ds:si]
    mov ah, [ds:si + 1]    
    test cx, cx
    je even_index
    shr ax, 4
    jmp cleanup 
even_index:
    and ax, 0xfff
cleanup:
    pop si
    ret

%endif

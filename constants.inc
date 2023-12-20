%ifndef constants_def_
%define constants_def_

;Offsets and sectors for boot stages and kernel
STAGE2_ADDR equ 0xF00
KERNEL_SECTOR equ 0xFFFF
KERNEL_OFFSET equ 0x10
KERNEL_ADDR equ 0x100000
KERNEL_VIRT_ADDR equ 0xC0000000

;Address for information to give to the kernel
BOOT_INFO_ADDR equ 0x1F00

;GDT defines
GDT_SIZE equ 0x100
GDT_ADDR equ 0x500

;IDT defines
IDT_ADDR equ 0x600
IDT_SIZE equ 0x800

;Paging directory defines
PAGE_DIR_ADDR equ 0x11000
PAGE_DIR_ENTRY_COUNT equ 0x1000
PAGE_TABLES_ADDR equ 0x10100000
PAGE_TABLES_VIRT_ADDR equ 0xD0000000
KERNEL_DATA_VIRT_ADDR equ 0xD0400000 

;Disk geometry
HEAD_COUNT equ 2

;ASCII encodings
CR equ 0x0D
LF equ 0x0A

;Memory map defines
STACK_TOP equ 0x7C00
FAT1_ADDR equ 0x7E00
FAT2_ADDR equ 0x9000
ROOT_DIR_ADDR equ 0xA200

;Disk sector offsets
RESERVED_SECTOR_COUNT equ 2
FAT1_SECTOR equ RESERVED_SECTOR_COUNT
FAT2_SECTOR equ (FAT1_SECTOR + 9)
ROOT_DIR_SECTOR equ (FAT2_SECTOR + 9)
DATA_SECTOR equ (ROOT_DIR_SECTOR + 14)

;BPB constants 
BPB_ADDR equ 0x7C03
BPB_SECTORS_PER_TRACK_OFFSET equ 21

;Directory entry offsets
DIR_NAME_OFFSET equ 0
DIR_FIRST_CLUSTER_LO_OFFSET equ 26

%endif

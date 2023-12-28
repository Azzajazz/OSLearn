#include "types.h"
#include "x86.h"

#define global static

#include "string.c"
#include "print.c"
#include "keyboard.c"

// Memory allocator
typedef uint32 phys_addr;

typedef struct {
    void* base;
    uint32 size;
} memory_arena;

typedef struct {
    memory_arena arena;
    void* nextFree;
} bump_allocator;

/*
 * Calculate the size of the bitmap.
 * The number is somewhat unusual since the bootloader has already claimed some physical memory, namely:
 * - The page starting at 0x10100000. This is where the page table for identity mapping the first 0x100000 bytes is located.
 * - The pages from 0x10101000 - 0x10201000. This is where the page tables mapping 0x100000 - 0x10100000 to 0xC0000000 - 0xD0000000 are located.
 */
#define KERNEL_DATA_PHYS 0x10201000
#define KERNEL_DATA_VIRT 0xD0000000
#define KiB(n) (1024 * (n))
#define MiB(n) (1024 * KiB(n))
#define GiB(n) (1024 * (uint64)MiB(n))
#define ADDRESS_SPACE GiB(4)
#define PAGE_SIZE KiB(4)

#define CEIL_DIV(x, y) (((x) + (y) - 1) / (y))

#define PAGE_BITMAP_SIZE ((ADDRESS_SPACE - KERNEL_DATA_PHYS) / PAGE_SIZE) / 32
global uint32 pageBitmap[PAGE_BITMAP_SIZE];
global memory_arena pageTableMemory;
global bump_allocator kernelVirtualMemory;

/*
 * Initialisation for global allocators and memory arenas.
 * - 0xD0000000 - 0xD0100000: Page table memory
 * - 0xD0100000 - 0xFFFFFFFF: Kernel memory
 */
void memoryInit() {
    pageTableMemory.base = (void*)KERNEL_DATA_VIRT;
    pageTableMemory.size = 0x100000;

    kernelVirtualMemory.arena.base = (void*)0xD0100000;
    kernelVirtualMemory.arena.size = 0xFFFFFFFF - 0xD0100000 + 1;
    kernelVirtualMemory.nextFree = (void*)0xD0100000;
}

phys_addr allocatePage() {
    uint32 chunkIndex = 0;
    while (pageBitmap[chunkIndex] == 0xFFFFFFFF) {
        chunkIndex++;
    };
    uint32 bitIndexInChunk = 0;
    while (pageBitmap[chunkIndex] & (1 << bitIndexInChunk)) {
        bitIndexInChunk++;
    }
    pageBitmap[chunkIndex] |= (1 << bitIndexInChunk);
    uint32 pageIndex = chunkIndex * 32 + bitIndexInChunk;
    return KERNEL_DATA_PHYS + pageIndex * PAGE_SIZE;
}

void freePage(phys_addr address) {
    uint32 pageIndex = (address - KERNEL_DATA_PHYS) / PAGE_SIZE;
    uint32 chunkIndex = pageIndex / 32;
    uint32 bitIndexInChunk = pageIndex % 32; 
    pageBitmap[chunkIndex] &= ~(1 << bitIndexInChunk);
}

// File API
typedef uint32 file_handle;

uint32 getFileSize(file_handle file) {
    (void)file;
    return 0;
}

char* readEntireFile(file_handle file) {
    (void)file;
    return "";
}
//

extern void testHandler(void);
extern void pitHandler(void);
extern void keyboardHandler(void);
typedef void (*interruptHandler)(void);

typedef struct __attribute__((packed)) {
    uint16 offsetLow;
    uint16 segmentSelector;
    uint8 reserved;
    uint8 attributes;
    uint16 offsetHigh;
} idt_gate_descriptor;

typedef struct {
    unsigned present : 1;
    unsigned readWrite : 1;
    unsigned userSupervisor : 1;
    unsigned writeThrough : 1;
    unsigned cacheDisable : 1;
    unsigned accessed : 1;
    unsigned dirty : 1;
    unsigned attributeTable : 1;
    unsigned globalPage : 1;
    unsigned available : 3;
    unsigned pageAddress : 20;
} page_table_entry;

typedef struct {
    unsigned present : 1;
    unsigned readWrite : 1;
    unsigned userSupervisor : 1;
    unsigned writeThrough : 1;
    unsigned cacheDisable : 1;
    unsigned accessed : 1;
    unsigned availableLow : 1;
    unsigned pageSize : 1;
    unsigned availableHigh : 4;
    unsigned pageTableAddress : 20;
} page_directory_entry;

typedef page_table_entry page_table[1024];

typedef struct {
    idt_gate_descriptor* idtAddress;
    page_directory_entry* pageDirectoryAddress;
    page_table* pageTablesAddress;
    uint8* kernelDataAddress;
} boot_info;

void setInterrupt(boot_info* info, uint8 idtIndex, interruptHandler handler) {
    uint32 offset = (uint32)handler;
    info->idtAddress[idtIndex].offsetLow = (uint16)offset;
    info->idtAddress[idtIndex].offsetHigh = (uint16)(offset >> 16);
}

void printKeyPress(key_event event) {
    if (event.code == ' ') {
        printChar(' ');
    }
    else if (event.metaMask & META_SHIFT) {
        if (event.code >= 'a' && event.code <= 'z') {
            printChar((char)event.code - 32);
        }
        else {
            switch (event.code) {
                case '1': {
                    printChar('!');
                } break;
                case '2': {
                    printChar('@');
                } break;
                case '3': {
                    printChar('#');
                } break;
                case '4': {
                    printChar('$');
                } break;
                case '5': {
                    printChar('%');
                } break;
                case '6': {
                    printChar('^');
                } break;
                case '7': {
                    printChar('&');
                } break;
                case '8': {
                    printChar('*');
                } break;
                case '9': {
                    printChar('(');
                } break;
                case '0': {
                    printChar(')');
                } break;
                case '-': {
                    printChar('_');
                } break;
                case '=': {
                    printChar('+');
                } break;
                case '\\': {
                    printChar('|');
                } break;
                case '[': {
                    printChar('{');
                } break;
                case ']': {
                    printChar('}');
                } break;
                case ';': {
                    printChar(':');
                } break;
                case '\'': {
                    printChar('"');
                } break;
                case ',': {
                    printChar('<');
                } break;
                case '.': {
                    printChar('>');
                } break;
                case '/': {
                    printChar('?');
                } break;
                default: break;
            }
        }
    }
    else {
        printChar((char)event.code);
    }
}

void getLineSized(char* buffer, uint32 size, bool32 printStrokes) {
    uint32 charCount = 0;
    while (charCount < size) {
        OPTIONAL(key_event) event = queryKeyEvent();
        while (!event.exists) {
            event = queryKeyEvent();
        }
        if (event.inner.pressed && !isMeta(event.inner.code)) {
            if (event.inner.code == KEY_CODE_ENTER) {
                break;
            }
            buffer[charCount++] = event.inner.code;
            if (printStrokes) {
                printKeyPress(event.inner);
            }
        }
    }
    if (charCount < size) {
        buffer[charCount] = '\0';
    }
    else {
        buffer[size - 1] = '\0';
    }
}

void kernelRepl() {
    char input[128]; //@TODO: We have no dynamic allocator yet, and maybe we don't want one (potential failure point)
    bool32 printStrokes = true;
    while (true) {
        printFmt("> ");
        getLineSized(input, 128, printStrokes);
        printChar('\n');
        OPTIONAL(string_split) split_result = stringSplitOn(input, ' ');
        if (split_result.exists) {
            string_view command = split_result.inner.first;
            if (svEqualsCString(command, "echo")) {
                printFmt("%sv\n", split_result.inner.second);
            }
        }
    }
}

void kernelMain(boot_info* info) {
    // vgaInit();
    memoryInit();
    vgaEnableCursor();
    vgaSetCursorLocation(0);

    printString("Now in protected mode!\n");
    printString("Executing kernel.\n");
    printString("Boot info:\n");
    printFmt("  IDT address:         %p\n", info->idtAddress);
    printFmt("  Page dir address:    %p\n", info->pageDirectoryAddress);
    printFmt("  Page tables address: %p\n", info->pageTablesAddress);
    printFmt("  Kernel data address: %p\n", info->kernelDataAddress);

    setInterrupt(info, 0x42, &testHandler);
    setInterrupt(info, 0x20, &pitHandler);
    setInterrupt(info, 0x21, &keyboardHandler);
    x86InterruptEnable();

    x86Interrupt(0x42);

//    phys_addr allocations[6];
//    for (uint32 allocationIndex = 0; allocationIndex < 6; ++allocationIndex) {
//        allocations[allocationIndex] = allocatePage();
//        printFmt("Allocating page %p\n", allocations[allocationIndex]);
//    }
//    for (uint32 freeIndex = 0; freeIndex < 6; freeIndex += 2) {
//        printFmt("Freeing page %p\n", allocations[freeIndex]);
//        freePage(allocations[freeIndex]);
//    }
//    for (uint32 allocationIndex = 0; allocationIndex < 6; ++allocationIndex) {
//        printFmt("Allocating page %p\n", allocatePage());
//    }

    kernelRepl();

    for(;;);
}

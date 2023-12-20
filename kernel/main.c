#include "inttypes.h"
#include "x86.h"

#define global static

#include "string.c"
#include "print.c"
#include "keyboard.c"

// Memory allocator
typedef uint32 phys_addr;

extern uint32 _kernel_finish;

#define KiB(n) (1024 * (n))
#define MiB(n) (1024 * KiB(n))
#define GiB(n) (1024 * (uint64)MiB(n))
#define ADDRESS_SPACE GiB(4)
#define PAGE_SIZE KiB(4)

#define PAGE_BITMAP_SIZE (ADDRESS_SPACE / PAGE_SIZE) / 32
global uint32 pageBitmap[PAGE_BITMAP_SIZE];

phys_addr alignPage(phys_addr pointer) {
    return PAGE_SIZE * ((pointer + PAGE_SIZE - 1) / PAGE_SIZE);
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
    return alignPage((phys_addr)&_kernel_finish) + pageIndex * PAGE_SIZE;
}

void freePage(phys_addr address) {
    uint32 pageIndex = (address - alignPage((phys_addr)&_kernel_finish)) / PAGE_SIZE;
    uint32 chunkIndex = pageIndex / 32;
    uint32 bitIndexInChunk = pageIndex % 32; 
    pageBitmap[chunkIndex] &= ~(1 << bitIndexInChunk);
}

//USAGE:
//uint32 physicalAddress = allocatePage();
//void* virtualAddress = mapPage(physicalAddress);

//


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
    idt_gate_descriptor* idtAddress;
    uint8* pageDirectoryAddress;
    uint8* pageTablesAddress;
    uint8* kernelDataAddress;
} boot_info;

void setInterrupt(boot_info* info, uint8 idtIndex, interruptHandler handler) {
    uint32 offset = (uint32)handler;
    info->idtAddress[idtIndex].offsetLow = (uint16)offset;
    info->idtAddress[idtIndex].offsetHigh = (uint16)(offset >> 16);
}

void kernelMain(boot_info* info) {
    printFmt("%p\r\n", alignPage((phys_addr)&_kernel_finish));

    stringPrint("Now in protected mode!\r\n");
    stringPrint("Executing kernel.\r\n");
    stringPrint("Boot info:\r\n");
    printFmt("  IDT address:         %p\r\n", info->idtAddress);
    printFmt("  Page dir address:    %p\r\n", info->pageDirectoryAddress);
    printFmt("  Page tables address: %p\r\n", info->pageTablesAddress);
    printFmt("  Kernel data address: %p\r\n", info->kernelDataAddress);

    setInterrupt(info, 0x42, &testHandler);
    setInterrupt(info, 0x20, &pitHandler);
    setInterrupt(info, 0x21, &keyboardHandler);

    x86Interrupt(0x42);

    phys_addr pages[10];
    for (uint32 allocationIndex = 0; allocationIndex < 10; ++allocationIndex) {
        pages[allocationIndex] = allocatePage();
        printFmt("Allocating physical memory: %x32\r\n", pages[allocationIndex]);
    }
    for (uint32 freeIndex = 0; freeIndex < 10; freeIndex += 2) {
        printFmt("Freeing physical memory: %x32\r\n", pages[freeIndex]);
        freePage(pages[freeIndex]);
    }
    for (uint32 allocationIndex = 0; allocationIndex < 5; ++allocationIndex) {
        phys_addr address = allocatePage();
        printFmt("Allocating physical memory: %x32\r\n", address);
    }

    for(;;) {
        key_query_result queryResult = queryKeyEvent();
        if (queryResult.exists && queryResult.event.pressed && queryResult.event.code >= 'a' && queryResult.event.code <= 'z') {
            if (queryResult.event.metaMask & META_SHIFT) {
                printChar(queryResult.event.code - 32);
            }
            else {
                printChar(queryResult.event.code);
            }
        }
    }
}

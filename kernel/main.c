#include "inttypes.h"
#include "x86.h"

#define global static

#include "string.c"
#include "print.c"
#include "keyboard.c"

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

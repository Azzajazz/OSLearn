#ifndef X86_H_
#define X86_H_

#include "types.h"

#define x86Interrupt(code) asm volatile ("int $"#code)

/* Interrupt enabling and disabling */
void __attribute__((fastcall)) x86InterruptEnable(void);
void __attribute__((fastcall)) x86InterruptDisable(void);

/* Port IO functions */
void __attribute__((fastcall)) x86Out8(uint16 port, uint8 message);
void __attribute__((fastcall)) x86Out16(uint16 port, uint16 message);
void __attribute__((fastcall)) x86Out32(uint16 port, uint32 message);

uint8 __attribute__((fastcall)) x86In8(uint16 port);
uint16 __attribute__((fastcall)) x86In16(uint16 port);
uint32 __attribute__((fastcall)) x86In32(uint16 port);

void __attribute__((fastcall)) x86IoWait(void);

/* PS/2 controller functions */
void ps2WaitForReadable(void);
void ps2WaitForWriteable(void);
void ps2SendEcho(void);

/* VGA stuff */
// Register indexes
#define CRTC_MAX_SCAN_LINE 0x09
#define CRTC_CURSOR_START 0x0A
#define CRTC_CURSOR_END 0x0B
#define CRTC_CURSOR_LOC_HIGH 0x0E
#define CRTC_CURSOR_LOC_LOW 0x0F 

// Register contents
#define CURSOR_DISABLE 0x20

void vgaInit();
uint8 vgaReadMor();
void vgaWriteMor(uint8 data, uint8 mask);
void vgaWriteCrtRegister(uint8 address, uint8 data, uint8 mask);
uint8 vgaReadCrtRegister(uint8 address);
void vgaSetCursorLocation(uint16 location);
uint16 vgaGetCursorLocation();
void vgaEnableCursor();
void vgaDisableCursor();
void vgaClearScreen();
void vgaSetWriteLocation(uint32 location);
uint32 vgaGetWriteLocation();
uint32 vgaGetBufferWidth();
uint32 vgaGetBufferHeight();
void vgaScrollScreen();
void vgaWriteChar(char c);
void vgaCopyBuffer(uint8* buffer, uint32 length);

#endif

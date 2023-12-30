#include "x86.h"
#include "types.h"

#define global static

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64

void ps2WaitForReadable() {
    uint8 status;
    do {
        status = x86In8(PS2_COMMAND);
    } while ((status & 0x1) == 0);
}

void ps2WaitForWriteable() {
    uint8 status;
    do {
        status = x86In8(PS2_COMMAND);
    } while ((status & 0x2) != 0);
}

void ps2SendEcho() {
    ps2WaitForWriteable();
    x86Out8(PS2_COMMAND, 0xEE);
}

// Miscellaneous Output Register
#define VGA_MOR_READ 0x3CC
#define VGA_MOR_WRITE 0x3C2
#define MOR_IO_ADDR_SELECT 0x01

// CRTC ports
#define VGA_CRTC_ADDRESS_PORT 0x3D4
#define VGA_CRTC_DATA_PORT 0x3D5

#define VGA_BUFFER_WIDTH 80
#define VGA_BUFFER_HEIGHT 25
global volatile uint16* vgaBuffer = (uint16*)0xB8000;
global uint32 vgaIndex;

uint8 vgaReadMor() {
    return x86In8(VGA_MOR_READ);
}

void vgaWriteMor(uint8 data, uint8 mask) {
    uint8 oldData = vgaReadMor();
    uint8 newData = (data & mask) | (oldData & ~mask);
    x86Out8(VGA_MOR_WRITE, newData);
    x86IoWait();
}

void vgaWriteCrtRegister(uint8 address, uint8 data, uint8 mask) {
    uint8 oldAddress = x86In8(VGA_CRTC_ADDRESS_PORT);
    x86Out8(VGA_CRTC_ADDRESS_PORT, address);
    x86IoWait();
    uint8 oldData = x86In8(VGA_CRTC_DATA_PORT);
    uint8 newData = (data & mask) | (oldData & ~mask);
    x86Out8(VGA_CRTC_DATA_PORT, newData);
    x86IoWait();
    x86Out8(VGA_CRTC_ADDRESS_PORT, oldAddress);
    x86IoWait();
}

uint8 vgaReadCrtRegister(uint8 address) {
    uint8 oldAddress = x86In8(VGA_CRTC_ADDRESS_PORT);
    x86Out8(VGA_CRTC_ADDRESS_PORT, address);
    x86IoWait();
    uint8 data = x86In8(VGA_CRTC_DATA_PORT);
    x86Out8(VGA_CRTC_ADDRESS_PORT, oldAddress);
    x86IoWait();
    return data;
}

void vgaEnableCursor() {
    vgaWriteCrtRegister(CRTC_CURSOR_START, 0, CURSOR_DISABLE);
    uint8 maximumScanLine = vgaReadCrtRegister(CRTC_MAX_SCAN_LINE) & 0x1F;
    vgaWriteCrtRegister(CRTC_CURSOR_END, maximumScanLine, 0x1F);
}

void vgaDisableCursor() {
    vgaWriteCrtRegister(CRTC_CURSOR_START, CURSOR_DISABLE, CURSOR_DISABLE);
}

void vgaSetCursorLocation(uint16 location) {
    uint8 locationHigh = (uint8)(location >> 8);
    uint8 locationLow = (uint8)location;
    vgaWriteCrtRegister(CRTC_CURSOR_LOC_HIGH, locationHigh, 0xFF);
    x86IoWait();
    vgaWriteCrtRegister(CRTC_CURSOR_LOC_LOW, locationLow, 0xFF);
    x86IoWait();
}

uint16 vgaGetCursorLocation() {
    uint8 locationHigh = vgaReadCrtRegister(CRTC_CURSOR_LOC_HIGH);
    uint8 locationLow = vgaReadCrtRegister(CRTC_CURSOR_LOC_LOW);
    return (locationHigh << 8) | locationLow;
}

void vgaClearScreen() {
    for (uint32 bufferIndex = 0; bufferIndex < VGA_BUFFER_WIDTH * VGA_BUFFER_HEIGHT; ++bufferIndex) {
        vgaBuffer[bufferIndex] &= ~0xFF;
    }
    vgaIndex = 0;
}

void vgaSetWriteLocation(uint32 location) {
    vgaIndex = location;
}

uint32 vgaGetWriteLocation() {
    return vgaIndex;
}

void vgaScrollScreen() {
    // Scroll the screen
    for (uint32 scrollIndex = VGA_BUFFER_WIDTH; scrollIndex < VGA_BUFFER_WIDTH * VGA_BUFFER_HEIGHT; ++scrollIndex) {
        vgaBuffer[scrollIndex - VGA_BUFFER_WIDTH] = vgaBuffer[scrollIndex];
    }
    // Clear the last line
    for (uint32 clearIndex = VGA_BUFFER_WIDTH * (VGA_BUFFER_HEIGHT - 1); clearIndex < VGA_BUFFER_WIDTH * VGA_BUFFER_HEIGHT; ++clearIndex) {
        vgaBuffer[clearIndex] = 0;
    }
}

uint32 vgaGetBufferWidth() {
    return VGA_BUFFER_WIDTH;
}

uint32 vgaGetBufferHeight() {
    return VGA_BUFFER_HEIGHT;
}

void vgaWriteChar(char c) {
    vgaBuffer[vgaIndex] = (0xf << 8) | c; 
}

void vgaCopyBuffer(uint8* buffer, uint32 length) {
    for (uint32 bufferIndex = 0; bufferIndex < length; ++bufferIndex) {
        vgaBuffer[vgaIndex + bufferIndex] = (0xf << 8) | buffer[bufferIndex];
    }
}

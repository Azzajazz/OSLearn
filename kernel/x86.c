#include "x86.h"
#include "types.h"

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

void vgaInit() {
    uint8 morValue = x86In8(VGA_MOR_READ);
    morValue |= 1;
    x86Out8(VGA_MOR_WRITE, morValue);
    x86IoWait();
    vgaWriteCrtRegister(CRTC_CURSOR_START, 0, 0x3F);
    x86IoWait();
    uint8 maximumScanLine = vgaReadCrtRegister(CRTC_MAX_SCAN_LINE) & 0x1F;
    vgaWriteCrtRegister(CRTC_CURSOR_END, maximumScanLine, 0x1F);
    x86IoWait();
}

void vgaWriteCrtRegister(uint8 address, uint8 data, uint8 mask) {
    uint8 oldAddress = x86In8(VGA_CRTC_ADDRESS_PORT);
    x86Out8(VGA_CRTC_ADDRESS_PORT, address);
    x86IoWait();
    uint8 oldData = x86In8(VGA_CRTC_DATA_PORT);
    x86Out8(VGA_CRTC_DATA_PORT, data & (oldData | mask));
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

void vgaSetCursorLocation(uint16 location) {
    uint8 locationHigh = location >> 8;
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

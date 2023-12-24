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

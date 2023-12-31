#ifndef X86_H_
#define X86_H_

#include "inttypes.h"

// Port IO functions
extern void __attribute__((fastcall)) x86Out8(uint8 port, uint8 message);
extern void __attribute__((fastcall)) x86Out16(uint8 port, uint16 message);
extern void __attribute__((fastcall)) x86Out32(uint8 port, uint32 message);

extern uint8 __attribute__((fastcall)) x86In8(uint8 port);
extern uint16 __attribute__((fastcall)) x86In16(uint8 port);
extern uint32 __attribute__((fastcall)) x86In32(uint8 port);

extern void __attribute__((fastcall)) x86IoWait(void);

// PS/2 controller functions
extern void __attribute__((fastcall)) ps2WaitForReadable(void);
extern void __attribute__((fastcall)) ps2WaitForWriteable(void);
extern void __attribute__((fastcall)) ps2SendEcho(void);

#endif

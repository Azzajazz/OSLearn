#include <stdarg.h>

#include "inttypes.h"
#include "string.h"

#define global static

global volatile uint16* VgaBuffer = (uint16*)0xB8000;
global uint32 VgaIndex;
global uint32 VgaBufferHeight = 25;
global uint32 VgaBufferWidth = 80;

void printChar(char c) {
    if (c == '\r') {
        VgaIndex -= VgaIndex % VgaBufferWidth;
    }
    else if (c == '\n') {
        VgaIndex += VgaBufferWidth;
    }
    else {
        VgaBuffer[VgaIndex++] = (0xF << 8) | c;
    }
}

void svPrint(string_view sv) {
    uint16 charIndex;
    for (charIndex = 0; charIndex < sv.size; ++charIndex) {
        printChar(sv.data[charIndex]);
    }
}

void stringPrint(char* cstr) {
    for (; *cstr; cstr++) {
        printChar(*cstr);
    }
}

void uint64Print(uint64 integer) {
    char digits[19]; //int16 has at most 19 digits
    uint16 digitCount = 0;
    if (integer == 0) {
        printChar('0');
        return;
    }
    while (integer > 0) {
        digits[digitCount++] = '0' + (uint8)integer % 10;
        integer = integer / 10;
    }
    for (uint16 digitIndex = digitCount; digitIndex > 0; --digitIndex) {
        printChar(digits[digitIndex - 1]);
    }
}

void int64Print(int64 integer) {
    if (integer < 0) {
        printChar('-');
        integer = -integer;
    }
    uint64Print((uint64)integer);
}

//@TODO: Fold uintxxPrintHex functions together, maybe using a macro
void uint32PrintHex(uint32 integer) {
    printChar('0');
    printChar('x');
    char digits[8]; //uint32 has 8 digits when printed in hex
    for (uint32 digitIndex = 0; digitIndex < 8; ++digitIndex) {
        uint8 digit = integer & 0xF;
        if (digit < 10) {
            digits[digitIndex] = digit + '0';
        }
        else {
            digits[digitIndex] = digit - 10 + 'A';
        }
        integer >>= 4;
    }
    for (uint32 digitIndex = 8; digitIndex > 0; --digitIndex) {
        printChar(digits[digitIndex - 1]);
    }
}

void uint8PrintHex(uint8 integer) {
    printChar('0');
    printChar('x');
    char digits[2]; //uint32 has 8 digits when printed in hex
    for (uint32 digitIndex = 0; digitIndex < 2; ++digitIndex) {
        uint8 digit = integer & 0xF;
        if (digit < 10) {
            digits[digitIndex] = digit + '0';
        }
        else {
            digits[digitIndex] = digit - 10 + 'A';
        }
        integer >>= 4;
    }
    for (uint32 digitIndex = 2; digitIndex > 0; --digitIndex) {
        printChar(digits[digitIndex - 1]);
    }
}

//@TODO: Maybe this should be extended to include maximum width, etc? Right now, this is not needed.
void printFmt(char* format, ...) {
    va_list args;
    va_start(args, format);
    while (*format) {
        if (*format == '%') {
            format++;
            switch (*format) {
                case '%': {
                    printChar('%');
                } break;
                case 's': {
                    if (format[1] == 'v') {
                        format++;
                        svPrint(va_arg(args, string_view));
                    }
                    else {
                        stringPrint(va_arg(args, char*));
                    }
                } break;
                case 'i': {
                    if (format[1] == '8') {
                        format++;
                        int64Print((int64)va_arg(args, int32)); // va_args cannot be less than 32 bits (in protected mode)
                    }
                    else if (format[1] == '1' && format[2] == '6') {
                        format += 2;
                        int64Print((int64)va_arg(args, int32));
                    }
                    else if (format[1] == '3' && format[2] == '2') {
                        format += 2;
                        int64Print((int64)va_arg(args, int32));
                    }
                    else if (format[1] == '6' && format[2] == '4') {
                        format += 2;
                        int64Print((int64)va_arg(args, int64));
                    }
                } break;
                case 'u': {
                    if (format[1] == '8') {
                        format++;
                        uint64Print((uint64)va_arg(args, uint32));
                    }
                    else if (format[1] == '1' && format[2] == '6') {
                        format += 2;
                        uint64Print((uint64)va_arg(args, uint32));
                    }
                    else if (format[1] == '3' && format[2] == '2') {
                        format += 2;
                        uint64Print((uint64)va_arg(args, uint32));
                    }
                    else if (format[1] == '6' && format[2] == '4') {
                        format += 2;
                        uint64Print((uint64)va_arg(args, uint64));
                    }
                } break;
                case 'x': {
                    //@TODO: uint16 and uint64 hex printing
                    if (format[1] == '8') {
                        format++;
                        uint8PrintHex((uint64)va_arg(args, uint32));
                    }
                    else if (format[1] == '3' && format[2] == '2') {
                        format += 2;
                        uint32PrintHex((uint64)va_arg(args, uint32));
                    }
                } break;
                case 'c': {
                    printChar(va_arg(args, int32));
                } break;
                case 'p': {
                    uint32PrintHex((uint32)va_arg(args, void*));
                } break;
            }
        }
        else {
            printChar(*format);
        }
        format++;
    }
    va_end(args);
}

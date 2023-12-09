#ifndef PRINT_H_
#define PRINT_H_

#include "inttypes.h"
#include "string.h"

void printChar(char c);
void svPrint(string_view sv);
void stringPrint(char* cstr);
void uint64Print(uint64 integer);
void int64Print(int64 integer);
void printFmt(char* format, ...);

#endif

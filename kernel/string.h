#ifndef STRING_H_
#define STRING_H_

#include "types.h"

typedef struct {
    char* data;
    uint16 length;
} string_view;

typedef struct {
    string_view first;
    string_view second;
} string_split;

OPTIONAL_DEF(string_split);

#endif

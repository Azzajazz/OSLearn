#ifndef STRING_H_
#define STRING_H_

#include "inttypes.h"

typedef struct {
    char* data;
    uint16 size;
} string_view;

#endif

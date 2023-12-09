#ifndef STRINGVIEW_H_
#define STRINGVIEW_H_

#include "inttypes.h"

typedef struct {
    char* data;
    uint16 size;
} string_view;

string_view svCreate(char* cstr);

#endif

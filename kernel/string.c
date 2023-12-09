#include "string.h"

uint16 cstringLength(char* cstr) {
    uint16 result = 0;
    char* p;
    for (p = cstr; *p; ++p) {
        result++;
    }
    return result;
}

string_view svCreate(char* cstr) {
    string_view result;
    result.size = cstringLength(cstr);
    result.data = cstr;
    return result;
}

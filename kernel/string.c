#include "string.h"

uint16 cstringLength(char* cstr) {
    uint16 result = 0;
    char* p;
    for (p = cstr; *p; ++p) {
        result++;
    }
    return result;
}

string_view svFromCString(char* cstr) {
    string_view result;
    result.length = cstringLength(cstr);
    result.data = cstr;
    return result;
}

string_view svFromSize(char* start, uint32 length) {
    string_view result;
    result.length = length;
    result.data = start;
    return result;
}

bool32 svEqualsCString(string_view sv, char* cstr) {
    for (uint32 charIndex = 0; charIndex < sv.length; ++charIndex) {
        if (sv.data[charIndex] != cstr[charIndex]) {
            return false;
        }
    }
    return cstr[sv.length] == '\0';
}

OPTIONAL(string_split) stringSplitOn(char* string, char delimiter) {
    OPTIONAL(string_split) result;
    uint32 firstLength = 0;
    while (string[firstLength] != delimiter && string[firstLength]) {
        firstLength++;
    }
    if (string[firstLength] == '\0') {
        result.exists = false;
    }
    else {
        result.inner.first = svFromSize(string, firstLength);
        result.inner.second = svFromCString(&string[firstLength + 1]);
    }
    return result;
}

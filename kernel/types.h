#ifndef TYPES_H_
#define TYPES_H_

#define uint8 unsigned char
#define uint16 unsigned short
#define uint32 unsigned long
#define uint64 unsigned long long

#define int8 char
#define int16 short
#define int32 long
#define int64 long long

#define bool32 int32
#define true 1
#define false 0

#define OPTIONAL_DEF(type) \
typedef struct {           \
    bool32 exists;         \
    type inner;            \
} optional_##type

#define OPTIONAL(type) optional_##type

#endif

#ifndef FATUTILS_H_
#define FATUTILS_H_

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    char oemName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t fatCount;
    uint16_t rootEntryCount;
    uint16_t totalSectorCount;
    uint8_t mediaIdentifier;
    uint16_t fatSectorCount;
    uint16_t sectorsPerTrack;
    uint16_t headCount;
    uint32_t hiddenSectorCount;
    uint32_t fat32Reserved;
    uint8_t driveNumber;
    uint8_t reserved;
    uint8_t bootSignature;
    uint32_t volumeSerialNumber;
    char volumeLabel[11];
    char fileSystemType[8];
} bios_parameter_block;
#pragma pack(pop)

void fillWithSpacePaddedByteString(char* destination, char* source, size_t length) {
    size_t lengthToCopy = strlen(source);
    if (lengthToCopy > length) {
        lengthToCopy = length;
    }
    memcpy(destination, source, lengthToCopy);
    for (size_t spaceIndex = lengthToCopy; spaceIndex < length; ++spaceIndex) {
        destination[spaceIndex] = ' ';
    }
}

void fillWithTruncatedSpacePaddedByteString(char* destination, char* source, size_t destinationLength, size_t sourceLength) {
    if (sourceLength > destinationLength) {
        sourceLength = destinationLength;
    }
    memcpy(destination, source, sourceLength);
    for (size_t spaceIndex = sourceLength; spaceIndex < destinationLength; ++spaceIndex) {
        destination[spaceIndex] = ' ';
    }
}

void tryFSeek(FILE* stream, size_t offset, int origin, char* fileName) {
    if (fseek(stream, (long)offset, origin) != 0) {
        fprintf(stderr, "[ERROR]: Could not seek in file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }
}

long tryFTell(FILE* stream, char* fileName) {
    long result = ftell(stream);
    if (result == -1) {
        fprintf(stderr, "[ERROR]: Could not get cursor position in file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }
    return result;
}

void tryFWrite(void* buffer, size_t memberSize, size_t memberCount, FILE* stream, char* fileName) {
    if (fwrite(buffer, memberSize, memberCount, stream) != memberCount) {
        fprintf(stderr, "[ERROR]: Could not write to file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }
}

FILE* tryFOpen(char* fileName, char* mode) {
    FILE* result = fopen(fileName, mode);
    if (result == NULL) {
        fprintf(stderr, "[ERROR]: Could not open file %s: %s\n", fileName, strerror(errno));
        exit(1);
    }
    return result;
}

#endif

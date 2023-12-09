#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fatcommon.h"

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONGNAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

size_t getFileSize(FILE* stream, char* fileName) {
    long cursorPosition = tryFTell(stream, fileName); 
    tryFSeek(stream, 0, SEEK_END, fileName);
    size_t result = tryFTell(stream, fileName);
    tryFSeek(stream, cursorPosition, SEEK_SET, fileName);
    return result;
}

uint16_t readFatClusterEntry(uint8_t* fatTable, size_t clusterIndex) {
    // The two adjacent bytes are loaded as little-endian, so two entries of the form
    // 0x123 and 0x456 are actually loaded as 0x23 0x61 0x45 in the fatTable.
    size_t entryPairIndex = 3 * (clusterIndex / 2) + (clusterIndex % 2) + 3;
    uint16_t result = (fatTable[entryPairIndex + 1] << 8) | fatTable[entryPairIndex];
    if (clusterIndex % 2 == 0) {
        result &= 0xfff;
    }
    else {
        result >>= 4;
    }
    return result;
}

void populateFatClusterEntry(uint8_t* fatTable, size_t clusterIndex, uint16_t value) {
    // The two adjacent bytes are loaded as little-endian, so two entries of the form
    // 0x123 and 0x456 are actually loaded as 0x23 0x61 0x45 in the fatTable.
    size_t entryPairIndex = 3 * (clusterIndex / 2) + (clusterIndex % 2) + 3;
    uint16_t fatEntry = (fatTable[entryPairIndex + 1] << 8) | fatTable[entryPairIndex];
    if (clusterIndex % 2 == 0) {
        // From the example above, we can rebuild the fat entry as 0x6123 and set just the lower 12 bits.
        fatEntry = (fatEntry & 0xf000) | value;
    }
    else {
        // From the same example, we can rebuild the fat entry as 0x4561 and set just the upper 12 bits.
        fatEntry = fatEntry & 0xf | (value << 4);
    }
    fatTable[entryPairIndex] = fatEntry & 0xff;
    fatTable[entryPairIndex + 1] = fatEntry >> 8;
}

#pragma pack(push, 1)
typedef struct {
    char name[11];
    uint8_t attributes;
    uint8_t ntReserved;
    // Creation time is split into creationTime (granularity of 2 seconds) and creationTimeTenths (between 0 and 199).
    uint8_t creationTimeTenths;
    uint16_t creationTime;
    uint16_t creationDate;
    uint16_t lastAccessDate;
    uint16_t firstDataClusterHigh;
    uint16_t lastWriteTime;
    uint16_t lastWriteDate;
    uint16_t firstDataClusterLow;
    uint32_t fileSizeInBytes;
} fat_directory_entry;
#pragma pack(pop)

int main(int argc, char** argv) {
    if (argc < 3) {
        //TODO: Better usage message
        fprintf(stderr, "Invalid usage.\n");
        exit(1);
    }

    char* imageName = argv[1];
    FILE* imageFile = tryFOpen(imageName, "rb+");
    tryFSeek(imageFile, 3, SEEK_SET, imageName);
    bios_parameter_block parameterBlock;
    fread(&parameterBlock, 1, sizeof(bios_parameter_block), imageFile);

    char* sourcePath = argv[2];
    // Only consider the base name of the file (i.e, the part after the last \)
    char* sourceName = strchr(sourcePath, '\\');
    if (sourceName == NULL) {
        sourceName = sourcePath;
    }
    else {
        sourceName++;
        char* nextSlash = strchr(sourceName, '\\');
        while (nextSlash != NULL) {
            sourceName = nextSlash + 1;
            nextSlash = strchr(sourceName, '\\');
        }
    }
    
    fat_directory_entry directoryEntry = {0};
    char* extension = strchr(sourceName, '.');
    // If the base name and/or extension are too long, they are truncated to 7 characters and 3 characters respectively.
    if (extension == NULL) {
        fillWithSpacePaddedByteString(directoryEntry.name, sourceName, 11);
    }
    else {
        size_t baseNameLength = strlen(sourceName) - strlen(extension);
        // Periods are not valid characters in the directory name, so we add only the extension after the final period.
        char* nextDot = strchr(extension + 1, '.');
        while (nextDot != NULL) {
            extension = nextDot;
            nextDot = strchr(extension + 1, '.');
        }
        fillWithTruncatedSpacePaddedByteString(directoryEntry.name, sourceName, 8, baseNameLength);
        fillWithSpacePaddedByteString(directoryEntry.name + 8, extension + 1, 3);
    }

    //TODO: Handle attributes for directories, etc.
    // directoryEntry.attributes = ...;

    //TODO: Can we populate creationTimeTenths? We need something more granular than time()
    time_t secondsSinceEpoch = time(NULL);
    struct tm* timeData = localtime(&secondsSinceEpoch);
    directoryEntry.creationTime = ((timeData->tm_sec / 2) & 0b11111)
        | ((timeData->tm_min & 0b111111) << 5)
        | ((timeData->tm_hour & 0b11111) << 11);
    directoryEntry.creationDate = (timeData->tm_mday & 0b11111)
        | (((timeData->tm_mon + 1) & 0b1111) << 5)
        | (((timeData->tm_year - 80) & 0b1111111) << 9);
    directoryEntry.lastAccessDate = directoryEntry.creationDate;
    directoryEntry.lastWriteTime = directoryEntry.creationTime;
    directoryEntry.lastWriteDate = directoryEntry.creationDate;

    //TODO: firstDataClusterHigh and firstDataClusterLow
    
    FILE* sourceFile = tryFOpen(sourcePath, "rb");
    size_t sourceFileSizeInBytes = getFileSize(sourceFile, sourceName);
    directoryEntry.fileSizeInBytes = (uint32_t)sourceFileSizeInBytes;
    size_t bytesPerCluster = parameterBlock.bytesPerSector * parameterBlock.sectorsPerCluster;
    size_t sourceClusterNotEofCount = (sourceFileSizeInBytes / bytesPerCluster);

    // Allocate the clusters in the FAT
    size_t fatByteCount = parameterBlock.fatSectorCount * parameterBlock.bytesPerSector;
    size_t fatByteOffset = parameterBlock.reservedSectorCount * parameterBlock.bytesPerSector;
    uint8_t* fatTable = malloc(fatByteCount);
    tryFSeek(imageFile, fatByteOffset, SEEK_SET, imageName);
    fread(fatTable, 1, fatByteCount, imageFile);

    //TODO: The sourceClusterNotEofCount could be too large to allocate
    size_t firstEmptyClusterIndex = 0;
    while (readFatClusterEntry(fatTable, firstEmptyClusterIndex) != 0) {
        firstEmptyClusterIndex++;
    }
    directoryEntry.firstDataClusterLow = (uint16_t)firstEmptyClusterIndex;
    directoryEntry.firstDataClusterHigh = (uint16_t)((firstEmptyClusterIndex >> 16) & 0xffff);

    for (size_t clusterIndex = firstEmptyClusterIndex; clusterIndex < firstEmptyClusterIndex + sourceClusterNotEofCount; ++clusterIndex) {
        populateFatClusterEntry(fatTable, clusterIndex, (uint16_t)clusterIndex + 1);
    }
    populateFatClusterEntry(fatTable, firstEmptyClusterIndex + sourceClusterNotEofCount, 0xfff);
    tryFSeek(imageFile, fatByteOffset, SEEK_SET, imageName);
    tryFWrite(fatTable, 1, fatByteCount, imageFile, imageName);
    free(fatTable);

    // Set root directory entry
    size_t rootDirectorySectorOffset = parameterBlock.reservedSectorCount + parameterBlock.fatCount * parameterBlock.fatSectorCount;
    size_t rootDirectoryByteOffset = rootDirectorySectorOffset * parameterBlock.bytesPerSector;
    size_t rootDirectoryByteCount = parameterBlock.rootEntryCount * 32;
    uint8_t* rootDirectoryEntries = malloc(rootDirectoryByteCount);
    tryFSeek(imageFile, rootDirectoryByteOffset, SEEK_SET, imageName);
    fread(rootDirectoryEntries, 1, rootDirectoryByteCount, imageFile);
    size_t firstEmptyEntryByteOffset = 0;
    while (rootDirectoryEntries[firstEmptyEntryByteOffset] != 0) {
        firstEmptyEntryByteOffset += 32;
    }
    memcpy(&rootDirectoryEntries[firstEmptyEntryByteOffset], &directoryEntry, sizeof(directoryEntry));
    tryFSeek(imageFile, rootDirectoryByteOffset, SEEK_SET, imageName);
    tryFWrite(rootDirectoryEntries, 1, rootDirectoryByteCount, imageFile, imageName);
    free(rootDirectoryEntries);

    // Store the data in the data region
    uint8_t* sourceFileContents = malloc(sourceFileSizeInBytes);
    fread(sourceFileContents, 1, sourceFileSizeInBytes, sourceFile);
    size_t dataAreaByteOffset = rootDirectoryByteOffset + rootDirectoryByteCount;
    size_t firstEmptyDataSectorByteOffset = dataAreaByteOffset + firstEmptyClusterIndex * parameterBlock.bytesPerSector;
    tryFSeek(imageFile, firstEmptyDataSectorByteOffset, SEEK_SET, imageName);
    tryFWrite(sourceFileContents, 1, sourceFileSizeInBytes, imageFile, imageName);
    free(sourceFileContents);

    fclose(sourceFile);
    fclose(imageFile);
}

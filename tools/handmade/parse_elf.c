#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t magic[4];
    uint8_t class;
    uint8_t data;
    uint8_t versionI;
    uint8_t osAbi;
    uint8_t abiVersion;
    uint8_t padding[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uint32_t entry;
    uint32_t programHeaderOffset;
    uint32_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t headerSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderEntryCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderEntryCount;
    uint16_t sectionNamesIndex;
    uint8_t leftover[];
} elf_format;
#pragma pack(pop)

size_t getFileSize(FILE* file) {
    long position = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, position, SEEK_SET);
    return (size_t)size;
}

char* readEntireFile(FILE* file) {
    size_t fileSize = getFileSize(file);
    char* contents = malloc(fileSize);
    fread(contents, 1, fileSize, file);
    return contents;
}

void printByteArray(uint8_t* array, size_t arraySize) {
    if (!arraySize) {
        printf("[]\n");
        return;
    }
    printf("[%hhu", array[0]);
    for (size_t arrayIndex = 1; arrayIndex < arraySize; ++arrayIndex) {
        printf(", %hhu", array[arrayIndex]);
    }
    printf("]\n");
}

int main(void) {
    char* filePath = "simple_elf";
    FILE* elf = fopen(filePath, "rb");
    elf_format* elfContents = (elf_format*)readEntireFile(elf);

    printf("Magic bytes: ");
    printByteArray(elfContents->magic, sizeof(elfContents->magic));
    printf("Class: %hhu\n", elfContents->class);
    printf("Data: %hhu\n", elfContents->data);
    printf("Version (ident): %hhu\n", elfContents->versionI);
    printf("OS ABI: %hhu\n", elfContents->osAbi);
    printf("ABI Version: %hhu\n", elfContents->abiVersion);
    printf("Padding: ");
    printByteArray(elfContents->padding, sizeof(elfContents->padding));
    printf("Type: %hu\n", elfContents->type);
    printf("Machine: %hu\n", elfContents->machine);
    printf("Version: %u\n", elfContents->version);
    printf("Entry: %08x\n", elfContents->entry);
    printf("Program header offset: %u\n", elfContents->programHeaderOffset);
    printf("Section header offset: %u\n", elfContents->sectionHeaderOffset);
    printf("Flags: %08x\n", elfContents->flags);
    printf("Header size: %hu\n", elfContents->headerSize);
    printf("Program header entry size: %hu\n", elfContents->programHeaderEntrySize);
    printf("Program header entry count: %hu\n", elfContents->programHeaderEntryCount);
    printf("Section header entry size: %hu\n", elfContents->sectionHeaderEntrySize);
    printf("Section header entry count: %hu\n", elfContents->sectionHeaderEntryCount);
    printf("Section names index: %hu\n", elfContents->sectionNamesIndex);

    free(elfContents);
    return 0;
}

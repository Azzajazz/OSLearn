#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>

void writeGdtEntry(uint8_t* buffer, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
    *(uint16_t*)buffer = (uint16_t)limit & 0xFFFF;
    *(uint16_t*)(buffer + 2) = (uint16_t)base & 0xFFFF;
    *(buffer + 4) = (base >> 16) & 0xFF;
    *(buffer + 5) = access;
    *(buffer + 6) = (uint8_t)((flags << 4) | (limit >> 16));
    *(buffer + 7) = base >> 24;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Invalid options!\n");
    }
    uint8_t gdt[0x100];
    writeGdtEntry(gdt, 0, 0, 0, 0);
    //Kernel code segment
    writeGdtEntry(gdt + 8, 0, 0xFFFFF, 0x9B, 0xC);
    //Kernel data segment
    writeGdtEntry(gdt + 16, 0, 0xFFFFF, 0x93, 0xC);
    //User code segment
    writeGdtEntry(gdt + 24, 0, 0xFFFFF, 0xFB, 0xC);
    //User data segment
    writeGdtEntry(gdt + 32, 0, 0xFFFFF, 0xF3, 0xC);
    //TSS segment
    writeGdtEntry(gdt + 40, 0xE00, 0x6B, 0x89, 0x0);
    FILE* image = fopen(argv[1], "rb+");
    fseek(image, 512, SEEK_SET);
    fwrite(gdt, sizeof(uint8_t), 0x100, image);
    fclose(image);
    return 0;
}

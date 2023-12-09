#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

#include "fatcommon.h"

void writeReservedFatEntries(FILE* stream, size_t offset, uint8_t mediaIdentifier, char* fileName) { 
    tryFSeek(stream, (long)offset, SEEK_SET, fileName);
    // The first two entries of the table are reserved. The first has value 0xf<mediaIdentifier>, the second 0xfff (EOC value).
    // When combined and stored, this has format <mediaIdentifier> ff ff
    uint8_t reservedBytes[] = {0, 0xff, 0xff};
    reservedBytes[0] = mediaIdentifier;
    tryFWrite(reservedBytes, 1, 3, stream, fileName);
}

int main(int argc, char** argv) {
    if (argc == 1) {
        //TODO: Better usage message.
        fprintf(stderr, "Incorrect usage.\n");
        exit(1);
    }

    //TODO: Make this customizable.
    bios_parameter_block parameterBlock = {0};
    fillWithSpacePaddedByteString(parameterBlock.oemName, "IMAGOAT!", 8);
    parameterBlock.bytesPerSector = 512;
    parameterBlock.sectorsPerCluster = 1; 
    parameterBlock.reservedSectorCount = 2;
    parameterBlock.fatCount = 2;
    parameterBlock.rootEntryCount = 224;
    parameterBlock.totalSectorCount = 2880;
    parameterBlock.mediaIdentifier = 0xf0;
    parameterBlock.fatSectorCount = 9;
    parameterBlock.sectorsPerTrack = 18;
    parameterBlock.headCount = 2;
    parameterBlock.bootSignature = 0x29;
    parameterBlock.volumeSerialNumber = 0x12345678;
    fillWithSpacePaddedByteString(parameterBlock.volumeLabel, "DUMMYLABEL", 11);
    fillWithSpacePaddedByteString(parameterBlock.fileSystemType, "FAT12", 8);

    char* imageName = argv[1];
    FILE* imageFile = tryFOpen(imageName, "r+");

    // Write the BPB
    tryFSeek(imageFile, 3, SEEK_SET, imageName);
    tryFWrite(&parameterBlock, 1, sizeof(bios_parameter_block), imageFile, imageName);

    // Write reserved bytes into both FATs
    size_t reservedSectorsInBytes = parameterBlock.reservedSectorCount * parameterBlock.bytesPerSector;
    size_t firstFatByteOffset = reservedSectorsInBytes;
    writeReservedFatEntries(imageFile, firstFatByteOffset, parameterBlock.mediaIdentifier, imageName);
    if (parameterBlock.fatCount == 2) {
        size_t fatSizeInBytes = parameterBlock.fatSectorCount * parameterBlock.bytesPerSector;
        size_t secondFatByteOffset = reservedSectorsInBytes + fatSizeInBytes;
        writeReservedFatEntries(imageFile, secondFatByteOffset, parameterBlock.mediaIdentifier, imageName);
    }

    fclose(imageFile);
}

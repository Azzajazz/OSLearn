#include <stdint.h>
#include <stdio.h>
#include <iostream>

/*
 * @TODO: File names! Nothing can be located by name yet.
 */

constexpr int floppy_size = 1474560; // Size of a 1.44MB floppy in bytes
constexpr int sector_size = 512;

// This struct is required to be packed, but alignment works out nicely so that an __attribute__((packed)) is not necessary.
struct TFS_Header {
    uint32_t reserved_jump;
    uint16_t FAT_sector_offset;
    uint16_t TAT_sector_offset;
    uint8_t  FAT_sector_count;
    uint8_t  TAT_sector_count;
    uint16_t TFM_sector_offset;
    uint8_t  TFM_sector_count;
    uint8_t  reserved_sector_count;
    uint16_t tag_data_sector_offset;
    uint16_t tag_data_sector_count;
    uint16_t file_data_sector_offset;
};

struct Read_Entire_File_Result {
    uint8_t* content;
    int length;
};

Read_Entire_File_Result read_entire_file(std::string file_name) {
    FILE* file = fopen(file_name.c_str(), "rb");
    if (!file) {
        std::cerr << "[ERROR]: Could not open file " << file_name << "\n";
        std::abort();
    }
    fseek(file, 0, SEEK_END);
    int length = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t* content = new uint8_t[length];
    fread(content, 1, length, file);
    fclose(file);
    return {content, length};
}

uint16_t read_fat(uint8_t* fat, int entry_index) {
    /*
     * @NOTE: Because of endianness, two consecutive fat entries 0xabc and 0xdef are stored as bc fa de. Two consecutive 16-bit reads will therefore read 0xfabc and 0xdefa.
     * Mask the first off to get 0xabc and shift the second to get 0xdef.
     */
    int low_index = entry_index / 2 * 3;
    if (entry_index % 2 == 0) {
        uint16_t entry = *(uint16_t*)(fat + low_index);
        return entry & 0xfff;
    }
    else {
        uint16_t entry = *(uint16_t*)(fat + low_index + 1);
        return entry >> 4;
    }
}

void write_fat(uint8_t* fat, int entry_index, uint16_t value) {
    // @NOTE: For a note on the math here, see read_fat
    int low_index = entry_index / 2 * 3;
    if (entry_index % 2 == 0) {
        uint16_t* entry_pointer = (uint16_t*)(fat + low_index);
        uint16_t new_entry = (*entry_pointer & 0xf000) | value;
        *entry_pointer = new_entry;
    }
    else {
        uint16_t* entry_pointer = (uint16_t*)(fat + low_index + 1);
        uint16_t new_entry = (*entry_pointer & 0x000f) | (value << 4);
        *entry_pointer = new_entry;
    }
}

void write_sector(uint8_t* file_data_region, uint8_t* content, int content_length, int file_data_sector_index, int content_sector_index) {
    int content_byte_offset = content_sector_index * sector_size;
    int file_data_byte_offset = file_data_sector_index * sector_size;
    if (content_length - content_byte_offset < sector_size) {
        memcpy(file_data_region + file_data_byte_offset, content + content_byte_offset, content_length - content_byte_offset);
    }
    else {
        memcpy(file_data_region + file_data_byte_offset, content + content_byte_offset, sector_size);
    }
}

int allocate_sector(uint8_t* fat, int fat_byte_count) {
    for (int i = 0; i < fat_byte_count / 3 * 2; ++i) {
        if (read_fat(fat, i) == 0) {
            write_fat(fat, i, 0xfff);
            return i;
        }
    }
    // This should always succeed, so we should not get here.
    std::abort();
}

int get_image_space(uint8_t* fat, int fat_byte_count) {
    int space = 0;
    for (int i = 0; i < fat_byte_count / 3 * 2; ++i) {
        if (read_fat(fat, i) == 0) space += sector_size;
    }
    return space;
}

void usage(char* program_name) {
    std::cout << "Usage: " << program_name << " <subcommand>\n\n";
    std::cout << "  <subcommand>: One of\n";
    std::cout << "    format <image>: Format <image> using a predefined TFS header.\n";
    std::cout << "    write <image> <file>: Writes <file> to <image>.\n";
    // @TODO: Keep adding stuff here
}

bool format(std::string image) {
    uint8_t floppy_data[floppy_size];
    
    FILE* in_file = fopen(image.c_str(), "rb+");
    if (!in_file) {
        std::cerr << "[ERROR]: Could not open file " << image << "\n";
        return false;
    }
    fread(floppy_data, 1, floppy_size, in_file);

    // @TODO: Make these configurable via command line
    TFS_Header* header = reinterpret_cast<TFS_Header*>(floppy_data);
    header->reserved_sector_count = 3; // @TODO: Make this dynamic based on the size of the assembled binary
    header->FAT_sector_offset = header->reserved_sector_count;
    header->FAT_sector_count = 1;
    header->TAT_sector_offset = header->FAT_sector_offset + header->FAT_sector_count;
    header->TAT_sector_count = 1;
    header->TFM_sector_offset = header->TAT_sector_offset + header->TAT_sector_count;
    header->TFM_sector_count = 1;
    header->tag_data_sector_offset = header->TFM_sector_offset + header->TFM_sector_count;
    header->tag_data_sector_count = 10; // @TODO: This is a guess for now
    header->file_data_sector_offset = header->tag_data_sector_offset + header->tag_data_sector_count;

    fseek(in_file, 0, SEEK_SET);
    fwrite(floppy_data, 1, floppy_size, in_file);
    fclose(in_file);

    return true;
}

bool write(std::string image, std::string file_name) {
    uint8_t floppy_data[floppy_size];
    
    FILE* in_file = fopen(image.c_str(), "rb+");
    if (!in_file) {
        std::cerr << "[ERROR]: Could not open in_file " << image << "\n";
        return false;
    }
    fread(floppy_data, 1, floppy_size, in_file);

    TFS_Header* header = reinterpret_cast<TFS_Header*>(floppy_data);
    uint8_t* fat = floppy_data + header->FAT_sector_offset * sector_size;

    // Check that the file fits. After this, any write of the file is guaranteed to succeed
    Read_Entire_File_Result read_result = read_entire_file(file_name);
    int image_space = get_image_space(fat, header->FAT_sector_count * sector_size);
    if (image_space < read_result.length) {
        std::cerr << "[ERROR]: There isn't enough space to write that (need " << read_result.length << ", have " << image_space << ")\n";
        return false;
    }

    int file_sector_count = (read_result.length + sector_size - 1) / sector_size;
    // Sector indices start at 1 since 0 denotes a free sector, so we can safely initalize these to 0 and assume they are invalid.
    int previous_sector = 0;
    int current_sector = 0;
    for (int i = 0; i < file_sector_count; ++i) {
        current_sector = allocate_sector(fat, header->FAT_sector_count * sector_size) + 1;

        if (previous_sector) {
            write_fat(fat, previous_sector - 1, current_sector);
        }

        write_sector(floppy_data + header->file_data_sector_offset * sector_size, read_result.content, read_result.length, current_sector - 1, i);
        previous_sector = current_sector;
    }

    fseek(in_file, 0, SEEK_SET);
    fwrite(floppy_data, 1, floppy_size, in_file);
    fclose(in_file);

    return true;
}

enum class Subcommand {
    Format,
    Write,
};

int main(int argc, char** argv) {
    if (argc <= 1) {
        std::cerr << "[ERROR]: Insufficient arguments\n\n";
        usage(argv[0]);
        return 1;
    }

    Subcommand subcommand;
    std::string subcommand_name = argv[1];
    if (subcommand_name == "format") subcommand = Subcommand::Format;
    else if (subcommand_name == "write") subcommand = Subcommand::Write;
    
    switch (subcommand) {
        case Subcommand::Format: {
            if (argc != 3) {
                std::cerr << "[ERROR]: The format subcommand expects exactly one argument <image>\n\n";
                usage(argv[0]);
                return 1;
            }

            std::string image = argv[2];
            if (format(image)) {
                std::cout << "[INFO]: Format successful!\n";
                return 0;
            }
            else {
                std::cerr << "[ERROR]: Format failed!\n";
                return 1;
            }
        } break;

        case Subcommand::Write: {
            if (argc != 4) {
                std::cerr << "ERROR]: The write subcommand expects exactly two arguments <image> and <file>\n\n";
                usage(argv[0]);
                return 1;
            }

            std::string image = argv[2];
            std::string file = argv[3];
            if (write(image, file)) {
                std::cout << "[INFO]: Write successful!\n";
                return 0;
            }
            else {
                std::cerr << "[ERROR]: Write failed!\n";
                return 1;
            }
        } break;

        default: {
            std::abort();
        } break;
    }
    
    return 0;
}

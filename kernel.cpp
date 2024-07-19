#include <stdint.h>
#include <stdarg.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;

typedef float  f32;
typedef double f64;

#define KiB(x) ((x) * 1024)
#define MiB(x) (KiB(x) * 1024)

// These are symbols defined in the linker script
extern char kernel_virt_start;
extern char kernel_virt_end;

u8 io_in_8(u16 port) {
    u8 result;
    asm volatile("in %%dx, %%al" : "=a" (result) : "d" (port));
    return result;                                             
}

u16 io_in_16(u16 port) {
    u16 result;
    asm volatile("in %%dx, %%ax" : "=a" (result) : "d" (port));
    return result;
}

u32 io_in_32(u16 port) {
    u32 result;
    asm volatile("in %%dx, %%eax" : "=a" (result) : "d" (port));
    return result;
}

void io_out_8(u16 port, u8 value) {
    asm volatile("out %%al, %%dx" :: "a" (value), "d" (port));
}

void io_out_16(u16 port, u16 value) {
    asm volatile("out %%ax, %%dx" :: "a" (value), "d" (port));
}

void io_out_32(u16 port, u32 value) {
    asm volatile("out %%eax, %%dx" :: "a" (value), "d" (port));
}

void io_wait() {
    io_out_8(0x80, 0);
}

struct String {
    const char* data;
    u32 length;
};

u32 c_str_length(const char* c_str) {
    u32 length = 0;
    for (const char* p = c_str; *p; ++p) {
        length += 1;
    }
    return length;
}

String as_string(const char* c_str) {
    return (String){c_str, c_str_length(c_str)};
}

// @TODO: This is how it will be after paging:
// static volatile u16* video_address = (u16*)0xb8000;
constexpr int ROW_COUNT = 25;
constexpr int COL_COUNT = 80;
constexpr u16 WHITE_ON_BLACK = 0x0f00;

constexpr u16 CRTC_ADDR_REG = 0x3D4;
constexpr u16 CRTC_DATA_REG = 0x3D5;

constexpr u8 CRTC_CURSOR_LOC_HIGH_IDX = 0x0E;
constexpr u8 CRTC_CURSOR_LOC_LOW_IDX = 0x0F;
constexpr u8 CRTC_CURSOR_START_IDX = 0x0A;
constexpr u8 CRTC_CURSOR_END_IDX = 0x0B;
constexpr u8 CRTC_MAX_SCAN_LINE_IDX = 0x09;

void set_cursor_location(u16 location) {
    u8 old_address = io_in_8(CRTC_ADDR_REG);
    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_LOC_LOW_IDX);
    io_out_8(CRTC_DATA_REG, location & 0xff);
    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_LOC_HIGH_IDX);
    io_out_8(CRTC_DATA_REG, location >> 8);
    io_out_8(CRTC_ADDR_REG, old_address);
}

u16 get_cursor_location(void) {
    u8 old_address = io_in_8(CRTC_ADDR_REG);
    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_LOC_LOW_IDX);
    u16 location = io_in_8(CRTC_DATA_REG);
    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_LOC_HIGH_IDX);
    location |= io_in_8(CRTC_DATA_REG) << 8;
    io_out_8(CRTC_ADDR_REG, old_address);
    return location;
}

void enable_cursor(void) {
    u8 old_address = io_in_8(CRTC_ADDR_REG);
    io_out_8(CRTC_ADDR_REG, CRTC_MAX_SCAN_LINE_IDX);
    u8 max_scan_line = io_in_8(CRTC_DATA_REG) & 0x1f;

    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_START_IDX);
    u8 data = io_in_8(CRTC_DATA_REG);
    data &= ~0x3f;
    io_out_8(CRTC_DATA_REG, data);

    io_out_8(CRTC_ADDR_REG, CRTC_CURSOR_END_IDX);
    data = io_in_8(CRTC_DATA_REG);
    data &= ~0x7f;
    data |= max_scan_line;
    io_out_8(CRTC_DATA_REG, data);

    io_out_8(CRTC_ADDR_REG, old_address);

    set_cursor_location(0);
}

void scroll_screen(void) {
    // @Temporary
    volatile u16* video_address = (u16*)0xb8000;

    for (int i = 0; i < ROW_COUNT * COL_COUNT; ++i) {
        video_address[i] = video_address[i + COL_COUNT];
    }

    u16 location = get_cursor_location();
    set_cursor_location(location - location % COL_COUNT);
}

void clear_screen(void) {
    // @Temporary
    volatile u16* video_address = (u16*)0xb8000;

    for (int i = 0; i < ROW_COUNT * COL_COUNT; ++i) {
        video_address[i] = WHITE_ON_BLACK | ' ';
    }
}

void print_char(char value) {
    // @Temporary
    volatile u16* video_address = (u16*)0xb8000;

    u16 location = get_cursor_location();
    if (value == '\r') {
        set_cursor_location(location - location % COL_COUNT);
        return;
    }

    if (value == '\n') {
        if (location >= (ROW_COUNT - 1) * COL_COUNT) {
            scroll_screen();
        }
        else {
            location += COL_COUNT;
            set_cursor_location(location - location % COL_COUNT);
        }
        return;
    }

    if (location == ROW_COUNT * COL_COUNT) {
        scroll_screen();
    }

    video_address[location] = WHITE_ON_BLACK | value;
    set_cursor_location(location + 1);
}

void print_string(String string) {
    for (u32 i = 0; i < string.length; ++i) {
        print_char(string.data[i]);
    }
}

void print_u32(u32 value) {
    if (value == 0) {
        print_char('0');
        return;
    }

    char digits[10]; // max of 10 base ten digits for u32 (0-4294967295)
    int digit_count = 0;

    while (value > 0) {
        digits[digit_count++] = value % 10 + '0';
        value /= 10;
    }

    for (int i = digit_count - 1; i >= 0; --i) {
        print_char(digits[i]);
    } 
}

void print_u32_hex(u32 value) {
    print_char('0');
    print_char('x');

    if (value == 0) {
        print_char('0');
        return;
    }

    char digits[8]; // max of 8 base 16 digits for u32 (0-ffffffff)
    int digit_count = 0;

    while (value > 0) {
        u32 digit = value & 0xf;
        if (digit > 9) {
            digits[digit_count++] = digit - 10 + 'a';
        }
        else {
            digits[digit_count++] = digit + '0';
        }

        value >>= 4;
    }

    for (int i = digit_count - 1; i >= 0; --i) {
        print_char(digits[i]);
    } 
}

enum class Fmt_Print_State {
    NoFormat,
    /*
    Flags,
    Width,
    Precision,
    Length,
    */
    Specifier,
    Unsigned,
    /*
    Signed,
    */
    Hexadecimal,
};

void fmt_print(String fmt, ...) {
    Fmt_Print_State state = Fmt_Print_State::NoFormat;

    va_list args;
    va_start(args, fmt);

    u32 i = 0;

    while (i < fmt.length) {
        switch (state) {
            case Fmt_Print_State::NoFormat: {
                if (fmt.data[i] == '%') {
                    state = Fmt_Print_State::Specifier;
                }
                else {
                    print_char(fmt.data[i]);
                }
                i++;
            } break;

            case Fmt_Print_State::Specifier: {
                switch (fmt.data[i]) {
                    case 's': {
                        print_string(va_arg(args, String));
                        state = Fmt_Print_State::NoFormat;
                    } break;
                    case 'u': {
                        state = Fmt_Print_State::Unsigned;
                    } break;
                    case 'x': {
                        state = Fmt_Print_State::Hexadecimal;
                    } break;
                    /*
                    case 'd': {
                        state = Fmt_Print_State::Signed;
                    } break;
                    */
                    case '%': {
                        print_char('%');
                        state = Fmt_Print_State::NoFormat;
                    } break;
                    default: {
                        print_char(fmt.data[i]);
                        state = Fmt_Print_State::NoFormat;
                    } break;
                }
                i++;
            } break;

            case Fmt_Print_State::Unsigned: {
                if (fmt.data[i] == '8') {
                    print_u32(va_arg(args, u32));
                    i++;
                }
                else if ((fmt.data[i] == '1' && fmt.data[i + 1] == '6')
                        || (fmt.data[i] == '3' && fmt.data[i + 1] == '2')) {
                    print_u32(va_arg(args, u32));
                    i += 2;
                }
                state = Fmt_Print_State::NoFormat;
            } break;

            /*
            case Fmt_Print_State::Signed {
            } break;
            */

            case Fmt_Print_State::Hexadecimal: {
                if (fmt.data[i] == '8') {
                    print_u32_hex(va_arg(args, u32));
                    i++;
                }
                else if ((fmt.data[i] == '1' && fmt.data[i + 1] == '6')
                        || (fmt.data[i] == '3' && fmt.data[i + 1] == '2')) {
                    print_u32_hex(va_arg(args, u32));
                    i += 2;
                }
                state = Fmt_Print_State::NoFormat;
            } break;
        }
    }

    va_end(args);
}

constexpr u32 PAGE_TABLE_PRESENT      = 0x1;
constexpr u32 PAGE_TABLE_READWRITE    = 0x2;
constexpr u32 PAGE_TABLE_SUPERVISOR   = 0x4;
constexpr u32 PAGE_TABLE_WRITETHROUGH = 0x8;
constexpr u32 PAGE_TABLE_CACHEDISABLE = 0x10;
constexpr u32 PAGE_TABLE_ACCESSED     = 0x20;
constexpr u32 PAGE_TABLE_PAGESIZE     = 0x80;

constexpr u32 PAGE_PRESENT      = 0x1;
constexpr u32 PAGE_READWRITE    = 0x2;
constexpr u32 PAGE_SUPERVISOR   = 0x4;
constexpr u32 PAGE_WRITETHROUGH = 0x8;
constexpr u32 PAGE_CACHEDISABLE = 0x10;
constexpr u32 PAGE_ACCESSED     = 0x20;
constexpr u32 PAGE_DIRTY        = 0x40;
constexpr u32 PAGE_PAT          = 0x80;
constexpr u32 PAGE_GLOBAL       = 0x100;

#define PAGE_SIZE KiB(4)
#define PAGE_TABLE_SIZE 4 * 1024 // Page tables are 1024 4-byte entries

extern "C" int kmain(void) {
    clear_screen();
    enable_cursor();
    fmt_print(as_string("Hello, world!"));

    for(;;);
}

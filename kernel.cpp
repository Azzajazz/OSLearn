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

#include "interrupts.cpp"

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

// This is extern "C" since it is called from interrupt handlers in kernel_entry.asm
extern "C" void print_cstring(const char* cstring) {
    for (const char* p = cstring; *p; ++p) {
        print_char(*p);
    }
}

// This is extern "C" since it is called from interrupt handlers in kernel_entry.asm
extern "C" void print_u32(u32 value) {
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

void fmt_print(const char* fmt, ...) {
    Fmt_Print_State state = Fmt_Print_State::NoFormat;

    va_list args;
    va_start(args, fmt);

    const char* p = fmt;

    while (*p) {
        switch (state) {
            case Fmt_Print_State::NoFormat: {
                if (*p == '%') {
                    state = Fmt_Print_State::Specifier;
                }
                else {
                    print_char(*p);
                }
                p++;
            } break;

            case Fmt_Print_State::Specifier: {
                switch (*p) {
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
                    case 'c': {
                        print_char(va_arg(args, u32));
                        state = Fmt_Print_State::NoFormat;
                    } break;
                    default: {
                        print_char(*p);
                        state = Fmt_Print_State::NoFormat;
                    } break;
                }
                p++;
            } break;

            case Fmt_Print_State::Unsigned: {
                if (*p == '8') {
                    print_u32(va_arg(args, u32));
                    p++;
                }
                else if ((*p == '1' && p[1] == '6')
                        || (*p == '3' && p[1] == '2')) {
                    print_u32(va_arg(args, u32));
                    p += 2;
                }
                state = Fmt_Print_State::NoFormat;
            } break;

            /*
            case Fmt_Print_State::Signed {
            } break;
            */

            case Fmt_Print_State::Hexadecimal: {
                if (*p == '8') {
                    print_u32_hex(va_arg(args, u32));
                    p++;
                }
                else if ((*p == '1' && p[1] == '6')
                        || (*p == '3' && p[1] == '2')) {
                    print_u32_hex(va_arg(args, u32));
                    p += 2;
                }
                state = Fmt_Print_State::NoFormat;
            } break;
        }
    }

    va_end(args);
}

/*
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
*/                                 

constexpr u8 PIC_MASTER_COMMAND = 0x20;
constexpr u8 PIC_MASTER_DATA = 0x21;
constexpr u8 PIC_SLAVE_COMMAND = 0xa0;
constexpr u8 PIC_SLAVE_DATA = 0xa1;

constexpr u8 PIC_EOI_CODE = 0x20;

constexpr u8 ICW1_ICW4_PRESENT = 0x01;        // 0 = ICW4 not present, 1 = ICW4 present
constexpr u8 ICW1_SINGLE = 0x02;              // 0 = cascade mode, 1 = single mode
constexpr u8 ICW1_CALL_ADDR_INTERVAL4 = 0x04; // 0 = call address interval 4, 1 = call address interval 8
constexpr u8 ICW1_LEVEL_TRIGGERED = 0x08;     // 0 = edge triggered mode, 1 = level triggered mode
constexpr u8 ICW1_INIT = 0x10;                // Initialization, must be 1

constexpr u8 ICW4_8086 = 0x01;                 // 0 = MCS-80/85 mode, 1 = 8086/88 mode
constexpr u8 ICW4_AUTO_EOI = 0x02;             // 0 = normal EOI, 1 = automatic EOI
constexpr u8 ICW4_BUFFERED_SLAVE = 0x08;       // 0 = no buffered slave mode, 1 = buffered slave mode
constexpr u8 ICW4_BUFFERED_MASTER = 0x0c;      // 0 = no buffered master mode, 1 = buffered master mode
constexpr u8 ICW4_SPECIAL_FULLY_NESTED = 0x10; // 0 = not special fully nested, 1 = special fully nested

void init_pic(int master_offset, int slave_offset) {
    u8 master_mask = io_in_8(PIC_MASTER_DATA);
    u8 slave_mask = io_in_8(PIC_SLAVE_DATA);
    io_out_8(PIC_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4_PRESENT);
    io_wait();
    io_out_8(PIC_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4_PRESENT);
    io_wait();
    io_out_8(PIC_MASTER_DATA, master_offset);
    io_wait();
    io_out_8(PIC_SLAVE_DATA, slave_offset);
    io_wait();
    io_out_8(PIC_MASTER_DATA, 4);
    io_wait();
    io_out_8(PIC_SLAVE_DATA, 2);
    io_wait();
    
    io_out_8(PIC_MASTER_DATA, ICW4_8086);
    io_wait();
    io_out_8(PIC_SLAVE_DATA, ICW4_8086);
    io_wait();
    
    io_out_8(PIC_MASTER_DATA, master_mask);
    io_out_8(PIC_SLAVE_DATA, slave_mask);
}

extern "C" void pic_send_eoi(u8 irq) {
    if (irq >= 8)
        io_out_8(PIC_SLAVE_COMMAND, PIC_EOI_CODE);

    io_out_8(PIC_MASTER_COMMAND, PIC_EOI_CODE);
}

template<typename T>
struct Optional {
    bool exists;
    T value;
};

template<typename T, u32 n>
struct Sized_Queue {
    T data[n];
    u32 head;
    u32 tail;
};

template<typename T, u32 n>
bool push_back(Sized_Queue<T, n>* queue, T elem) {
    u32 next_tail = (queue->tail + 1) % n;
    if (next_tail == queue->head) return false;
    
    queue->data[queue->tail] = elem;
    queue->tail = next_tail;
    return true;
}

template<typename T, u32 n>
bool is_empty(Sized_Queue<T, n>* queue) {
    return queue->head == queue->tail;
}

template <typename T, u32 n>
Optional<T> pop_front(Sized_Queue<T, n>* queue) {
    if (queue->head == queue->tail) return {};

    Optional<T> result = {};
    result.exists = true;
    result.value = queue->data[queue->head];
    queue->head = (queue->head + 1) % n;
    return result;
}

static Sized_Queue<u8, 10> key_event_queue = {};
template <typename T, u32 n>
T pop_front_unchecked(Sized_Queue<T, n>* queue) {
    T result = queue->data[queue->head];
    queue->head = (queue->head + 1) % n;
    return result;
}

extern "C" void keyboard_handler_inner(void) {
    u8 key = io_in_8(0x60);
    fmt_print(as_string("%u8\n"), key);
    push_back(&key_event_queue, key);
}

extern "C" int kmain(void) {
    clear_screen();
    enable_cursor();
    fmt_print(as_string("Hello, world!\n"));

    init_idt();
    init_pic(0x20, 0x28);
    asm volatile (
        "lidt (0x6000)\n\t"
        "sti\n\t"
    );

    for(;;);
}

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

template <typename T, u32 n>
T pop_front_unchecked(Sized_Queue<T, n>* queue) {
    T result = queue->data[queue->head];
    queue->head = (queue->head + 1) % n;
    return result;
}

struct Key_Event {
    u8 key_code;
    u8 ascii_code;
    bool pressed;
    u8 meta_mask;
};

static Sized_Queue<Key_Event, 10> g_key_event_queue = {};
static bool g_key_states[256] = {};
constexpr u8 g_scan_code_1_single_map[] = {
    0x00, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c,
    0x6c,
    0xa0,
    0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
    0x20,
    0x80,
    0x2d,
    0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b,
    0x33,
    0xa2, 0xa3,
    0x60,
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    0x31,
    0x0e,
    0x51, 0x52, 0x53,
    0x34,
    0x6d, 0x6e, 0x6f,
    0x54,
    0x8d, 0x8e, 0x8f,
    0xaa, 0xab,
    0, 0, 0, // Padding
    0x0b, 0x0c,
};

constexpr u8 g_scan_code_1_multi_map[] = {
    0xe0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0xe1,
    0, 0,
    0x90, 0xa6,
    0, 0,
    0xe2, 0xe3, 0xe4,
    0,
    0xe5, 
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0xe6,
    0,
    0xe7,
    0,
    0xe8,
    0, 0,
    0x32,
    0, 0,
    0xa4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0x2f, 0x8c, 0x30,
    0,
    0xa7,
    0,
    0x4f, 0xa8, 0x50, 0x2e, 0x4e,
    0, 0, 0, 0, 0, 0, 0,
    0xf2, 0xf3, 0xf4, 0xf5, 0xf6,
    0, 0, 0,
    0xf7,
    0,
    0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 
};

constexpr u8 g_us_qwerty_lower_ascii_map[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,    0,   0,   0, 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '`', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',  '-', '=',    0,   0,   0, 0,   0, '/', '*', '-', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p',  '[', ']', '\\',   0,   0, 0, '7', '8', '9', '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',   0,  '4', '5', '6', 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',    0,   0,  '1', '2', '3', 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0,   0,   0, ' ',   0,   0,   0,   0,   0,   0, '0',  '.',   0,    0,   0,   0, 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

constexpr u8 g_us_qwerty_upper_ascii_map[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    '~', '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',   0,   0,   0, 0,   0, '/', '*', '-', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '|',   0,   0, 0, '7', '8', '9', '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',   0, '4', '5', '6', 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,   0, '1', '2', '3', 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0,   0,   0, ' ',   0,   0,   0,   0,   0,   0, '0', '.',   0,   0,   0,   0, 0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

constexpr u8 LEFT_SHIFT = 0x01;
constexpr u8 LEFT_ALT = 0x02;
constexpr u8 LEFT_CTRL = 0x04;
constexpr u8 RIGHT_SHIFT = 0x08;
constexpr u8 RIGHT_ALT = 0x10;
constexpr u8 RIGHT_CTRL = 0x20;

enum class Scan_Code_1_State {
    Single,
    Multi,
    PrintScreenPressed1,
    PrintScreenPressed2,
    PrintScreenReleased1,
    PrintScreenReleased2,
    PausePressed1,
    PausePressed2,
    PausePressed3,
    PausePressed4,
    PausePressed5,
};

static Scan_Code_1_State state = Scan_Code_1_State::Single;

extern "C" void keyboard_handler_inner(void) {
    u8 key = io_in_8(0x60);

    switch (state) {
        case Scan_Code_1_State::Single: {
            if (key == 0xe0) {
                state = Scan_Code_1_State::Multi;
                return;
            }
            if (key == 0xe1) {
                state = Scan_Code_1_State::PausePressed1;
                return;
            }

            Key_Event event = {};
            event.pressed = key < 0x81;   
            if (key < 0x81)
                event.key_code = g_scan_code_1_single_map[key - 1];
            else
                event.key_code = g_scan_code_1_single_map[key - 0x81];

            // Toggle the key states
            // @TODO: Is it possible to start with a key pressed?
            // @TODO: Localized caps lock, scroll lock, num lock
            if (!event.pressed || (event.key_code != 0x60 && event.key_code != 0x31 && event.key_code != 0x0e)) {
                // Caps lock (0x60), num lock (0x31) and scroll lock (0x0e) are toggled in the key state map on press and not on release
                g_key_states[event.key_code] = !g_key_states[event.key_code];
            }
            
            if (g_key_states[0x80]) event.meta_mask |= LEFT_SHIFT;
            if (g_key_states[0xa0]) event.meta_mask |= LEFT_CTRL;
            if (g_key_states[0xa2]) event.meta_mask |= LEFT_ALT;
            if (g_key_states[0x8b]) event.meta_mask |= RIGHT_SHIFT;
            if (g_key_states[0xa6]) event.meta_mask |= RIGHT_CTRL;
            if (g_key_states[0xa4]) event.meta_mask |= RIGHT_ALT;

            if (g_key_states[0x60]) {
                // Caps lock enabled
                if ((event.meta_mask & LEFT_SHIFT) || (event.meta_mask & RIGHT_SHIFT))
                    event.ascii_code = g_us_qwerty_lower_ascii_map[event.key_code];
                else
                    event.ascii_code = g_us_qwerty_upper_ascii_map[event.key_code];
            }
            else {
                if ((event.meta_mask & LEFT_SHIFT) || (event.meta_mask & RIGHT_SHIFT))
                    event.ascii_code = g_us_qwerty_upper_ascii_map[event.key_code];
                else
                    event.ascii_code = g_us_qwerty_lower_ascii_map[event.key_code];
            }

            push_back(&g_key_event_queue, event);
        } break;

        case Scan_Code_1_State::Multi: {
            if (key == 0x2a) {
                state = Scan_Code_1_State::PrintScreenPressed1;
                return;
            }
            if (key == 0xb7) {
                state = Scan_Code_1_State::PrintScreenReleased1;
                return;
            }

            Key_Event event = {};
            event.pressed = key < 0x99;   
            if (key < 0x99)
                event.key_code = g_scan_code_1_multi_map[key - 0x10];
            else
                event.key_code = g_scan_code_1_multi_map[key - 0x90];

            // Toggle the key states
            // @TODO: Is it possible to start with a key pressed?
            g_key_states[event.key_code] = !g_key_states[event.key_code];
            
            if (g_key_states[0x80]) event.meta_mask |= LEFT_SHIFT;
            if (g_key_states[0xa0]) event.meta_mask |= LEFT_CTRL;
            if (g_key_states[0xa2]) event.meta_mask |= LEFT_ALT;
            if (g_key_states[0x8b]) event.meta_mask |= RIGHT_SHIFT;
            if (g_key_states[0xa6]) event.meta_mask |= RIGHT_CTRL;
            if (g_key_states[0xa4]) event.meta_mask |= RIGHT_ALT;

            if (g_key_states[0x60]) {
                // Caps lock enabled
                if ((event.meta_mask & LEFT_SHIFT) || (event.meta_mask & RIGHT_SHIFT))
                    event.ascii_code = g_us_qwerty_lower_ascii_map[event.key_code];
                else
                    event.ascii_code = g_us_qwerty_upper_ascii_map[event.key_code];
            }
            else {
                if ((event.meta_mask & LEFT_SHIFT) || (event.meta_mask & RIGHT_SHIFT))
                    event.ascii_code = g_us_qwerty_upper_ascii_map[event.key_code];
                else
                    event.ascii_code = g_us_qwerty_lower_ascii_map[event.key_code];
            }

            push_back(&g_key_event_queue, event);
            state = Scan_Code_1_State::Single;
        } break;
        
        case Scan_Code_1_State::PrintScreenPressed1: {
            if (key == 0xE0) {
                state = Scan_Code_1_State::PrintScreenPressed2;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PrintScreenPressed2: {
            if (key != 0x37) {
                state = Scan_Code_1_State::Single;
                return;
            }

            Key_Event event = {};
            event.key_code = 0x0d;
            event.pressed = true;

            // Toggle key state
            g_key_states[event.key_code] = !g_key_states[event.key_code];

            if (g_key_states[0x80]) event.meta_mask |= LEFT_SHIFT;
            if (g_key_states[0xa0]) event.meta_mask |= LEFT_CTRL;
            if (g_key_states[0xa2]) event.meta_mask |= LEFT_ALT;
            if (g_key_states[0x8b]) event.meta_mask |= RIGHT_SHIFT;
            if (g_key_states[0xa6]) event.meta_mask |= RIGHT_CTRL;
            if (g_key_states[0xa4]) event.meta_mask |= RIGHT_ALT;

            push_back(&g_key_event_queue, event);
            state = Scan_Code_1_State::Single;
        } break;

        case Scan_Code_1_State::PrintScreenReleased1: {
            if (key == 0xE0) {
                state = Scan_Code_1_State::PrintScreenReleased2;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PrintScreenReleased2: {
            if (key != 0xAA) {
                state = Scan_Code_1_State::Single;
                return;
            }

            Key_Event event = {};
            event.key_code = 0x0d;
            event.pressed = false;

            // Toggle key state
            g_key_states[event.key_code] = !g_key_states[event.key_code];

            if (g_key_states[0x80]) event.meta_mask |= LEFT_SHIFT;
            if (g_key_states[0xa0]) event.meta_mask |= LEFT_CTRL;
            if (g_key_states[0xa2]) event.meta_mask |= LEFT_ALT;
            if (g_key_states[0x8b]) event.meta_mask |= RIGHT_SHIFT;
            if (g_key_states[0xa6]) event.meta_mask |= RIGHT_CTRL;
            if (g_key_states[0xa4]) event.meta_mask |= RIGHT_ALT;

            push_back(&g_key_event_queue, event);
            state = Scan_Code_1_State::Single;
        } break;

        case Scan_Code_1_State::PausePressed1: {
            if (key == 0x1d) {
                state = Scan_Code_1_State::PausePressed2;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PausePressed2: {
            if (key == 0x45) {
                state = Scan_Code_1_State::PausePressed2;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PausePressed3: {
            if (key == 0xe1) {
                state = Scan_Code_1_State::PausePressed4;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PausePressed4: {
            if (key == 0x9d) {
                state = Scan_Code_1_State::PausePressed5;
            }
            else {
                state = Scan_Code_1_State::Single;
            }
        } break;

        case Scan_Code_1_State::PausePressed5: {
            if (key != 0xC5) {
                state = Scan_Code_1_State::Single;
                return;
            }

            Key_Event event = {};
            event.key_code = 0x0d;
            event.pressed = true;

            if (g_key_states[0x80]) event.meta_mask |= LEFT_SHIFT;
            if (g_key_states[0xa0]) event.meta_mask |= LEFT_CTRL;
            if (g_key_states[0xa2]) event.meta_mask |= LEFT_ALT;
            if (g_key_states[0x8b]) event.meta_mask |= RIGHT_SHIFT;
            if (g_key_states[0xa6]) event.meta_mask |= RIGHT_CTRL;
            if (g_key_states[0xa4]) event.meta_mask |= RIGHT_ALT;

            push_back(&g_key_event_queue, event);
            state = Scan_Code_1_State::Single;
        } break;
    }
}

Key_Event get_key_event(void) {
    // @TODO: Implement a spin-lock to remove potential race conditions with the keyboard interrupt.
    while (is_empty(&g_key_event_queue));
    return pop_front_unchecked(&g_key_event_queue);
}

extern "C" int kmain(void) {
    clear_screen();
    enable_cursor();
    fmt_print("Hello, world!\n");

    init_idt();
    init_pic(0x20, 0x28);
    asm volatile (
        "lidt (0x6000)\n\t"
        "sti\n\t"
    );

    for (;;) {
        Key_Event event = get_key_event();
        (void)event;
//        fmt_print("Key code: %x8\n", event.key_code);
//        fmt_print("Ascii code: %c\n", event.ascii_code);
//        fmt_print("Pressed: %u8\n", event.pressed);
        if (event.pressed && event.ascii_code) {
            if (event.meta_mask & LEFT_CTRL || event.meta_mask & RIGHT_CTRL)
                print_char('^');
            print_char(event.ascii_code);
        }
    }
}

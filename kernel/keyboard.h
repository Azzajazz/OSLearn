#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include "types.h"

typedef enum {
    // ASCII key codes
    // These are designed to have the values of their ASCII counterparts
    KEY_CODE_BACKTICK = '`',
    KEY_CODE_MINUS = '-',
    KEY_CODE_EQUALS = '=',
    KEY_CODE_BACKSLASH = '\\',
    KEY_CODE_LEFTBRACKET = '[',
    KEY_CODE_RIGHTBRACKET = ']',
    KEY_CODE_SEMICOLON = ';',
    KEY_CODE_QUOTE = '\'',
    KEY_CODE_COMMA = ',',
    KEY_CODE_PERIOD = '.',
    KEY_CODE_FORWARDSLASH = '/',
    KEY_CODE_SPACE = ' ',
    KEY_CODE_1 = '1',
    KEY_CODE_2 = '2',
    KEY_CODE_3 = '3',
    KEY_CODE_4 = '4',
    KEY_CODE_5 = '5',
    KEY_CODE_6 = '6',
    KEY_CODE_7 = '7',
    KEY_CODE_8 = '8',
    KEY_CODE_9 = '9',
    KEY_CODE_0 = '0',
    KEY_CODE_A = 'a',
    KEY_CODE_B = 'b',
    KEY_CODE_C = 'c',
    KEY_CODE_D = 'd',
    KEY_CODE_E = 'e',
    KEY_CODE_F = 'f',
    KEY_CODE_G = 'g',
    KEY_CODE_H = 'h',
    KEY_CODE_I = 'i',
    KEY_CODE_J = 'j',
    KEY_CODE_K = 'k',
    KEY_CODE_L = 'l',
    KEY_CODE_M = 'm',
    KEY_CODE_N = 'n',
    KEY_CODE_O = 'o',
    KEY_CODE_P = 'p',
    KEY_CODE_Q = 'q',
    KEY_CODE_R = 'r',
    KEY_CODE_S = 's',
    KEY_CODE_T = 't',
    KEY_CODE_U = 'u',
    KEY_CODE_V = 'v',
    KEY_CODE_W = 'w',
    KEY_CODE_X = 'x',
    KEY_CODE_Y = 'y',
    KEY_CODE_Z = 'z',

    // Function keys
    KEY_CODE_F1 = KEY_CODE_Z + 1,
    KEY_CODE_F2,
    KEY_CODE_F3,
    KEY_CODE_F4,
    KEY_CODE_F5,
    KEY_CODE_F6,
    KEY_CODE_F7,
    KEY_CODE_F8,
    KEY_CODE_F9,
    KEY_CODE_F10,
    KEY_CODE_F11,
    KEY_CODE_F12,

    //Numpad keys
    KEY_CODE_NUM1,
    KEY_CODE_NUM2,
    KEY_CODE_NUM3,
    KEY_CODE_NUM4,
    KEY_CODE_NUM5,
    KEY_CODE_NUM6,
    KEY_CODE_NUM7,
    KEY_CODE_NUM8,
    KEY_CODE_NUM9,
    KEY_CODE_NUM0,
    KEY_CODE_NUMSTAR,
    KEY_CODE_NUMSLASH,
    KEY_CODE_NUMMINUS,
    KEY_CODE_NUMPLUS,
    KEY_CODE_NUMPERIOD,
    KEY_CODE_NUMENTER,

    // Meta keys
    KEY_CODE_LEFTCTRL,
    KEY_CODE_RIGHTCTRL,
    KEY_CODE_LEFTALT,
    KEY_CODE_RIGHTALT,
    KEY_CODE_LEFTSHIFT,
    KEY_CODE_RIGHTSHIFT,

    // Special keys
    KEY_CODE_ESC,
    KEY_CODE_ENTER,
    KEY_CODE_TAB,
    KEY_CODE_BACKSPACE,
    KEY_CODE_HOME,
    KEY_CODE_END,
    KEY_CODE_INSERT,
    KEY_CODE_DELETE,
    KEY_CODE_PAGEUP,
    KEY_CODE_PAGEDOWN,
    KEY_CODE_SCROLLLOCK,
    KEY_CODE_NUMLOCK,
    KEY_CODE_CAPSLOCK,

    // Cursor keys
    KEY_CODE_CURSORUP,
    KEY_CODE_CURSORLEFT,
    KEY_CODE_CURSORRIGHT,
    KEY_CODE_CURSORDOWN,

    // Multimedia keys
    KEY_CODE_PREVTRACK,
    KEY_CODE_NEXTTRACK,
    KEY_CODE_PLAY,
    KEY_CODE_STOP,
    KEY_CODE_MUTE,
    KEY_CODE_VOLUMEUP,
    KEY_CODE_VOLUMEDOWN,
    KEY_CODE_MEDIASELECT,
    KEY_CODE_LEFTGUI,
    KEY_CODE_RIGHTGUI,
    KEY_CODE_APPS,
    KEY_CODE_CALC,
    KEY_CODE_MYCOMPUTER,
    KEY_CODE_EMAIL,

    // Web keys
    KEY_CODE_WWWHOME,
    KEY_CODE_WWWSEARCH,
    KEY_CODE_WWWFAVOURITES,
    KEY_CODE_WWWREFRESH,
    KEY_CODE_WWWFORWARD,
    KEY_CODE_WWWBACK,
    KEY_CODE_WWWSTOP,

    // ACPI keys
    KEY_CODE_POWER,
    KEY_CODE_SLEEP,
    KEY_CODE_WAKE,
} key_code;

typedef struct {
    bool32 pressed;
    key_code code;
    uint8 metaMask;
} key_event;

OPTIONAL_DEF(key_event);

#define META_LEFT_SHIFT 0x01
#define META_RIGHT_SHIFT 0x02
#define META_SHIFT (META_LEFT_SHIFT | META_RIGHT_SHIFT)
#define META_LEFT_CTRL 0x04
#define META_RIGHT_CTRL 0x08
#define META_CTRL (META_LEFT_CTRL | META-RIGHT_CTRL)
#define META_LEFT_ALT 0x10
#define META_RIGHT_ALT 0x20
#define META_ALT (META_LEFT_ALT | META-RIGHT_ALT)

#endif

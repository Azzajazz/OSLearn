#include "types.h"
#include "keyboard.h"

#define KEY_EVENT_QUEUE_LENGTH 5
typedef struct {
    key_event queue[KEY_EVENT_QUEUE_LENGTH];
    uint32 head;
    uint32 tail;
} key_event_queue;
global key_event_queue keyEventQueue;

global key_code keyCodeMapping[] = {
    KEY_CODE_ESC,
    KEY_CODE_1,
    KEY_CODE_2,
    KEY_CODE_3,
    KEY_CODE_4,
    KEY_CODE_5,
    KEY_CODE_6,
    KEY_CODE_7,
    KEY_CODE_8,
    KEY_CODE_9,
    KEY_CODE_0,
    KEY_CODE_MINUS,
    KEY_CODE_EQUALS,
    KEY_CODE_BACKSPACE,
    KEY_CODE_TAB,
    KEY_CODE_Q,
    KEY_CODE_W,
    KEY_CODE_E,
    KEY_CODE_R,
    KEY_CODE_T,
    KEY_CODE_Y,
    KEY_CODE_U,
    KEY_CODE_I,
    KEY_CODE_O,
    KEY_CODE_P,
    KEY_CODE_LEFTBRACKET,
    KEY_CODE_RIGHTBRACKET,
    KEY_CODE_ENTER,
    KEY_CODE_LEFTCTRL,
    KEY_CODE_A,
    KEY_CODE_S,
    KEY_CODE_D,
    KEY_CODE_F,
    KEY_CODE_G,
    KEY_CODE_H,
    KEY_CODE_J,
    KEY_CODE_K,
    KEY_CODE_L,
    KEY_CODE_SEMICOLON,
    KEY_CODE_QUOTE,
    KEY_CODE_BACKTICK,
    KEY_CODE_LEFTSHIFT,
    KEY_CODE_BACKSLASH,
    KEY_CODE_Z,
    KEY_CODE_X,
    KEY_CODE_C,
    KEY_CODE_V,
    KEY_CODE_B,
    KEY_CODE_N,
    KEY_CODE_M,
    KEY_CODE_COMMA,
    KEY_CODE_PERIOD,
    KEY_CODE_FORWARDSLASH,
    KEY_CODE_RIGHTSHIFT,
    KEY_CODE_NUMSTAR,
    KEY_CODE_LEFTALT,
    KEY_CODE_SPACE,
    KEY_CODE_CAPSLOCK,
    KEY_CODE_F1,
    KEY_CODE_F2,
    KEY_CODE_F3,
    KEY_CODE_F4,
    KEY_CODE_F5,
    KEY_CODE_F6,
    KEY_CODE_F7,
    KEY_CODE_F8,
    KEY_CODE_F9,
    KEY_CODE_F10,
    KEY_CODE_NUMLOCK,
    KEY_CODE_SCROLLLOCK,
    KEY_CODE_NUM7,
    KEY_CODE_NUM8,
    KEY_CODE_NUM9,
    KEY_CODE_NUMMINUS,
    KEY_CODE_NUM4,
    KEY_CODE_NUM5,
    KEY_CODE_NUM6,
    KEY_CODE_NUMPLUS,
    KEY_CODE_NUM1,
    KEY_CODE_NUM2,
    KEY_CODE_NUM3,
    KEY_CODE_NUM0,
    KEY_CODE_NUMPERIOD,
    KEY_CODE_F11,
    KEY_CODE_F12,

    // Start of scan codes with two bytes
    KEY_CODE_PREVTRACK,
    0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
    KEY_CODE_NEXTTRACK,
    0,
	0,
    KEY_CODE_ENTER,
    KEY_CODE_RIGHTCTRL,
    0,
	0,
    KEY_CODE_MUTE,
    KEY_CODE_CALC,
    KEY_CODE_PLAY,
    0,
    KEY_CODE_STOP,
    0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
    KEY_CODE_VOLUMEDOWN,
    0,
    KEY_CODE_VOLUMEUP,
    0,
    KEY_CODE_WWWHOME,
    0,
	0,
    KEY_CODE_NUMSLASH,
    0,
	0,
    KEY_CODE_RIGHTALT,
    0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
    0,
    0,
    0,
    0,
    0,
    0,
    KEY_CODE_HOME,
    KEY_CODE_CURSORUP,
    KEY_CODE_PAGEUP,
    0,
    KEY_CODE_CURSORLEFT,
    0,
    KEY_CODE_CURSORRIGHT,
    0,
    KEY_CODE_END,
    KEY_CODE_CURSORDOWN,
    KEY_CODE_PAGEDOWN,
    KEY_CODE_INSERT,
    KEY_CODE_DELETE,
    0,
	0,
	0,
	0,
	0,
	0,
	0,
    KEY_CODE_LEFTGUI,
    KEY_CODE_RIGHTGUI,
    KEY_CODE_APPS,
    KEY_CODE_POWER,
    KEY_CODE_SLEEP,
    0,
	0,
	0,
    KEY_CODE_WAKE,
    0,
    KEY_CODE_WWWSEARCH,
    KEY_CODE_WWWFAVOURITES,
    KEY_CODE_WWWREFRESH,
    KEY_CODE_WWWSTOP,
    KEY_CODE_WWWFORWARD,
    KEY_CODE_WWWBACK,
    KEY_CODE_MYCOMPUTER,
    KEY_CODE_EMAIL,
    KEY_CODE_MEDIASELECT,
};

global uint8 metaMask;
global bool32 doubleCode;

//@TODO: This switch statement business is nasty. Clean this up.
void keyboardInterrupt(uint8 scanCode) {
    if (scanCode == 0xE0) {
        doubleCode = true;
        return;
    }

    bool32 pressed = (doubleCode && (scanCode < 0x90))
            || (!doubleCode && (scanCode < 0x81));
    if (!pressed) {
        scanCode -= 0x80;
    }

    key_code keyCode;
    if (doubleCode) {
        keyCode = keyCodeMapping[scanCode - 0x10 + 85];
    }
    else {
        keyCode = keyCodeMapping[scanCode - 1];
    }

    switch (keyCode) {
        case KEY_CODE_LEFTSHIFT: {
            if (pressed) {
                metaMask |= META_LEFT_SHIFT;
            }
            else {
                metaMask &= ~META_LEFT_SHIFT;
            }
        } break;
        case KEY_CODE_RIGHTSHIFT: {
            if (pressed) {
                metaMask |= META_RIGHT_SHIFT;
            }
            else {
                metaMask &= ~META_RIGHT_SHIFT;
            }
        } break;
        case KEY_CODE_LEFTCTRL: {
            if (pressed) {
                metaMask |= META_LEFT_CTRL;
            }
            else {
                metaMask &= ~META_LEFT_CTRL;
            }
        } break;
        case KEY_CODE_RIGHTCTRL: {
            if (pressed) {
                metaMask |= META_RIGHT_CTRL;
            }
            else {
                metaMask &= ~META_RIGHT_CTRL;
            }
        } break;
        case KEY_CODE_LEFTALT: {
            if (pressed) {
                metaMask |= META_LEFT_ALT;
            }
            else {
                metaMask &= ~META_LEFT_ALT;
            }
        } break;
        case KEY_CODE_RIGHTALT: {
            if (pressed) {
                metaMask |= META_RIGHT_ALT;
            }
            else {
                metaMask &= ~META_RIGHT_ALT;
            }
        } break;
        default: break;
    }

    uint32 onePastTail = keyEventQueue.tail + 1;
    if (onePastTail >= KEY_EVENT_QUEUE_LENGTH) {
        onePastTail = 0;
    }
    if (onePastTail != keyEventQueue.head) {
        key_event event;
        event.pressed = pressed;
        event.code = keyCode;
        event.metaMask = metaMask;
        keyEventQueue.queue[keyEventQueue.tail++] = event;
        if (keyEventQueue.tail >= KEY_EVENT_QUEUE_LENGTH) {
            keyEventQueue.tail = 0;
        }
    }
}

OPTIONAL(key_event) queryKeyEvent(void) {
    OPTIONAL(key_event) result;
    if (keyEventQueue.head != keyEventQueue.tail) {
        result.exists = true;
        result.inner = keyEventQueue.queue[keyEventQueue.head++];
        if (keyEventQueue.head >= KEY_EVENT_QUEUE_LENGTH) {
            keyEventQueue.head = 0;
        }
    }
    else {
        result.exists = false;
    }
    return result;
}

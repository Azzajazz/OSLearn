#include "keyboard.h"
#include "inttypes.h"

#define global static

#define KEY_EVENT_QUEUE_LENGTH 5
typedef struct {
    key_event queue[KEY_EVENT_QUEUE_LENGTH];
    uint32 head;
    uint32 tail;
} key_event_queue;
global key_event_queue keyEventQueue;

global key_code keyCodeMapping[KEY_CODE_COUNT] = {
    KEY_CODE_ESC,
    KEY_CODE_1, KEY_CODE_2, KEY_CODE_3, KEY_CODE_4, KEY_CODE_5, KEY_CODE_6, KEY_CODE_7,KEY_CODE_8, KEY_CODE_9, KEY_CODE_0,
    KEY_CODE_MINUS, KEY_CODE_EQUALS, KEY_CODE_BACKSPACE, KEY_CODE_TAB,
    KEY_CODE_Q, KEY_CODE_W, KEY_CODE_E, KEY_CODE_R, KEY_CODE_T, KEY_CODE_Y, KEY_CODE_U, KEY_CODE_I, KEY_CODE_O, KEY_CODE_P,
    KEY_CODE_LEFTBRACKET, KEY_CODE_RIGHTBRACKET, KEY_CODE_ENTER, KEY_CODE_LEFTCTRL,
    KEY_CODE_A, KEY_CODE_S, KEY_CODE_D, KEY_CODE_F, KEY_CODE_G, KEY_CODE_H, KEY_CODE_J, KEY_CODE_K, KEY_CODE_L,
    KEY_CODE_SEMICOLON, KEY_CODE_QUOTE, KEY_CODE_BACKTICK, KEY_CODE_LEFTSHIFT, KEY_CODE_BACKSLASH,
    KEY_CODE_Z, KEY_CODE_X, KEY_CODE_C, KEY_CODE_V, KEY_CODE_B, KEY_CODE_N, KEY_CODE_M,
    KEY_CODE_COMMA, KEY_CODE_PERIOD, KEY_CODE_FORWARDSLASH,
    KEY_CODE_RIGHTSHIFT, KEY_CODE_NUMSTAR, KEY_CODE_LEFTALT, KEY_CODE_SPACE, KEY_CODE_CAPSLOCK,
    KEY_CODE_F1, KEY_CODE_F2, KEY_CODE_F3, KEY_CODE_F4, KEY_CODE_F5, KEY_CODE_F6, KEY_CODE_F7, KEY_CODE_F8, KEY_CODE_F9, KEY_CODE_F10,
    KEY_CODE_NUMLOCK, KEY_CODE_SCROLLLOCK,
    KEY_CODE_NUM7, KEY_CODE_NUM8, KEY_CODE_NUM9, KEY_CODE_NUMMINUS,
    KEY_CODE_NUM4, KEY_CODE_NUM5, KEY_CODE_NUM6, KEY_CODE_NUMPLUS,
    KEY_CODE_NUM1, KEY_CODE_NUM2, KEY_CODE_NUM3, KEY_CODE_NUM0, KEY_CODE_NUMPERIOD,
    KEY_CODE_F11, KEY_CODE_F12,
};

void keyboardInterrupt(uint8 scanCode) {
    uint32 onePastTail = keyEventQueue.tail + 1;
    if (onePastTail >= KEY_EVENT_QUEUE_LENGTH) {
        onePastTail = 0;
    }

    if (onePastTail != keyEventQueue.head) {
        key_event event;
        if (scanCode < KEY_CODE_COUNT) { 
            event.pressed = true;
            event.code = keyCodeMapping[scanCode - 1];
        }
        else {
            event.pressed = false;
            event.code = keyCodeMapping[scanCode - 0x81];
        }
        keyEventQueue.queue[keyEventQueue.tail++] = event;
        if (keyEventQueue.tail >= KEY_EVENT_QUEUE_LENGTH) {
            keyEventQueue.tail = 0;
        }
    }
}

key_query_result queryKeyEvent(void) {
    key_query_result result;
    if (keyEventQueue.head != keyEventQueue.tail) {
        result.exists = true;
        result.event = keyEventQueue.queue[keyEventQueue.head++];
        if (keyEventQueue.head >= KEY_EVENT_QUEUE_LENGTH) {
            keyEventQueue.head = 0;
        }
    }
    else {
        result.exists = false;
    }
    return result;
}

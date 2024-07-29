; This file exists primarily so that the linker does not move the entry point of the kernel to a new location.
; It also contains the default interrupt and trap handlers
segment .text

extern _kmain
extern _print_cstring
extern _print_u32
extern _pic_send_eoi
bits 32

jmp _kmain

; @Handler
; This handler has been written early in development since the PIT is automatically enabled and will fire interrupts that must be handled
global _pit_handler
_pit_handler:
    push 0
    call _pic_send_eoi
    add esp, 4
    iret

; @Handler
global _keyboard_handler
extern _keyboard_handler_inner
_keyboard_handler:
    push KEYBOARD_MESSAGE
    call _keyboard_handler_inner
    call _pic_send_eoi
    add esp, 4
    iret

%macro handler 1
global _handler_%1
_handler_%1:
    push INTERRUPT_MESSAGE
    call _print_cstring
    add esp, 4
    push %1
    call _print_u32
    add esp, 4
    cli
    hlt
%endmacro

handler 0
handler 1
handler 2
handler 3
handler 4
handler 5
handler 6
handler 7
handler 8
handler 9
handler 10
handler 11
handler 12
handler 13
handler 14
handler 15
handler 16
handler 17
handler 18
handler 19
handler 20
handler 21
handler 22
handler 23
handler 24
handler 25
handler 26
handler 27
handler 28
handler 29
handler 30
handler 31
handler 32
handler 33
handler 34
handler 35
handler 36
handler 37
handler 38
handler 39
handler 40
handler 41
handler 42
handler 43
handler 44
handler 45
handler 46
handler 47
handler 48
handler 49
handler 50
handler 51
handler 52
handler 53
handler 54
handler 55
handler 56
handler 57
handler 58
handler 59
handler 60
handler 61
handler 62
handler 63
handler 64
handler 65
handler 66
handler 67
handler 68
handler 69
handler 70
handler 71
handler 72
handler 73
handler 74
handler 75
handler 76
handler 77
handler 78
handler 79
handler 80
handler 81
handler 82
handler 83
handler 84
handler 85
handler 86
handler 87
handler 88
handler 89
handler 90
handler 91
handler 92
handler 93
handler 94
handler 95
handler 96
handler 97
handler 98
handler 99
handler 100
handler 101
handler 102
handler 103
handler 104
handler 105
handler 106
handler 107
handler 108
handler 109
handler 110
handler 111
handler 112
handler 113
handler 114
handler 115
handler 116
handler 117
handler 118
handler 119
handler 120
handler 121
handler 122
handler 123
handler 124
handler 125
handler 126
handler 127
handler 128
handler 129
handler 130
handler 131
handler 132
handler 133
handler 134
handler 135
handler 136
handler 137
handler 138
handler 139
handler 140
handler 141
handler 142
handler 143
handler 144
handler 145
handler 146
handler 147
handler 148
handler 149
handler 150
handler 151
handler 152
handler 153
handler 154
handler 155
handler 156
handler 157
handler 158
handler 159
handler 160
handler 161
handler 162
handler 163
handler 164
handler 165
handler 166
handler 167
handler 168
handler 169
handler 170
handler 171
handler 172
handler 173
handler 174
handler 175
handler 176
handler 177
handler 178
handler 179
handler 180
handler 181
handler 182
handler 183
handler 184
handler 185
handler 186
handler 187
handler 188
handler 189
handler 190
handler 191
handler 192
handler 193
handler 194
handler 195
handler 196
handler 197
handler 198
handler 199
handler 200
handler 201
handler 202
handler 203
handler 204
handler 205
handler 206
handler 207
handler 208
handler 209
handler 210
handler 211
handler 212
handler 213
handler 214
handler 215
handler 216
handler 217
handler 218
handler 219
handler 220
handler 221
handler 222
handler 223
handler 224
handler 225
handler 226
handler 227
handler 228
handler 229
handler 230
handler 231
handler 232
handler 233
handler 234
handler 235
handler 236
handler 237
handler 238
handler 239
handler 240
handler 241
handler 242
handler 243
handler 244
handler 245
handler 246
handler 247
handler 248
handler 249
handler 250
handler 251
handler 252
handler 253
handler 254
handler 255

TRAP_MESSAGE: db "TRAP!", 0x0a, 0
INTERRUPT_MESSAGE: db "INTERRUPT!", 0x0a, 0
KEYBOARD_MESSAGE: db "IN KEYBOARD HANDLER!", 0x0a, 0

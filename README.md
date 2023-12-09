# OSLearn

This is a hobbyist OS, written entirely from scratch.
Please **DO NOT** replace any long-standing OS with OSLearn. There are no safety guarantees provided. This is for educational reasons only.

## Bootloader

OSLearn has a custom bootloader. Right now it is simple and will probably not work on actual hardware. The bootloader does the following:
- Sets up BIOS graphics mode
- Switches to 32-bit real mode
- Initializes memory paging
- Initializes interrupts
- Passes important information to the kernel

## Kernel
The OSLearn kernel still needs heavy development. Currently, it does the following:
- Supports rudimentary format printing (under development)
- Understands and decodes keyboard strokes (under development)

## Ideal design decisions

OSLearn is developed under the philosophy that programmers should be able to reach as close to the hardware as possible. This means drivers will be avoided where practical. Instead, some user space programs will be used to provide default handling of hardware. The user can choose to bypass them if needed.

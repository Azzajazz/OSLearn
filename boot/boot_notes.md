# Browser Design Notes

## Stage 1 Responsibilities

Base address: 0x7C00

- Loading stage 2
- Loading GDT from disk

## Stage 2 Responsibilities

Base address: 0xF00

- Initialise the GDTR
- Enable A20 line
- Switch to protected mode
- Initialise the PIC
- Initialise the IDT
- Provide default interrupt handlers

### TODO:
- Load the kernel and far jump

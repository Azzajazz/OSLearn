@echo off

if not exist build mkdir build

echo [Build]: Building bootloader...
CustomTools\NASM\nasm.exe -f bin stage1.asm -o build\boot.bin
fsutil file seteof build\boot.bin 1474560

echo [Build]: Building kernel binary...
CustomTools\NASM\nasm.exe -f win32 kernel_entry.asm -o build\kernel_entry.o
g++ -Wall -Wextra -Werror -ffreestanding -nostdlib -fno-exceptions -fno-rtti -nostartfiles -c -m32 -o build\kernel.o kernel.cpp
ld -Map build\kernel.map -T link.ld -m i386pe -o build\kernel.tmp build\kernel_entry.o build\kernel.o
objcopy -O binary build\kernel.tmp build\kernel.bin
REM del kernel.tmp

echo [Build]: Setting up floppy file system...
CustomTools\tfs.exe format "build/boot.bin"
CustomTools\tfs.exe write "build/boot.bin" "build/kernel.bin"

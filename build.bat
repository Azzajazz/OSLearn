@echo off


set handmadeToolsDir=tools\handmade\build\
set nasmFlags=-Wall -Werror -i . -i boot
set nasmExe=tools\NASM\nasm.exe
set wccFlags=-q -d3 -mc -wx -we -zl -s -ecc -i=tools\Watcom\h
set gccFlags=-ffreestanding -Wall -Wpedantic -Wextra -Werror -Wno-unused -c
set mingwPath=C:\MinGW\bin\

pushd W:\
if not exist build\ (
    mkdir build
)

REM Compile stage2.bin
%nasmExe% %nasmFlags% -o build\stage2.obj -fobj boot\stage2.asm
wlink name build\stage2.bin file build\stage2.obj @boot\stage2.lnk

REM Compile kernel.bin
REM @TODO: Make this more automated
%mingwPath%gcc %gccFlags% -o build\main.obj kernel\main.c
%mingwPath%gcc %gccFlags% -o build\string.obj kernel\string.c
%mingwPath%gcc %gccFlags% -o build\print.obj kernel\print.c
%mingwPath%gcc %gccFlags% -o build\x86_c.obj kernel\x86.c
%mingwPath%gcc %gccFlags% -o build\keyboard.obj kernel\keyboard.c
%nasmExe% %nasmFlags% -o build\kernel_entry.obj -fwin32 kernel\kernel_entry.asm
%nasmExe% %nasmFlags% -o build\interrupts.obj -fwin32 kernel\interrupts.asm
%nasmExe% %nasmFlags% -o build\x86.obj -fwin32 kernel\x86.asm
%mingwPath%ld -o build\kernel.tmp build\main.obj build\kernel_entry.obj build\string.obj build\print.obj build\interrupts.obj build\x86.obj build\x86_c.obj build\keyboard.obj -T kernel\kernel.lnk -Map build\kernel.map -LC:\MinGW\lib\gcc\mingw32\6.3.0 -lgcc
%mingwPath%objcopy -O binary build\kernel.tmp build\kernel.bin

REM Preparing the boot image
%nasmExe% %nasmFlags% -fobj -o build\stage1.obj boot\stage1.asm
wlink name build\boot.img file build\stage1.obj @boot\stage1.lnk

fsutil file seteof build\boot.img 1474560
%handmadeToolsDir%mkfat12 build\boot.img
%handmadeToolsDir%cpfat12 build\boot.img build\stage2.bin
%handmadeToolsDir%cpfat12 build\boot.img build\kernel.bin
%handmadeToolsDir%write_gdt build\boot.img
popd

@echo off

pushd W:\

if "%~1"=="b" (
    .\tools\bochs-2.7\obj-release\bochs.exe
)
if "%~1"=="q" (
    .\tools\qemu\qemu-system-i386.exe -fda build\boot.img -m 4096 -s -S
)

popd

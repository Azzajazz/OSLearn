@echo off

pushd W:\tools\handmade
cl -nologo -Wall -WX -Zi -Fd:build\mkfat12 -Fo:build\mkfat12 -Fe:build\mkfat12 mkfat12.c
cl -nologo -Wall -WX -Zi -Fd:build\cpfat12 -Fo:build\cpfat12 -Fe:build\cpfat12 cpfat12.c
cl -nologo -Wall -WX -Zi -Fd:build\write_gdt -Fo:build\write_gdt -Fe:build\write_gdt write_gdt.c
cl -nologo -Wall -WX -Zi -Fd:build\parse_elf -Fo:build\parse_elf -Fe:build\parse_elf parse_elf.c -wd4200 -wd5045
popd

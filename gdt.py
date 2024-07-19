base = int(input("Base address: "))
limit = int(input("Limit: "), 16)
privelege_level = int(input("Privelege level (0 = highest to 3 = lowest): "))
descriptor_type = 0 if input("Descriptor type: ") == "system" else 1
executable = 1 if input("Executable (Y/N): ") == "Y" else 0
direction_or_conforming = int(input("Direction bit (data) or conforming bit (code): "))
readable_or_writable = 1 if input("Readable (code) or writable (data) (Y/N): ") == "Y" else 0
block_granularity = 1 if input("4KiB block granularity (Y/N): ") == "Y" else 0
size = 1 if input("32-bit size - otherwise 16-bit size (Y/N): ") == "Y" else 0

print(f"dw 0x{limit & 0xffff:04x}")
print(f"dw 0x{base & 0xffff:04x}")
print(f"db 0x{base >> 16:02x}")
print(f"db 0b1{privelege_level:02b}{descriptor_type:01b}{executable:01b}{direction_or_conforming:01b}{readable_or_writable:01b}0")
print(f"db 0b{block_granularity:01b}{size:01b}00{limit >> 16:04b}")
print(f"db 0x{base >> 24:02x}")

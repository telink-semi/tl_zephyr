KEEP(*(.ram_code))
KEEP(*(".ram_code.*"))
KEEP(*(.ram_code_ble))
KEEP(*(".ram_code_ble.*"))

_ramcode_size_ = . - 0x80000000;
_ramcode_size_div_16_ = (. - 0x80000000  + 15 ) / 16;
_ramcode_size_div_256_ = (.  - 0x80000000 + 255) / 256;
_ramcode_size_div_16_align_256_ = ( (. - 0x80000000 + 255) / 256) * 16;
_ramcode_size_align_256_ = ( (.  - 0x80000000  + 255) / 256) * 256;
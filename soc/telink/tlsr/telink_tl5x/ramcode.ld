. = ALIGN(4);
KEEP(*(.ram_code))
KEEP(*(".ram_code.*"))
KEEP(*(.ram_code_ble))
KEEP(*(".ram_code_ble.*"))

_ramcode_size_ = . - 0x80000000;
/* _ramcode_size_div_16_ = (. - 0x80000000  + 15 ) / 16; */
/* _ramcode_size_div_256_ = (.  - 0x80000000 + 255) / 256; */
_ramcode_size_div_16_align_256_ = (( ((_ramcode_size_ & 0x7FFFFFFF) + 255) / 256) * 16) - 0x80000000;
/* _ramcode_size_div_16_align_256_ = ( (. - 0x80000000 + 255) / 256) * 16; */
_ramcode_size_align_256_ = ( (.  - 0x80000000  + 255) / 256) * 256;
_ictag_start_ = 0x80840000 + _ramcode_size_align_256_ ;
_ictag_end_ = 0x80840000 + _ramcode_size_align_256_ + 0x100;
. = _ramcode_size_align_256_  + 0x80000000 ;
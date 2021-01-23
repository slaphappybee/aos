#pragma pack(push)
#pragma pack(1)
struct idt_entry {
    idt_entry() = default;
    
    idt_entry(void (*handler)())
    : offset_lo(reinterpret_cast<uint32_t>(handler) & 0xffff)
    , selector(0x20)
    , zero()
    , type_attr(0x8e)
    , offset_hi(reinterpret_cast<uint32_t>(handler) >> 16)
    {}

    uint16_t offset_lo;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_hi;
};
#pragma pack(pop)

void remap_pic() {
    outb(0x11, 0x20);
    outb(0x11, 0xA0);
    outb(0x20, 0x21);
    outb(40,   0xA1);
    outb(0x04, 0x21);
    outb(0x02, 0xA1);
    outb(0x01, 0x21);
    outb(0x01, 0xA1);
    outb(0x0,  0x21);
    outb(0x0,  0xA1);
}

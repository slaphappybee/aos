namespace as {
    constexpr uint8_t pusha = 0x60;
    constexpr uint16_t lcall_imm = 0x1dff;
    constexpr uint16_t popa_iret = 0xcf61;
}

template<uint8_t IRQ>
void irq_handler() {
    outb(0x20, 0x20);
}

template<> void irq_handler<8>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<9>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<10>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<11>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<12>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<13>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<14>() { outb(0x20, 0xa0); outb(0x20, 0x20); }
template<> void irq_handler<15>() { outb(0x20, 0xa0); outb(0x20, 0x20); }

#pragma pack(push)
#pragma pack(1)
struct irq_stub_t {
    uint8_t pusha;
    uint16_t lcall;
    void (*target)();
    uint16_t popa_iret; 
};

struct idt_entry {
    idt_entry() = default;
    idt_entry(irq_stub_t const *stub)
    : offset_lo(reinterpret_cast<uint32_t>(stub) & 0xffff)
    , selector(0x08)
    , zero()
    , type_attr(0x8e)
    , offset_hi(reinterpret_cast<uint32_t>(stub) >> 16)
    {}

    uint16_t offset_lo;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_hi;
};
#pragma pack(pop)

template<uint8_t IRQ>
irq_stub_t irq_stub {as::pusha, as::lcall_imm, &irq_handler<IRQ>, as::popa_iret};

void init_idt(idt_entry* table) {
    table[32] = idt_entry(&irq_stub<0>);
    table[33] = idt_entry(&irq_stub<1>);
    table[34] = idt_entry(&irq_stub<2>);
    table[35] = idt_entry(&irq_stub<3>);
    table[36] = idt_entry(&irq_stub<4>);
    table[37] = idt_entry(&irq_stub<5>);
    table[38] = idt_entry(&irq_stub<6>);
    table[39] = idt_entry(&irq_stub<7>);
    table[40] = idt_entry(&irq_stub<8>);
    table[41] = idt_entry(&irq_stub<9>);
    table[42] = idt_entry(&irq_stub<10>);
    table[43] = idt_entry(&irq_stub<11>);
    table[44] = idt_entry(&irq_stub<12>);
    table[45] = idt_entry(&irq_stub<13>);
    table[46] = idt_entry(&irq_stub<14>);
    table[47] = idt_entry(&irq_stub<15>);
}

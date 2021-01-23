#define FB ((volatile char*)0xb8000)
#define FB_WIDTH 80
#include <sys/io.h>
#include <cstdint>
#include <vector>
#include "irq.h++"

void con_setcurpos(uint16_t off) {
    outb(0x0F, 0x3D4);
	outb((off & 0xFF), 0x3D5);
	outb(0x0E, 0x3D4);
	outb(((off >> 8) & 0xFF), 0x3D5);
}

uint16_t con_getcurpos() {
    uint16_t pos = 0;
    outb(0x0F, 0x3D4);
    pos |= inb(0x3D5);
    outb(0x0E, 0x3D4);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos;
}


void cls() {
    for(uint16_t i=0; i<80*25; i+=2) {
        FB[i] = 0;
        FB[i+1] = 0x08;
    }
    con_setcurpos(0);
}

void kprint(const char* buf) {
    uint16_t curpos = con_getcurpos();
    uint16_t i=0;
    for (; buf[i]; i++) {
        if (buf[i] == '\n') {
            curpos = curpos - (curpos % FB_WIDTH) + FB_WIDTH;
        } else {
            FB[curpos*2] = buf[i];
            curpos++;
        }
    }
    con_setcurpos(curpos);
}

void sprinthex(char *buf, size_t digits, uint32_t val) {
    const char *hex = "0123456789abcdef";
    
    size_t i = 0;
    for(i=0; i<digits; i++) {
        buf[digits-1-i] = hex[(val >> (4 * i)) & 0xf];
    }
}

void kprinthex16(uint16_t val) {
    char buf[7] = "0x0000";
    sprinthex(&buf[2], 4, val);
    
    kprint(buf);
}

void kprinthex32(uint32_t val) {
    char buf[11] = "0x00000000";
    sprinthex(&buf[2], 8, val);
    
    kprint(buf);
}

void hexdump_buf(uint8_t const *data, size_t size) {
    char buffer[3] = "00";
    size_t i;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0) {
            char widebuf[11] = "00000000: ";
            sprinthex(widebuf, 8, i);
            kprint(widebuf);
        }
        sprinthex(buffer, 2, data[i]);
        kprint(buffer);
        if (i % 2 == 1) kprint(" ");
        if (i % 16 == 15) kprint("\n");
    }
    
    if (i % 16 != 15) kprint("\n");
}

template<class T>
void hexdump(T const *object, size_t size) {
    hexdump_buf(reinterpret_cast<uint8_t const *>(object), size);
}

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA 0xCFC

uint16_t pci_cfg_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address;
    uint32_t lbus  = (uint32_t)bus;
    uint32_t lslot = (uint32_t)slot;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    /* create configuration address as per Figure 1 */
    address = (uint32_t)((lbus << 16) | (lslot << 11) |
              (lfunc << 8) | (offset & 0xfc) | ((uint32_t)0x80000000));
 
    /* write out the address */
    outl(address, PCI_CONFIG_ADDRESS);
    /* read in the data */
    /* (offset & 2) * 8) = 0 will choose the first word of the 32 bits register */
    tmp = (uint16_t)((inl(PCI_CONFIG_DATA) >> ((offset & 2) * 8)) & 0xffff);
    return (tmp);
}

void pci_describe(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t class_code;
    uint16_t header_type;
    uint16_t id;
    vendor_id = pci_cfg_read_word(bus, device, 0, 0);
    if(vendor_id == 0xFFFF) return;

    device_id = pci_cfg_read_word(bus, device, 0, 2);
    class_code = pci_cfg_read_word(bus, device, 0, 10);
    
    header_type = pci_cfg_read_word(bus, device, 0, 14);
    
    kprint("Id: ");
    id = (bus << 8) | (device << 4) | function;
    kprinthex16(id);
    kprint(", vendor: ");
    kprinthex16(vendor_id);
    kprint(", device: ");
    kprinthex16(device_id);
    kprint(", class: ");
    kprinthex16(class_code);
    kprint(", header: ");
    kprinthex16(header_type);
    kprint("\n");
    
//     if((function == 0) && (header_type & 0x80)) {
//         uint8_t function;
//         for(function = 1; function < 8; function++) {
//             pci_describe(bus, device, function);
//         }
//     }
}
 
void pci_enumerate(void) {
     uint16_t bus;
     uint8_t device;
    
     for(bus = 0; bus < 256; bus++) {
         for(device = 0; device < 32; device++) {
             pci_describe(bus, device, 0);
         }
     }
}

#define PIO_MASTER_IO_DATA         0x01f0
#define PIO_MASTER_IO_SECTOR_COUNT 0x01f2
#define PIO_MASTER_IO_LBA_LO       0x01f3
#define PIO_MASTER_IO_LBA_MID      0x01f4
#define PIO_MASTER_IO_LBA_HI       0x01f4
#define PIO_MASTER_IO_DRIVE_HEAD   0x01f6
#define PIO_MASTER_IO_COMMAND      0x01f7

#define PIO_MASTER_CTRL_DEVICE     0x03f6
#define PIO_MASTER_CTRL_DRV_ADDR   0x03f7

#define PIO_COMMAND_READ_SECTORS 0x20

#define PIO_STATUS_DATA_REQUEST 0x08
#define PIO_STATUS_BUSY         0x80

void pio_poll_ready() {
    uint8_t status;
    uint8_t ready;
    do {
        status = inb(PIO_MASTER_IO_COMMAND);
        ready = (PIO_STATUS_DATA_REQUEST & 0x8) && !(status & PIO_STATUS_BUSY); 
    } while(!ready);
}

void pio_read_block(uint32_t lba_block, uint8_t sector_count, uint16_t *buffer) {
    outb(0, PIO_MASTER_CTRL_DEVICE);
    
    uint8_t set_drive_head_cmd = 0xe0 | ((lba_block >> 24) & 0x0f);
    outb(set_drive_head_cmd, PIO_MASTER_IO_DRIVE_HEAD);
    outb(sector_count, PIO_MASTER_IO_SECTOR_COUNT);

    outb((uint8_t)(lba_block & 0x0f), PIO_MASTER_IO_LBA_LO);
    outb((uint8_t)((lba_block >> 8) & 0x0f), PIO_MASTER_IO_LBA_MID);
    outb((uint8_t)((lba_block >> 16) & 0x0f), PIO_MASTER_IO_LBA_HI);
    outb(PIO_COMMAND_READ_SECTORS, PIO_MASTER_IO_COMMAND);
    
    pio_poll_ready();
    for(uint16_t i = 0; i < sector_count * 256; i++) {
        buffer[i] = inl(PIO_MASTER_IO_DATA);
    }
}

char *mem_base = reinterpret_cast<char *>(0x200000);

void *kmalloc(std::size_t size) {
    char *buf_start = mem_base;
    mem_base = mem_base + size;
    return buf_start;
}

void kfree(void *ptr) {
}


template<class T>
struct kallocator {
    typedef T value_type;
    
    T* allocate(std::size_t size) {
        return static_cast<T *>(kmalloc(size));
    }
    
    void deallocate(T *value, std::size_t size) {
        kfree(value);
    }
};

template<class T>
using kvector =  std::vector<T, kallocator<T>>;

void load_idt(idt_entry *idt) {
    uint32_t idt_base = reinterpret_cast<uint32_t>(idt);
    uint32_t idt_ptr[2];
    idt_ptr[0] = ((sizeof(idt_entry) * 256) - 1) | (idt_base << 16);
    idt_ptr[1] = idt_base >> 16;

    hexdump(idt_ptr, 8);
    
    __asm__ (
        "lidtl (%0)"
    :: "r"(idt_ptr)
    );
}

template<uint8_t IRQ>
void generic_irq_handler() {
    kprinthex16(IRQ);
    for(;;);
}

void irq_exception() {
    kprint("Exception");
    for(;;);
}

void irq_gpf(uint32_t eip) {
    kprint("General Protection Fault at ");
    kprinthex32(eip);
    for(;;);
}

void irq_timer() {
    //__asm__("pusha");
    //kprint("Timer\n");
    //__asm__("popa");
    __asm__("iret");
}

void irq_syscall() {
    kprint("Syscall\n");
}

void init_idt(idt_entry *idt) {
    for(size_t i = 0; i < 32; i++)
        idt[i] = idt_entry(irq_exception);
    
    for(size_t i = 32; i < 256; i++)
        idt[i] = idt_entry(generic_irq_handler<255>);
    
    idt[13] = idt_entry(reinterpret_cast<void (*)()>(reinterpret_cast<void*>(irq_gpf)));
    idt[32] = idt_entry(irq_timer);
    idt[0x80] = idt_entry(irq_syscall);
}

extern "C"
void kmain() {
    cls();

    kprint("AOS version 0.1 starting\n");
    kprint("Enumerating PCI devices \n");
    pci_enumerate();
    
    kprint("Preparing interrupts\n");
    kvector<idt_entry> idt(256);
    
    remap_pic();
    init_idt(idt.data());
    load_idt(idt.data());
    
    __asm__("int $0x80");
    
    kprint("Enabling interrupts\n");
    __asm__("sti");
    
    //hexdump(idt.data(), 64);
    
    //kprint("Back from the exception\n");
    
    //kprint("Reading 1 block from PIO\n");
    
    //kvector<uint16_t> buffer(256);
    //pio_read_block(0, 1, buffer.data());
    
    //kprint("...OK\n");
    //kprinthex16(buffer[0]);
    
    for(;;);
}

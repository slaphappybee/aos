#include <sys/io.h>
#include <cstdint>
#include <vector>
#include "irq.h++"
#include "console.h++"
#include "pci.h++"

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
        return static_cast<T *>(kmalloc(size * sizeof (T)));
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

void irq_gpf(uint32_t eip, uint32_t sgt) {
    kprint("General Protection Fault at ");
    kprinthex32(eip);
    kprint(" due to: ");
    kprinthex32(sgt);
    for(;;);
}

void irq_eopcode(uint32_t eip) {
    kprint("Invalid Opcode at ");
    kprinthex32(eip);
    for(;;);
}

extern "C"
void irq_timer_thunk();

extern "C"
void irq_timer() {
    kprint("Timer\n");
    outb(0x20, 0x20);
}

extern "C"
void irq_ata_thunk();

extern "C"
void irq_ata() {
    kprint("ATA\n");
    outb(0x20, 0x20);
}

void irq_syscall() {
    kprint("Syscall\n");
}

void init_idt(idt_entry *idt) {
    for(size_t i = 0; i < 32; i++)
        idt[i] = idt_entry(irq_exception);

    idt[0x06] = idt_entry(reinterpret_cast<void (*)()>(reinterpret_cast<void*>(irq_eopcode)));;
    
    for(size_t i = 32; i < 256; i++)
        idt[i] = idt_entry(generic_irq_handler<255>);
    
    idt[0x0d] = idt_entry(reinterpret_cast<void (*)()>(reinterpret_cast<void*>(irq_gpf)));
    idt[0x20] = idt_entry(irq_timer_thunk);
    idt[0x2e] = idt_entry(irq_ata_thunk);
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
    
    kprint("Reading 1 block from PIO\n");
    
    hexdump(&idt[0x20], sizeof (idt_entry));
    kvector<uint16_t> buffer(256);
    pio_read_block(0, 1, buffer.data());
    
    kprint("...OK\n");
    hexdump(&idt[0x20], sizeof (idt_entry));
    
    for(;;);
}

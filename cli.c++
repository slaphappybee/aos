#define FB ((volatile char*)0xb8000)
#define FB_WIDTH 80
#include <sys/io.h>
#include <stdint.h>

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

void kprinthex16(uint16_t val) {
    char buf[7] = "0x0000";
    const char *hex = "0123456789abcdef";
    
    uint8_t i = 0;
    for(i=0; i<4; i++) {
        buf[5-i] = hex[(val >> (4 * i)) & 0xf];
    }
    
    kprint(buf);
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

extern "C"
void kmain() {
    uint16_t buffer[256];
    
    cls();

    kprint("AOS version 0.1 starting\n");
    kprint("Enumerating PCI devices \n");
    pci_enumerate();
    
    kprint("Reading 1 block from PIO\n");
    pio_read_block(0, 1, buffer);
    
    kprint("...OK\n");
    kprinthex16(buffer[0]);
    for(;;);
}

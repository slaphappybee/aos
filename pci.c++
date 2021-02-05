#include "pci.h++"
#include "console.h++"

#include <sys/io.h>

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

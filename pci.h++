#include <cstdint>

uint16_t pci_cfg_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void pci_describe(uint8_t bus, uint8_t device, uint8_t function);
void pci_enumerate(void);

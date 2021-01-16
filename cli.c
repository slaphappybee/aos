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

void kprint(const char* buf)
{
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

void kmain() {
    cls();

    kprint("AOS version 0.1 starting\n");
    for(;;);
}

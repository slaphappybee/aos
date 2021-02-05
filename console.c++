#include "console.h++"

#include <sys/io.h>

#define FB ((volatile char*)0xb8000)
#define FB_WIDTH 80

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


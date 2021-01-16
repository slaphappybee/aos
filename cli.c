#define FB ((char*)0xb8000)

void kmain() {
    FB[0] = 'K';
    for(;;);
}

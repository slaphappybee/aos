#include <cstddef>
#include <cstdint>

void cls();
void hexdump_buf(uint8_t const *data, size_t size);
void kprinthex16(uint16_t val);
void kprinthex32(uint32_t val);
void kprint(const char* buf);

template<class T>
void hexdump(T const *object, size_t size) {
    hexdump_buf(reinterpret_cast<uint8_t const *>(object), size);
}

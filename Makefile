CPP = g++
CFLAGS = -O2 -m32 -fno-pie -fno-stack-protector -std=c++20
AS = as
ASFLAGS = -msyntax=intel -mnaked-reg --32

all: image.bin

boot.o: boot.S
	as -msyntax=intel -mnaked-reg boot.S -o boot.o

boot.bin: boot.o
	ld -o boot.bin --oformat binary -e init boot.o --section-start=.text=0x0

.S.o:
	as $(ASFLAGS) $< -o $@

kernel.elf: kernel.o cli.o console.o pci.o irq_thunk.o kbd.o
	ld -o kernel.elf -e start $^ -T kernel.ld -melf_i386 --gc-sections

image.bin: kernel.elf boot.bin
	cat boot.bin kernel.elf > image.bin
	./pad.sh 12288 image.bin

test: image.bin
	qemu-system-x86_64 -drive file=image.bin,if=ide,format=raw

%.o: %.c++ *.h++
	$(CPP) -c $< -o $@ $(CFLAGS)

clean:
	rm *.o *.bin

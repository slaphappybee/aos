all: image.bin

boot.o: boot.S
	as -msyntax=intel -mnaked-reg boot.S -o boot.o

boot.bin: boot.o
	ld -o boot.bin --oformat binary -e init boot.o --section-start=.text=0x0

kernel.o: kernel.S
	as -msyntax=intel -mnaked-reg kernel.S -o kernel.o --32

irq_thunk.o: irq_thunk.S
	as -msyntax=intel -mnaked-reg irq_thunk.S -o irq_thunk.o --32

kernel.bin: kernel.o cli.o console.o pci.o
	ld -o kernel.bin --oformat binary -e start kernel.o cli.o console.o pci.o -T kernel.ld -melf_i386 --gc-sections

kernel.elf: kernel.o cli.o console.o pci.o irq_thunk.o
	ld -o kernel.elf -e start kernel.o cli.o console.o pci.o irq_thunk.o -T kernel.ld -melf_i386 --gc-sections

image.bin: kernel.elf boot.bin
	cat boot.bin kernel.elf > image.bin
	./pad.sh 12288 image.bin

test: image.bin
	qemu-system-x86_64 -drive file=image.bin,if=ide,format=raw

objdump: kernel.o
	objdump -D kernel.o -M i8086 -M intel

cli.o: cli.c++ irq.h++
	g++ -c cli.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20

console.o: *.h++ console.c++
	g++ -c console.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20

pci.o: *.h++ pci.c++
	g++ -c pci.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20
	
clean:
	rm *.o *.bin

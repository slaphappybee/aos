all: image.bin

boot.o: boot.S
	as -msyntax=intel -mnaked-reg boot.S -o boot.o

boot.bin: boot.o
	ld -o boot.bin --oformat binary -e init boot.o --section-start=.text=0x0

kernel.o: kernel.S
	as -msyntax=intel -mnaked-reg kernel.S -o kernel.o --32

kernel.bin: kernel.o cli.o console.o pci.o
	ld -o kernel.bin --oformat binary -e start kernel.o cli.o console.o pci.o -T kernel.ld -melf_i386 --gc-sections

image.bin: kernel.bin boot.bin
	cat boot.bin kernel.bin > image.bin

test: image.bin
	qemu-system-x86_64 -drive file=image.bin,if=ide,format=raw

objdump: kernel.o
	objdump -D kernel.o -M i8086 -M intel

cli.o: cli.c++ irq.h++
	g++ -c cli.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20
	objcopy --remove-section=.comment --remove-section=.eh_frame cli.o cli.o

console.o: *.h++ console.c++
	g++ -c console.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20
	objcopy --remove-section=.comment --remove-section=.eh_frame console.o console.o

pci.o: *.h++ pci.c++
	g++ -c pci.c++ -O2 -m32 -fno-pie -fno-stack-protector -std=c++20
	objcopy --remove-section=.comment --remove-section=.eh_frame pci.o pci.o
	
clean:
	rm *.o *.bin

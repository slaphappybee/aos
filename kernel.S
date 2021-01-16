.code16
.global start

start:
  jmp program

mesg_booting:
  .asciz "Booting AOS"

gdt:
  .quad 0x0000000000000000
  .quad 0x008F9A000000FFFF  # 0x08 16bit Code segment
  .quad 0x008F92000000FFFF  # 0x10 16bit Data segment
  .quad 0x008F920B8000FFFF  # 0x18 16bit Video buffer
  .quad 0x00CF9A000000FFFF  # 0x20 32bit Code segment
  .quad 0x00CF92000000FFFF  # 0x28 32bit Data segment

gdt_desc:
  .word gdt_desc-gdt
  .word OFFSET gdt
  .word 0  # Double word

program:
  mov di, 0
  mov si, OFFSET mesg_booting
  call putstr

# Enter protected mode
  # Disable interrupts
  cli 
  
  # Fast A20 Gate
  in al, 0x92
  or al, 2
  out 0x92, al
  
  # Clear DS
  mov ax, 0
  mov ds, ax
  
  # LGDT
  .word 0x010f
  .byte 0b00010110
  .word OFFSET gdt_desc
  
  # Set CR0/0
  mov eax, cr0
  or eax, 1
  mov cr0, eax

  # Long jump
  .byte 0x66
  .byte 0xea
  .word OFFSET protected32
  .word 0
  .word 0x20
  .word 0

protected16:
  mov ax, 0x18
  mov ds, ax  # Video buffer
  mov al, 'X'
  mov di, 0
  mov [di], al
  jmp loop

.code32
protected32:
  mov ax, 0x28
  mov ds, ax
  mov ss, ax
  mov gs, ax
  mov fs, ax
  mov edi, 0xb8000
  mov al, 'Y'
  mov [edi], al
  mov esp, 0x090000
  call kmain
  
loop:
  jmp loop

.code16

putstr:
  # Writes cstr si to di
  mov al, [si]
  test al, al
  jz putstr_end
  call putchar
  inc si
  inc di
  inc di
  jmp putstr
putstr_end:
  ret

putchar:
  # Writes al to screen at offset di
  mov bx, 0xb800
  mov ds, bx
  mov ah, 07
  mov [di], ax
  mov bx, 0x0
  mov ds, bx
  ret

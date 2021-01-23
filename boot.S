.code16
.global init

init:
  mov ah, 0x05
  mov al, 0
  int 0x10

  mov ah, 0x09
  mov al, 'X'
  mov bh, 0
  mov bl, 0x07
  mov cx, 10
  int 0x10

  mov ax, 0
  mov es, ax

  mov ah, 0x02
  mov al, 40  # No of sectors
  mov cx, 2  # Cylinder/sector
  mov dh, 0  # Head
  mov dl, 0x80  # HD 0
  mov bx, 0x8000
  int 0x13
  
success:
  mov ah, 0x0e
  mov al, 'Y'
  int 0x10

  mov sp, 0x6000
  # Absolute jump
  .byte 0xea
  .word 0x8000

.fill 510-(.-init)
.word 0xaa55

.section text-kernel
.org 0x8000

start:
  jmp program

mesg:
  .asciz "Booting AOS"

program:
  mov ah, 0x0e
  mov al, 'O'
  int 0x10
  mov al, 'C'
  mov di, 0
  call putchar_real

  mov di, 2
  mov si, mesg
  call putstr_real

loop:
  jmp loop

putstr_real:
  # Writes cstr si to di
  mov al, [si]
  test al, al
  jz putstr_real_end
  call putchar_real
  inc si
  inc di
  inc di
  jmp putstr_real
putstr_real_end:
  ret

putchar_real:
  # Writes al to screen at offset di
  mov bx, 0xb800
  mov ds, bx
  mov ah, 07
  mov [di], ax
  mov bx, 0x0
  mov ds, bx
  ret


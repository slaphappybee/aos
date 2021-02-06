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
  
  # ELF file at 0x200 + EOF Offset 0x1000 -> Cylinder 9 -> cl=10
  mov cx, 10  # Cylinder/sector
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

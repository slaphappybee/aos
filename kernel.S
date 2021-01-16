.code16
.global start

start:
  jmp program

mesg:
  .asciz "Booting AOS"

program:
  mov di, 0
  mov si, OFFSET mesg
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

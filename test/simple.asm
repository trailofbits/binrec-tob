bits 32
cpu 386

extern puts
global main

section .rodata
message:    db "args detected", 0

section .text
main:
    mov  eax, [esp + 4]
    jmp  .start
.start:
    cmp  eax, 1
    jle  .fail
.printmessage:
    push message
    call puts
    add esp, 4
    mov  eax, 0
    jmp  .end
.fail:
    mov  eax, 1
.end:
    ret

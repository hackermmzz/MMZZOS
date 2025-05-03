[bits 32]
extern main
extern exit
section .text
global _start
_start:
    push ecx
    push ebx
    call main

    call exit
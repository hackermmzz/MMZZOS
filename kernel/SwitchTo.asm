[bits 32]
section .text
global SwitchTo
SwitchTo:
    push esi
    push edi
    push ebx
    push ebp
    mov eax,[esp+20] ;cur
    mov [eax],esp ;更新栈指针

    mov eax,[esp+24] ;next
    mov esp,[eax] ;切换栈指针
    pop ebp
    pop ebx
    pop edi
    pop esi
    ret

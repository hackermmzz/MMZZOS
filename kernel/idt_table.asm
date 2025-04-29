[bits 32]
extern interrupt_table
section .data
global interrupt_entry_table
interrupt_entry_table:
%macro Table 2
section .text
interrupt_%1_entry:
    ;压入错误码(如果需要)
    %2
    ;保存上下文
    push ds
    push es
    push fs
    push gs
    pushad
    ;中断处理完成
    mov al,0x20
    out 0xa0,al
    out 0x20,al
    ;调用处理函数
    push %1
    call [interrupt_table+%1*4]
    jmp itr_exit
section .data
    dd interrupt_%1_entry
%endmacro
section .text
global itr_exit
itr_exit:
    add esp,4
    ;恢复上下文
    popad
    pop gs
    pop fs
    pop es
    pop ds
    ;弹出错误码
    add esp,4
    ;
    iretd
;系统调用函数
[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
    push 0x119
    push ds
    push es
    push fs
    push gs
    pushad
    ;压入中断号
    push 0x80
    ;压入三个参数
    push edx
    push ecx
    push ebx
    call [syscall_table+eax*4]
    add esp,12
    ;返回值存入内核栈的eax内存区域
    mov [esp+8*4],eax
    ;返回
    jmp itr_exit
%define NOP nop
%define ERROR_CODE push 0x119;鄙人生日

Table 0x0 ,ERROR_CODE
Table 0X1 ,ERROR_CODE
Table 0X2 ,ERROR_CODE
Table 0x3 ,ERROR_CODE
Table 0X4 ,ERROR_CODE
Table 0X5 ,ERROR_CODE
Table 0x6 ,ERROR_CODE
Table 0X7 ,ERROR_CODE
Table 0X8 ,NOP
Table 0x9 ,ERROR_CODE
Table 0XA ,NOP
Table 0XB ,NOP
Table 0XC ,NOP
Table 0XD ,NOP
Table 0XE ,NOP
Table 0XF ,ERROR_CODE
Table 0X10 ,ERROR_CODE
Table 0X11 ,NOP
Table 0x12 ,ERROR_CODE
Table 0X13 ,ERROR_CODE
Table 0X14 ,ERROR_CODE
Table 0x15 ,ERROR_CODE
Table 0X16 ,ERROR_CODE
Table 0X17 ,ERROR_CODE
Table 0X18 ,ERROR_CODE
Table 0X19 ,ERROR_CODE
Table 0X1A ,ERROR_CODE
Table 0X1B ,ERROR_CODE
Table 0X1C ,ERROR_CODE
Table 0X1D ,ERROR_CODE
Table 0X1E ,NOP                               
Table 0X1F ,ERROR_CODE					
Table 0X20 ,ERROR_CODE	
Table 0X21 ,ERROR_CODE					;键盘中断
Table 0X22 ,ERROR_CODE					;级联
Table 0X23 ,ERROR_CODE					;串口2
Table 0X24 ,ERROR_CODE					;串口1
Table 0X25 ,ERROR_CODE					;并口2
Table 0X26 ,ERROR_CODE					;软盘
Table 0X27 ,ERROR_CODE					;并口1
Table 0X28 ,ERROR_CODE					;实时时钟
Table 0X29 ,ERROR_CODE					;重定向
Table 0X2A ,ERROR_CODE					;保留
Table 0x2B ,ERROR_CODE					;保留
Table 0x2C ,ERROR_CODE					;ps/2 鼠标
Table 0x2D ,ERROR_CODE					;fpu 浮点单元异常
Table 0x2E ,ERROR_CODE					;硬盘
Table 0x2F ,ERROR_CODE					;保留
Table 0x30 ,ERROR_CODE
Table 0x31 ,ERROR_CODE
Table 0x32 ,ERROR_CODE
Table 0x33 ,ERROR_CODE
Table 0x34 ,ERROR_CODE
Table 0x35 ,ERROR_CODE
Table 0x36 ,ERROR_CODE
Table 0x37 ,ERROR_CODE
Table 0x38 ,ERROR_CODE
Table 0x39 ,ERROR_CODE
Table 0x3A ,ERROR_CODE
Table 0x3B ,ERROR_CODE
Table 0x3C ,ERROR_CODE
Table 0x3D ,ERROR_CODE
Table 0x3E ,ERROR_CODE
Table 0x3F ,ERROR_CODE
Table 0x40 ,ERROR_CODE
Table 0x41 ,ERROR_CODE
Table 0x42 ,ERROR_CODE
Table 0x43 ,ERROR_CODE
Table 0x44 ,ERROR_CODE
Table 0x45 ,ERROR_CODE
Table 0x46 ,ERROR_CODE
Table 0x47 ,ERROR_CODE
Table 0x48 ,ERROR_CODE
Table 0x49 ,ERROR_CODE
Table 0x4A ,ERROR_CODE
Table 0x4B ,ERROR_CODE
Table 0x4C ,ERROR_CODE
Table 0x4D ,ERROR_CODE
Table 0x4E ,ERROR_CODE
Table 0x4F ,ERROR_CODE
Table 0x50 ,ERROR_CODE
Table 0x51 ,ERROR_CODE
Table 0x52 ,ERROR_CODE
Table 0x53 ,ERROR_CODE
Table 0x54 ,ERROR_CODE
Table 0x55 ,ERROR_CODE
Table 0x56 ,ERROR_CODE
Table 0x57 ,ERROR_CODE
Table 0x58 ,ERROR_CODE
Table 0x59 ,ERROR_CODE
Table 0x5A ,ERROR_CODE
Table 0x5B ,ERROR_CODE
Table 0x5C ,ERROR_CODE
Table 0x5D ,ERROR_CODE
Table 0x5E ,ERROR_CODE
Table 0x5F ,ERROR_CODE
Table 0x60 ,ERROR_CODE
Table 0x61 ,ERROR_CODE
Table 0x62 ,ERROR_CODE
Table 0x63 ,ERROR_CODE
Table 0x64 ,ERROR_CODE
Table 0x65 ,ERROR_CODE
Table 0x66 ,ERROR_CODE
Table 0x67 ,ERROR_CODE
Table 0x68 ,ERROR_CODE
Table 0x69 ,ERROR_CODE
Table 0x6A ,ERROR_CODE
Table 0x6B ,ERROR_CODE
Table 0x6C ,ERROR_CODE
Table 0x6D ,ERROR_CODE
Table 0x6E ,ERROR_CODE
Table 0x6F ,ERROR_CODE
Table 0x70 ,ERROR_CODE
Table 0x71 ,ERROR_CODE
Table 0x72 ,ERROR_CODE
Table 0x73 ,ERROR_CODE
Table 0x74 ,ERROR_CODE
Table 0x75 ,ERROR_CODE
Table 0x76 ,ERROR_CODE
Table 0x77 ,ERROR_CODE
Table 0x78 ,ERROR_CODE
Table 0x79 ,ERROR_CODE
Table 0x7A ,ERROR_CODE
Table 0x7B ,ERROR_CODE
Table 0x7C ,ERROR_CODE
Table 0x7D ,ERROR_CODE
Table 0x7E ,ERROR_CODE
Table 0x7F ,ERROR_CODE
Table 0x80 ,ERROR_CODE
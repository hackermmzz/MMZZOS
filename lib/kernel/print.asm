%include "boot.inc"
SELECTOR_VIDEO equ (0x003<<3)|0|0
[bits 32]
section .data
idx: dd 0 ;光标位置
section .text
global put_char
put_char:
    pushad
    ;装载gs选择子
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    ;获取当前光标的位置
    mov eax,[idx]
    mov ebx,[idx]
    ;对字符进行分类,如果是\r \n则换行处理,如果是backspace则回退一个字符,其他则直接打印
    mov ecx,[esp+36];获取字符
    ;回退
    cmp ecx,8;backspace
    je backspace
    ;换行
    cmp ecx,10;\n
    je newLine
    cmp ecx,13;\r
    je newLine
    ;其他字符
    jmp showchar
rollScreen:
    ;esi源 edi目的 edx剩余拷贝的数量
    mov esi,160
    mov edi,0
    ;
    rollScreenLoopBegin:
        cmp edi,3840
        je rollScreenLoopEnd
        mov eax,[gs:esi]
        mov [gs:edi],eax
        add esi,4
        add edi,4
        jmp rollScreenLoopBegin
    rollScreenLoopEnd:
    ;最后一行清空
    rollScreenClsBegin:
        cmp edi,4000
        je rollScreenClsEnd
        mov dword [gs:edi],0
        add edi,4
        jmp rollScreenClsBegin
    rollScreenClsEnd:
    ;
    add ebx,-80;字符减去80个
    jmp updateCursor

showchar:
    shl eax,1
    mov byte [gs:eax],cl
    inc eax
    mov byte [gs:eax],0x7
    inc ebx;新增一个字符
    jmp updateCursor
backspace:
    dec eax
    shl eax,1
    mov word [gs:eax],0
    dec ebx;回退一个字符
    jmp updateCursor
newLine:
    xor edx,edx
    mov esi,80
    div si
    sub bx,dx
    add ebx,80;新增一行
    jmp updateCursor
updateCursor:
    cmp ebx,2000
    jge rollScreen;如果满屏幕了,则向上滑屏
    ;设置光标
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5
    mov al,bh
    out dx,al

    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,bl
    out dx,al
    ;更新idx
    mov [idx],ebx
    ;光标处显示空格即可
    shl ebx,1
    mov word [gs:ebx],0x0700
    ;返回
    popad
    ret

global Clear_Screen
Clear_Screen:;将屏幕清空
    pushad
    ;清空屏幕
    mov ax,SELECTOR_VIDEO
    mov gs,ax
    mov edx,0
    ClearScreenLoopBegin:
        cmp edx,4000
        je ClearScreenLoopEnd
        mov dword [gs:edx],0x0
        add edx,4
        jmp ClearScreenLoopBegin
    ClearScreenLoopEnd:
    ;把光标移动到开头
    mov dx,0x03d4
    mov al,0x0e
    out dx,al
    mov dx,0x03d5
    mov al,0
    out dx,al

    mov dx,0x03d4
    mov al,0x0f
    out dx,al
    mov dx,0x03d5
    mov al,0
    out dx,al
    ;
    mov dword [idx],0
    ;
    mov word [gs:0],0x0700
    ;
    popad
    ret
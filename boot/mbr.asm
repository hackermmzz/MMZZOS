%include "boot.inc"
section mbr vstart=0x7c00
    mov ax,cs
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov fs,ax
    mov sp,0x7c00
    mov ax,0xb800
    mov gs,ax
    ;
    mov ax,0600h
    mov bx,0700h
    mov cx,0
    mov dx,184fh
    int 10h
    ;show tips
    mov eax,message0
    call .PrintStr
    ;load loder
    mov eax,LOADER_START_SECTOR;加载器在磁盘的扇区位置
    mov ebx,LOADER_BASE_ADDR;加载器被加载到的地址
    mov ecx,4;读取的扇区的数量
    call .LoadLoader
    ;
    mov eax,message1
    call .PrintStr
    ;
    jmp LOADER_BASE_ADDR
.PrintStr:
    mov esi,eax
    .ShowTip:
        mov eax,[esi]
        cmp eax,0
        je .ShowTipEnd
        call .Print
        add esi,1
        jmp .ShowTip
    .ShowTipEnd:
    ret
.Print:
    ;eax char,ebx pos
    mov ebx,[pointerPos]
    add ebx,1
    mov [pointerPos],ebx
    add ebx,-1
    shl ebx,2
    mov  [gs:bx],ax
    add ebx,1
    mov byte[gs:bx],0x7
    ret
.LoadLoader:
    ;备份eax喝ecx
    mov esi,eax
    mov edi,ecx
    ;设置要读取的扇区数量
    mov dx,0x1f2
    mov al,cl
    out dx,al
    mov eax,esi
    ;将LBA地址存入寄存器
    mov dx,0x1f3
    out dx,al

    mov cl,8
    shr eax,cl
    mov dx,0x1f4
    out dx,al

    shr eax,cl
    mov dx,0x1f5
    out dx,al

    shr eax,cl
    and al,0x0f
    or al,0xe0
    mov dx,0x1f6
    out dx,al
    ;写入读命令
    mov dx,0x1f7
    mov al,0x20
    out dx,al
    ;检查磁盘状态是否可读
    .notReady:
    nop
    in al,dx
    and al,0x88
    cmp al,0x8
    jnz .notReady
    ;读取数据
    mov ax,di
    mov dx,256
    mul dx
    mov cx,ax
    mov dx,0x1f0
    .goOnRead:
    in ax,dx
    mov [bx],ax
    add bx,2
    loop .goOnRead
    ret
    ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    message0 db "Please wait for loading LoaderProgram!",0
    message1 db "Finished!",0
    pointerPos dd 0
    times 510-($-$$) db 0
    db 0x55,0xaa
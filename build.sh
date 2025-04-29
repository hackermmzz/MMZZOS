#编译kernel
gcc -m32 -c -o build/init.o kernel/init.c -fno-stack-protector #编译init文件
gcc -m32 -c -o build/interrupt.o kernel/interrupt.c -fno-stack-protector #编译中断文件
gcc -m32 -ffreestanding -c -o  build/kernel.o  kernel/kernel.c -fno-stack-protector #编译内核文件 
gcc -m32 -c -o build/printc.o lib/kernel/print.c -fno-stack-protector #编译print.c文件
gcc -m32 -c -o build/timer.o device/timer.c -fno-stack-protector #timer.c文件
gcc -m32 -c -o build/assert.o lib/kernel/assert.c -fno-stack-protector #assert.c文件
gcc -m32 -c -o build/string.o lib/string.c -fno-stack-protector  #string.c文件
gcc -m32 -c -o build/bitmap.o lib/kernel/bitmap.c -fno-stack-protector   #编译bitmap.c文件
gcc -m32 -c -o build/memory.o kernel/memory.c -fno-stack-protector #编译memory.c文件
gcc -m32 -c -o build/list.o lib/kernel/list.c -fno-stack-protector #list.c文件
gcc -m32 -c -o build/thread.o kernel/thread.c -fno-stack-protector #thread.c文件
gcc -m32 -c -o build/aid.o lib/kernel/aid.c -fno-stack-protector #thread.c文件
gcc -m32 -c -o build/keyboard.o device/keyboard.c -fno-stack-protector #keyboard.c文件
gcc -m32 -c -o build/ioqueue.o lib/kernel//ioqueue.c -fno-stack-protector #ioQueue.c文件
gcc -m32 -c -o build/tss.o userprog/tss.c -fno-stack-protector #tss.c文件
gcc -m32 -c -o build/process.o userprog/process.c -fno-stack-protector #process.c文件
gcc -m32 -c -o build/syscall.o lib/user/syscall.c -fno-stack-protector #syscall.c文件
gcc -m32 -c -o build/stdio.o lib/stdio.c -fno-stack-protector #stdio.c文件
gcc -m32 -c -o build/minHeap.o lib/kernel/minHeap.c -fno-stack-protector #minHeap.c文件
gcc -m32 -c -o build/ide.o  device/ide.c -fno-stack-protector #ide.c文件
gcc -m32 -c -o build/math.o  lib/math.c -fno-stack-protector #math.c文件
gcc -m32 -c -o build/fs.o  fs/fs.c -fno-stack-protector #fs.c文件
gcc -m32 -c -o build/inode.o  fs/inode.c -fno-stack-protector #inode.c文件
gcc -m32 -c -o build/file.o  fs/file.c -fno-stack-protector #file.c文件
gcc -m32 -c -o build/dir.o  fs/dir.c -fno-stack-protector #dir.c文件
nasm -f elf -I include/ -o build/print.o lib/kernel/print.asm #编译aid.asm文件
nasm -f elf -o build/idt_table.o kernel/idt_table.asm #编译中断向量表
nasm -f elf -o build/SwitchTo.o kernel/SwitchTo.asm #编译上下文切换函数
nasm -I include/ -o boot/loader.bin boot/loader.asm #编译loader文件

cd build
ld -m elf_i386  -Ttext 0xc0001500 kernel.o dir.o inode.o file.o fs.o  syscall.o  minHeap.o math.o  stdio.o ide.o  aid.o process.o tss.o SwitchTo.o ioqueue.o keyboard.o memory.o thread.o interrupt.o idt_table.o init.o  list.o print.o  printc.o timer.o assert.o string.o bitmap.o -e main -o kernel.bin
dd if=../boot/mbr.bin  of=/home/mmzz/bochs/bochs/bin/hd60M.img  bs=512 count=1 conv=notrunc 
dd if=../boot/loader.bin  of=/home/mmzz/bochs/bochs/bin/hd60M.img  bs=512 count=5 seek=2 conv=notrunc 
dd if=kernel.bin  of=/home/mmzz/bochs/bochs/bin/hd60M.img  bs=512 count=200 seek=9 conv=notrunc 
rm *
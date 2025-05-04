gcc -m32 -c -o grep.o  ../bin/grep.c -fno-stack-protector -ffreestanding 
gcc -m32 -c -o cat.o  ../bin/cat.c -fno-stack-protector -ffreestanding 
ld -m elf_i386 -Ttext 0x8060000  -e _start start.o grep.o syscall.o string.o stdio.o  -o grep.bin
ld -m elf_i386 -Ttext 0x8060000  -e _start start.o cat.o syscall.o string.o stdio.o  -o cat.bin
dd if=grep.bin  of=/home/mmzz/bochs/bochs/bin/hd60M.img  bs=512 count=200 seek=300  conv=notrunc 
dd if=cat.bin  of=/home/mmzz/bochs/bochs/bin/hd60M.img  bs=512 count=200 seek=500  conv=notrunc 
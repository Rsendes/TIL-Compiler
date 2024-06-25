./til -o test.asm $1 &> /dev/null
yasm -felf32 test.asm 
ld -m elf_i386 -o test test.o -lrts -L$HOME/compiladores/root/usr/lib
./test 

#include "assert.h"
void PANIC(const char*filename,int line,const char*func,const char*condition){
    interrupt_disable();
    put_str("\nError!\n");
    put_str("file:");put_str(filename);
    put_char('\n');
    put_str("line:");put_num(line);
    put_char('\n');
    put_str("function:");put_str(func);
    put_char('\n');
    put_str("conditon:");put_str(condition);
    while(1);
} 
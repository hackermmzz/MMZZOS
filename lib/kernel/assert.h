#ifndef MMZZ_ASSERT_H
#define MMZZ_ASSERT_H
#include"print.h"
extern void interrupt_disable();
///////////////////////////////////////
void PANIC(const char*filename,int line,const char*func,const char*condition);
#define PANIC_MACRO(...) PANIC(__FILE__,__LINE__,__func__,__VA_ARGS__)
#ifdef MMZZNODEBUG
#define ASSERT(condtion) (void(0))
#else
#define ASSERT(condition) {if(!(condition))PANIC_MACRO(#condition);}
#endif
///////////////////////////////////////
#endif
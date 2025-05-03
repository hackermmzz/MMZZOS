#ifndef MMZZ_WAIT_EXIT_H
#define MMZZ_WAIT_EXIT_H
#include"process.h"
///////////////////////////////////////////
pid_t syscall_wait(int32_t*);
void syscall_exit(int32_t);
///////////////////////////////////////////
#endif
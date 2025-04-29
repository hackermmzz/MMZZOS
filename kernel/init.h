#ifndef MMZZ_INIT_H
#define MMZZ_INIT_H
#include "interrupt.h"
#include "../device/timer.h"
#include "../lib/kernel/assert.h"
#include "memory.h"
#include "thread.h"
#include "../device/keyboard.h"
#include "../userprog/tss.h"
#include "../lib/user/syscall.h"
#include "../device/ide.h"
#include"../fs/fs.h"
/////////////////////////////////////////
void MMZZOS_Init();
/////////////////////////////////////////
#endif
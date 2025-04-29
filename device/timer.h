#ifndef MMZZ_TIMER_H
#define MMZZ_TIMER_H
#include"../lib/kernel/io.h"
///////////////////////////////////
extern uint32_t systemTicks;
///////////////////////////////////
void frequency_set(uint8_t port,uint8_t number,uint8_t rwl,uint8_t mode,uint16_t count);
void timer_init();
void timer_interrupt(uint8_t);
///////////////////////////////////
#endif
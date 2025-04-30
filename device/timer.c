#include "timer.h"
#include "../lib/kernel/print.h"
#include "../kernel/thread.h"
////////////////////////////////
#define IRQ0_FREQUENCY 	1000
#define INPUT_FREQUENCY        1193180
#define COUNTER0_VALUE		INPUT_FREQUENCY / IRQ0_FREQUENCY
#define COUNTER0_PORT		0X40
#define COUNTER0_NO 		0
#define COUNTER_MODE		2
#define READ_WRITE_LATCH	3
#define PIT_COUNTROL_PORT	0x43
#define mil_second_per_init	1000 / IRQ0_FREQUENCY
////////////////////////////////
void frequency_set(uint8_t port, uint8_t number, uint8_t rwl, uint8_t mode, uint16_t count)
{
    outB(PIT_COUNTROL_PORT,(uint8_t) (number << 6 | rwl << 4 | mode << 1));
    outB(port,(uint8_t)count);
    outB(port,(uint8_t)(count >> 8));
}

void timer_init()
{
    put_str("timer init start\n");
    frequency_set(COUNTER0_PORT,COUNTER0_NO,READ_WRITE_LATCH,COUNTER_MODE,COUNTER0_VALUE);
    put_str("timer init done\n");
}

void timer_interrupt(uint8_t code){
    //
    struct PCB*cur=RunningThread();
    ASSERT(cur->stackMagic==1190650);//检查是否栈溢出
    //
    ++systemTicks;
    ++(cur->totalTicks);
    //
    if((cur->ticks)==0){
        schedule();//调度
    }
    else{
        --(cur->ticks);
    }
}
 
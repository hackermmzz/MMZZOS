#include "aid.h"

uint32_t randint(int a, int b)
{
    static uint32_t next=1;
    next=next * 1103515245 + 12345;
    return next%RAND_MAX;
}

int isdigit(int ch)
{
    return ch>='0'&&ch<='9';
}

int isalpha(int ch)
{
    ch&=~(32);
    return ch>='A'&&ch<='Z';
}

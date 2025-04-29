#include "aid.h"

uint32_t randint(int a, int b)
{
    static uint32_t next=1;
    next=next * 1103515245 + 12345;
    return next%RAND_MAX;
}
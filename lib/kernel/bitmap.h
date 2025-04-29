#ifndef MMZZ_BITMAP_H
#define MMZZ_BITMAP_H
#include "../stdint.h"
#include "assert.h"
#include "../string.h"
//////////////////////////////////
struct BitMap{
    uint32_t len;
    uint8_t*bits;
};
//初始化
void BitMapInit(struct BitMap*bp);
//置0
void BitMapReset(struct BitMap*bp,uint32_t idx);
//置1
void BitMapSet(struct BitMap*bp,uint32_t idx);
//判断指定位置是否为1
bool BitMapTest(struct BitMap*bp,uint32_t idx);
//申请连续cnt个位置为0的位置
uint32_t BitMapAllocate(struct BitMap*bp,uint32_t cnt);
//////////////////////////////////
#endif
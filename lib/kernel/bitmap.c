#include "bitmap.h"

void BitMapInit(struct BitMap *bp)
{
    memset(bp->bits,0,bp->len);
}

void BitMapReset(struct BitMap *bp, uint32_t idx)
{
    //如果索引超出范围
    ASSERT((idx>>3)<=bp->len);
    //
    uint8_t*ptr=bp->bits+(idx>>3);
    idx&=0x7;
    *ptr=(*ptr)&(~(1<<idx));
}

void BitMapSet(struct BitMap *bp, uint32_t idx)
{
    //如果索引超出范围
    ASSERT((idx>>3)<=bp->len);
    //
    uint8_t*ptr=bp->bits+(idx>>3);
    idx&=0x7;
    *ptr=(*ptr)|(1<<idx);
}

bool BitMapTest(struct BitMap *bp, uint32_t idx)
{
    //如果索引超出范围
    ASSERT((idx>>3)<=bp->len);
    //
    uint8_t*ptr=bp->bits+(idx>>3);
    idx&=0x7;
    return (*ptr)>>idx&1;
}

uint32_t BitMapAllocate(struct BitMap *bp, uint32_t cnt)
{
    //目前先暴力求解(滑动窗口)
    uint32_t target=-1;
    uint8_t*cur=bp->bits;
    int j;
    for(int i=0,l=0,r=0;i<bp->len;++i,++cur){
        for(j=0;j<8;++j,++r){
            if((*cur)>>j&1)l=r+1;
            if(r-l+1>=cnt){
                target=l;
                break;
            }
        }
        if(~target)break;
    }
    return target;
}

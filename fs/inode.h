#ifndef MMZZ_INODE_H
#define MMZZ_INODE_H
#include "../lib/stdint.h"
#include "../lib/kernel/list.h"
#include"../device/ide.h"
////////////////////////////////////////
struct Inode{
    uint32_t index;//在inode表的索引
    uint32_t size;//文件的大小
    uint32_t open_cnt;//文件被打开的次数
    bool write_deny;//文件是否被写入
    uint32_t i_sectors[15];//
    struct ListNode tag;
};


struct Inode* InodeOpen(struct partition*part,uint32_t index);
void InodeWrite(struct partition*part,struct Inode*inode);//将inode写入磁盘
bool InodeClose(struct Inode*inode);//返回值表示inode是否真的被关闭了(openCnt为0就会被真正关闭)
void InodeInit(struct Inode*inode,uint32_t index);
struct Inode*FindOpenInode(struct partition*part,int32_t index);
bool InodeRecycle(struct partition*part,struct Inode*inode,uint32_t*addr_buf);
////////////////////////////////////////
#endif
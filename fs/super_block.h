#ifndef MMZZ_SUPER_BLOCK_H
#define MMZZ_SUPER_BLOCK_H
#include "../lib/stdint.h"
/////////////////////////////////////
struct SuperBlock{
    uint32_t magic;//标识文件系统的类型
    //
    uint32_t sector_cnt;//扇区总数
    uint32_t inode_cnt;//文件最大总数
    uint32_t lba;//起始扇区
    //
    uint32_t sector_bitmap_lba;//块位图的起始扇区
    uint32_t sector_bitmap_sector_cnt;//块位图占用的扇区数量
    //
    uint32_t inode_bitmap_lba;//inode位图的起始扇区
    uint32_t inode_bitmap_sector_cnt;//inode位图占用的扇区数量
    //
    uint32_t inode_table_lba;//inode表的起始扇区
    uint32_t inode_table_sector_cnt;//inode表占用的扇区
    //
    uint32_t data_lba;//存储实际数据的起始扇区
    uint32_t root_inode_idx;//根目录的inode编号
    uint32_t dir_entry_size;//目录项大小
    //
    uint8_t pad[460];
} __attribute__ ((packed));

/////////////////////////////////////
#endif
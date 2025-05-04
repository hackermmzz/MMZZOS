#ifndef MMZZ_IDE_H
#define MMZZ_IDE_H
#include "../lib/stdint.h"
#include"../kernel/thread.h"
#include "../lib/stdio.h"
/////////////////////////////////////
typedef struct partition
{
    uint32_t start_lba;          //起始扇区
    uint32_t sec_cnt;            //扇区数
    struct disk* my_disk;        //分区所属硬盘
    struct ListNode tag;   //所在队列中的标记
    char name[8];                //分区名字
    struct SuperBlock* sb;	  //本分区 超级块
    struct BitMap sector_bitmap;  //块位图
    struct BitMap inode_bitmap;  //i结点位图
    struct List open_inodes;     //本分区打开
}partition;

struct partition_table_entry
{
    uint8_t bootable;		  //是否可引导
    uint8_t start_head;	  //开始磁头号
    uint8_t start_sec;		  //开始扇区号
    uint8_t start_chs;		  //起始柱面号
    uint8_t fs_type;		  //分区类型
    uint8_t end_head;		  //结束磁头号
    uint8_t end_sec;		  //结束扇区号
    uint8_t end_chs;		  //结束柱面号
    uint32_t start_lba;	  //本分区起始的lba地址
    uint32_t sec_cnt;		  //本扇区数目
} __attribute__ ((packed));

struct boot_sector
{
    uint8_t other[446];	  			//446 + 64 + 2 446是拿来占位置的
    struct partition_table_entry partition_table[4];  //分区表中4项 64字节
    uint16_t signature;				//最后的标识标志 魔数0x55 0xaa				
} __attribute__ ((packed));

//硬盘
typedef struct disk
{
    char name[8];		      //本硬盘的名称
    struct ide_channel* my_channel;    //这块硬盘归属于哪个ide通道
    uint8_t dev_no;		      //0表示主盘 1表示从盘
    struct partition prim_parts[4];  //主分区顶多是4个
    struct partition logic_parts[8]; //逻辑分区最多支持8个
}disk ;

// ata 通道结构
struct ide_channel
{
    char name[8];		      //ata通道名称
    uint16_t port_base;	      //本通道的起始端口号
    uint8_t  irq_no;		      //本通道所用的中断号
    struct Mutex lock;                //通道锁 一个硬盘一通道 不能同时
    bool expecting_intr;	      //期待硬盘中断的bool
    struct Semaphore disk_done;      //用于阻塞 唤醒驱动程序  和锁不一样 把自己阻塞后 把cpu腾出来
    struct disk devices[2];	      //一通道2硬盘 1主1从
};
////////////////////////////////////////////
extern struct ide_channel ide_channel[2];///默认只有2个
extern uint8_t channel_cnt;
extern struct List partition_list;	  //分区队列
///////////////////////////////////////////
void ide_init();
void ide_read(struct  disk*hd,void*buf,uint32_t offset,uint32_t cnt);
void ide_write(struct  disk*hd,void*buf,uint32_t offset,uint32_t cnt);
void disk_itr_handler(uint8_t code);
///////////////////////////////////////////
#endif
#include "ide.h"
////////////////////////////////////////////
#define reg_data(channel) 	  (channel->port_base + 0)
#define reg_error(channel) 	  (channel->port_base + 1)
#define reg_sect_cnt(channel)    (channel->port_base + 2)
#define reg_lba_l(channel)	  (channel->port_base + 3)
#define reg_lba_m(channel)	  (channel->port_base + 4)
#define reg_lba_h(channel)	  (channel->port_base + 5)
#define reg_dev(channel)	  (channel->port_base + 6)
#define reg_status(channel)	  (channel->port_base + 7)
#define reg_cmd(channel)	  (reg_status(channel))
#define reg_alt_status(channel)  (channel->port_base + 0x206)
#define reg_ctl(channel)	  reg_alt_status(channel)

#define BIT_STAT_BSY	  	  0X80		//硬盘忙
#define BIT_STAT_DRDY	  	  0X40		//驱动器准备好啦
#define BIT_STAT_DRQ 	  	  0x8		//数据传输准备好了
#define BIT_DEV_MBS		  0XA0		
#define BIT_DEV_LBA		  0X40
#define BIT_DEV_DEV		  0X10

#define CMD_IDENTIFY		  0XEC		//identify指令
#define CMD_READ_SECTOR	  0X20		//读扇区指令
#define CMD_WRITE_SECTOR	  0X30		//写扇区指令
////////////////////////////////////////////
uint8_t channel_cnt;
struct ide_channel ide_channel[2];///默认只有2个
int32_t ext_lba_base = 0;	  //记录总拓展分区lba 初始为0
uint8_t p_no = 0,l_no = 0;	  //记录硬盘主分区下标 逻辑分区下标
struct List partition_list;	  //分区队列
////////////////////////////////////////////
//选择读写的硬盘
void select_disk(struct disk*hd){
    uint8_t reg_device=BIT_DEV_MBS|BIT_DEV_LBA;
    if(hd->dev_no)reg_device|=BIT_DEV_DEV;
    outB(reg_dev(hd->my_channel),reg_device);
}
//选择读写开始的扇区以及扇区个数
void select_sector(struct disk*hd,uint32_t offset,uint8_t cnt){
    struct ide_channel*channel=hd->my_channel;
    //设置操作的扇区数量
    outB(reg_sect_cnt(channel),cnt);
    //设置开始的偏移
    outB(reg_lba_l(channel),offset);
    outB(reg_lba_m(channel),offset>>8);
    outB(reg_lba_h(channel),offset>>16);
    outB(reg_dev(channel),(hd->dev_no==1?BIT_DEV_DEV:0)|BIT_DEV_MBS|BIT_DEV_LBA|offset>>24);
}

//发送指令
void CmdOut(struct ide_channel*channel,uint8_t cmd){
    channel->expecting_intr=1;
    outB(reg_cmd(channel),cmd);
}
//从扇区读取连续个扇区
void ReadFromSector(struct disk*hd,void*buf,uint32_t cnt){
    if(cnt==0)return;
    uint32_t bytes=cnt*512;
    inSW(reg_data(hd->my_channel),buf,bytes/2);
}
//写入磁盘cnt个扇区
void WriteToSector(struct disk*hd,void*buf,uint32_t cnt){
    if(cnt==0)return;
    uint32_t bytes=cnt*512;
    outSW(reg_data(hd->my_channel),buf,bytes/2);
}
//

//
bool WaitForHdWork(struct disk*hd){
    static uint32_t msecond=10*1000;//默认最多等待10s
    struct ide_channel* channel = hd->my_channel;
    uint32_t time=msecond;
    while(time>0){
        if(!(inB(reg_status(channel))&BIT_STAT_BSY)){
            return (inB(reg_status(channel))&BIT_STAT_DRQ);
        }
        time-=10;
        SleepWithoutHang(10);
    }
    return 0;
}
//从硬盘偏移处读取cnt个扇区到缓冲区
void ide_read(struct  disk*hd,void*buf,uint32_t offset,uint32_t cnt){
    MutexAcquire(&(hd->my_channel->lock));
    //选择磁盘
    select_disk(hd);
    //
    uint32_t curCnt=0;
    uint32_t needSelect=0;
    while(curCnt<cnt){
        if(curCnt+255<=cnt){
            needSelect=255;
        }else{
            needSelect=cnt-curCnt;
        }
        //设置要读取扇区偏移和个数
        select_sector(hd,offset+curCnt,needSelect);
        //写入指令寄存器
        CmdOut(hd->my_channel,CMD_READ_SECTOR);
        //阻塞线程
        SemaphoreDec(&(hd->my_channel->disk_done));
        //检查是否可读
        if(!WaitForHdWork(hd)){
            char err[40];
            sprintf(err,"%s0 read %d selector failed!\n",hd->my_channel->name,offset);
            PANIC_MACRO(err);
        } 
        //读取数据
        ReadFromSector(hd,(void*)(((uint32_t)buf)+curCnt*512),needSelect);
        //
        curCnt+=needSelect;
    }
    MutexRelease(&(hd->my_channel->lock));
}

void ide_write(disk *hd, void *buf, uint32_t offset, uint32_t cnt)
{
    MutexAcquire(&(hd->my_channel->lock));
    //选择磁盘
    select_disk(hd);
    //
    uint32_t curCnt=0;
    uint32_t needSelect=0;
    while(curCnt<cnt){
        if(curCnt+255<=cnt){
            needSelect=255;
        }else{
            needSelect=cnt-curCnt;
        }
        //设置要读取扇区偏移和个数
        select_sector(hd,offset+curCnt,needSelect);
        //写入指令寄存器
        CmdOut(hd->my_channel,CMD_WRITE_SECTOR);
        //检查是否可读
        if(!WaitForHdWork(hd)){
            char err[40];
            sprintf(err,"%s write %d selector failed!\n",hd->my_channel->name,offset);
            PANIC_MACRO(err);
        }
        //读取数据
        WriteToSector(hd,(void*)(((uint32_t)buf)+curCnt*512),needSelect);
        //阻塞线程
        SemaphoreDec(&(hd->my_channel->disk_done));
        //
        curCnt+=needSelect;
    }
    MutexRelease(&(hd->my_channel->lock));
}

void disk_itr_handler(uint8_t code)
{
    ASSERT(code==0x2e||code==0x2f);
    uint8_t channel_no=code-0x2e;
    struct ide_channel*channel=&ide_channel[channel_no];
    //
    if(channel->expecting_intr){
        channel->expecting_intr=0;
        SemaphoreInc(&(channel->disk_done));
        inB(reg_status(channel));
    }
}


void swap_pairs_bytes(const char* dst,char* buf,uint32_t len)
{
    uint8_t idx;
    for(idx = 0;idx < len;idx += 2)
    {
    	buf[idx+1] = *(dst++);
    	buf[idx]   = *(dst++);
    }
}
void identify_disk(struct disk* hd)
{
    char id_info[500];
    select_disk(hd);
    CmdOut(hd->my_channel,CMD_IDENTIFY);
    if(!WaitForHdWork(hd))
    {
    	char error[64];
    	sprintf(error,"%s identify failed!!!!!!\n",hd->my_channel->name);
    	PANIC_MACRO(error);
    }
    ReadFromSector(hd,id_info,1);//现在硬盘已经把硬盘的参数准备好了了 我们把参数读到自己的缓冲区中
    char buf[64] = {0};
    uint8_t sn_start = 10 * 2,sn_len = 20,md_start = 27*2,md_len = 40;
    swap_pairs_bytes(&id_info[sn_start],buf,sn_len);
    printk("    disk %s info:        SN: %s\n",hd->name,buf);
    swap_pairs_bytes(&id_info[md_start],buf,md_len);
    printk("    MODULE: %s\n",buf);
    uint32_t sectors = *(uint32_t*)&id_info[60*2];
    printk("    SECTORS: %d\n",sectors);
    printk("    CAPACITY: %dMB\n",sectors * 512 / 1024 / 1024);
} 
// 扫描硬盘 hd 中地址为 ext_lba 的扇区中的所有分区舍
void partition_scan(struct disk* hd,uint32_t ext_lba)
{
    struct boot_sector* bs = syscall_malloc(sizeof(struct boot_sector));
    //读取信息，原著使用ide_read(hd,bs,ext_lba,1);但是我这里会出现缺页错误，所以我就使用和原著不一样的了，以后再改bug
    select_disk(hd);
    select_sector(hd,ext_lba,1);
    CmdOut(hd->my_channel,CMD_READ_SECTOR);
    if(!WaitForHdWork(hd)){
        char err[40];
        sprintf(err,"%s read %d selector failed!\n",hd->my_channel->name,ext_lba);
        PANIC_MACRO(err);
    } 
    ReadFromSector(hd,bs,1);
    //
    uint8_t part_idx = 0;
    struct partition_table_entry* p = bs->partition_table; //p为分区表开始的位置
    while((part_idx++) < 4)
    {

    	if(p->fs_type == 0x5) //拓展分区
    	{
    	    if(ext_lba_base != 0)
    	    {
    	    	partition_scan(hd,p->start_lba + ext_lba_base);	//继续递归转到下一个逻辑分区再次得到表
    	    }
    	    else //第一次读取引导块
    	    {
    	    	ext_lba_base = p->start_lba;
    	    	partition_scan(hd,ext_lba_base);
    	    }
    	}
    	else if(p->fs_type != 0)
    	{
    	    if(ext_lba == 0)	//主分区
    	    {
    	    	hd->prim_parts[p_no].start_lba = ext_lba + p->start_lba;
    	    	hd->prim_parts[p_no].sec_cnt = p->sec_cnt;
    	    	hd->prim_parts[p_no].my_disk = hd;
    	    	ListPushBack(&partition_list,&hd->prim_parts[p_no].tag);
    	    	sprintf(hd->prim_parts[p_no].name,"%s%d",hd->name,p_no+1);
    	    	p_no++;
    	    	ASSERT(p_no<4);	//0 1 2 3 最多四个
    	    }
    	    else		//其他分区
    	    {
    	    	hd->logic_parts[l_no].start_lba = ext_lba + p->start_lba;
    	    	hd->logic_parts[l_no].sec_cnt = p->sec_cnt;
    	    	hd->logic_parts[l_no].my_disk = hd;
    	    	ListPushBack(&partition_list,&hd->logic_parts[l_no].tag);
    	    	sprintf(hd->logic_parts[l_no].name,"%s%d",hd->name,l_no+5); //从5开始
    	    	l_no++;
    	    	if(l_no >= 8)	return; //只支持8个
    	    }
    	}
    	++p;
    }
    syscall_free(bs);
}

bool partition_info(struct ListNode* pelem,int arg)
{
    struct partition* part = elem2entry(struct partition,tag,pelem);
    printk("    %s  start_lba:0x%x,sec_cnt:0x%x\n",part->name,part->start_lba,part->sec_cnt);
    return 0; //list_pop完
}

void ide_init()
{
    put_str("ide init start!\n");
    /////////////////////////////////////////
    //获取磁盘数量
    uint8_t hd_cnt=*(uint8_t*)(0x475);
    put_str("The disk cnt is:");put_num(hd_cnt);put_char('\n');
    channel_cnt=DIV_ROUND_UP(hd_cnt,2);
    struct ide_channel*channel;
    uint8_t channel_no=0;
    //
    ListInit(&partition_list);
    //
    while(channel_no<channel_cnt){
        channel=&ide_channel[channel_no];
        //给每个ide通道命名
        sprintf(channel->name,"ide%d",channel_no);
        //为每个ide通道初始化端口基址以及中断向量
        if(channel_no==0){
            channel->port_base=0x1f0;
            channel->irq_no=0x2e;
        }else if(channel_no==1){
            channel->port_base=0x170;
            channel->irq_no=0x2f;
        }else PANIC_MACRO("HERE!\n");
        channel->expecting_intr=0;
        //初始化锁和信号量
        MutexInit(&(channel->lock));
        SemaphoreInit(&(channel->disk_done),0);
        //获取两个磁盘信息
        uint8_t dev_no=0;
        while(dev_no < 2)
    	{
    	    struct disk* hd = &channel->devices[dev_no];
    	    hd->my_channel = channel;
    	    hd->dev_no = dev_no;
    	    sprintf(hd->name,"sd%c",'a' + channel_no * 2 + dev_no);
    	    identify_disk(hd);
    	    if(dev_no != 0)partition_scan(hd,0);
    	    p_no = 0,l_no = 0;
    	    dev_no++;
    	}
        //
        ++channel_no;
        //
    }
    printk("\n    all partition info\n");
    ListTraversal(&partition_list,partition_info,(int)NULL);
    ////////////////////////////////////////
    put_str("ide init finished!\n");
}
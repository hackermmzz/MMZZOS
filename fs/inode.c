#include "inode.h"
#include"fs.h"
#include"super_block.h"
#include"file.h"
///////////////////////////////////
struct Inode* InodeOpen(struct partition*part,uint32_t index)
{
    ASSERT(index<MAXFILE_PER_PART);//
    //现在已经打开的inode链表寻找
    struct ListNode*cur=part->open_inodes.head.nxt;
    struct ListNode*end=&(part->open_inodes.tail);
    struct Inode*inode;
    while(cur!=end){
        inode=elem2entry(struct Inode,tag,cur);
        if(inode->index==index)
        {
            ++(inode->open_cnt);
            return inode;
        }
        cur=cur->nxt;
    }
    //
    struct PCB*pcb=RunningThread();
    uint32_t pgaddr=pcb->pageaddr;
    pcb->pageaddr=0;//使得其是在内核分配的内存,以至以后可以共享
    inode=syscall_malloc(sizeof(*inode));
    pcb->pageaddr=pgaddr;
    ASSERT(inode);
    //获取inode在磁盘的偏移
    uint32_t offset=sizeof(struct Inode)*index;
    uint32_t sector_begin=offset/SECTOR_SIZE;//起始扇区编号
    uint32_t sector_end=(offset+sizeof(struct Inode)-1)/SECTOR_SIZE;//结束扇区编号
    //读取inode数据
    uint8_t cnt=2;
    if(sector_begin==sector_end)cnt=1;
    void*buf=syscall_malloc(cnt*BLOCK_SIZE);
    ASSERT(buf);
    ide_read(part->my_disk,buf,sector_begin+part->sb->inode_table_lba,cnt);//
    //
    struct Inode*inode_rd=(struct Inode*)(((uint32_t)buf)+offset%BLOCK_SIZE);
    *inode=*inode_rd;
    inode->open_cnt=1;//打开一次
    inode->write_deny=0;
    inode->index=index;
    ListPushFront(&(part->open_inodes),&(inode->tag));
    syscall_free(buf);
    return inode;
}
void InodeWrite(struct partition *part, struct Inode *inode)
{
    ASSERT(inode->index<MAXFILE_PER_PART);//
    //获取inode在磁盘的偏移
    uint32_t offset=sizeof(struct Inode)*inode->index;
    uint32_t sector_begin=offset/SECTOR_SIZE;//起始扇区编号
    uint32_t sector_end=(offset+sizeof(struct Inode)-1)/SECTOR_SIZE;//结束扇区编号
    //读取inode数据
    uint8_t cnt=2;
    if(sector_begin==sector_end)cnt=1;
    void*buf=syscall_malloc(cnt*BLOCK_SIZE);
    ASSERT(buf);
    ide_read(part->my_disk,buf,sector_begin+part->sb->inode_table_lba,cnt);//
    //将新的inode拷贝到指定位置
    memcpy((void*)(((uint32_t)buf)+offset%BLOCK_SIZE),inode,sizeof(struct Inode));
    //存入磁盘
    ide_write(part->my_disk,buf,part->sb->inode_table_lba+sector_begin,cnt);
    syscall_free(buf);
}

void InodeClose(struct Inode *inode)
{
    uint8_t old=interrupt_status();
    interrupt_disable();
    //
    if(--(inode->open_cnt)==0){
        ListRemove(0,&(inode->tag));
        struct PCB*pcb=RunningThread();
        uint32_t pgaddr=pcb->pageaddr;
        pcb->pageaddr=0;
        syscall_free(inode);
        pcb->pageaddr=pgaddr;
    }
    //
    if(old)interrupt_enable();
}

void InodeInit(struct Inode *inode, uint32_t index)
{
    inode->open_cnt=0;
    inode->size=0;
    inode->write_deny=0;
    inode->index=index;
    memset(inode->i_sectors,0,sizeof(inode->i_sectors));
}


struct Inode *FindOpenInode(struct partition *part, int32_t index)
{
    struct ListNode*cur=part->open_inodes.head.nxt;
    struct ListNode*end=&(part->open_inodes.tail);
    while(cur!=end){
        struct Inode*node=elem2entry(struct Inode,tag,cur);
        if(node->index==index){
            //文件被打开了不能被删除
            return node;
        }
        cur=cur->nxt;
    }
    return 0;
}

//回收inode的所有扇区和inode的标志
bool InodeRecycle(struct partition*part,struct Inode*inode,uint32_t*addr_buf){
    BitMapReset(&(part->inode_bitmap),inode->index);
    BitMapUpdate(part,inode->index,BITMAP_FOR_INODE);
    //获取文件分配到的所有扇区
    int32_t block_lba=part->sb->data_lba;
    memset(addr_buf,0,sizeof(addr_buf));
    memcpy(addr_buf,inode->i_sectors,48);
    if(inode->i_sectors[12]){
        ide_read(part->my_disk,&addr_buf[12],inode->i_sectors[12],1);
        //回收一级间接表占的空间
        int idx=inode->i_sectors[12]-block_lba;
        BitMapReset(&(part->block_bitmap),idx);
        BitMapUpdate(part,idx,BITMAP_FOR_BLOCK);
    }
    //回收所有的block
    for(int i=0;i<140;++i){
        if(addr_buf[i])
        {
            int32_t idx=addr_buf[i]-block_lba;
            BitMapReset(&(part->block_bitmap),idx);
            BitMapUpdate(part,idx,BITMAP_FOR_BLOCK);
        }
        else break;//保证文件的的扇区地址分布在磁盘上是连续的
    }
    return 1;
}
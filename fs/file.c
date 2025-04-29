#include"file.h"
#include"inode.h"
#include"super_block.h"
#include"../lib/math.h"
///////////////////////////////////////////
struct FILE GlobalFileTable[MAX_FILE_CNT_SYSTEM_OPEN];
///////////////////////////////////////////
int32_t GetFreeGlobalFd(){
    uint32_t idx=STD_FD_CNT;//其中0 1分别作为标准输入和标准输出
    while(idx<MAX_FILE_CNT_SYSTEM_OPEN){
        if(GlobalFileTable[idx].inode==0){
            break;
        }
        ++idx;
    }
    if(idx==MAX_FILE_CNT_SYSTEM_OPEN){
        printk("the open file count is full!\n");
        return -1;
    }
    return idx;
}

int32_t ProcessInstallFd(int16_t global_fd){
    uint32_t idx=STD_FD_CNT;
    struct PCB*pcb=RunningThread();
    while(idx<MAX_FILE_CNT_OPEN_PROCESS){
        if(pcb->fd[idx]==-1){
            pcb->fd[idx]=global_fd;
            break;
        }
        ++idx;
    }
    if(idx==MAX_FILE_CNT_OPEN_PROCESS){
        printk("%s exceed max open files!\n",pcb->name);
        return -1;
    }
    return idx;
}

int32_t InodeBitmapAllocate(struct partition*part){
    uint32_t idx=BitMapAllocate(&(part->inode_bitmap),1);
    if(idx==-1)return -1;
    BitMapSet(&(part->inode_bitmap),idx);
    return idx;
}
int32_t BlockBitmapAllocate(struct partition*part){
    uint32_t idx=BitMapAllocate(&(part->block_bitmap),1);
    if(idx==-1)return -1;
    BitMapSet(&(part->block_bitmap),idx);
    return idx;
}
//把idx所在扇区的512字节更新到磁盘
void BitMapUpdate(struct partition*part,uint32_t idx,enum FS_BITMAP_TYPE flag){
    uint32_t sector=idx/SECTOR_BITS;
    uint32_t offset=sector*BLOCK_SIZE;
    uint32_t lba;
    void*buf;
    if(flag==BITMAP_FOR_INODE){
        lba=part->sb->inode_bitmap_lba+sector;
        buf=part->inode_bitmap.bits+offset;
    }else{
        lba=part->sb->block_bitmap_lba;
        buf=part->block_bitmap.bits+offset;
    }
    ide_write(part->my_disk,buf,lba,1);
}
int32_t LocalFdToGlobalFd(int32_t fd_l){
    ASSERT(fd_l>=0&&fd_l<MAX_FILE_CNT_OPEN_PROCESS);
    struct PCB*pcb=RunningThread();
    return pcb->fd[fd_l];
}
int32_t file_create(struct Dir*dir,const char*file,uint8_t flag){
    //
    int32_t inode=InodeBitmapAllocate(CurPartition);
    if(inode==-1)return -1;
    //
    int rollback=0;
    int fd_g=GetFreeGlobalFd();
    if(fd_g==-1){
        rollback=1;
        goto RollBack;
    }
    //
    struct Inode*node=syscall_malloc(sizeof(struct Inode));
    if(node==NULL){
        rollback=2;
        goto RollBack;
    }
    InodeInit(node,inode);
    node->open_cnt=1;//被打开一次
    //
    GlobalFileTable[fd_g].fd_pos=0;
    GlobalFileTable[fd_g].flag=flag;
    GlobalFileTable[fd_g].inode=node;
    GlobalFileTable[fd_g].inode->write_deny=0;
    //创建目录项
    struct DirEntry entry;
    memcpy(entry.filename,file,FILENAME_MAX_LEN);
    entry.filetype=FT_FILE;
    entry.index=inode;
    if(!CreateDirEntry(CurPartition,dir,&entry)){
        rollback=3;
        goto RollBack;
    }
    //将inode父节点写入磁盘
    InodeWrite(CurPartition,dir->inode);
    //将inode信息写入磁盘
    InodeWrite(CurPartition,node);
    //将inode位图同步到内存
    BitMapUpdate(CurPartition,inode,BITMAP_FOR_INODE);
    //将新创建的inode放入打开的链表
    ListPushBack(&(CurPartition->open_inodes),&(node->tag));
    //安装到进程(这里不用管安装是否成功，因为只要最后创建成功文件就行了)
    return ProcessInstallFd(fd_g);
    //
    
    RollBack:
    switch (rollback)
    {
    case 3:
        memset(&GlobalFileTable[fd_g],0,sizeof(struct FILE));
    case 2:
       syscall_free(node);
    case 1:
        BitMapReset(&(CurPartition->inode_bitmap),inode);
    default:
        break;
    }
    return -1;
}

int32_t file_open(uint32_t index,uint8_t flag){
    //获取全局描述符
    int32_t fd_g=GetFreeGlobalFd();
    if(fd_g==-1)return -1;
    //打开inode
    struct Inode*inode=InodeOpen(CurPartition,index);
    ASSERT(inode);
    //
    struct FILE*file=&GlobalFileTable[fd_g];
    file->inode=inode;
    file->fd_pos=0;
    file->flag=flag;
    bool*writeDeny=&(file->inode->write_deny);
    //只要有写操作,那么必须使得writedeny为0
    if((flag&O_WRONLY)||(flag&O_RDWR)){
        uint8_t old=interrupt_status();
        interrupt_disable();
        if(*writeDeny){
            if(old)interrupt_enable();
            return -1;
        }else{
            *writeDeny=1;
            if(old)interrupt_enable();
        }
    }
    //
    int32_t fd_l=ProcessInstallFd(fd_g);
    //如果局部安装失败,回收全局的
    if(fd_l==-1){
        InodeClose(file->inode);
        memset(file,0,sizeof(*file));
        return -1;
    }
    return fd_l;
}
//为inode分配一个间接扇区，间接扇区紧挨着彼此,buf是调用者给的大于SECTOR_SIZE的缓冲区
int32_t AllocateIndirectBlock(struct partition*part,struct Inode*inode,void*buf){
    int idx0=-1,idx1=-1;
    if(inode->i_sectors[12]==0){
        //如果连间接块表都没建立，建立
        idx0=BlockBitmapAllocate(part);
        if(idx0!=-1){
            idx1=BlockBitmapAllocate(part);
            if(idx1!=-1){
                memset(buf,0,sizeof(SECTOR_SIZE));
                *(uint32_t*)buf=idx0;
                ide_write(part->my_disk,buf,part->sb->data_lba+idx0,1);//将表写入磁盘
                //同步block表
                BitMapUpdate(part,idx0,BITMAP_FOR_BLOCK);
                BitMapUpdate(part,idx1,BITMAP_FOR_BLOCK);
                //同步inode
                inode->i_sectors[12]=idx0+part->sb->data_lba;
                return idx1;
            }
            else{
                BitMapReset(&(part->block_bitmap),idx0);
                return -1;
            }
        }
    }
    //如果之前建立的间接表
    //读取已经分配的
    ide_read(part->my_disk,buf,inode->i_sectors[12],1);
    uint32_t*addr=(uint32_t*)buf;
    for(int i=0;i<(SECTOR_SIZE/4);++i){
        if(addr[i]==0){
            int32_t idx0=BlockBitmapAllocate(part);//申请一个新的扇区
            if(idx0==-1)return -1;
            addr[i]=idx0+part->sb->data_lba;
            ide_write(part->my_disk,addr,inode->i_sectors[12],1);//更新间接表
            BitMapUpdate(part,idx0,BITMAP_FOR_BLOCK);//同步block表
            return idx0+part->sb->data_lba;//返回分配的到的扇区
        }
    }
    //连间接表都满了，那就没办法了
    return -1;
}
//分配一个扇区给inode,这个扇区对应编号是第idx个,buf同上
int32_t AllocateOneSector(struct partition*part,struct Inode*inode,int idx,void*buf){
    if(idx<12){
        int32_t idx0=BlockBitmapAllocate(part);
        if(idx0==-1)return -1;
        idx0+=part->sb->data_lba;
        inode->i_sectors[idx]=idx0;
        return idx0;
    }
    return AllocateIndirectBlock(part,inode,buf);
}
//这个fd必须是全局的fd
uint32_t file_write(int32_t fd,const void*buf,uint32_t size){
    ASSERT(buf);//buf不能为0
    //
    ASSERT(fd>=0&&fd<MAX_FILE_CNT_SYSTEM_OPEN);
    //
    struct FILE*file=&GlobalFileTable[fd];
    ASSERT(file->inode);
    //
    void*buff=syscall_malloc(SECTOR_SIZE);
    ASSERT(buff);
    //
    uint32_t *pos=&(file->fd_pos);
    int32_t write=0;
    int32_t idx0=(*pos)/SECTOR_SIZE;//当前所在的块
    uint8_t flag=file->flag;
    int curPos=*pos;
    int32_t fileSize=file->inode->size;
    while(size&&idx0<140){
        int fillSecotor=(idx0+1)*SECTOR_SIZE-curPos;
        if(fillSecotor==0){
            //如果这个扇区填满了，换下一个扇区
            ++idx0;
            continue;
        }
        int nxtWrite=min(size,SECTOR_SIZE);
        nxtWrite=min(nxtWrite,fillSecotor);
        //申请一个区块
        int index;
        if(file->inode->i_sectors[idx0])index=file->inode->i_sectors[idx0];
        else index=AllocateOneSector(CurPartition,file->inode,idx0,buff);
        if(index==-1)break;
        //如果是读写模式打开,并且当前写入的位置小于源文件大小,肯定要先读取扇区
        if(flag==O_RDWR&&curPos<fileSize)ide_read(CurPartition->my_disk,buff,index,1);
        memcpy((void*)(((uint32_t)buff)+(curPos%SECTOR_SIZE)),(void*)(((uint32_t)buf)+write),nxtWrite);
        //写入磁盘
        ide_write(CurPartition->my_disk,buff,index,1);
        //
        write+=nxtWrite;
        curPos+=nxtWrite;
        size-=nxtWrite;
        //
        ++idx0;
    }
    //释放内存
    syscall_free(buff);
    //同步inode到内存
    file->inode->size=max(curPos,file->inode->size);//很关键
    InodeWrite(CurPartition,file->inode);
    //更新全局描述表的指针
    file->fd_pos=curPos;
    //
    return write;
}
int32_t file_read(int32_t fd,void*buf,uint32_t cnt){
    //
    ASSERT(fd>=0&&fd<MAX_FILE_CNT_SYSTEM_OPEN);
    //
    struct FILE*file=&(GlobalFileTable[fd]);
    ASSERT(file->inode);
    //
    void*buff=syscall_malloc(SECTOR_SIZE);
    ASSERT(buff);
    //
    uint32_t*addr=syscall_malloc(140*4);//存储所有的块地址
    ASSERT(addr);
    memcpy(addr,file->inode->i_sectors,sizeof(uint32_t)*12);
    if(file->inode->i_sectors[12])
    ide_read(CurPartition->my_disk,(void*)(((uint32_t)addr)+48),file->inode->i_sectors[12],1);//读取出所有的间接块地址
    //
    int32_t curRead=0;
    uint32_t*pos=&(file->fd_pos);
    int idx0=(*pos)/SECTOR_SIZE;
    int fileSize=file->inode->size;
    int curP=*pos;
    while(cnt&&curP<fileSize&idx0<140){
        int sectorLeft=(idx0+1)*SECTOR_SIZE-curP;
        if(sectorLeft==0){
            ++idx0;
            continue;
        }
        if(addr[idx0]==0)break;
        //
        int needRead=min(SECTOR_SIZE,cnt);
        needRead=min(needRead,sectorLeft);
        needRead=min(needRead,fileSize-curP);
        //读取一个扇区
        ide_read(CurPartition->my_disk,buff,addr[idx0],1);
        memcpy((void*)(((uint32_t)buf)+curRead),buff,needRead);
        //
        curRead+=needRead;
        cnt-=needRead;
        curP+=needRead;
    }
    //
    syscall_free(buff);
    syscall_free(addr);
    //
    file->fd_pos=curP;
    //
    return curRead;
}
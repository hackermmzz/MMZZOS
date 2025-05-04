#include"dir.h"
#include"super_block.h"
#include"file.h"
//
struct Dir rootDir;
//在dir目录下查询名称为name的目录或者文件,将结果存入entry
bool SearchDirEntry(struct partition*part,struct Dir*dir,const char*name,struct DirEntry*entry){
    uint32_t blockCnt=(12+BLOCK_SIZE/4);//
    uint32_t*blockOff=syscall_malloc(blockCnt*sizeof(uint32_t));//目前仅实现一级间接表
    if(!blockOff)return 0;
    for(int i=0;i<12;++i)blockOff[i]=dir->inode->i_sectors[i];
    if(dir->inode->i_sectors[12])ide_read(part->my_disk,blockOff+12,dir->inode->i_sectors[12],BLOCK_OCCUPY_SECTOR);
    uint8_t*buff=dir->dir_buf;
    //开始读取每个块
    int cnt=BLOCK_SIZE/(sizeof(struct DirEntry));
    for(int i=0;i<blockCnt;++i){
        if(!blockOff[i])continue;
        ide_read(part->my_disk,buff,blockOff[i],BLOCK_OCCUPY_SECTOR);
        struct DirEntry*entrys=(struct DirEntry*)buff;
        for(int i=0;i<cnt;++i){
            struct DirEntry*ptr=&entrys[i];
            if(ptr->filetype!=FT_UNKOWN&&!strcmp(ptr->filename,name)){
                memcpy(entry,ptr,sizeof(struct DirEntry));
                syscall_free(blockOff);
                return 1;
            }
        }
    }
    //
    syscall_free(blockOff);
    return 0;
}

void DirClose(struct Dir*dir){
    if(dir==&rootDir){//不会关闭根目录
        return;
    }
    InodeClose(dir->inode);
    syscall_free(dir);
}

struct Dir* DirOpen(struct partition*part,uint32_t index){
    struct Dir*dir=syscall_malloc(sizeof(struct Dir));
    if(dir){
        dir->inode=InodeOpen(part,index);
        dir->dir_pos=0;
    }
    return dir;
}
//把entry写入到parent目录下
bool CreateDirEntry(struct partition*part,struct Dir*parent,struct DirEntry*entry){
    uint32_t blockCnt=(12+BLOCK_SIZE/4);//
    uint32_t*blockOff=syscall_malloc(blockCnt*sizeof(uint32_t));//目前仅实现一级间接表
    if(!blockOff){
        printk("allocate memory failed!\n");
        return 0;
    }
    for(int i=0;i<12;++i)blockOff[i]=parent->inode->i_sectors[i];
    if(parent->inode->i_sectors[12])ide_read(part->my_disk,blockOff+12,parent->inode->i_sectors[12],BLOCK_OCCUPY_SECTOR);
    uint8_t*buff=parent->dir_buf;
    if(!buff){
        syscall_free(blockOff);
        return 0;
    }
    //
    bool flag=0;
    int idx0=-1,idx1=-1;
    for(int i=0;i<blockCnt;++i){
        if(blockOff[i]==0){
            //分配块
            idx0=BlockBitmapAllocate(part);
            if(idx0==-1){
                syscall_free(blockOff);
                return 0;
            }
            //前12块直接可以读写
            if(i<12){
                parent->inode->i_sectors[i]=blockOff[i]=idx0+part->sb->data_lba;
                memcpy(buff,entry,sizeof(struct DirEntry));
                ide_write(part->my_disk,buff,blockOff[i],BLOCK_OCCUPY_SECTOR);
                flag=1;
            }
            //如果这正好是第12个块，那么他充当一级间接块
            else if(i==12){
                idx1=BlockBitmapAllocate(part);//分配块
                if(idx1==-1){
                    syscall_free(blockOff);
                    for(int i=0;i<BLOCK_OCCUPY_SECTOR;++i)
                    BitMapReset(&(part->sector_bitmap),idx0+i);
                    return 0;
                }
                parent->inode->i_sectors[i]=idx0+part->sb->data_lba;//一级间接表存储的扇区
                memset(buff,0,BLOCK_SIZE);
                uint32_t data_lba=idx1+part->sb->data_lba;
                *(uint32_t*)buff=data_lba;//一级间接表第一项指向的位置
                ide_write(part->my_disk,buff,parent->inode->i_sectors[i],BLOCK_OCCUPY_SECTOR);//将位置写入磁盘
                *(uint32_t*)buff=0;
                memcpy(buff,entry,sizeof(*entry));
                ide_write(part->my_disk,buff,data_lba,BLOCK_OCCUPY_SECTOR);//把数据写入磁盘
                flag=1;
            }else{
                //大于12块的先分配数据区,并把数据区指针写入一级间接表
                ide_read(part->my_disk,buff,parent->inode->i_sectors[12],BLOCK_OCCUPY_SECTOR);//读取一级间接表
                ((uint32_t*)buff)[i-12]=idx0+part->sb->data_lba;
                ide_write(part->my_disk,buff,parent->inode->i_sectors[12],BLOCK_OCCUPY_SECTOR);//更新一级间接表
                memset(buff,0,BLOCK_SIZE);
                memcpy(buff,entry,sizeof(*entry));
                ide_write(part->my_disk,buff,idx0+part->sb->data_lba,BLOCK_OCCUPY_SECTOR);//把entry写入扇区
                flag=1;
            }
        }
        else{
            //直接读取对应的块
            ide_read(part->my_disk,buff,blockOff[i],BLOCK_OCCUPY_SECTOR);
            for(int i=0;i<BLOCK_SIZE;i+=sizeof(*entry)){
                struct DirEntry*tar=( struct DirEntry*)((uint32_t)buff+i);
                if(tar->filetype==FT_UNKOWN){
                    *tar=*entry;
                    flag=1;
                    break;
                }
            }
            if(flag)ide_write(part->my_disk,buff,blockOff[i],BLOCK_OCCUPY_SECTOR);//写入磁盘
        }
        if(flag)break;
    }
    if(flag){
        //同步位图到磁盘
        if(idx0!=-1)
        BitMapUpdate(part,idx0,BITMAP_FOR_BLOCK);
        if(idx1!=-1)
        BitMapUpdate(part,idx1,BITMAP_FOR_BLOCK);
        //同步目录大小
        ++(parent->inode->size);
        InodeWrite(part,parent->inode);
    }
    syscall_free(blockOff);
    return flag;
}

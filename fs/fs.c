#include"fs.h"
#include"inode.h"
#include"dir.h"
#include"../lib/math.h"
#include"super_block.h"
#include "file.h"
#include"../lib/kernel/aid.h"
//
struct partition*CurPartition;//默认情况下使用的分区
extern struct Dir rootDir;
//格式化分区
void partition_format(struct partition*part){
    printk("partition format begin!\n");

    uint32_t boot_sector=1;
    uint32_t super_block_sector=1;
    uint32_t inode_bitmap_sector=DIV_ROUND_UP(MAXFILE_PER_PART,SECTOR_BITS);
    uint32_t inode_table_sector=DIV_ROUND_UP(sizeof(struct Inode)*MAXFILE_PER_PART,SECTOR_SIZE);
    //已经使用掉的扇区
    uint32_t used_secotr=boot_sector+super_block_sector+inode_bitmap_sector+inode_table_sector;
    //
    uint32_t free_sector=part->sec_cnt-used_secotr;
    //处理块位图占的扇区数
    uint32_t block_bitmap_sector=DIV_ROUND_UP(free_sector,SECTOR_BITS);
    uint32_t block_bitmap_len=free_sector-block_bitmap_sector;
    block_bitmap_sector=DIV_ROUND_UP(block_bitmap_len,SECTOR_BITS);
    //超级块初始化
    struct SuperBlock superblock;
    superblock.magic=SUPERBLOCK_MAGIC;
    superblock.sector_cnt=part->sec_cnt;
    superblock.inode_cnt=MAXFILE_PER_PART;
    superblock.lba=part->start_lba;
    
    superblock.block_bitmap_lba=superblock.lba+2;//
    superblock.block_bitmap_sector_cnt=block_bitmap_sector;

    superblock.inode_bitmap_lba=superblock.block_bitmap_lba+superblock.block_bitmap_sector_cnt;
    superblock.inode_bitmap_sector_cnt=inode_bitmap_sector;//

    superblock.inode_table_lba=superblock.inode_bitmap_lba+superblock.inode_bitmap_sector_cnt;//
    superblock.inode_table_sector_cnt=inode_table_sector;//

    superblock.data_lba=superblock.inode_table_lba+superblock.inode_table_sector_cnt;//
    superblock.root_inode_idx=0;
    superblock.dir_entry_size=sizeof(struct DirEntry);
    //将超级块写入分区
    struct disk*hd=part->my_disk;
    ide_write(hd,(void*)&superblock,part->start_lba+1,1);
    printk("The SuperBlock has been written into the partition!\n");
    /////写入其他的数据
    uint32_t buffer_size=max(
        superblock.block_bitmap_sector_cnt,
        max(
            superblock.inode_bitmap_sector_cnt,
            superblock.inode_table_sector_cnt))*SECTOR_SIZE;
    uint8_t*buf=(uint8_t*)syscall_malloc(buffer_size);
    //初始化块位图
    buf[0]|=0x01;//第一块留给根目录
    uint32_t invalidLastByte=block_bitmap_len/8;
    uint32_t lastbit=block_bitmap_len%8;
    uint32_t invalidSize=SECTOR_SIZE-(invalidLastByte%SECTOR_SIZE);
    memset(&buf[invalidLastByte],0xff,invalidSize);//清空最后一个扇区不能用的位置
    int bitIdx=0;
    while(bitIdx<=lastbit){
        buf[invalidLastByte]&=~(1<<bitIdx);
        ++bitIdx;
    }
    ide_write(hd,buf,superblock.block_bitmap_lba,superblock.block_bitmap_sector_cnt);
    printk("The block bitmap has been written into the partition!\n");
    //初始化inode表位图
    memset(buf,0,buffer_size);
    buf[0]|=0x1;//第一个留给根目录
    //这里直接写入,因为inode表刚好占满1页
    ASSERT(MAXFILE_PER_PART==SECTOR_BITS);//防止我乱改
    ide_write(hd,buf,superblock.inode_bitmap_lba,superblock.inode_bitmap_sector_cnt);
    printk("The inode bitmap has been written into the partition!\n");
    //初始化inode表
    memset(buf,0,buffer_size);
    struct Inode*root=(struct Inode*)buf;
    root->index=0;
    root->size=2;//只有.和..
    root->i_sectors[0]=superblock.data_lba;//他的数据就在数据扇区起始位置
    ide_write(hd,buf,superblock.inode_table_lba,superblock.inode_table_sector_cnt);
    printk("The inode table has been written into the partition!\n");
    //将根目录写入数据扇区起始位置
    memset(buf,0,buffer_size);
    struct DirEntry*dir=(struct DirEntry*)buf;
    memcpy(dir->filename,".",1);//本目录的名字
    dir->index=0;
    dir->filetype=FT_DIR;
    ++dir;
    memcpy(dir->filename,"..",2);//父目录的名字
    dir->index=0;//根目录的父目录指向自己
    dir->filetype=FT_DIR;

    ide_write(hd,buf,superblock.data_lba,1);
    //
    syscall_free(buf);
    printk("partition format done!\n");
}

//从分区链表里面寻找到名字为arg的分区
bool mount_partition(struct ListNode*node,int arg){
    const char*name=(const char*)arg;
    struct partition*part=elem2entry(struct partition,tag,node);
    if(!strcmp(name,part->name)){
        CurPartition=part;//
        struct SuperBlock*sb=syscall_malloc(sizeof(struct SuperBlock));
        ASSERT(sb);//
        //读取超级块
        ide_read(part->my_disk,sb,part->start_lba+1,1);
        //把超级块复制过来
        CurPartition->sb=syscall_malloc(sizeof(struct SuperBlock));
        ASSERT(CurPartition->sb);
        memcpy(CurPartition->sb,sb,sizeof(struct SuperBlock));
        //读取块位图
        CurPartition->block_bitmap.bits=syscall_malloc(sb->block_bitmap_sector_cnt*SECTOR_SIZE);
        ASSERT(CurPartition->block_bitmap.bits);
        CurPartition->block_bitmap.len=sb->block_bitmap_sector_cnt*SECTOR_SIZE;
        ide_read(CurPartition->my_disk,CurPartition->block_bitmap.bits,sb->block_bitmap_lba,sb->block_bitmap_sector_cnt);
        //读取inode位图
        CurPartition->inode_bitmap.bits=syscall_malloc(sb->inode_bitmap_sector_cnt*SECTOR_SIZE);
        ASSERT(CurPartition->inode_bitmap.bits);
        CurPartition->inode_bitmap.len=sb->inode_bitmap_sector_cnt*SECTOR_SIZE;
        ide_read(CurPartition->my_disk,CurPartition->inode_bitmap.bits,sb->inode_bitmap_lba,sb->inode_bitmap_sector_cnt);
        //
        ListInit(&(CurPartition->open_inodes));
        //释放内存
        syscall_free(sb);
        //
        printk("%s is mounted!\n",name);
        return 1;
    }
    return 0;
}

void FileSystem_init(){
    //搜索所有的文件系统,如果没有,则格式化创建一个
    uint8_t channel_no=0;
    struct SuperBlock*sb=syscall_malloc(sizeof(struct SuperBlock));
    ASSERT(sb);//
    printk("Searching for file system......\n");
    while(channel_no<channel_cnt){
        struct ide_channel*channel=&ide_channel[channel_no];
        struct disk*hd=&(channel->devices[1]);//不用考虑第0个
        struct partition*part=hd->prim_parts;
        for(uint8_t partidx=0;partidx<12;++partidx,++part){
            if(partidx==4)part=hd->logic_parts;
            if(part->sec_cnt){
                //如果分区存在
                memset(sb,0,sizeof(*sb));
                //读取超级块
                ide_read(hd,sb,part->start_lba+1,1);
                //根据魔数判断是否为格式化后的文件系统
                if(sb->magic==SUPERBLOCK_MAGIC){
                    printk("%s has file system!\n",part->name);
                }else{
                    //格式化
                    printk("%s will be formated!\n",part->name);
                    partition_format(part);
                }
            }
        }
        ++channel_no;
    }
    //释放内存
    syscall_free(sb);
    //挂载分区
    const char*defalut_part="sdb1";//默认挂载这个分区
    //
    ListTraversal(&(partition_list),mount_partition,(int)defalut_part);
    //打开根目录
    rootDir.dir_pos=0;
    rootDir.inode=InodeOpen(CurPartition,0);
    //初始化全局文件表
    memset(GlobalFileTable,0,sizeof(GlobalFileTable));
}
//从path里面解析一层路径出来
const char*ParsePath(const char*path,char*buf){
    if(path[0]!='/'){
        return 0;
    }
    while(*path&&*path=='/')++path;
    while(*path&&*path!='/'){
        *buf=*path;
        ++path;
        ++buf;
    }
    *buf=0;
    return path;
}
//判断路径是否合法,把最终可以变为合法的路径放入buf
bool PreprocessingPath(const char*path,char*buf){
    //交给内核处理的路径只能是/a/b///c//d/e格式,其他都不行
    //如果前缀不是/
    if(path[0]!='/')return 0;
    //拷贝字符串
    strcpy(buf,path);
    int len=strlen(buf);
    //去掉后缀的/
    while(len>=1&&buf[len-1]=='/')--len;
    buf[len]=0;
    //文件名只能是字母、数字、下划线和.
    for(int i=0;i<len;++i){
        char ch=buf[i];
        if(ch=='/'||ch=='.'||ch=='_'||isalpha(ch)||isdigit(ch)){
            continue;
        }
        return 0;
    }
    return 1;
}
/*
buf里面存的最后解析的文件名或者目录名
查询path指定的目录或者文件是否存在，若存在填充entry，反之返回entry填充FT_UNKOWN,
parent为目标目录或者目标文件的父目录，若未解析到最后一层,那么parent为-1
dirNeed表示最后解析成功的目录项是否打开，若打开返回打开的dir，反之返回0
若文件存在或者没有解析到最后一层，返回0，反之返回最后一层父目录
*/
struct Dir* SearchFile(const char*path,char*buf,struct DirEntry*entry,int32_t*parentIdx,bool dirOpen){
    //根目录直接返回
    if(!strcmp(path,"/")||!strcmp(path,"/..")||!strcmp(path,"/.")){
        *buf=0;
        entry->filename[0]=0;
        entry->filetype=FT_DIR;
        entry->index=rootDir.inode->index;
        if(parentIdx)*parentIdx=rootDir.inode->index;//根目录的父目录还是根目录
        return dirOpen?DirOpen(CurPartition,0):0;
    }
    struct Dir*last;
    last=&rootDir;
    //逐层解析,形如/a/b/c/d
    entry->filetype=FT_UNKOWN;
    do{
        if(parentIdx)*parentIdx=last->inode->index;
        path=ParsePath(path,buf);
        if(SearchDirEntry(CurPartition,last,buf,entry)){
            //如果找到的是目录
            if(entry->filetype==FT_DIR){
                DirClose(last);
                last=DirOpen(CurPartition,entry->index);
            }
            //如果找到的是文件
            else if(entry->filetype==FT_FILE){
                if(!dirOpen)DirClose(last);
                if(*path){
                    //如果没到路径末尾就解析出了文件，直接返回
                    entry->filetype=FT_UNKOWN;
                    if(parentIdx)*parentIdx=-1;//解析错误，父目录为-1
                    return last;
                }
                return last;//否则解析成功,返回
            }
            else PANIC_MACRO("HERE!\n");
        }else{
            //如果没解析到末尾
            entry->filetype=FT_UNKOWN;
            if(!dirOpen)DirClose(last);
            if(*path&&parentIdx)*parentIdx=-1;
            return last;//解析到了末尾，只不过文件不存在
        }
    }while(*path);
    //解析成功直接返回
    if(!dirOpen)DirClose(last);
    return last;
}


bool DeleteDirectoryDirEntry(struct partition*part,struct Dir*dir, const char *entry,uint32_t*addr_buf)
{
    //
    memcpy((void*)addr_buf,(void*)(dir->inode->i_sectors),48);
    int32_t end=12;
    if(dir->inode->i_sectors[12]){
        end=140;
        ide_read(part->my_disk,&addr_buf[12],dir->inode->i_sectors[12],1);
    } 
    //
    void*buf=dir->dir_buf;
    ASSERT(buf);
    int32_t cnt=SECTOR_SIZE/sizeof(struct DirEntry);
    for(int idx=0;idx<end;++idx){
        if(addr_buf[idx]){
            //读取信息
            ide_read(part->my_disk,buf,addr_buf[idx],1);
            struct DirEntry*entrys=(struct DirEntry*)buf;
            for(int i=0;i<cnt;++i){
                if(entrys[i].filetype!=FT_UNKOWN){
                    if(!strcmp(entrys[i].filename,entry)){
                        memset(&entrys[i],0,sizeof(struct DirEntry));
                        ide_write(part->my_disk,buf,addr_buf[idx],1);
                        //同步文件夹大小
                        dir->inode->size-=1;
                        InodeWrite(part,dir->inode);
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}
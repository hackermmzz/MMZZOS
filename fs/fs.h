#ifndef MMZZ_FS_H
#define MMZZ_FS_H
#include "../device/ide.h"
#include "../lib/string.h"
/////////////////////////////////
#define MAXFILE_PER_PART 4096//每个分区可以创建的最多文件数量
#define SECTOR_SIZE 512//扇区的大小
#define SECTOR_BITS (SECTOR_SIZE*8)//扇区的位数
#define BLOCK_SIZE PAGE_SIZE//块大小,这里暂定为一页大小(方便以后虚拟内存的实现)
#define BLOCK_OCCUPY_SECTOR (BLOCK_SIZE/SECTOR_SIZE)//一块占几个扇区
#define SUPERBLOCK_MAGIC 0x2005119//我的生日
/////////////////////////////////////
extern struct partition*CurPartition;//默认情况下使用的分区
struct Dir;
struct DirEntry;
/////////////////////////////////////
enum FileType{
    FT_UNKOWN,//未知
    FT_FILE,//普通文件
    FT_DIR,//目录文件
};

struct stat{
    int32_t inode;//inode编号
    uint32_t size;//如果是目录,代表目录的数量，如果是文件，代表文件的大小
    enum FileType filetype;//文件类型
};

enum{
    SEEK_SET=1,
    SEEK_CUR,
    SEEK_END,
};
void FileSystem_init();
struct Dir* SearchFile(const char*path,char*buf,struct DirEntry*entry,int32_t*parentIdx,bool dirOpen);
bool DeleteDirectoryDirEntry(struct partition*part,struct Dir*dir,const char*entry,uint32_t*addr_buf);
bool PreprocessingPath(const char*path,char*buf);
///////////////////////////////////
#endif
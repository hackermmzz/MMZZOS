#ifndef MMZZ_DIR_H
#define MMZZ_DIR_H
#include "../lib/stdint.h"
#include "inode.h"
#include"fs.h"
#define FILENAME_MAX_LEN 16 //最大文件名长度
///////////////////////////////////////
struct Dir{
    struct Inode*inode;
    uint32_t dir_pos;//
    uint8_t dir_buf[512];//目录的数据缓存
};
struct DirEntry{
    char filename[FILENAME_MAX_LEN];//文件名
    uint32_t index;//inode的编号
    enum FileType filetype;//文件类型,是普通文件还是目录
};
/////////////////////////////////////////////////
extern struct Dir rootDir;
/////////////////////////////////////////////////
bool SearchDirEntry(struct partition*part,struct Dir*dir,const char*name,struct DirEntry*entry);
struct Dir* DirOpen(struct partition*part,uint32_t index);
void DirClose(struct Dir*dir);
bool CreateDirEntry(struct partition*part,struct Dir*parent,struct DirEntry*entry);
///////////////////////////////////////
#endif
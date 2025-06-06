#ifndef MMZZOS_STDINIT_H
#define MMZZOS_STDINIT_H
/*
该文件包含大部分常用的数据类型
*/
////////////////////////////////////////////////
#define NULL ((void*)(0))
////////////////////////////////////////////////
typedef signed char  int8_t;
typedef unsigned char uint8_t;

typedef signed short int int16_t;
typedef unsigned short int uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int int64_t;
typedef unsigned long long int uint64_t;

typedef signed char bool;
typedef unsigned char byte;

typedef uint16_t pid_t;
////////////////////////////////////////////////
#endif
#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <io.h>
#include <sys/stat.h> // 新增：用于判断是文件还是文件夹
#include <direct.h>   // 新增：用于解压时自动创建文件夹 (_mkdir)

#define MAX_CHAR 256
#define MAX_FILES 8192 // 扩容：文件夹中可能包含数千个文件
#define MAX_PATH_LEN 1024 // 新增：文件路径可能很长
#define BUF_SIZE 65536 // 64KB 高速缓冲

typedef struct Node {
    unsigned char ch;
    unsigned int freq;
    struct Node* left, * right;
} Node;

typedef struct {
    Node** data;
    int size;
} MinHeap;

typedef struct {
    unsigned int freq[MAX_CHAR];
    char codes[MAX_CHAR][MAX_CHAR];
    Node* root;
    uint32_t totalChars;
} HuffmanContext;

typedef struct {
    char* inputFiles[MAX_FILES];
    int fileCount;
    const char* archiveName;
    const char* tempName;
    int isCompressMode;
} AppConfig;
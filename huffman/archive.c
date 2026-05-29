#include "archive.h"

// 【新增辅助函数】：逐层创建目录树 (类似 mkdir -p)
static void ensureDirectoriesExist(const char* filepath) {
    char tempPath[MAX_PATH_LEN];
    strncpy(tempPath, filepath, MAX_PATH_LEN - 1);
    tempPath[MAX_PATH_LEN - 1] = '\0';

    for (int i = 0; tempPath[i] != '\0'; i++) {
        if (tempPath[i] == '\\' || tempPath[i] == '/') {
            char temp = tempPath[i];
            tempPath[i] = '\0';
            _mkdir(tempPath); // 如果文件夹已存在，它会静默失败，这正是我们想要的
            tempPath[i] = temp;
        }
    }
}

int createTarball(int fileCount, char* files[], const char* tempName) {
    FILE* out = fopen(tempName, "wb");
    if (!out) { printf("无法创建临时打包文件！\n"); return 0; }

    uint32_t count = (uint32_t)fileCount;
    fwrite(&count, sizeof(uint32_t), 1, out);

    unsigned char* buffer = (unsigned char*)malloc(BUF_SIZE);
    int successCount = 0;

    for (int i = 0; i < fileCount; i++) {
        FILE* in = fopen(files[i], "rb");
        if (!in) {
            printf("[警告] 无法打开或读取文件/目录: '%s'，已跳过。\n", files[i]);
            uint32_t nameLen = (uint32_t)strlen(files[i]);
            fwrite(&nameLen, sizeof(uint32_t), 1, out);
            fwrite(files[i], 1, nameLen, out);

            uint32_t zeroSize = 0;
            fwrite(&zeroSize, sizeof(uint32_t), 1, out);
            continue;
        }
        successCount++;
        uint32_t nameLen = (uint32_t)strlen(files[i]);
        fwrite(&nameLen, sizeof(uint32_t), 1, out);
        fwrite(files[i], 1, nameLen, out);

        fseek(in, 0, SEEK_END);
        uint32_t fileSize = (uint32_t)ftell(in);
        fseek(in, 0, SEEK_SET);
        fwrite(&fileSize, sizeof(uint32_t), 1, out);

        size_t bytes;
        while ((bytes = fread(buffer, 1, BUF_SIZE, in)) > 0) fwrite(buffer, 1, bytes, out);
        fclose(in);
    }
    free(buffer); fclose(out);
    return successCount;
}

void extractTarball(const char* tempName) {
    FILE* in = fopen(tempName, "rb");
    if (!in) return;

    uint32_t fileCount;
    if (fread(&fileCount, sizeof(uint32_t), 1, in) != 1) { fclose(in); return; }

    unsigned char* buffer = (unsigned char*)malloc(BUF_SIZE);
    for (uint32_t i = 0; i < fileCount; i++) {
        uint32_t nameLen;
        if (fread(&nameLen, sizeof(uint32_t), 1, in) != 1) break;

        if (nameLen >= MAX_PATH_LEN) nameLen = MAX_PATH_LEN - 1;
        char fileName[MAX_PATH_LEN] = { 0 };
        fread(fileName, 1, nameLen, in);
        fileName[nameLen] = '\0';

        uint32_t fileSize;
        fread(&fileSize, sizeof(uint32_t), 1, in);

        if (fileSize == 0) continue; // 大小为0的文件直接跳过

        // 【核心修改】：在创建文件前，先保障它的父目录存在
        ensureDirectoriesExist(fileName);

        FILE* out = fopen(fileName, "wb");
        if (!out) {
            printf("[错误] 无法创建文件: %s (可能是权限不足)\n", fileName);
            fseek(in, fileSize, SEEK_CUR);
            continue;
        }

        uint32_t remaining = fileSize;
        while (remaining > 0) {
            size_t toRead = remaining < BUF_SIZE ? remaining : BUF_SIZE;
            size_t bytes = fread(buffer, 1, toRead, in);
            if (bytes == 0) break;
            fwrite(buffer, 1, bytes, out);
            remaining -= (uint32_t)bytes;
        }
        fclose(out);
        printf(" -> 还原: %s\n", fileName);
    }
    free(buffer); fclose(in);
}
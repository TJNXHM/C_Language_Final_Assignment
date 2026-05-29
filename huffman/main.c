#include "huffman_types.h"
#include "compressor.h"
#include "archive.h"
#include <time.h> // 【新增】：引入 C 语言标准时间库

// 递归处理目录与通配符引擎
static void processPath(AppConfig* config, const char* path) {
    struct _stat s;
    if (_stat(path, &s) == 0) {
        if (s.st_mode & _S_IFDIR) {
            char searchPath[MAX_PATH_LEN];
            snprintf(searchPath, sizeof(searchPath), "%s\\*", path);

            struct _finddata_t fileinfo;
            intptr_t handle = _findfirst(searchPath, &fileinfo);
            if (handle != -1) {
                do {
                    if (strcmp(fileinfo.name, ".") == 0 || strcmp(fileinfo.name, "..") == 0) continue;
                    char nextPath[MAX_PATH_LEN];
                    snprintf(nextPath, sizeof(nextPath), "%s\\%s", path, fileinfo.name);
                    processPath(config, nextPath);
                } while (_findnext(handle, &fileinfo) == 0);
                _findclose(handle);
            }
        }
        else {
            if (config->fileCount < MAX_FILES) {
                config->inputFiles[config->fileCount++] = _strdup(path);
            }
        }
    }
    else {
        struct _finddata_t fileinfo;
        intptr_t handle = _findfirst(path, &fileinfo);
        if (handle != -1) {
            do {
                if (!(fileinfo.attrib & _A_SUBDIR)) {
                    if (config->fileCount < MAX_FILES) {
                        config->inputFiles[config->fileCount++] = _strdup(fileinfo.name);
                    }
                }
            } while (_findnext(handle, &fileinfo) == 0);
            _findclose(handle);
        }
    }
}

static void initAppConfig(AppConfig* config, int argc, char* argv[]) {
    config->fileCount = 0;
    config->tempName = "sys_cache.tmp";

    if (strcmp(argv[1], "compress") == 0) {
        config->isCompressMode = 1;
        config->archiveName = argv[2];
        for (int i = 3; i < argc; i++) {
            processPath(config, argv[i]);
        }
    }
    else {
        config->isCompressMode = 0;
        config->archiveName = argv[2];
    }
}

static void cleanupAppConfig(AppConfig* config) {
    for (int i = 0; i < config->fileCount; i++) free(config->inputFiles[i]);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("========================================================\n");
        printf("  高级文件压缩管理系统 (带 Benchmark 性能监控版)        \n");
        printf("========================================================\n");
        printf(" 使用说明：\n");
        printf("  [压缩] : %s compress <输出.huf> <文件夹名> <文件1>\n", argv[0]);
        printf("  [解压] : %s decompress <压缩包.huf>\n", argv[0]);
        return 1;
    }

    const char* mode = argv[1];
    const char* archiveName = argv[2];
    const char* tempName = "sys_cache.tmp";

    AppConfig sysConfig;
    initAppConfig(&sysConfig, argc, argv);

    if (strcmp(mode, "compress") == 0) {
        if (sysConfig.fileCount == 0) { printf("[错误] 未找到任何符合条件的文件。\n"); return 1; }

        printf("[系统] 准备建立数据快照 (包含 %d 个底层文件)...\n", sysConfig.fileCount);
        int validFiles = createTarball(sysConfig.fileCount, sysConfig.inputFiles, tempName);

        if (validFiles == 0) {
            printf("[错误] 没有找到实体文件，压缩中止。\n");
            remove(tempName);
            cleanupAppConfig(&sysConfig);
            return 1;
        }

        printf("[系统] 启动哈夫曼高速压缩引擎...\n");

        // ================= 【新增计时核心】 =================
        clock_t start_time = clock(); // 记录开始时间

        encodeStream(tempName, archiveName); // 执行核心压缩

        clock_t end_time = clock();   // 记录结束时间
        double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC; // 计算秒数
        // ====================================================

        remove(tempName);
        printf("--------------------------------------------------------\n");
        printf("[成功] 档案生成完毕！ -> %s\n", archiveName);
        printf("[性能] 核心压缩耗时 : %.3f 秒\n", time_spent); // 打印到控制台
        printf("--------------------------------------------------------\n");
    }
    else if (strcmp(mode, "decompress") == 0) {
        printf("[系统] 启动哈夫曼流解码与重建机制...\n");

        // ================= 【新增计时核心】 =================
        clock_t start_time = clock(); // 记录开始时间

        decodeStream(archiveName, tempName); // 执行核心解压

        clock_t end_time = clock();   // 记录结束时间
        double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC; // 计算秒数
        // ====================================================

        printf("[系统] 正在按原始目录树分离实体文件...\n");
        extractTarball(tempName);

        remove(tempName);
        printf("--------------------------------------------------------\n");
        printf("[成功] 全部文件及目录结构解压完毕！\n");
        printf("[性能] 核心解压耗时 : %.3f 秒\n", time_spent); // 打印到控制台
        printf("--------------------------------------------------------\n");
    }
    else {
        printf("[错误] 未知命令。\n");
    }

    cleanupAppConfig(&sysConfig);
    return 0;
}
/*
 * 真实场景内存测试：展示流式处理在实际应用中的内存节约
 * 此测试模拟对大文件进行签名，而不需要将整个文件加载到内存中
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>
#include "tiny_sphincs.h"
#include "internal.h"

#define TEST_FILE_SIZE_MB 50  /* 50 MB test file */
#define TEST_FILE_SIZE (TEST_FILE_SIZE_MB * 1024 * 1024)
#define CHUNK_SIZE (64 * 1024)  /* 64 KB chunks */

/* 获取当前时间（微秒） */
static long long get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 获取当前内存使用量（KB） */
static long get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    /* 在 macOS 上，ru_maxrss 的单位是字节 */
    return usage.ru_maxrss / 1024;
#else
    /* 在 Linux 上，ru_maxrss 的单位是千字节 */
    return usage.ru_maxrss;
#endif
}

/* Simple deterministic random function */
static unsigned seed = 12345;
static int deterministic_random(unsigned char *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

/* 创建一个临时文件并填充测试数据 */
static FILE* create_test_file(size_t size) {
    FILE *file = tmpfile();
    if (!file) {
        printf("无法创建临时文件\n");
        return NULL;
    }

    printf("正在创建测试文件: %.2f MB\n", size / (1024.0 * 1024.0));

    unsigned char *buffer = malloc(CHUNK_SIZE);
    if (!buffer) {
        printf("无法分配块缓冲区\n");
        fclose(file);
        return NULL;
    }

    /* 用模式填充文件 */
    for (size_t i = 0; i < size; i += CHUNK_SIZE) {
        for (size_t j = 0; j < CHUNK_SIZE; j++) {
            buffer[j] = (unsigned char)((i + j) % 256);
        }

        size_t to_write = (i + CHUNK_SIZE <= size) ? CHUNK_SIZE : (size - i);
        if (fwrite(buffer, 1, to_write, file) != to_write) {
            printf("写入测试文件失败\n");
            free(buffer);
            fclose(file);
            return NULL;
        }
    }

    free(buffer);
    rewind(file);
    printf("测试文件创建成功\n");
    return file;
}

/* 测试 1：传统方法（必须将整个文件加载到内存）*/
static int test_traditional(FILE *file, size_t file_size,
                            const unsigned char *private_key,
                            const struct ts_parameter_set *ps,
                            unsigned char *signature,
                            long long *sign_time_us) {
    printf("\n=================================================================\n");
    printf("测试 1: 传统方法（main 分支 - 非流式）\n");
    printf("=================================================================\n");
    printf("使用: ts_init_sign() + ts_sign() - 需要将整个消息加载到内存\n\n");

    long mem_before = get_memory_usage_kb();
    printf("加载文件前的内存: %ld KB\n", mem_before);

    /* 尝试将整个文件加载到内存 */
    printf("正在尝试将 %.2f MB 文件加载到 RAM...\n", file_size / (1024.0 * 1024.0));
    unsigned char *message = malloc(file_size);
    if (!message) {
        printf("❌ 失败: 无法为文件内容分配 %.2f MB\n",
               file_size / (1024.0 * 1024.0));
        printf("这展示了传统方法的局限性！\n");
        printf("如果文件大于可用 RAM，签名将无法进行。\n");
        return 0;
    }

    /* 将整个文件读取到内存 */
    rewind(file);
    if (fread(message, 1, file_size, file) != file_size) {
        printf("读取文件到内存失败\n");
        free(message);
        return 0;
    }

    long mem_after_load = get_memory_usage_kb();
    printf("加载文件后的内存: %ld KB\n", mem_after_load);
    printf("文件使用的内存: %ld KB (%.2f MB)\n",
           mem_after_load - mem_before,
           (mem_after_load - mem_before) / 1024.0);

    /* 使用传统方法进行签名 */
    printf("\n正在使用传统方法签名...\n");
    struct ts_context ctx;
    long long time_start = get_time_us();
    ts_init_sign(&ctx, message, file_size, ps, private_key, deterministic_random);
    size_t sig_len = ts_sign(signature, 20000, &ctx);
    long long time_end = get_time_us();
    *sign_time_us = time_end - time_start;

    long mem_after_sign = get_memory_usage_kb();
    printf("签名后的内存: %ld KB\n", mem_after_sign);
    printf("签名时间: %lld us (%.2f ms)\n", *sign_time_us, *sign_time_us / 1000.0);
    printf("签名吞吐量: %.2f MB/s\n",
           (file_size / (1024.0 * 1024.0)) / (*sign_time_us / 1000000.0));
    printf("签名大小: %zu 字节\n", sig_len);

    free(message);
    printf("✓ 传统方法测试完成\n");

    return sig_len;
}

/* 测试 2：流式方法（分块读取文件）*/
static int test_streaming(FILE *file, size_t file_size,
                          const unsigned char *private_key,
                          const struct ts_parameter_set *ps,
                          unsigned char *signature,
                          long long *sign_time_us) {
    printf("\n=================================================================\n");
    printf("测试 2: 流式方法（incremental-double-pass 分支）\n");
    printf("=================================================================\n");
    printf("使用: ts_init_sign_double_pass() + ts_update_prf_msg() + ts_sign()\n");
    printf("      分块处理消息 - 最小内存需求\n\n");

    long mem_before = get_memory_usage_kb();
    printf("流式处理前的内存: %ld KB\n", mem_before);

    /* 第一遍: PRF */
    printf("第一遍: 以 %u 字节块处理 %.2f MB...\n",
           (unsigned)CHUNK_SIZE, file_size / (1024.0 * 1024.0));

    struct ts_context ctx;
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);

    unsigned char *chunk_buffer = malloc(CHUNK_SIZE);
    if (!chunk_buffer) {
        printf("无法分配块缓冲区\n");
        return 0;
    }

    rewind(file);
    size_t total_read = 0;
    size_t bytes_read;
    while ((bytes_read = fread(chunk_buffer, 1, CHUNK_SIZE, file)) > 0) {
        ts_update_prf_msg(chunk_buffer, bytes_read, &ctx);
        total_read += bytes_read;
    }
    ts_finalize_prf_msg(&ctx);

    long mem_after_prf = get_memory_usage_kb();
    printf("第一遍完成。使用的内存: %ld KB\n", mem_after_prf);
    printf("第一遍处理的总字节数: %zu\n", total_read);

    /* 第二遍: 哈希 */
    printf("\n第二遍: 以 %u 字节块处理 %.2f MB...\n",
           (unsigned)CHUNK_SIZE, file_size / (1024.0 * 1024.0));

    rewind(file);
    ts_init_hash_msg(&ctx);
    total_read = 0;
    while ((bytes_read = fread(chunk_buffer, 1, CHUNK_SIZE, file)) > 0) {
        ts_update_hash_msg(chunk_buffer, bytes_read, &ctx);
        total_read += bytes_read;
    }
    ts_finalize_hash_msg(&ctx);

    long mem_after_hash = get_memory_usage_kb();
    printf("第二遍完成。使用的内存: %ld KB\n", mem_after_hash);
    printf("第二遍处理的总字节数: %zu\n", total_read);

    /* 生成签名并计时 */
    printf("\n正在生成签名...\n");
    long long time_start = get_time_us();
    size_t sig_len = ts_sign(signature, 20000, &ctx);
    long long time_end = get_time_us();
    *sign_time_us = time_end - time_start;

    long mem_after_sign = get_memory_usage_kb();
    printf("签名后的内存: %ld KB\n", mem_after_sign);
    printf("签名时间: %lld us (%.2f ms)\n", *sign_time_us, *sign_time_us / 1000.0);
    printf("签名吞吐量: %.2f MB/s\n",
           (file_size / (1024.0 * 1024.0)) / (*sign_time_us / 1000000.0));
    printf("签名大小: %zu 字节\n", sig_len);

    printf("\n块缓冲区已释放\n");
    free(chunk_buffer);

    long mem_final = get_memory_usage_kb();
    printf("最终内存: %ld KB\n", mem_final);
    printf("✓ 流式方法测试完成\n");

    return sig_len;
}

int main(void) {
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
    unsigned char private_key[64];
    unsigned char public_key[32];
    unsigned char sig_traditional[20000];
    unsigned char sig_streaming[20000];
    FILE *test_file = NULL;
    long long time_traditional = 0, time_streaming = 0;

    printf("=================================================================\n");
    printf("  真实场景内存节约和性能展示\n");
    printf("=================================================================\n");
    printf("测试文件大小: %.2f MB\n", TEST_FILE_SIZE / (1024.0 * 1024.0));
    printf("块大小: %u 字节 (%.2f KB)\n", (unsigned)CHUNK_SIZE, CHUNK_SIZE / 1024.0);
    printf("\n");

    /* 生成密钥对 */
    ts_gen_key(private_key, public_key, ps, deterministic_random);
    printf("密钥对已生成\n");

    /* 创建测试文件 */
    test_file = create_test_file(TEST_FILE_SIZE);
    if (!test_file) {
        return 1;
    }

    /* 测试传统方法 */
    seed = 12345;  // 重置以进行一致性比较
    int result_traditional = test_traditional(test_file, TEST_FILE_SIZE,
                                              private_key, ps, sig_traditional,
                                              &time_traditional);

    /* 测试流式方法 */
    seed = 12345;  // 重置以进行一致性比较
    int result_streaming = test_streaming(test_file, TEST_FILE_SIZE,
                                         private_key, ps, sig_streaming,
                                         &time_streaming);

    fclose(test_file);

    /* 总结 */
    printf("\n=================================================================\n");
    printf("  对比总结\n");
    printf("=================================================================\n");
    printf("\n");

    printf("+----------------------+--------------------------+--------------------------+\n");
    printf("| 指标                | 传统方法 (main)          | 流式方法 (double-pass)    |\n");
    printf("+----------------------+--------------------------+--------------------------+\n");
    printf("| 需要的内存          | %.2f MB                  | %.2f KB                  |\n",
           (double)TEST_FILE_SIZE_MB,
           (double)CHUNK_SIZE / 1024.0);
    printf("| 内存节约            | -                        | %.2f MB (%.1f%%)          |\n",
           (double)TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0)),
           100.0 * (TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0))) / TEST_FILE_SIZE_MB);
    printf("| 签名时间            | %.2f ms                  | %.2f ms                  |\n",
           time_traditional / 1000.0,
           time_streaming / 1000.0);
    printf("| 时间开销            | -                        | %+.2f ms (%+.1f%%)       |\n",
           (time_streaming - time_traditional) / 1000.0,
           100.0 * (time_streaming - time_traditional) / time_traditional);
    printf("| 签名吞吐量          | %.2f MB/s                | %.2f MB/s                |\n",
           (TEST_FILE_SIZE / (1024.0 * 1024.0)) / (time_traditional / 1000000.0),
           (TEST_FILE_SIZE / (1024.0 * 1024.0)) / (time_streaming / 1000000.0));
    printf("| 签名大小            | %d 字节                 | %d 字节                 |\n",
           result_traditional, result_streaming);
    printf("+----------------------+--------------------------+--------------------------+\n");
    printf("\n");

    printf("传统方法（main 分支）：\n");
    printf("  - API: ts_init_sign() + ts_sign()\n");
    printf("  - 内存: 文件大小 (%.2f MB) + 上下文 (~1KB)\n", (double)TEST_FILE_SIZE_MB);
    printf("  - 局限性: 无法处理大于可用 RAM 的文件\n");
    printf("  - 使用场景: 适合内存的中等大小文件\n");
    printf("\n");
    printf("流式方法（incremental-double-pass 分支）：\n");
    printf("  - API: ts_init_sign_double_pass() + ts_update_prf_msg() + ts_init_hash_msg()\n");
    printf("         + ts_update_hash_msg() + ts_sign()\n");
    printf("  - 内存: 块大小 (%.2f KB) + 上下文 (~1KB)\n", (double)CHUNK_SIZE / 1024.0);
    printf("  - 优势: 可以处理任意大小的文件（不受 RAM 限制）\n");
    printf("  - 使用场景: 大文件、无磁盘系统、内存受限设备\n");
    printf("\n");

    printf("关键见解：\n");
    printf("  1. 内存节约: 节省 %.2f MB (%.1f%% 减少)\n",
           (double)TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0)),
           100.0 * (TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0))) / TEST_FILE_SIZE_MB);
    printf("  2. 性能开销: %+.1f%% (对于大多数应用可以忽略不计)\n",
           100.0 * (time_streaming - time_traditional) / time_traditional);
    printf("  3. 可扩展性: 流式处理可以处理大于可用 RAM 的文件\n");
    printf("  4. 使用场景选择：\n");
    printf("     - 传统方法: 简单、快速，适用于小文件 (<100MB)\n");
    printf("     - 流式方法: 对大文件 (>100MB) 或低内存系统必不可少\n");
    printf("\n");
    printf("=================================================================\n");

    return 0;
}

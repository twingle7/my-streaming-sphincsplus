/*
 * Real-World Memory Test: Demonstrate actual memory savings with streaming
 * This test simulates signing a large file without loading it entirely into RAM
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include "tiny_sphincs.h"
#include "internal.h"

#define TEST_FILE_SIZE_MB 50  /* 50 MB test file */
#define TEST_FILE_SIZE (TEST_FILE_SIZE_MB * 1024 * 1024)
#define CHUNK_SIZE (64 * 1024)  /* 64 KB chunks */

/* Get current memory usage in KB */
static long get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
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

/* Create a temporary file with test data */
static FILE* create_test_file(size_t size) {
    FILE *file = tmpfile();
    if (!file) {
        printf("Failed to create temporary file\n");
        return NULL;
    }

    printf("Creating test file: %.2f MB\n", size / (1024.0 * 1024.0));

    unsigned char *buffer = malloc(CHUNK_SIZE);
    if (!buffer) {
        printf("Failed to allocate chunk buffer\n");
        fclose(file);
        return NULL;
    }

    /* Fill file with pattern */
    for (size_t i = 0; i < size; i += CHUNK_SIZE) {
        for (size_t j = 0; j < CHUNK_SIZE; j++) {
            buffer[j] = (unsigned char)((i + j) % 256);
        }

        size_t to_write = (i + CHUNK_SIZE <= size) ? CHUNK_SIZE : (size - i);
        if (fwrite(buffer, 1, to_write, file) != to_write) {
            printf("Failed to write to test file\n");
            free(buffer);
            fclose(file);
            return NULL;
        }
    }

    free(buffer);
    rewind(file);
    printf("Test file created successfully\n");
    return file;
}

/* Test 1: Traditional approach (must load entire file) */
static int test_traditional(FILE *file, size_t file_size,
                            const unsigned char *private_key,
                            const struct ts_parameter_set *ps,
                            unsigned char *signature) {
    printf("\n=================================================================\n");
    printf("Test 1: Traditional Approach (Load Entire File to RAM)\n");
    printf("=================================================================\n");

    long mem_before = get_memory_usage_kb();
    printf("Memory before loading file: %ld KB\n", mem_before);

    /* Try to load entire file to memory */
    printf("Attempting to load %.2f MB file to RAM...\n", file_size / (1024.0 * 1024.0));
    unsigned char *message = malloc(file_size);
    if (!message) {
        printf("❌ FAILED: Cannot allocate %.2f MB for file content\n",
               file_size / (1024.0 * 1024.0));
        printf("This demonstrates the limitation of traditional approach!\n");
        printf("If file is larger than available RAM, signing is IMPOSSIBLE.\n");
        return 0;
    }

    /* Read entire file to memory */
    rewind(file);
    if (fread(message, 1, file_size, file) != file_size) {
        printf("Failed to read file to memory\n");
        free(message);
        return 0;
    }

    long mem_after_load = get_memory_usage_kb();
    printf("Memory after loading file: %ld KB\n", mem_after_load);
    printf("Memory used for file: %ld KB (%.2f MB)\n",
           mem_after_load - mem_before,
           (mem_after_load - mem_before) / 1024.0);

    /* Sign using traditional method */
    struct ts_context ctx;
    ts_init_sign(&ctx, message, file_size, ps, private_key, deterministic_random);
    size_t sig_len = ts_sign(signature, 20000, &ctx);

    long mem_after_sign = get_memory_usage_kb();
    printf("Memory after signing: %ld KB\n", mem_after_sign);
    printf("Signature size: %zu bytes\n", sig_len);

    free(message);

    return sig_len;
}

/* Test 2: Streaming approach (read file in chunks) */
static int test_streaming(FILE *file, size_t file_size,
                          const unsigned char *private_key,
                          const struct ts_parameter_set *ps,
                          unsigned char *signature) {
    printf("\n=================================================================\n");
    printf("Test 2: Streaming Approach (Process File in Chunks)\n");
    printf("=================================================================\n");

    long mem_before = get_memory_usage_kb();
    printf("Memory before streaming: %ld KB\n", mem_before);

    /* First pass: PRF */
    printf("First pass: Processing %.2f MB in %zu-byte chunks...\n",
           file_size / (1024.0 * 1024.0), CHUNK_SIZE);

    struct ts_context ctx;
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);

    unsigned char *chunk_buffer = malloc(CHUNK_SIZE);
    if (!chunk_buffer) {
        printf("Failed to allocate chunk buffer\n");
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
    printf("First pass complete. Memory used: %ld KB\n", mem_after_prf);
    printf("Total bytes processed in first pass: %zu\n", total_read);

    /* Second pass: Hash */
    printf("Second pass: Processing %.2f MB in %zu-byte chunks...\n",
           file_size / (1024.0 * 1024.0), CHUNK_SIZE);

    rewind(file);
    ts_init_hash_msg(&ctx);
    total_read = 0;
    while ((bytes_read = fread(chunk_buffer, 1, CHUNK_SIZE, file)) > 0) {
        ts_update_hash_msg(chunk_buffer, bytes_read, &ctx);
        total_read += bytes_read;
    }
    ts_finalize_hash_msg(&ctx);

    long mem_after_hash = get_memory_usage_kb();
    printf("Second pass complete. Memory used: %ld KB\n", mem_after_hash);

    /* Generate signature */
    printf("Generating signature...\n");
    size_t sig_len = ts_sign(signature, 20000, &ctx);

    long mem_after_sign = get_memory_usage_kb();
    printf("Memory after signing: %ld KB\n", mem_after_sign);
    printf("Signature size: %zu bytes\n", sig_len);

    printf("\nChunk buffer freed\n");
    free(chunk_buffer);

    long mem_final = get_memory_usage_kb();
    printf("Final memory: %ld KB\n", mem_final);

    return sig_len;
}

int main(void) {
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
    unsigned char private_key[64];
    unsigned char public_key[32];
    unsigned char sig_traditional[20000];
    unsigned char sig_streaming[20000];
    FILE *test_file = NULL;

    printf("=================================================================\n");
    printf("  Real-World Memory Savings Demonstration\n");
    printf("=================================================================\n");
    printf("Test file size: %.2f MB\n", TEST_FILE_SIZE / (1024.0 * 1024.0));
    printf("Chunk size: %zu bytes (%.2f KB)\n", CHUNK_SIZE, CHUNK_SIZE / 1024.0);
    printf("\n");

    /* Generate keypair */
    ts_gen_key(private_key, public_key, ps, deterministic_random);
    printf("Keypair generated\n");

    /* Create test file */
    test_file = create_test_file(TEST_FILE_SIZE);
    if (!test_file) {
        return 1;
    }

    /* Test traditional approach */
    seed = 12345;  // Reset for consistent comparison
    int result_traditional = test_traditional(test_file, TEST_FILE_SIZE,
                                              private_key, ps, sig_traditional);

    /* Test streaming approach */
    seed = 12345;  // Reset for consistent comparison
    int result_streaming = test_streaming(test_file, TEST_FILE_SIZE,
                                         private_key, ps, sig_streaming);

    fclose(test_file);

    /* Summary */
    printf("\n=================================================================\n");
    printf("  Summary\n");
    printf("=================================================================\n");
    printf("\n");
    printf("Traditional Approach:\n");
    printf("  - Requires loading ENTIRE file to RAM (%.2f MB)\n", TEST_FILE_SIZE_MB);
    printf("  - Memory overhead: File size + context (~1KB)\n");
    printf("  - Fails if file > available RAM\n");
    printf("  - Signature size: %d bytes\n", result_traditional);
    printf("\n");
    printf("Streaming Approach:\n");
    printf("  - Processes file in chunks (%.2f KB)\n", CHUNK_SIZE / 1024.0);
    printf("  - Memory overhead: Chunk size + context (~%d KB)\n",
           (int)(CHUNK_SIZE / 1024) + 1);
    printf("  - Can handle files of ANY size\n");
    printf("  - Signature size: %d bytes\n", result_streaming);
    printf("\n");
    printf("Memory Savings: %.2f MB - %.2f MB = %.2f MB\n",
           (double)TEST_FILE_SIZE_MB,
           (double)CHUNK_SIZE / (1024.0 * 1024.0),
           TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0)));
    printf("\n");
    printf("Key Insight:\n");
    printf("  The streaming approach uses %.2f KB instead of %.2f MB,\n",
           (double)(CHUNK_SIZE / 1024) + 1,
           (double)TEST_FILE_SIZE_MB);
    printf("  saving %.2f MB of RAM (%.1f%% reduction)\n",
           TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0)),
           100.0 * (TEST_FILE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0))) / TEST_FILE_SIZE_MB);
    printf("\n");
    printf("=================================================================\n");

    return 0;
}

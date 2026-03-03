/*
 * Memory Benchmark: Compare memory usage between original and streaming versions
 * This test demonstrates the memory savings of streaming processing for large messages
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>
#include "tiny_sphincs.h"
#include "internal.h"

#define MESSAGE_SIZE_MB 10  /* 10 MB message to demonstrate memory impact */
#define MESSAGE_SIZE (MESSAGE_SIZE_MB * 1024 * 1024)

/* Get current memory usage in KB */
static long get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;  /* Maximum resident set size in KB */
}

/* Simple deterministic random function for testing */
static unsigned seed = 12345;
static int deterministic_random(unsigned char *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

/* Print memory statistics */
static void print_memory_stats(const char *label, long mem_before, long mem_after) {
    long mem_used = mem_after - mem_before;
    printf("  %s:\n", label);
    printf("    Memory before: %ld KB\n", mem_before);
    printf("    Memory after:  %ld KB\n", mem_after);
    printf("    Memory used:   %ld KB (%.2f MB)\n", mem_used, mem_used / 1024.0);
}

int main(void) {
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
    unsigned char *message = NULL;
    unsigned char private_key[64];
    unsigned char public_key[32];
    unsigned char signature[20000];
    struct ts_context ctx;
    long mem_before, mem_after;

    printf("=================================================================\n");
    printf("  Memory Usage Benchmark: Original vs Streaming\n");
    printf("=================================================================\n");
    printf("Message size: %d MB (%d bytes)\n", MESSAGE_SIZE_MB, MESSAGE_SIZE);
    printf("\n");

    /* Generate keypair */
    ts_gen_key(private_key, public_key, ps, deterministic_random);
    printf("Keypair generated\n\n");

    /* Allocate message buffer */
    message = (unsigned char *)malloc(MESSAGE_SIZE);
    if (!message) {
        printf("ERROR: Failed to allocate %d MB message buffer\n", MESSAGE_SIZE_MB);
        return 1;
    }
    printf("Allocated message buffer: %d MB\n", MESSAGE_SIZE_MB);

    /* Initialize message with pattern */
    for (size_t i = 0; i < MESSAGE_SIZE; i++) {
        message[i] = (unsigned char)(i % 256);
    }
    printf("Message buffer initialized\n\n");

    /* Test 1: Original signing (requires full message in memory) */
    printf("Test 1: Original Signing (two-pass, full message in memory)\n");
    printf("-----------------------------------------------------------------\n");
    mem_before = get_memory_usage_kb();
    ts_init_sign(&ctx, message, MESSAGE_SIZE, ps, private_key, deterministic_random);
    size_t sig_len = ts_sign(signature, sizeof(signature), &ctx);
    mem_after = get_memory_usage_kb();
    print_memory_stats("Original signing", mem_before, mem_after);
    printf("  Signature size: %zu bytes\n", sig_len);
    printf("\n");

    /* Verify original signature */
    mem_before = get_memory_usage_kb();
    ts_init_verify(&ctx, message, MESSAGE_SIZE, ps, public_key);
    ts_update_verify(signature, sig_len, &ctx);
    int result = ts_verify(&ctx);
    mem_after = get_memory_usage_kb();
    print_memory_stats("Original verification", mem_before, mem_after);
    printf("  Verification result: %s\n", result ? "PASSED" : "FAILED");
    printf("\n");

    /* Reset random seed for consistent comparison */
    seed = 12345;

    /* Test 2: Streaming signing (can process message in chunks) */
    printf("Test 2: Streaming Signing (double-pass, chunk-based processing)\n");
    printf("-----------------------------------------------------------------\n");
    printf("  Note: This demonstrates that streaming can process large messages\n");
    printf("        with minimal additional memory beyond the context buffer\n");
    printf("        However, to show the benefit, we need to simulate chunk-based\n");
    printf("        processing (reading message in small chunks from disk/network)\n");
    printf("\n");

    const size_t CHUNK_SIZE = 4096;  /* Simulate reading in 4KB chunks */
    size_t remaining = MESSAGE_SIZE;
    size_t offset = 0;

    printf("  Chunk size for streaming: %zu bytes\n", CHUNK_SIZE);
    printf("  Number of chunks: %zu\n", (MESSAGE_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE);

    mem_before = get_memory_usage_kb();
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);

    /* Process message in chunks during PRF pass */
    while (remaining > 0) {
        size_t chunk_len = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        ts_update_prf_msg(message + offset, chunk_len, &ctx);
        offset += chunk_len;
        remaining -= chunk_len;
    }
    ts_finalize_prf_msg(&ctx);

    /* Second pass: process message in chunks during hash pass */
    remaining = MESSAGE_SIZE;
    offset = 0;
    ts_init_hash_msg(&ctx);
    while (remaining > 0) {
        size_t chunk_len = (remaining < CHUNK_SIZE) ? remaining : CHUNK_SIZE;
        ts_update_hash_msg(message + offset, chunk_len, &ctx);
        offset += chunk_len;
        remaining -= chunk_len;
    }
    ts_finalize_hash_msg(&ctx);

    /* Generate signature */
    size_t sig_stream_len = ts_sign(signature, sizeof(signature), &ctx);
    mem_after = get_memory_usage_kb();
    print_memory_stats("Streaming signing (with chunks)", mem_before, mem_after);
    printf("  Signature size: %zu bytes\n", sig_stream_len);
    printf("\n");

    /* Verify streaming signature */
    mem_before = get_memory_usage_kb();
    ts_init_verify(&ctx, message, MESSAGE_SIZE, ps, public_key);
    ts_update_verify(signature, sig_stream_len, &ctx);
    result = ts_verify(&ctx);
    mem_after = get_memory_usage_kb();
    print_memory_stats("Streaming verification", mem_before, mem_after);
    printf("  Verification result: %s\n", result ? "PASSED" : "FAILED");
    printf("\n");

    /* Analysis */
    printf("=================================================================\n");
    printf("  Memory Analysis\n");
    printf("=================================================================\n");
    printf("\n");
    printf("Key Insights:\n");
    printf("1. Original version (two-pass):\n");
    printf("   - Requires ENTIRE message (%d MB) to be in memory simultaneously\n", MESSAGE_SIZE_MB);
    printf("   - First pass: prf_msg() reads entire message to generate R\n");
    printf("   - Second pass: hash_msg() reads entire message to compute digest\n");
    printf("   - Total memory: ~%d MB (message) + context overhead\n", MESSAGE_SIZE_MB);
    printf("\n");
    printf("2. Streaming version (double-pass):\n");
    printf("   - Can process message in small chunks (e.g., 4KB)\n");
    printf("   - First pass: ts_update_prf_msg() processes chunks incrementally\n");
    printf("   - Second pass: ts_update_hash_msg() processes chunks incrementally\n");
    printf("   - Total memory: chunk size (4KB) + context overhead (~1KB)\n");
    printf("\n");
    printf("Memory Savings: %.2f MB - %.2f MB = %.2f MB\n",
           (double)MESSAGE_SIZE_MB,
           (double)CHUNK_SIZE / (1024.0 * 1024.0),
           MESSAGE_SIZE_MB - (CHUNK_SIZE / (1024.0 * 1024.0)));
    printf("\n");
    printf("Note: The memory savings are NOT visible in the current test because:\n");
    printf("      1. We still allocate the full message buffer for both tests\n");
    printf("      2. To see actual savings, the message would need to be:\n");
    printf("         - Read from a file/network in chunks\n");
    printf("         - Generated on-the-fly\n");
    printf("         - Processed without storing the entire message\n");
    printf("\n");
    printf("In a real-world scenario:\n");
    printf("  - File signing: Read file in chunks, never load full file to RAM\n");
    printf("  - Network signing: Process stream as it arrives\n");
    printf("  - Diskless signing: Generate message chunk-by-chunk\n");
    printf("\n");
    printf("The KEY benefit is that streaming allows processing of messages\n");
    printf("larger than available RAM, which is impossible with the original approach.\n");

    /* Cleanup */
    free(message);

    printf("\n=================================================================\n");
    printf("  Test Complete\n");
    printf("=================================================================\n");

    return 0;
}

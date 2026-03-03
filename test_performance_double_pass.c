/*
 * Performance Test for Incremental Double-Pass Version
 * Comprehensive performance testing with signature output for comparison with main branch
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "tiny_sphincs.h"
#include "internal.h"

/* Test configuration */
#define NUM_ITERATIONS 10

/* Simple random number generator for testing */
static int simple_random(unsigned char *buf, size_t len) {
    static unsigned int seed = 12345;
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        buf[i] = (unsigned char)(seed >> 24);
    }
    return 1;
}

/* Get current time in microseconds */
static long long get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Print signature hex dump */
static void print_signature(const unsigned char *signature, unsigned signature_size, const char *label) {
    printf("\n%s (first 64 bytes, %u bytes total):\n", label, signature_size);
    printf("  ");
    for (unsigned i = 0; i < 64 && i < signature_size; i++) {
        printf("%02x", signature[i]);
        if ((i + 1) % 32 == 0 && i != 63) {
            printf("\n  ");
        }
    }
    printf("\n");
}

/* Print public key hex dump */
static void print_public_key(const unsigned char *public_key, unsigned public_key_size) {
    printf("\nPublic Key (%u bytes):\n", public_key_size);
    printf("  ");
    for (unsigned i = 0; i < public_key_size && i < 64; i++) {
        printf("%02x", public_key[i]);
        if ((i + 1) % 32 == 0) {
            printf("\n  ");
        }
    }
    printf("\n");
}

/* Print message hex dump (first 64 bytes) */
static void print_message(const unsigned char *message, size_t msg_len) {
    printf("\nMessage (first 64 bytes, %zu bytes total):\n", msg_len);
    printf("  ");
    for (size_t i = 0; i < 64 && i < msg_len; i++) {
        printf("%02x", message[i]);
        if ((i + 1) % 32 == 0) {
            printf("\n  ");
        }
    }
    printf("\n");
}

/* Test a single parameter set with double-pass incremental signing */
static void test_parameter_set(const char *name, const struct ts_parameter_set *ps) {
    printf("\n");
    printf("========================================\n");
    printf("Testing Parameter Set: %s\n", name);
    printf("========================================\n");

    /* Print basic parameters */
    printf("\nBasic Parameters:\n");
    printf("  Hash size (n):         %d bytes\n", ps->n);
    printf("  FORS trees (k):       %d\n", ps->k);
    printf("  FORS tree height (t): %d\n", ps->t);
    printf("  Hypertree height (h): %d\n", ps->h);
    printf("  Merkle levels (d):    %d\n", ps->d);
    printf("  Merkle height:        %d\n", ps->merkle_h);
    printf("  Hash function:        %s\n", ps->sha2 ? "SHA2" : "SHAKE256");

    /* Calculate sizes */
    unsigned private_key_size = ts_size_private_key(ps);
    unsigned public_key_size = ts_size_public_key(ps);
    unsigned signature_size = ts_size_signature(ps);
    unsigned context_size = sizeof(struct ts_context);

    printf("\nSize Metrics:\n");
    printf("  Private Key Size:      %u bytes\n", private_key_size);
    printf("  Public Key Size:       %u bytes\n", public_key_size);
    printf("  Signature Size:        %u bytes\n", signature_size);
    printf("  Context Buffer Size:   %u bytes\n", context_size);

    /* Calculate buffer usage breakdown */
    printf("\nBuffer Usage Breakdown:\n");
    printf("  Main buffer (TS_MAX_HASH):          %d bytes\n", TS_MAX_HASH);
    printf("  ADR structure:                      %d bytes\n", ADR_SIZE);
    printf("  Auth path buffer:                   %d bytes\n", TS_MAX_HASH);
    printf("  FORS stack max:                     %d bytes\n",
           (TS_MAX_T-1) * TS_MAX_HASH);
    printf("  Merkle stack max:                   %d bytes\n",
           (TS_MAX_MERKLE_H-1) * TS_MAX_HASH);
    printf("  WOTS digits max:                    %d bytes\n",
           TS_MAX_WOTS_DIGITS);
    printf("  FORS nodes array:                    %lu bytes\n",
           TS_MAX_FORS * sizeof(unsigned short));
    printf("  FORS node stack:                    %d bytes\n",
           (TS_MAX_T-1) * TS_MAX_HASH);

    /* Generate keys */
    unsigned char *private_key = malloc(private_key_size);
    unsigned char *public_key = malloc(public_key_size);
    unsigned char *signature = malloc(signature_size);

    if (!private_key || !public_key || !signature) {
        printf("ERROR: Memory allocation failed\n");
        free(private_key);
        free(public_key);
        free(signature);
        return;
    }

    printf("\nKey Generation:\n");
    long long start = get_time_us();
    if (!ts_gen_key(private_key, public_key, ps, simple_random)) {
        printf("ERROR: Key generation failed\n");
        free(private_key);
        free(public_key);
        free(signature);
        return;
    }
    long long end = get_time_us();
    printf("  Key generation time: %lld us (%.2f ms)\n",
           end - start, (end - start) / 1000.0);

    /* Print public key for comparison */
    print_public_key(public_key, public_key_size);

    /* Test signing for different message sizes */
    printf("\nDouble-Pass Signing Performance:\n");
    printf("  Message Size |  Total Time (us) |  Avg Time (us) |  Throughput (KB/s)\n");
    printf("  ------------+------------------+----------------+------------------\n");

    size_t msg_sizes[] = {32, 256, 1024, 8192};
    int num_sizes = 4;

    for (int i = 0; i < num_sizes; i++) {
        size_t msg_len = msg_sizes[i];
        unsigned char *message = malloc(msg_len);
        if (!message) {
            printf("ERROR: Memory allocation for message failed\n");
            break;
        }

        /* Fill message with test data (deterministic for reproducibility) */
        for (size_t j = 0; j < msg_len; j++) {
            message[j] = (unsigned char)(j & 0xFF);
        }

        /* Warm-up run with double-pass signing */
        struct ts_context ctx;
        memset(&ctx, 0, sizeof(ctx));  /* Clear context */
        ts_init_sign_double_pass(&ctx, ps, private_key, simple_random);

        /* Pass 1: Compute R */
        for (size_t offset = 0; offset < msg_len; offset += 256) {
            size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
            ts_update_prf_msg(&message[offset], len, &ctx);
        }
        ts_finalize_prf_msg(&ctx);

        /* Pass 2: Compute message hash using R */
        ts_init_hash_msg(&ctx);
        for (size_t offset = 0; offset < msg_len; offset += 256) {
            size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
            ts_update_hash_msg(&message[offset], len, &ctx);
        }
        ts_finalize_hash_msg(&ctx);

        /* Generate signature */
        ts_sign(signature, signature_size, &ctx);

        /* Measure double-pass signing time */
        long long total_time = 0;
        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            struct ts_context ctx;
            memset(&ctx, 0, sizeof(ctx));  /* Clear context */
            start = get_time_us();

            /* Double-pass signing */
            ts_init_sign_double_pass(&ctx, ps, private_key, simple_random);

            /* Pass 1 */
            for (size_t offset = 0; offset < msg_len; offset += 256) {
                size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
                ts_update_prf_msg(&message[offset], len, &ctx);
            }
            ts_finalize_prf_msg(&ctx);

            /* Pass 2 */
            ts_init_hash_msg(&ctx);
            for (size_t offset = 0; offset < msg_len; offset += 256) {
                size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
                ts_update_hash_msg(&message[offset], len, &ctx);
            }
            ts_finalize_hash_msg(&ctx);

            /* Generate signature */
            ts_sign(signature, signature_size, &ctx);

            end = get_time_us();
            total_time += (end - start);
        }

        double avg_time = (double)total_time / NUM_ITERATIONS;
        double throughput = (msg_len / 1024.0) / (avg_time / 1000000.0);

        printf("  %12zu | %16lld | %14.2f | %16.2f\n",
               msg_len, total_time, avg_time, throughput);

        /* For the last message size, print signature and message for comparison */
        if (i == num_sizes - 1) {
            print_message(message, msg_len);
            print_signature(signature, signature_size, "Signature from Double-Pass");
        }

        free(message);
    }

    /* Test verification */
    printf("\nVerification Performance:\n");
    printf("  Message Size |  Total Time (us) |  Avg Time (us) |  Throughput (KB/s)\n");
    printf("  ------------+------------------+----------------+------------------\n");

    for (int i = 0; i < num_sizes; i++) {
        size_t msg_len = msg_sizes[i];
        unsigned char *message = malloc(msg_len);
        if (!message) {
            printf("ERROR: Memory allocation for message failed\n");
            break;
        }

        /* Fill message with test data */
        for (size_t j = 0; j < msg_len; j++) {
            message[j] = (unsigned char)(j & 0xFF);
        }

        /* Generate signature using double-pass */
        struct ts_context sign_ctx;
        memset(&sign_ctx, 0, sizeof(sign_ctx));  /* Clear context */
        ts_init_sign_double_pass(&sign_ctx, ps, private_key, simple_random);

        /* Pass 1 */
        for (size_t offset = 0; offset < msg_len; offset += 256) {
            size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
            ts_update_prf_msg(&message[offset], len, &sign_ctx);
        }
        ts_finalize_prf_msg(&sign_ctx);

        /* Pass 2 */
        ts_init_hash_msg(&sign_ctx);
        for (size_t offset = 0; offset < msg_len; offset += 256) {
            size_t len = (offset + 256 > msg_len) ? (msg_len - offset) : 256;
            ts_update_hash_msg(&message[offset], len, &sign_ctx);
        }
        ts_finalize_hash_msg(&sign_ctx);

        /* Generate signature */
        ts_sign(signature, signature_size, &sign_ctx);

        /* Warm-up run */
        struct ts_context verify_ctx;
        memset(&verify_ctx, 0, sizeof(verify_ctx));  /* Clear context */
        ts_init_verify(&verify_ctx, message, msg_len, ps, public_key);
        ts_update_verify(signature, signature_size, &verify_ctx);
        int verify_result = ts_verify(&verify_ctx);
        printf("  Warm-up verification result: %s\n", verify_result ? "PASSED" : "FAILED");

        /* Measure verification time */
        long long total_time = 0;
        int failed_count = 0;
        for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
            struct ts_context verify_ctx;
            memset(&verify_ctx, 0, sizeof(verify_ctx));  /* Clear context */
            start = get_time_us();
            ts_init_verify(&verify_ctx, message, msg_len, ps, public_key);
            ts_update_verify(signature, signature_size, &verify_ctx);
            int result = ts_verify(&verify_ctx);
            end = get_time_us();
            total_time += (end - start);

            if (!result) {
                failed_count++;
            }
        }
        if (failed_count > 0) {
            printf("  WARNING: %d out of %d verifications failed\n", failed_count, NUM_ITERATIONS);
        }

        double avg_time = (double)total_time / NUM_ITERATIONS;
        double throughput = (msg_len / 1024.0) / (avg_time / 1000000.0);

        printf("  %12zu | %16lld | %14.2f | %16.2f\n",
               msg_len, total_time, avg_time, throughput);

        free(message);
    }

    /* Memory efficiency metrics */
    printf("\nMemory Efficiency Metrics:\n");
    printf("  Context size vs signature:   %.2f%%\n",
           (context_size * 100.0) / signature_size);
    printf("  Private key vs signature:   %.2f%%\n",
           (private_key_size * 100.0) / signature_size);
    printf("  Public key vs signature:    %.2f%%\n",
           (public_key_size * 100.0) / signature_size);
    printf("  Total key size vs sig:       %.2f%%\n",
           ((private_key_size + public_key_size) * 100.0) / signature_size);

    free(private_key);
    free(public_key);
    free(signature);
}

/* Main function */
int main(void) __attribute__((unused));
int main(void) {
    printf("=================================================================\n");
    printf("  Tiny SPHINCS+ Performance Test (Incremental Double-Pass)\n");
    printf("=================================================================\n");
    printf("Number of iterations: %d\n", NUM_ITERATIONS);
    printf("Note: This test outputs signature data for comparison with main branch\n");

    /* Test SHA2-128f as an example */
    #if TS_SUPPORT_SHA2
    test_parameter_set("sha2_128f_simple", &ts_ps_sha2_128f_simple);
    #endif

    printf("\n");
    printf("=================================================================\n");
    printf("  Testing Complete\n");
    printf("=================================================================\n");
    printf("Comparison Instructions:\n");
    printf("1. Save the signature, public key, and message output above\n");
    printf("2. Run the same test on the main branch with test_performance_orig\n");
    printf("3. Compare the signatures to verify they are identical\n");
    printf("4. Public keys should also match between implementations\n");

    return 0;
}

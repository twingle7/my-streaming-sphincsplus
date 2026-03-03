/*
 * Test program for double-pass incremental signing
 * This tests the new API that allows signing large messages without buffering them
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tiny_sphincs.h"
#include "sha2_func.h"
#include "shake256_func.h"

/* Helper function to generate a random message */
static void generate_random_message(unsigned char *message, size_t len) {
    for (size_t i = 0; i < len; i++) {
        message[i] = rand() & 0xff;
    }
}

/* Simple deterministic random function for testing */
static int deterministic_random(unsigned char *output, size_t len) {
    static unsigned seed = 12345;
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

/* External functions from key_gen.c */
extern int ts_gen_key(unsigned char *private_key,
                     unsigned char *public_key,
                     const struct ts_parameter_set *ps,
                     int (*random_function)(unsigned char *, size_t));

/* Test double-pass streaming signing */
static int test_double_pass_signing(const struct ts_parameter_set *ps,
                                     const char *ps_name,
                                     size_t message_size,
                                     size_t chunk_size) {
    int success = 0;
    unsigned char private_key[4 * 32];  /* Max size for all parameter sets */
    unsigned char public_key[2 * 32];
    struct ts_context ctx;
    unsigned char *message = NULL;
    unsigned char *signature = NULL;
    unsigned char *sig_ptr = NULL;
    size_t sig_total;

    printf("Testing double-pass signing for %s with message size %zu bytes, chunk size %zu\n",
           ps_name, message_size, chunk_size);

    /* Generate keypair */
    ts_gen_key(private_key, public_key, ps, deterministic_random);

    /* Allocate message and signature */
    message = malloc(message_size);
    signature = malloc(ts_size_signature(ps));

    if (!message || !signature) {
        printf("  Memory allocation failed\n");
        goto cleanup;
    }

    /* Generate random message */
    generate_random_message(message, message_size);

    /* Test double-pass signing */
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);

    /* Pass 1: Compute R */
    for (size_t offset = 0; offset < message_size; offset += chunk_size) {
        size_t len = (offset + chunk_size > message_size) ?
                     (message_size - offset) : chunk_size;
        if (!ts_update_prf_msg(&message[offset], len, &ctx)) {
            printf("  Pass 1 update failed at offset %zu\n", offset);
            goto cleanup;
        }
    }
    ts_finalize_prf_msg(&ctx);
    printf("  Pass 1 (compute R) completed\n");

    /* Pass 2: Compute message hash using R */
    ts_init_hash_msg(&ctx);
    for (size_t offset = 0; offset < message_size; offset += chunk_size) {
        size_t len = (offset + chunk_size > message_size) ?
                     (message_size - offset) : chunk_size;
        if (!ts_update_hash_msg(&message[offset], len, &ctx)) {
            printf("  Pass 2 update failed at offset %zu\n", offset);
            goto cleanup;
        }
    }
    ts_finalize_hash_msg(&ctx);
    printf("  Pass 2 (compute hash) completed\n");

    /* Generate signature */
    sig_ptr = signature;
    sig_total = 0;
    while (1) {
        size_t bytes_generated = ts_sign(sig_ptr,
                                         ts_size_signature(ps) - sig_total,
                                         &ctx);
        if (bytes_generated == 0) break;
        sig_ptr += bytes_generated;
        sig_total += bytes_generated;
        if (sig_total >= ts_size_signature(ps)) break;
    }

    if (sig_total != ts_size_signature(ps)) {
        printf("  Signature generation failed: expected %u, got %zu\n",
               ts_size_signature(ps), sig_total);
        goto cleanup;
    }
    printf("  Signature generated (%zu bytes)\n", sig_total);

    /* Debug: print first few bytes of signature (should be R value) */
    printf("  First 16 bytes of signature: ");
    for (int i = 0; i < 16 && i < sig_total; i++) {
        printf("%02x", signature[i]);
    }
    printf("\n");

    /* Verify the signature */
    ts_init_verify(&ctx, message, message_size, ps, public_key);
    if (!ts_update_verify(signature, sig_total, &ctx)) {
        printf("  Verification update failed\n");
        success = 0;
        goto cleanup;
    }
    if (!ts_verify(&ctx)) {
        printf("  Signature verification FAILED\n");
        success = 0;
        goto cleanup;
    }
    printf("  Signature verification PASSED\n");
    success = 1;

cleanup:
    if (message) free(message);
    if (signature) free(signature);

    return success;
}

int main(int argc, char **argv) {
    int all_passed = 1;
    int test_all = 0;

    if (argc > 1 && strcmp(argv[1], "all") == 0) {
        test_all = 1;
    }

    /* Test various message sizes and chunk sizes */
    size_t message_sizes[] = {100, 1000, 10000};
    size_t chunk_sizes[] = {32, 256, 1024};

    /* Test SHA2-128s (SHA2 L1) */
#if TS_SUPPORT_SHA2 && TS_SUPPORT_L1
    printf("\n=== Testing SHA2-128s (SHA2 L1) ===\n");
    for (int i = 0; i < (test_all ? 3 : 1); i++) {
        for (int j = 0; j < (test_all ? 3 : 1); j++) {
            if (!test_double_pass_signing(&ts_ps_sha2_128s_simple,
                                         "SHA2-128s",
                                         message_sizes[i],
                                         chunk_sizes[j])) {
                all_passed = 0;
            }
        }
    }
#endif

    /* Test SHA2-128f (SHA2 L1) */
#if TS_SUPPORT_SHA2 && TS_SUPPORT_L1
    printf("\n=== Testing SHA2-128f (SHA2 L1) ===\n");
    for (int i = 0; i < (test_all ? 3 : 1); i++) {
        for (int j = 0; j < (test_all ? 3 : 1); j++) {
            if (!test_double_pass_signing(&ts_ps_sha2_128f_simple,
                                         "SHA2-128f",
                                         message_sizes[i],
                                         chunk_sizes[j])) {
                all_passed = 0;
            }
        }
    }
#endif

    /* Test SHAKE-128s */
#if TS_SUPPORT_SHAKE
    printf("\n=== Testing SHAKE-128s ===\n");
    for (int i = 0; i < (test_all ? 3 : 1); i++) {
        for (int j = 0; j < (test_all ? 3 : 1); j++) {
            if (!test_double_pass_signing(&ts_ps_shake_128s_simple,
                                         "SHAKE-128s",
                                         message_sizes[i],
                                         chunk_sizes[j])) {
                all_passed = 0;
            }
        }
    }
#endif

    /* Test SHAKE-128f */
#if TS_SUPPORT_SHAKE
    printf("\n=== Testing SHAKE-128f ===\n");
    for (int i = 0; i < (test_all ? 3 : 1); i++) {
        for (int j = 0; j < (test_all ? 3 : 1); j++) {
            if (!test_double_pass_signing(&ts_ps_shake_128f_simple,
                                         "SHAKE-128f",
                                         message_sizes[i],
                                         chunk_sizes[j])) {
                all_passed = 0;
            }
        }
    }
#endif

    printf("\n========================================\n");
    if (all_passed) {
        printf("All tests PASSED\n");
        return EXIT_SUCCESS;
    } else {
        printf("Some tests FAILED\n");
        return EXIT_FAILURE;
    }
}

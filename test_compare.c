/*
 * Compare double-pass streaming signing with normal signing
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiny_sphincs.h"
#include "internal.h"
#include "shake256_func.h"

/* Simple deterministic random function for testing */
static unsigned seed = 12345;
static int deterministic_random(unsigned char *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

/* Reset random seed */
static void reset_random_seed() {
    seed = 12345;
}

/* Helper function to generate a random message */
static void generate_random_message(unsigned char *message, size_t len) {
    for (size_t i = 0; i < len; i++) {
        message[i] = rand() & 0xff;
    }
}

int main() {
    const struct ts_parameter_set *ps = &ts_ps_shake_128s_simple;
    unsigned char private_key[4 * 32];
    unsigned char public_key[2 * 32];
    unsigned char message[100];
    unsigned char sig_normal[10000];
    unsigned char sig_stream[10000];
    struct ts_context ctx;
    size_t sig_len;

    /* Generate keypair */
    ts_gen_key(private_key, public_key, ps, deterministic_random);
    printf("Key generated\n");

    /* Generate a random message */
    generate_random_message(message, sizeof(message));
    printf("Message generated\n");

    /* Normal signing */
    reset_random_seed();  /* Reset to ensure consistent opt_buffer */
    ts_init_sign(&ctx, message, sizeof(message), ps, private_key, deterministic_random);
    sig_len = ts_sign(sig_normal, sizeof(sig_normal), &ctx);
    printf("Normal signature generated (%zu bytes)\n", sig_len);

    /* Print first 16 bytes of normal signature */
    printf("Normal signature first 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sig_normal[i]);
    }
    printf("\n");

    /* Verify normal signature */
    ts_init_verify(&ctx, message, sizeof(message), ps, public_key);
    if (!ts_update_verify(sig_normal, sig_len, &ctx)) {
        printf("Normal signature verification update failed\n");
        return 1;
    }
    if (!ts_verify(&ctx)) {
        printf("Normal signature verification FAILED\n");
        return 1;
    }
    printf("Normal signature verification PASSED\n");

    /* Streaming signing (reset random seed to use the same opt_buffer) */
    reset_random_seed();
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);
    ts_update_prf_msg(message, sizeof(message), &ctx);
    ts_finalize_prf_msg(&ctx);
    ts_init_hash_msg(&ctx);
    ts_update_hash_msg(message, sizeof(message), &ctx);
    ts_finalize_hash_msg(&ctx);

    unsigned char *sig_ptr = sig_stream;
    size_t sig_total = 0;
    while (sig_total < ts_size_signature(ps)) {
        size_t bytes_generated = ts_sign(sig_ptr,
                                         ts_size_signature(ps) - sig_total,
                                         &ctx);
        if (bytes_generated == 0) break;
        sig_ptr += bytes_generated;
        sig_total += bytes_generated;
    }
    printf("Streaming signature generated (%zu bytes)\n", sig_total);

    /* Print first 16 bytes of streaming signature */
    printf("Streaming signature first 16 bytes: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sig_stream[i]);
    }
    printf("\n");

    /* Compare signatures */
    if (sig_len != sig_total) {
        printf("Signature lengths differ: normal=%zu, streaming=%zu\n", sig_len, sig_total);
    }
    int cmp = memcmp(sig_normal, sig_stream, sig_len);
    if (cmp == 0) {
        printf("Signatures are IDENTICAL\n");
    } else {
        printf("Signatures are DIFFERENT\n");
    }

    /* Verify streaming signature */
    ts_init_verify(&ctx, message, sizeof(message), ps, public_key);
    if (!ts_update_verify(sig_stream, sig_total, &ctx)) {
        printf("Streaming signature verification update failed\n");
        return 1;
    }
    if (!ts_verify(&ctx)) {
        printf("Streaming signature verification FAILED\n");
        return 1;
    }
    printf("Streaming signature verification PASSED\n");

    return 0;
}

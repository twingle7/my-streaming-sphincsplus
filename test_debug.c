/*
 * Simple test to debug signature generation
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiny_sphincs.h"
#include "internal.h"

/* Simple deterministic random function */
static unsigned seed = 12345;
static int deterministic_random(unsigned char *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

int main(void) {
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
    unsigned char private_key[64];
    unsigned char public_key[32];
    unsigned char message[] = "Hello, world! This is a test message.";
    unsigned char sig_traditional[20000];
    unsigned char sig_streaming[20000];
    struct ts_context ctx;

    /* Generate keypair */
    ts_gen_key(private_key, public_key, ps, deterministic_random);
    printf("Key generated\n");

    /* Traditional signing */
    seed = 12345;
    ts_init_sign(&ctx, message, sizeof(message), ps, private_key, deterministic_random);
    size_t sig_len_trad = ts_sign(sig_traditional, 20000, &ctx);
    printf("Traditional signature generated (%zu bytes)\n", sig_len_trad);
    printf("R (first 16 bytes): ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sig_traditional[i]);
    }
    printf("\n");

    /* Verify traditional signature */
    ts_init_verify(&ctx, message, sizeof(message), ps, public_key);
    if (!ts_update_verify(sig_traditional, sig_len_trad, &ctx)) {
        printf("Traditional signature verification update failed\n");
        return 1;
    }
    if (!ts_verify(&ctx)) {
        printf("Traditional signature verification FAILED\n");
        return 1;
    }
    printf("Traditional signature verification PASSED\n");

    /* Streaming signing - Pass 1 */
    seed = 12345;
    ts_init_sign_double_pass(&ctx, ps, private_key, deterministic_random);
    ts_update_prf_msg(message, sizeof(message), &ctx);
    ts_finalize_prf_msg(&ctx);

    printf("\nAfter Pass 1, R in ctx->buffer: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", ctx.buffer[i]);
    }
    printf("\n");

    /* Streaming signing - Pass 2 */
    ts_init_hash_msg(&ctx);

    printf("\nAfter ts_init_hash_msg, R in ctx->buffer: ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", ctx.buffer[i]);
    }
    printf("\n");

    ts_update_hash_msg(message, sizeof(message), &ctx);
    ts_finalize_hash_msg(&ctx);

    printf("\nAfter ts_finalize_hash_msg, ctx.state=%d, ctx.buffer_offset=%d\n",
           ctx.state, ctx.buffer_offset);

    /* Generate streaming signature */
    unsigned char *sig_ptr = sig_streaming;
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
    printf("R (first 16 bytes): ");
    for (int i = 0; i < 16; i++) {
        printf("%02x", sig_streaming[i]);
    }
    printf("\n");

    /* Compare signatures */
    if (sig_len_trad != sig_total) {
        printf("Signature lengths differ: traditional=%zu, streaming=%zu\n", sig_len_trad, sig_total);
    } else {
        int cmp = memcmp(sig_traditional, sig_streaming, sig_len_trad);
        if (cmp == 0) {
            printf("Signatures are IDENTICAL\n");
        } else {
            printf("Signatures are DIFFERENT\n");
            for (size_t i = 0; i < sig_len_trad; i++) {
                if (sig_traditional[i] != sig_streaming[i]) {
                    printf("First difference at byte %zu: trad=0x%02x, stream=0x%02x\n",
                           i, sig_traditional[i], sig_streaming[i]);
                    break;
                }
            }
        }
    }

    /* Verify streaming signature */
    ts_init_verify(&ctx, message, sizeof(message), ps, public_key);
    if (!ts_update_verify(sig_streaming, sig_total, &ctx)) {
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

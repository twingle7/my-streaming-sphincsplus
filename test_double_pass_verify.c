/*
 * Simple test for double-pass signing and verification
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tiny_sphincs.h"
#include "internal.h"

/* Simple random number generator for testing */
static int simple_random(unsigned char *buf, size_t len) {
    static unsigned int seed = 12345;
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        buf[i] = (unsigned char)(seed >> 24);
    }
    return 1;
}

int main(void) {
    printf("Testing double-pass signing and verification...\n");

    /* Use SHA2-128f */
    #if TS_SUPPORT_SHA2
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
    #else
    printf("SHA2 not supported\n");
    return 1;
    #endif

    /* Generate keys */
    unsigned char private_key[4 * 32];
    unsigned char public_key[2 * 32];
    if (!ts_gen_key(private_key, public_key, ps, simple_random)) {
        printf("Key generation failed\n");
        return 1;
    }
    printf("Keys generated\n");

    /* Sign a message using double-pass */
    unsigned char message[32] = "Hello, world!";
    unsigned char signature[32 * 32];
    size_t msg_len = sizeof(message);

    struct ts_context ctx;
    memset(&ctx, 0, sizeof(ctx));
    ts_init_sign_double_pass(&ctx, ps, private_key, simple_random);

    /* Pass 1: Compute R */
    ts_update_prf_msg(message, msg_len, &ctx);
    ts_finalize_prf_msg(&ctx);
    printf("Pass 1 completed\n");

    /* Pass 2: Compute message hash using R */
    ts_init_hash_msg(&ctx);
    ts_update_hash_msg(message, msg_len, &ctx);
    ts_finalize_hash_msg(&ctx);
    printf("Pass 2 completed\n");

    /* Generate signature */
    ts_sign(signature, sizeof(signature), &ctx);
    printf("Signature generated\n");

    /* Print signature (first 32 bytes) */
    printf("Signature (first 32 bytes): ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", signature[i]);
    }
    printf("\n");

    /* Verify the signature */
    struct ts_context verify_ctx;
    memset(&verify_ctx, 0, sizeof(verify_ctx));
    ts_init_verify(&verify_ctx, message, msg_len, ps, public_key);
    printf("verify_ctx.state after init = %d\n", verify_ctx.state);

    if (!ts_update_verify(signature, sizeof(signature), &verify_ctx)) {
        printf("Verification update failed, state = %d\n", verify_ctx.state);
        return 1;
    }
    printf("Verification update successful\n");

    int result = ts_verify(&verify_ctx);
    printf("Verification result: %s\n", result ? "PASSED" : "FAILED");
    printf("Final state: %d\n", verify_ctx.state);

    return 0;
}

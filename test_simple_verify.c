/*
 * Simple test to verify that the test code works
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
    printf("Testing basic signing and verification...\n");

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

    /* Sign a message */
    unsigned char message[32] = "Hello, world!";
    unsigned char signature[32 * 32];

    struct ts_context sign_ctx;
    memset(&sign_ctx, 0, sizeof(sign_ctx));
    ts_init_sign(&sign_ctx, message, sizeof(message), ps, private_key, simple_random);
    ts_sign(signature, sizeof(signature), &sign_ctx);
    printf("Signature generated\n");

    /* Verify the signature */
    struct ts_context verify_ctx;
    memset(&verify_ctx, 0, sizeof(verify_ctx));
    ts_init_verify(&verify_ctx, message, sizeof(message), ps, public_key);
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

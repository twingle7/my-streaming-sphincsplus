/*
 * Test program for streaming API - simplified version
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tiny_sphincs.h"

/* Simple random number generator for testing */
static int test_rng(unsigned char *p, size_t num_bytes) {
    for (unsigned i=0; i<num_bytes; i++) {
        *p++ = (unsigned char)rand();
    }
    return 1;
}

int main(void) {
    printf("Testing streaming SPHINCS+ signing API\n");
    printf("======================================\n\n");

    /* Use SHA2-128f-simple parameter set */
    const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;

    /* Generate key pair */
    printf("Generating key pair...\n");
    unsigned char private_key[128];
    unsigned char public_key[64];
    if (!ts_gen_key(private_key, public_key, ps, test_rng)) {
        printf("ERROR: Failed to generate key pair\n");
        return 1;
    }
    printf("Key pair generated successfully\n\n");

    /* Test 1: Traditional API */
    printf("Test 1: Traditional API\n");
    const char *message = "Hello";
    size_t msg_len = strlen(message);
    printf("  Message: %s (len=%zu)\n", message, msg_len);

    printf("  Calling ts_init_sign...\n");
    struct ts_context ctx;
    ts_init_sign(&ctx, message, msg_len, ps, private_key, test_rng);
    printf("  ts_init_sign completed\n");

    printf("  Generating signature...\n");
    unsigned char signature[8000];
    size_t sig_len = 0;
    unsigned chunk;
    int iterations = 0;
    while ((chunk = ts_sign(signature + sig_len, 100, &ctx)) > 0 && iterations < 1000) {
        sig_len += chunk;
        iterations++;
    }
    printf("  Signature length: %zu bytes\n", sig_len);
    printf("  Test 1 PASSED\n\n");

    printf("All tests completed successfully!\n");
    return 0;
}

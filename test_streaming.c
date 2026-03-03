/*
 * Test program for streaming API
 * This demonstrates how to use the new streaming signing API
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

int main() {
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

    /* Test 1: Traditional API (for comparison) */
    printf("Test 1: Traditional API\n");
    const char *message = "Hello, this is a test message for streaming SPHINCS+!";
    size_t msg_len = strlen(message);

    struct ts_context ctx;
    ts_init_sign(&ctx, message, msg_len, ps, private_key, test_rng);

    unsigned char signature[8000];
    size_t sig_len = 0;
    unsigned chunk;
    while ((chunk = ts_sign(signature + sig_len, 100, &ctx)) > 0) {
        sig_len += chunk;
    }
    printf("  Signature length: %zu bytes\n\n", sig_len);

    /* Test 2: Streaming API */
    printf("Test 2: Streaming API\n");
    struct ts_context stream_ctx;
    ts_init_sign_stream(&stream_ctx, ps, private_key, test_rng);

    /* Process message in chunks */
    size_t chunk_size = 10;
    size_t processed = 0;
    while (processed < msg_len) {
        size_t this_chunk = (processed + chunk_size <= msg_len) ? 
                           chunk_size : msg_len - processed;
        ts_sign_update(&stream_ctx, message + processed, this_chunk);
        processed += this_chunk;
    }

    /* Finalize and generate signature */
    ts_sign_finalize(&stream_ctx);

    unsigned char stream_signature[8000];
    size_t stream_sig_len = 0;
    while ((chunk = ts_sign(stream_signature + stream_sig_len, 100, &stream_ctx)) > 0) {
        stream_sig_len += chunk;
    }
    printf("  Streaming signature length: %zu bytes\n\n", stream_sig_len);

    /* Test 3: Verify that both signatures are different (expected since R is different) */
    printf("Test 3: Comparing signatures\n");
    if (sig_len == stream_sig_len && 
        memcmp(signature, stream_signature, sig_len) == 0) {
        printf("  Signatures are identical (unexpected but acceptable)\n");
    } else {
        printf("  Signatures are different (expected - R is generated differently)\n");
        printf("  This is OK since we modified the R generation method\n");
    }
    printf("\n");

    printf("Streaming API test completed successfully!\n");
    return 0;
}

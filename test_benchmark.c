/*
 * Performance benchmark for tiny-sphincs
 * Measures signing and verification time for various parameter sets
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tiny_sphincs.h"
#include "internal.h"

/* Simple random function */
static unsigned seed = 12345;
static int deterministic_random(unsigned char *output, size_t len) {
    for (size_t i = 0; i < len; i++) {
        seed = seed * 1103515245 + 12345;
        output[i] = (seed >> 16) & 0xff;
    }
    return 1;
}

/* Helper to get current time in milliseconds */
static double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

/* Benchmark a parameter set */
static void benchmark_parameter_set(const char *name, const struct ts_parameter_set *ps) {
    unsigned char private_key[4 * 64];  // Max size: 4*N where N <= 64
    unsigned char public_key[2 * 64];   // Max size: 2*N where N <= 64
    unsigned char message[100];
    unsigned char signature[50000];      // Large enough for any signature
    struct ts_context ctx;
    size_t sig_len;
    double start, end;

    printf("\n=== Benchmarking %s ===\n", name);

    /* Key generation */
    start = get_time_ms();
    if (!ts_gen_key(private_key, public_key, ps, deterministic_random)) {
        printf("ERROR: Key generation failed\n");
        return;
    }
    end = get_time_ms();
    printf("Key generation:      %.2f ms\n", end - start);

    /* Normal signing */
    start = get_time_ms();
    ts_init_sign(&ctx, message, sizeof(message), ps, private_key, deterministic_random);
    sig_len = ts_sign(signature, sizeof(signature), &ctx);
    end = get_time_ms();
    printf("Signing (100 bytes): %.2f ms (%.2f KB/s)\n",
           end - start,
           (100.0 / 1024.0) / ((end - start) / 1000.0));

    /* Verification */
    start = get_time_ms();
    ts_init_verify(&ctx, message, sizeof(message), ps, public_key);
    ts_update_verify(signature, sig_len, &ctx);
    int result = ts_verify(&ctx);
    end = get_time_ms();

    if (result) {
        printf("Verification:         %.2f ms (%.2f KB/s)\n",
               end - start,
               (100.0 / 1024.0) / ((end - start) / 1000.0));
    } else {
        printf("ERROR: Verification failed\n");
    }

    printf("Signature size:       %zu bytes\n", sig_len);
    printf("Private key size:     %u bytes\n", ts_size_private_key(ps));
    printf("Public key size:      %u bytes\n", ts_size_public_key(ps));

    /* Estimate memory usage */
    printf("\nContext structure size: %zu bytes\n", sizeof(struct ts_context));
}

int main() {
    printf("========================================\n");
    printf("Tiny-SPHINCS Performance Benchmark\n");
    printf("========================================\n");

    /* SHAKE parameter sets */
    benchmark_parameter_set("SHAKE-128s", &ts_ps_shake_128s_simple);
    benchmark_parameter_set("SHAKE-128f", &ts_ps_shake_128f_simple);
    benchmark_parameter_set("SHAKE-192s", &ts_ps_shake_192s_simple);
    benchmark_parameter_set("SHAKE-192f", &ts_ps_shake_192f_simple);
    benchmark_parameter_set("SHAKE-256s", &ts_ps_shake_256s_simple);
    benchmark_parameter_set("SHAKE-256f", &ts_ps_shake_256f_simple);

    /* SHA2 parameter sets */
    benchmark_parameter_set("SHA2-128s", &ts_ps_sha2_128s_simple);
    benchmark_parameter_set("SHA2-128f", &ts_ps_sha2_128f_simple);
    benchmark_parameter_set("SHA2-192s", &ts_ps_sha2_192s_simple);
    benchmark_parameter_set("SHA2-192f", &ts_ps_sha2_192f_simple);
    benchmark_parameter_set("SHA2-256s", &ts_ps_sha2_256s_simple);
    benchmark_parameter_set("SHA2-256f", &ts_ps_sha2_256f_simple);

    printf("\n========================================\n");
    printf("Benchmark complete!\n");
    printf("========================================\n");

    return 0;
}

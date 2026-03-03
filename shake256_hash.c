/*
 * This file contains the functions that are common to all SHAKE
 * parameter sets, namely the PRF_msg, HASH_msg and PRF functions
 */

#include "internal.h"
#include "shake256_func.h"
#include "fips202.h"
#include "tune.h"
#include "tiny_sphincs.h"

#if TS_SUPPORT_SHAKE

/*
 * This computes the PRF_msg function for SHAKE parameter sets
 */
void ts_shake256_prf_msg( unsigned char *output,
	             const unsigned char *opt_rand,
		     const unsigned char *message, size_t len_message,
		     struct ts_context *sc ) {
    int n = sc->ps->n;
    const unsigned char *public_key = sc->public_key;
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;

    ts_shake256_inc_init(ctx);

    ts_shake256_inc_absorb(ctx, CONVERT_PUBLIC_KEY_TO_PRF(public_key, n), n);
    ts_shake256_inc_absorb(ctx, opt_rand, n);
    ts_shake256_inc_absorb(ctx, message, len_message);
    ts_shake256_inc_finalize(ctx);
    ts_shake256_inc_squeeze(output, n, ctx);
}

/*
 * ========== Incremental versions of prf_msg for double-pass streaming ==========
 */

/*
 * Initialize the incremental PRF_msg computation.
 * This sets up the SHAKE256 state.
 */
void ts_shake256_prf_msg_init( struct ts_context *sc ) {
    int n = sc->ps->n;
    const unsigned char *public_key = sc->public_key;
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;

    ts_shake256_inc_init(ctx);
    ts_shake256_inc_absorb(ctx, CONVERT_PUBLIC_KEY_TO_PRF(public_key, n), n);
    /* Note: opt_rand should be added separately (handled in tiny_sphincs.c) */
}

/*
 * Update the incremental PRF_msg computation with a message chunk.
 */
int ts_shake256_prf_msg_update( const unsigned char *chunk, size_t len,
                                 struct ts_context *sc ) {
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;
    ts_shake256_inc_absorb(ctx, chunk, len);
    return 1;
}

/*
 * Finalize the incremental PRF_msg computation and output R.
 */
void ts_shake256_prf_msg_final( unsigned char *output, struct ts_context *sc ) {
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;
    int n = sc->ps->n;

    ts_shake256_inc_finalize(ctx);
    ts_shake256_inc_squeeze(output, n, ctx);
}

/*
 * This computes the HASH_msg function for SHAKE parameter sets
 */
void ts_shake256_hash_msg( unsigned char *output, size_t len_output,
		     const unsigned char *randomness,
		     const unsigned char *message, size_t len_message,
		     struct ts_context *sc ) {
    int n = sc->ps->n;
    const unsigned char *public_key = sc->public_key;
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;

    ts_shake256_inc_init(ctx);

    const unsigned char *pk_seed = CONVERT_PUBLIC_KEY_TO_PUB_SEED(public_key, n);
    const unsigned char *pk_root = CONVERT_PUBLIC_KEY_TO_ROOT(public_key, n);

    ts_shake256_inc_absorb(ctx, randomness, n);
    ts_shake256_inc_absorb(ctx, pk_seed, n);
    ts_shake256_inc_absorb(ctx, pk_root, n);
    ts_shake256_inc_absorb(ctx, message, len_message);

    ts_shake256_inc_finalize(ctx);

    ts_shake256_inc_squeeze(output, len_output, ctx);
}

/*
 * ========== Incremental versions of hash_msg for double-pass streaming ==========
 */

/*
 * Initialize the incremental hash_msg computation.
 * This sets up the SHAKE256 state for R || PK.seed || PK.root || message
 */
void ts_shake256_hash_msg_init( struct ts_context *sc ) {
    int n = sc->ps->n;
    const unsigned char *public_key = sc->public_key;
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;
    const unsigned char *pk_seed = CONVERT_PUBLIC_KEY_TO_PUB_SEED(public_key, n);
    const unsigned char *pk_root = CONVERT_PUBLIC_KEY_TO_ROOT(public_key, n);

    /* Note: The SHAKE context is already initialized and R is already added
     * (handled in tiny_sphincs.c ts_init_hash_msg), so we just add PK.seed and PK.root
     */
    ts_shake256_inc_absorb(ctx, pk_seed, n);
    ts_shake256_inc_absorb(ctx, pk_root, n);
}

/*
 * Update the incremental hash_msg computation with a message chunk.
 */
int ts_shake256_hash_msg_update( const unsigned char *chunk, size_t len,
                                  struct ts_context *sc ) {
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;
    ts_shake256_inc_absorb(ctx, chunk, len);
    return 1;
}

/*
 * Finalize the incremental hash_msg computation and output the message hash.
 */
void ts_shake256_hash_msg_final( unsigned char *output, size_t len_output,
                                  struct ts_context *sc ) {
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;

    ts_shake256_inc_finalize(ctx);
    ts_shake256_inc_squeeze(output, len_output, ctx);
}

/*
 * This computes the PRF function for SHAKE parameter sets
 */
void ts_shake256_prf( unsigned char *output,
		     struct ts_context *sc ) {
    int n = sc->ps->n;
    SHAKE256_CTX *ctx = &sc->small_iter.shake256_simple;

    ts_shake256_inc_init(ctx);
 
    ts_shake256_inc_absorb(ctx, CONVERT_PUBLIC_KEY_TO_PUB_SEED(sc->public_key, n), n);
    ts_shake256_inc_absorb(ctx, sc->adr, ADR_SIZE);
    ts_shake256_inc_absorb(ctx, CONVERT_PUBLIC_KEY_TO_SEC_SEED(sc->public_key, n), n);

    ts_shake256_inc_finalize(ctx);

    ts_shake256_inc_squeeze(output, n, ctx);
}

#endif

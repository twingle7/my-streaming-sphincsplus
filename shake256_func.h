#if !defined( SHAKE256_FUNC_H_ )
#define SHAKE256_FUNC_H_

/*
 * The parameter set functions used by a SHAKE parameter set
 */
void ts_shake256_prf_msg( unsigned char *output,
	             const unsigned char *opt_buffer,
		     const unsigned char *message, size_t len_message,
	             struct ts_context *ctx);
void ts_shake256_hash_msg( unsigned char *output, size_t len_output,
		     const unsigned char *randomness,
		     const unsigned char *message, size_t len_message,
	             struct ts_context *ctx);

/* Incremental versions for double-pass streaming */
void ts_shake256_prf_msg_init( struct ts_context *ctx );
int ts_shake256_prf_msg_update( const unsigned char *chunk, size_t len,
                                 struct ts_context *ctx );
void ts_shake256_prf_msg_final( unsigned char *output, struct ts_context *ctx );

void ts_shake256_hash_msg_init( struct ts_context *ctx );
int ts_shake256_hash_msg_update( const unsigned char *chunk, size_t len,
                                  struct ts_context *ctx );
void ts_shake256_hash_msg_final( unsigned char *output, size_t len_output,
                                  struct ts_context *ctx );

void ts_shake256_prf( unsigned char *output, struct ts_context *ctx);
void ts_shake256_f_simple( unsigned char *output,
	             const unsigned char *inblock,
	             struct ts_context *ctx);
void ts_shake256_init_t_simple( union t_iterator *t,
		     struct ts_context *ctx );
void ts_shake256_next_t_simple( union t_iterator *t, const unsigned char *input,
		     const struct ts_context *ctx );
void ts_shake256_final_t_simple(unsigned char *output, union t_iterator *t,
		     const struct ts_context *ctx );

#endif /* SHAKE256_FUNC_H_ */

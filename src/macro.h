#ifndef MACRO_H__INCLUDED__
# define MACRO_H__INCLUDED__

#include "as-.h" // TOKEN MACROID_BITS MACROID
#include <stdint.h>

#define MACRO_MAX (1 << MACROID_BITS)

void macro_remove (MACROID id);
void macro_close (MACROID id);
MACROID macro_open (const char *str, unsigned len, unsigned argc, const TOKEN *args, const char *_source);

#define MACRO_ADD_NEWLINE 1
void macro_write (MACROID id, const char *text, const TOKEN *tokens, unsigned len, /*u1*/ uint8_t flags);

typedef struct {
	uint8_t req_args;
} LOOKEDUP;
MACROID macro_lookup (const char *str, unsigned len, LOOKEDUP *lup);

void macro_tokenize_reset (void *priv, unsigned len, const void *data);
const TOKEN *macro_tokenize_peek (void *priv, const char **source);
const TOKEN *macro_tokenize_next (void *priv, const char **source);
unsigned macro_tokenize_next_total (void *priv);
const TOKEN *macro_tokenize_next_parameter (void *priv, const char **source, unsigned *params_len);

#endif //def MACRO_H__INCLUDED__

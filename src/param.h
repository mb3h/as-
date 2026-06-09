#ifndef PARAM_H__INCLUDED__
# define PARAM_H__INCLUDED__

#include "as-.h" // TOKEN
#include <stdint.h>

#define PARAMID uint8_t
#define PARAMID_BITS 8

PARAMID param_open ();
void param_write (PARAMID id, const char *text, const TOKEN *tokens, unsigned tokens_len);
void param_close (PARAMID mid);
void param_remove (PARAMID id);

void param_tokenize_reset (void *priv, unsigned len, const void *data);
const TOKEN *param_tokenize_peek (void *priv, const char **source);
const TOKEN *param_tokenize_next (void *priv, const char **source);

#endif //def PARAM_H__INCLUDED__

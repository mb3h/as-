#ifndef POST_TOKENIZE_H__INCLUDED__
# define POST_TOKENIZE_H__INCLUDED__

#include <stdint.h>

void tokenize_init (const char *pre_defined_symbols);
void tokenize_get_C_first ();
const char *tokenize_get_C (unsigned *ret_len); // with preprocessing

void tokenize_validate_bitsize ();

#endif //def POST_TOKENIZE_H__INCLUDED__

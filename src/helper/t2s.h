#ifndef HELPER_TOKEN_H__INCLUDED__
# define HELPER_TOKEN_H__INCLUDED__

#include "as-.h" // TOKEN

// flags
#define T2S_ALL    ((1 << TOKENS_CNT_BITS) -1)
#define T2S_DETAIL  (1 << TOKENS_CNT_BITS)
#define T2S_LEN     (2 << TOKENS_CNT_BITS)

/* @param n       length of TOKEN[] (accept 0)
   @param _source base text of TOKEN[] (accept NULL) */
const char *t2s (const TOKEN *p,
                 unsigned n,
                 const char *_source,
                 const char *sgr,
                 unsigned flags_and_hilight);

#endif //def HELPER_TOKEN_H__INCLUDED__

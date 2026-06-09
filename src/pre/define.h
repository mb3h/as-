#ifndef PRE_DEFINE_H__INCLUDED__
# define PRE_DEFINE_H__INCLUDED__

#include "as-.h" // TOKEN
#include <stdint.h>

unsigned macro_define_first (const TOKEN *name, unsigned len);
unsigned macro_define_next (const TOKEN *top, unsigned len);

#endif //def PRE_DEFINE_H__INCLUDED__

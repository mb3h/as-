#include "helper/m2s.h"

#include <memory.h>
#ifndef min
# define min(a, b) ((a) < (b) ? (a) : (b))
#endif
///////////////////////////////////////////////////////////////

const char *m2s (const void *p, unsigned n)
{
static char s_text[8][128];
static unsigned s_i = 0;
unsigned i
	= ++s_i % 8;
	n = min(n, 128 -1);
	memcpy (s_text[i], p, n);
	s_text[i][n] = '\0';
	return s_text[i];
}

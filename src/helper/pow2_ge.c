#include "helper/pow2_ge.h"

#include "helper/assert.h"
#define   caller_bug assert

unsigned pow2_ge (unsigned n)
{
caller_bug(n <= (1U << 31)) // safety on unsigned == uint32_t
	if (0 == n) return 0;

unsigned r = 1;
	while (r < n)
		r <<= 1;
	return r;
}

#include "helper/memtoul.h"

#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <alloca.h>

typedef uint8_t u8;

unsigned long memtoul (const void *nptr, size_t n, void **_endptr, int base)
{
//input_bug(nptr && 0 < n)
char *tmp
	= alloca (n +1);
//assert(tmp)
	memcpy (tmp, nptr, n); tmp[n] = '\0';

	if (!_endptr)
		return strtoul (tmp, NULL, base);

char *tmp_endptr; unsigned long r
	= strtoul (tmp, &tmp_endptr, base);
const void *endptr = NULL;
	if (tmp_endptr)
		endptr = (const u8 *)nptr +(tmp_endptr -tmp);
	*_endptr = /*const_cast<>*/(void *)endptr;
	return r;
}

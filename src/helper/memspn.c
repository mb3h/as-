#include "helper/memspn.h"

#include <stdint.h>
#include <string.h>
#include <memory.h>

typedef uint8_t u8;

size_t memspn (const void *s, size_t n, const char *accept)
{
//input_bug(s && 0 < n)
//input_bug(accept && '\0' != *accept)
unsigned len
	= strlen (accept);
const u8 *p = (const u8 *)s, *end = p +n;
	while (p < end) {
		if (!memchr (accept, *p, len))
			break;
		++p;
	}
	return p - (const u8 *)s;
}

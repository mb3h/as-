#include "helper/mtoi.h"

#include <stdlib.h>
#include <memory.h>
#include <alloca.h>

int mtoi (const void *nptr, size_t n)
{
//input_bug(nptr && 0 < n)
char *tmp
	= alloca (n +1);
//assert(tmp)
	memcpy (tmp, nptr, n); tmp[n] = '\0';

	return atoi (tmp);
}

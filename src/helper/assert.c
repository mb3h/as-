#include "helper/assert.h"

#include <stdio.h>
#include <stdarg.h> // va_start
///////////////////////////////////////////////////////////////

void abort_ (const char *format, ...)
{
//	printf (stderr, VTRR "fatal" VTO ": ");
va_list ap;
	va_start(ap, format);
	vprintf (format, ap);
	va_end(ap);

	exit (-1);
}

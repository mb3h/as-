#ifndef HELPER_ASSERT_H__INCLUDED__
# define HELPER_ASSERT_H__INCLUDED__

#include "helper/vt100.h" // VTO VTRR
#include <stdio.h> // printf
#include <stdlib.h> // exit

#ifdef ASSERT_PREFIX_FUNC
# define ASSERT_PREFIX_FORMAT "%s "
# define COMMA_ASSERT_PREFIX_FUNC , ASSERT_PREFIX_FUNC
#else
# define ASSERT_PREFIX_FORMAT 
# define COMMA_ASSERT_PREFIX_FUNC 
#endif

// use without ';'
#define assert(expr) \
	do { \
		if (! (expr)) { \
printf (ASSERT_PREFIX_FORMAT VTRR "ASSERT" VTO "! (%d) " #expr " [%s()]" "\n" COMMA_ASSERT_PREFIX_FUNC, __LINE__, __func__); \
			exit (-1); \
		} \
	} while (0);

// use with ';'
#define SIGSERV \
	do { \
		*(char *)NULL = '\0'; \
	} while (0)

void abort_ (const char *format, ...);

#endif //def HELPER_ASSERT_H__INCLUDED__

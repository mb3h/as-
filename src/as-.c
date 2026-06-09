/* NOTE: (*1) specifying by compiler option
 */
#include "as-.h"
#include "pre/directive.h"
#include "post/tokenize.h"
#include "helper/assert.h"
#define caller_bug assert
#define   flow_bug assert

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

typedef  uint8_t u8;
typedef uint16_t u16;

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#define ustrlen(s) (unsigned)strlen(s)
///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

static MASKED s_masked_singlton = {0};
MASKED *masked () { return &s_masked_singlton; }

///////////////////////////////////////////////////////////////
// shared data

const TOKEN NEWLINE
= { .pos = 1, .len = 0 };

///////////////////////////////////////////////////////////////
// integrated log (with filtering)

static struct {
	unsigned mask;
	unsigned level;
} s_log = {
	  .mask  = DEFAULT_LOG_MASK  // NOTE: (*1)
	, .level = DEFAULT_LOG_LEVEL // NOTE: (*1)
};

#include <stdbool.h>
bool is_dbg (unsigned mask, unsigned level)
{
	if (s_log.level < level || !(0 == mask || s_log.mask & mask))
		return false;
	return true;
}

#include <stdarg.h> // va_start
void dbg (unsigned mask, unsigned level, const char *format, ...)
{
	if (!is_dbg (mask, level))
		return;
va_list ap;
	va_start(ap, format);
	vprintf (format, ap);
	va_end(ap);
}

// standard prefix
// 128: maximum length, 4: parallel support
const char *whereis ()
{
static char s_text[4][128];
static unsigned s_i = 0;
unsigned i
	= ++s_i % 4;

const char *path
	= masked()->fname;
unsigned line
	= masked()->line;
char *end = s_text[i];
	if (path && path[0]) {
		line = min(99999, line);
unsigned len
		= min(strlen (path), 128 -6 -1); // 6=':NNNNN'
		memcpy (end, path, len);
		end += len;
		*end++ = ':';
	}
	sprintf (end, "%d:", line);
//	end = strchr (end, '\0');

	return s_text[i];
}

///////////////////////////////////////////////////////////////
// library interface

extern void dbg_set_line (unsigned line, const char *fname)
{
	masked()->line = line;
	masked()->fname = fname;
}

extern void as_validate_bitsize ()
{
	directive_validate_bitsize ();
	tokenize_validate_bitsize ();
}

extern void as_directive_parse_line (const char *text, const void *tokenized, unsigned tokens_len)
{
	masked()->text = text;
	directive_parse_line (tokenized, tokens_len);
}

extern void as_tokenize_init (const char *pre_defined_symbols)
{
	tokenize_init (pre_defined_symbols);
}

extern void as_tokenize_get_C_first (const char *text, const void *tokenized, u8 tokens_len)
{
assert(text && strlen (text) < (1 << TOKEN_POS_BITS))
	masked()->text = text;
	masked()->root_tokenized = tokenized;
	masked()->root_tokens_len = tokens_len;

	tokenize_get_C_first ();
}

// @param got_buf accepted C token
// @return length of accepted C token
extern unsigned as_tokenize_get_C (char *buf, unsigned max_len) // with preprocessing
{
unsigned joined_len; const char *joined_tok
	= tokenize_get_C (&joined_len); // with preprocessing

	// end of line? (*)= completed all of tokens passed by as_tokenize_get_C_first()
	if (NULL == joined_tok) {
		return 0;
	}

	// buffer not enough?
	if (max_len < joined_len +1) {
abort_("%s " VTRR "error" VTO ": token buffer overflow (%d < " VTRR "%d" VTO "), fix coding.", whereis(), max_len, joined_len +1);
	}

	// succeeded
	if (0 < joined_len)
		memcpy (buf, joined_tok, joined_len);
	buf[joined_len] = '\0';
	return  joined_len;
}

extern void dbg0 (unsigned mask, unsigned level, const char *text)
{
	dbg (mask, level, "%s" "\n", text);
}

extern void dbg_set_mask (unsigned mask)
{ s_log.mask = mask; }

extern void dbg_set_level (unsigned level)
{ s_log.level = level; }

///////////////////////////////////////////////////////////////
// library interface (ELF utility)

extern unsigned strtab_id_from (const char *strtab,
                                unsigned strtab_len,
                                const char *cstr)
{
caller_bug(strtab)
caller_bug(0 < strtab_len)
caller_bug(cstr)

unsigned cstr_len
	= ustrlen (cstr);
const char *p, *end;
	p = strtab, end = strtab +strtab_len;
	while (p +cstr_len +1 <= end) {
const char *tail
		= (const char *)memchr (p, '\0', end -p);
		if (NULL == tail)
			break; // not found
		do {
			if (tail < p +cstr_len) // too short
				break;
			if (0 < cstr_len
			 && !(0 == memcmp (tail -cstr_len, cstr, cstr_len))
			) // not matched
				break;
unsigned strtab_id
			= (unsigned)((tail -cstr_len) -strtab);
			return strtab_id; // found
		} while (0);
		p = tail +1;
	}
	return strtab_len; // not found
}

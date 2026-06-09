#include "helper/t2s.h"

#include "helper/assert.h"
#include "helper/m2s.h"
#include "as-.h" // MACRO_PARAM_MAX TOKEN ...
#include <string.h>

#define min(a,b) ((a) < (b) ? (a) : (b))

#define MAX_LEN 768
const char *t2s (const TOKEN *t0,
                 unsigned n,
                 const char *_source,
                 const char *sgr0,
                 unsigned flags_and_hilight)
{
unsigned flags, hilight;
	flags = flags_and_hilight, hilight = (T2S_DETAIL -1) & flags_and_hilight;
assert(NULL == sgr0 || strlen (sgr0) <= 7)
	if (NULL == sgr0)
		sgr0 = "";
assert(NULL == _source || memchr (_source, '\0', (1 << TOKEN_POS_BITS))) // ATTENTION: getting slow

static char s_text[2][MAX_LEN];
static unsigned s_i = 0;

unsigned i
	= ++s_i % 2;
	n = min(n, MAX_LEN -1);

char *p = s_text[i];
unsigned header
	= (unsigned)(p - s_text[i]);

assert(0 == n || t0)
const TOKEN *end = t0 +n, *tok;
	for (tok = t0; tok < end; ++tok) {
		// needed memory: [ ]['<token>'][{+<pos>,<len>}]... (len=<token_count>)
unsigned need
		= 1 +3 +20 +1; // -> (*1a)(*2)(*3)
		if (_source)
			need += 13 +tok->len; // -> (*1b)
		if (T2S_DETAIL & flags)
			need += 30; // -> (*1c)

		if (s_text[i] +MAX_LEN < p +need) {
			strcpy (p, "..."); // +3 (*2)
			p = strchr (p, '\0');
			break;
		}
		// +1 (*1a)
		if (s_text[i] +header < p)
			*p++ = ' ';
const char *sgr = "", *fin = "";
		if (n <= hilight || (unsigned)(tok -t0) == hilight)
			sgr = sgr0, fin = VTO;
		// +12 = +7+1+[1|2]+4 (*) 7=VTYY 4=VTO
		if (! (tok->len < (1 << TOKEN_LEN_BITS) - MACRO_PARAM_MAX)) {
			sprintf (p, "%s" "$%d" "%s", sgr, (1 << TOKEN_LEN_BITS) -tok->len, fin);
			p = strchr (p, '\0');
			continue;
		}
		if (_source) {
			// +13+len = +1+7+len+4+1 (*1b)
			sprintf (p, "'" "%s" "%s" "%s" "'", sgr, m2s(_source +tok->pos, tok->len), fin);
			p = strchr (p, '\0');
		}
		if (T2S_DETAIL & flags) {
			// +30 = +2 +7+2+4 +1 +7+2+4 +1 (*1c)
			sprintf (p, "{+" "%s" "%d" "%s" "," "%s" "%d" "%s" "}", sgr, tok->pos, fin, sgr, tok->len, fin);
			p = strchr (p, '\0');
		}
	}
	if (T2S_LEN & flags) {
		// +20 = +6+7+2+4+1 (*3)
		sprintf (p, " (len=" "%s" "%d" VTO ")", sgr0, n);
		p = strchr (p, '\0');
	}
	return s_text[i];
}

#include "pre/undef.h"

#include "as-.h"
#include "macro.h"
#include "helper/assert.h"
#include "helper/m2s.h"

///////////////////////////////////////////////////////////////

// #undef ___
unsigned macro_undef (const TOKEN *tok, unsigned cnt)
{
const char *_text
	= masked()->text;

assert(1 == cnt)
	if (! (0 < cnt))
		return 0;

MACROID id
	= macro_lookup (_text +tok->pos, tok->len, NULL);
	if (0 == id) {
printf ("%d: " VTRR "warn" VTO ": " VTYY "cannot undef" VTO ": %s" "\n", masked()->line, m2s(_text +tok->pos, tok->len));
	}
	else
		macro_remove (id);
	return 1;
}

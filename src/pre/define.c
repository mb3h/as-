#include "pre/define.h"

#include "as-.h"
#include "macro.h"
#include "helper/assert.h"
#include "helper/m2s.h"

#include <stdint.h>
#include <stdbool.h>

typedef  uint8_t u8, u1;
typedef uint16_t u16;
///////////////////////////////////////////////////////////////
// local alias

#define FINISHED    MACRO_DEFINE_FINISHED

// export
#define  _first        macro_define_first
#define  _next         macro_define_next

// import
#define _mopen    macro_open
#define _mclose   macro_close
#define _mwrite   macro_write

#define POS         TOKEN_POS
#define PARAM_MAX   MACRO_PARAM_MAX
#define ADD_NEWLINE MACRO_ADD_NEWLINE

///////////////////////////////////////////////////////////////

// #define ___ [(...)]
unsigned _first (const TOKEN *name, unsigned len)
{
const char *_text
	= masked()->text;
assert(0 < len)

const TOKEN *tok = name;
	++tok;
	// no arguments?
	if (! (1 == tok->len && '(' == _text[tok->pos])) {
		// #define <symbol> ...
masked()->defining_is1st = TRUE, masked()->defining_id
		= _mopen (_text +name->pos, name->len, 0, NULL, NULL);
		return 1;
	}

	// parse arguments & prepare temporary lookup
	// #define <symbol>(...) ...
u8 argc = 0; TOKEN args[PARAM_MAX] = {0};
	do {
assert(tok +2 < name +len)
		++tok;
		// TODO: Symbol characters are all valid?
assert(argc +1 < PARAM_MAX)
		args[argc++] = *tok;
		++tok;
	} while (1 == tok->len && ',' == _text[tok->pos]);
assert(1 == tok->len && ')' == _text[tok->pos]) // PENDING: can't write multi-line of arguments
masked()->defining_is1st = TRUE, masked()->defining_id
	= _mopen (_text +name->pos, name->len, argc, args, _text);
	return (unsigned)(tok +1 -name);
}

// #define <symbol>[(...)] ___
// parse the definition of macro's body
/* NOTE: (*1) ignores newline without tokens after macro's identifier
              and parameter(s). (And never give any text through _mwrite())
   cf)
     #define XXX(a,b) \    <-
        ... \
        ...
     #define YYY           <-
 */
unsigned _next (const TOKEN *top, unsigned len)
{
const char *text
	= masked()->text;
/*assert(0 < len)*/
MACROID mid
	= masked()->defining_id;
assert(0 < mid)

const TOKEN *back
	= top +len -1;

bool isIgnore = false, isLast = true;
	do {
		// continuing into next line?
		if (0 < len && 1 == back->len && '\\' == text[back->pos])
			--len, isLast = false;
		// ignores line? NOTE: (*1)
		if (0 == len && masked()->defining_is1st)
			isIgnore = true;
	} while (0);

	if (!isIgnore) {
u1 flags
		= (isLast) ? 0 : ADD_NEWLINE;
		_mwrite (mid, text, top, len, flags);
	}
	if (isLast)
		_mclose (mid);

	masked()->defining_is1st = FALSE;
	return len;
}

#include "pre/directive.h"

#include "as-.h"
#include "macro.h"
#include "pre/undef.h"
#include "pre/define.h"
#include "helper/assert.h"

#include <stdbool.h>
#include <string.h>
#include <memory.h>

typedef  uint8_t u1, u2;

#define usizeof(x) (unsigned)sizeof(x)
///////////////////////////////////////////////////////////////
// alias

// import
#define ID_UNDEF   DIRECTIVE_ID_UNDEF
#define ID_DEFINE  DIRECTIVE_ID_DEFINE
#define ID_ENDIF   DIRECTIVE_ID_ENDIF
#define ID_IF      DIRECTIVE_ID_IF
#define ID_ELSE    DIRECTIVE_ID_ELSE
#define ID_IFDEF   DIRECTIVE_ID_IFDEF
#define ID_IFNDEF  DIRECTIVE_ID_IFNDEF
#define ID_ELIF    DIRECTIVE_ID_ELIF
#define ID_INCLUDE DIRECTIVE_ID_INCLUDE
#define POS       TOKEN_POS
#define caller_bug      assert
#define tiny_error(msg) assert((msg, false))
#define manage_bug      assert
#define input_bug       assert
#define bitsize_error   assert
#define not_done        assert

// export
#define _parse_line       directive_parse_line
#define _is_disable_by_if directive_is_disable_by_if

#define _validate_bitsize directive_validate_bitsize

///////////////////////////////////////////////////////////////
// errors that the compiler can't handle (at now version)

static unsigned get_if_depth_max ()
{ return (8 * usizeof(masked()->directive_if_disabler) / DIRECTIVE_STATE_PER_IF_BITS) -1; }

void _validate_bitsize ()
{
static bool is_done = false;
	if (is_done)
		return;
	is_done = true;

unsigned bitpos_max
	= get_if_depth_max() * DIRECTIVE_STATE_PER_IF_BITS;
u2 if_state_max
	= (1 << DIRECTIVE_STATE_PER_IF_BITS) -1;
MASKED test = {
	.directive_if_disabler = (if_state_max << bitpos_max),
	.directive_if_depth    = get_if_depth_max(),
	.directive_id          = DIRECTIVE_ID_MAX,
};
bitsize_error((if_state_max << bitpos_max) == test.directive_if_disabler)
bitsize_error(get_if_depth_max() == test.directive_if_depth)
bitsize_error(DIRECTIVE_ID_MAX  == test.directive_id)
}

///////////////////////////////////////////////////////////////
// PENDING: separates into filter.c

/* NOTE:
(*1) Even while the preprocessor disables source by '#if 0' etc,
     it needs to manage '#if' without ignoring.
	ex)
    #if 0
    # if ...
    # endif  <- (because)
    #endif

(*2) However, It has to ignore directives out of '#if/#endif/#else/#elif'.
	ex)
	 #if 0
	 # undef(|define) MMM <- (because)
	 #endif
 */

extern bool _is_disable_by_if ()
{
unsigned bitpos
	= masked()->directive_if_depth * DIRECTIVE_STATE_PER_IF_BITS;
unsigned mask
	= (1 << bitpos) -1;
manage_bug(0 == (~mask & masked()->directive_if_disabler))

bool any_disabled
	= (mask & masked()->directive_if_disabler) ? true : false;
	return any_disabled; // NOTE: (*1)
}

#define INITIAL    1
#define ENTER_TRUE 0 // NOTE: zero for optimized AND cf)_is_disable()
#define LEAVE_TRUE 2
static void filter_set_state (u2 state)
{
manage_bug(0 < masked()->directive_if_depth)
unsigned bitpos
	= (masked()->directive_if_depth -1) * DIRECTIVE_STATE_PER_IF_BITS;
u2 region
	= (1 << DIRECTIVE_STATE_PER_IF_BITS) -1;
	masked()->directive_if_disabler &= ~(region << bitpos);
	masked()->directive_if_disabler |= state << bitpos;
}

static u2 filter_get_state ()
{
manage_bug(0 < masked()->directive_if_depth)
unsigned bitpos
	= (masked()->directive_if_depth -1) * DIRECTIVE_STATE_PER_IF_BITS;
u2 region
	= (1 << DIRECTIVE_STATE_PER_IF_BITS) -1;
u2 retval
	= region & (masked()->directive_if_disabler >> bitpos);
	return retval;
}

static unsigned filter_calc (const TOKEN *tok, unsigned len, unsigned *result)
{
const char *_text = masked()->text;
unsigned parsed_cnt, calculated;
	if (1 == len && '0' == _text[tok->pos])
		calculated = 0, parsed_cnt = 1;
	else if (1 == len && '1' == _text[tok->pos])
		calculated = 1, parsed_cnt = 1;
	else
tiny_error("#if: now supports '0' '1' only.") // TODO: more

	if (result)
		*result = calculated;
	return parsed_cnt;
}

static bool macro_is_defined (const char *s, unsigned n)
{
LOOKEDUP _lup; MACROID found_id
	= macro_lookup (s, n, &_lup);
bool bFound
	= (0 == found_id) ? false : true;
	return bFound;
}

static unsigned _endif (const TOKEN *tok, unsigned len)
{
	if (! (0 < masked()->directive_if_depth))
abort_("%s " VTRR "error" VTO ": unexpected '#endif'" "\n", whereis());
//manage_bug(0 < masked()->directive_if_depth)
	filter_set_state (0);
	--masked()->directive_if_depth;

unsigned parsed_cnt = 0; // doesn't have parameter
	return parsed_cnt;
}

static unsigned _else (const TOKEN *tok, unsigned len)
{
input_bug(0 < masked()->directive_if_depth)
u2 state
	= filter_get_state ();
	state = (INITIAL == state) ? ENTER_TRUE : LEAVE_TRUE;

	filter_set_state (state);

unsigned parsed_cnt = 0; // doesn't have parameter
	return parsed_cnt;
}

static unsigned _if (const TOKEN *tok, unsigned len)
{
unsigned calculated, parsed_cnt
	= filter_calc (tok, len, &calculated);
bool bOK
	= (0 == calculated) ? false : true;

	if (get_if_depth_max() < masked()->directive_if_depth +1)
tiny_error("#if: Too many nesting.")
	++masked()->directive_if_depth;
manage_bug(0 == filter_get_state ())
	filter_set_state (bOK ? ENTER_TRUE : INITIAL);

	return parsed_cnt;
}

static unsigned _elif (const TOKEN *tok, unsigned len)
{
u2 state
	= filter_get_state ();
	if (ENTER_TRUE == state)
		filter_set_state (LEAVE_TRUE);
	if (! (INITIAL == state))
		return len; // ignore all tokens

unsigned calculated, parsed_cnt
	= filter_calc (tok, len, &calculated);

	if (! (0 == calculated))
		filter_set_state (ENTER_TRUE);

	return parsed_cnt;
}

static unsigned _ifdef (const TOKEN *tok, unsigned len)
{
const char *_text = masked()->text;

	if (! (0 < len))
tiny_error("#ifdef: missing symbol.")
bool bOK
	= !macro_is_defined (_text +tok->pos, tok->len) ? false : true;

	if (get_if_depth_max() < masked()->directive_if_depth +1)
tiny_error("#ifdef: Too many nesting.")
	++masked()->directive_if_depth;
manage_bug(0 == filter_get_state ())
	filter_set_state (bOK ? ENTER_TRUE : INITIAL);

unsigned parsed_cnt = 1; // always parse single token
	return parsed_cnt;
}

static unsigned _ifndef (const TOKEN *tok, unsigned len)
{
const char *_text = masked()->text;

	if (! (0 < len))
tiny_error("#ifndef: missing symbol.")
bool bOK
	= macro_is_defined (_text +tok->pos, tok->len) ? false : true;

	if (get_if_depth_max() < masked()->directive_if_depth +1)
tiny_error("#ifndef: Too many nesting.")
	++masked()->directive_if_depth;
manage_bug(0 == filter_get_state ())
	filter_set_state (bOK ? ENTER_TRUE : INITIAL);

unsigned parsed_cnt = 1; // always parse single token
	return parsed_cnt;
}

///////////////////////////////////////////////////////////////

static u2 _lookup (const char *s, unsigned n)
{
	if (5 == n && 0 == memcmp ("undef", s, n))
		return ID_UNDEF;
	else if (6 == n && 0 == memcmp ("define", s, n))
		return ID_DEFINE;
//	else if (7 == n && 0 == memcmp ("include", s, n))
//		return ID_INCLUDE;
	else if (5 == n && 0 == memcmp ("endif", s, n))
		return ID_ENDIF;
	else if (2 == n && 0 == memcmp ("if", s, n))
		return ID_IF;
	else if (4 == n && 0 == memcmp ("else", s, n))
		return ID_ELSE;
	else if (5 == n && 0 == memcmp ("ifdef", s, n))
		return ID_IFDEF;
	else if (6 == n && 0 == memcmp ("ifndef", s, n))
		return ID_IFNDEF;
	else if (4 == n && 0 == memcmp ("elif", s, n))
		return ID_ELIF;
	else
		return 0;
}

static unsigned _dispatch (unsigned id, const TOKEN *tok, unsigned len)
{
unsigned cnt;
bool group_of_if = true;
	switch (id) {
	case ID_ENDIF:
		cnt = _endif (tok, len);
		break;
	case ID_IF:
		cnt = _if (tok, len);
		break;
	case ID_ELSE:
		cnt = _else (tok, len);
		break;
	case ID_IFDEF:
		cnt = _ifdef (tok, len);
		break;
	case ID_IFNDEF:
		cnt = _ifndef (tok, len);
		break;
	case ID_ELIF:
		cnt = _elif (tok, len);
		break;
	default:
		group_of_if = false;
		break;
	}
	if (group_of_if) {
		return cnt;
	}

	if (_is_disable_by_if ())
		return len; // NOTE: (*2)

	switch (id) {
	case ID_UNDEF:
		cnt = macro_undef (tok, len);
		return cnt;
	case ID_DEFINE:
		cnt = macro_define_first (tok, len);
		return cnt;
//	case ID_INCLUDE:
//		cnt = prepro_include (tok, len);
//		return cnt;
	default:
tiny_error("invalid directive") // or not implemented yet
		return 0; // PENDING: dummy, error recovering not ensured
	}
}

#define FINISHED 1
static unsigned _dispatch_aux (unsigned id, const TOKEN *top, unsigned len)
{
	if (_is_disable_by_if ())
		return len; // NOTE: (*2)

	// TODO: macro_define_body ()
unsigned retval = 0;
	switch (id) {
	case ID_DEFINE:
		retval = macro_define_next (top, len);
		break;
	default:
tiny_error("too many tokens at directive")
		break;
	}
	return retval;
}

unsigned _parse_line (const void *tokenized, unsigned tokens_len)
{
const char *_text
	= masked()->text;

caller_bug(strlen (_text) < (1 << TOKEN_POS_BITS) -1) // bits over TOKEN_POS

const TOKEN *head, *tok, *tail;
	tok = head = (const TOKEN *)tokenized, tail = head +tokens_len;
bool bPoundStart
	= (tok < tail && 1 == tok->len && '#' == _text[head->pos]) ? true : false;

caller_bug(masked()->bDirectiveContinued || bPoundStart) // not diretive line

	// beginning of directive lines?
unsigned cnt1 = 0;
	if (bPoundStart && !masked()->bDirectiveContinued) {
		++tok, ++cnt1; // '#'
u2 id
		= _lookup (_text +tok->pos, tok->len);

		++tok, ++cnt1;
unsigned cnt
		= _dispatch (id, tok, tokens_len -cnt1);

		tok += cnt, cnt1 += cnt;
		masked()->directive_id = id;
	}

	// parse directive line
	// #define <symbol>[( ... )] ___
unsigned cnt2 = 0;
	if (cnt1 < tokens_len) {
		cnt2 = _dispatch_aux (masked()->directive_id, tok, tokens_len -cnt1);
		tok += cnt2;
	}

	if (cnt1 +cnt2 +1 == tokens_len && 1 == tok->len && '\\' == _text[tok->pos])
		masked()->bDirectiveContinued = TRUE;
	else
		masked()->bDirectiveContinued = FALSE;
	return cnt1 +cnt2;
}

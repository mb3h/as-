/*
TODO: separate: masked() -> draft(), directive(), ...
      separate: assert -> caller_bug, ...
        remove: _next_parameter()
        change: p -> stk, n -> stk_n
        change: a part of 'parameter' -> 'argument'
        remove: '_stack' completely
        change: masked()->got_XXX -> ???()->XXX
 */

/*	NOTE: (*1) When '#' token starts a literalizing,
	           this flow writes '"' into the top of array.
	           But same thing happens by a token like '".equ "',
	           therefore this flag is needed apart from it.
	NOTE: (*2) needed for the following
	ex)
	 #define fff($1)
	 #define ggg($1,$2) $2(a) <- (because)

	 ggg(b,fff)
	TODO: (*3) handling 'brackets not closed' error
	ex)
	 #define fff($1)
	 #define ggg($1) fff($1  <- (because)
	 
	 ggg(a) )
 */

#include "post/tokenize.h"

#include "as-.h"
#include "macro.h"
#include "param.h"
#define PARAMID_MAX (1 << PARAMID_BITS) // TODO: -> param.h
#include "pre/define.h"
#include "helper/m2s.h"
#include "helper/pow2_ge.h"
#include "helper/assert.h"
#define manage_bug assert
#define   flow_bug assert
#define   data_bug assert
#define caller_bug assert

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <alloca.h>

typedef  uint8_t u8, u1, u2;
typedef uint16_t u16;

#define usizeof(x) (unsigned)sizeof(x)
#define ustrlen(s) (unsigned)strlen(s)
///////////////////////////////////////////////////////////////
// alias

// import
#define _lookup       macro_lookup

#define PARAM_MAX     MACRO_PARAM_MAX
#define bitsize_error   assert
#define tiny_error(msg) assert((msg, false))

// export
#define _init             tokenize_init       
#define _get_C_first      tokenize_get_C_first
#define _get_C            tokenize_get_C      

#define _validate_bitsize tokenize_validate_bitsize

///////////////////////////////////////////////////////////////
// constants

#define PROGRESS_STACK_INITLEN 4

// STACK::type
#define  ROOT_RESOLVE 0
#define MACRO_RESOLVE 1
#define PARAM_RESOLVE 2
//#define XXXXX_RESOLVE 4 CAUTION: uses 3, causing bug cf)_pop_stack_until_peekable()

///////////////////////////////////////////////////////////////

typedef struct {
	void (*_reset)(void *priv, unsigned len, const void *data);
	const TOKEN *(*_peek)(void *priv, const char **source);
	const TOKEN *(*_next)(void *priv, const char **source);
	unsigned (*_next_total)(void *priv);
	const TOKEN *(*_next_parameter)(void *priv, const char **source, unsigned *params_len);
} HANDLER;

static void _reset (void *priv, unsigned len, const void * /*data*/);
static const TOKEN *_peek (void *priv, const char **source);
static const TOKEN *_next (void *priv, const char **source);
static const TOKEN *_next_parameter (void *priv, const char **source, unsigned *params_len);

static const HANDLER vtbl_from[3] = {{ //  ROOT_RESOLVE
  ._reset          = _reset
, ._peek           = _peek
, ._next           = _next
, ._next_total     = NULL
, ._next_parameter = _next_parameter
}, { // MACRO_RESOLVE
  ._reset          = macro_tokenize_reset
, ._peek           = macro_tokenize_peek
, ._next           = macro_tokenize_next
, ._next_total     = macro_tokenize_next_total
, ._next_parameter = macro_tokenize_next_parameter
}, { // PARAM_RESOLVE
  ._reset          = param_tokenize_reset
, ._peek           = param_tokenize_peek
, ._next           = param_tokenize_next
, ._next_total     = NULL
, ._next_parameter = NULL
}};

typedef struct {
	const HANDLER *vtbl;
	union {
		struct { // .type = MACRO_RESOLVE
			MACROID macro_id;
			u8 params_len; // <= MACRO_PARAM_MAX
			PARAMID params[PARAM_MAX];
		};
		struct { // .type = PARAM_RESOLVE
			u16 owner_stack_depth;
			PARAMID param_id;
		};
	};
	u8 type :2;
	u8 pad  :6;
	u8 priv[8];
} STACK;

static STACK *bottom() { return (STACK *)masked()->resolve_stack; }
static STACK *top() { return &((STACK *)masked()->resolve_stack)[masked()->stack_depth]; }
static u16 top_depth() { return masked()->stack_depth; }

///////////////////////////////////////////////////////////////

static void _stack_init ()
{
assert(NULL == masked()->resolve_stack && 0 == masked()->stack_max)
unsigned m
	= PROGRESS_STACK_INITLEN;

	masked()->resolve_stack = malloc (sizeof(STACK) * m);
	masked()->stack_max = m;
	masked()->stack_depth = 0;
}

static void _pop ()
{
assert(0 < masked()->stack_depth)
	// TODO: free()
	--masked()->stack_depth;
}

static void *_reserve_stack (unsigned need)
{
unsigned m
	= masked()->stack_max;
	if (m < need) {
		do {
			m *= 2;
		} while (m < need);
		masked()->resolve_stack = realloc (masked()->resolve_stack,
		                                   sizeof(STACK) * m);
		masked()->stack_max = m;
	}
	return masked()->resolve_stack;
}

// @param n stack depth
static void _release_stack (unsigned n)
{
STACK *_stack
	= (STACK *)masked()->resolve_stack;
STACK *p
	= &_stack[n];

u8 i; PARAMID param_id;
	switch (p->type) {
	default:
		break;
//	case ROOT_RESOLVE:
	case MACRO_RESOLVE:
		for (i = 0; i < p->params_len; ++i) {
			param_id = p->params[i];
			param_remove (param_id);
			p->params[i] = 0; // safety
		}
		break;
//	case PARAM_RESOLVE:
	}
}

static STACK *_push_stack (u2 type)
{
unsigned n
	= ++masked()->stack_depth;
STACK *_stack
	= _reserve_stack (n +1);
	top()->type = type;
	top()->vtbl = &vtbl_from[type];
	return _stack;
}

static void _pop_stack ()
{
unsigned n
	= masked()->stack_depth;
	_release_stack (n);
	top()->vtbl = NULL; // bug guard
	--masked()->stack_depth;
}

static const TOKEN *_pop_stack_until_peekable (u2 allow_types, const char **source)
{
caller_bug(allow_types) // deny ROOT_RESOLVE specified

const TOKEN *tok = NULL; const char *_source;
	while (bottom() < top()) {
data_bug(MACRO_RESOLVE == top()->type || PARAM_RESOLVE == top()->type)
		if (! (allow_types & top()->type))
			break;
manage_bug(top()->vtbl->_peek)
		if (tok = top()->vtbl->_peek (&top()->priv, &_source)) {
			break;
		}
		_pop_stack ();
	}
	if (tok && source)
		*source = _source;
	return tok;
}

///////////////////////////////////////////////////////////////

#define IS_PARAM(t) (PARAMID_MAX -PARAM_MAX <= (t)->len)
static bool _is_param_included (const TOKEN *tok, unsigned len)
{
	for (; 0 < len; --len, ++tok)
		if (IS_PARAM(tok))
			return true;
	return false;
}

static PARAMID _decode_token (const TOKEN *tok, u16 encoded_depth, u16 *decoded_owner_depth)
{
const STACK *_stack
	= (STACK *)masked()->resolve_stack;
unsigned encoded_index
	= (PARAMID_MAX -1) -tok->len;
assert(0 < encoded_depth)
u16 depth = encoded_depth;
	if (PARAM_RESOLVE == _stack[depth].type)
		depth = _stack[depth].owner_stack_depth;
assert(MACRO_RESOLVE == _stack[depth].type)
u8 param_index = encoded_index;
	if (! (param_index < _stack[depth].params_len))
		param_index -= _stack[depth].params_len, --depth;
assert(MACRO_RESOLVE == _stack[depth].type)
PARAMID param_id
	= _stack[depth].params[param_index];
	if (decoded_owner_depth)
		*decoded_owner_depth = depth;
	return param_id;
}

static void _encode_tokens (const TOKEN *src, unsigned len, u8 shifting, TOKEN *dst)
{
	for (; 0 < len; --len, ++src, ++dst) {
		*dst = *src;
		if (IS_PARAM(dst)) {
assert(PARAMID_MAX -PARAM_MAX <= dst->len -shifting) // TODO: should raise error and abort.
			dst->len -= shifting;
		}
	}
}

///////////////////////////////////////////////////////////////
typedef struct {
	u8 index;
} ROOT_TOKENIZE;

static void _reset (void *priv, unsigned len, const void * /*data*/)
{
ROOT_TOKENIZE *m_ = (ROOT_TOKENIZE *)priv;

assert(sizeof(ROOT_TOKENIZE) <= len)
	m_->index = 0;
}

static const TOKEN *_peek (void *priv, const char **source)
{
ROOT_TOKENIZE *m_ = (ROOT_TOKENIZE *)priv;

const TOKEN *_tokens = (const TOKEN *)masked()->root_tokenized;
	if (! (m_->index < masked()->root_tokens_len))
		return NULL;
	if (source)
		*source = masked()->text;
const TOKEN *r
	= &_tokens[m_->index];
	return r;
}
static const TOKEN *_next (void *priv, const char **source)
{
ROOT_TOKENIZE *m_ = (ROOT_TOKENIZE *)priv;

const TOKEN *r
	= _peek (priv, source);
	if (r)
		++m_->index;
	return r;
}

// @func gets one by one parameter consisted on one more tokens while calling macro.
static const TOKEN *_next_parameter (void *priv, const char **source, unsigned *params_len)
{
const char *_text
	= masked()->text;
ROOT_TOKENIZE *m_ = (ROOT_TOKENIZE *)priv;
const TOKEN *_tokens = (const TOKEN *)masked()->root_tokenized;
u8 index0 = m_->index;
	for (; m_->index < masked()->root_tokens_len; ++m_->index) {
const TOKEN *tok
		= &_tokens[m_->index];
assert(! (1 == tok->len && '(' == _text[tok->pos])) // PENDING: complecated parameter (yet cannot include '(')
		if (1 == tok->len && (')' == _text[tok->pos] || ',' == _text[tok->pos])) {
assert(source && params_len)
			*source = _text;
			*params_len = m_->index -index0;
			return &_tokens[index0];
		}
	}
assert(0) // PENDING: multiline support
	return NULL;
}

void _init (const char *pre_defined_symbols)
{
	// stack
	if (NULL == masked()->resolve_stack)
		_stack_init ();

	// pre-defined symbols
unsigned len
		= ustrlen (pre_defined_symbols);
	if (0 < len) { // PENDING: two more symbols (splitting)
unsigned pos = 0;
MACROID macro_id
		= macro_open (pre_defined_symbols +pos, len, 0, NULL, NULL);
		macro_close (macro_id);
	}

	// parsing state
	masked()->stack_depth = 0;
	top()->type = ROOT_RESOLVE, top()->macro_id = 0;
	top()->vtbl = &vtbl_from[top()->type];
	top()->vtbl->_reset (top()->priv, usizeof(top()->priv), NULL);
}

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

#define PUSHABLE PUSHABLE_PARAM_PARSER
typedef struct {
	// heap() optimizes about heap
	TOKEN *tokens;
	unsigned reserved, len;
	// parsing() gives streamable to input
#define PRE_PARAMETER 1
#define PARAMETER     2
#define PRE_MACRO     3
	u8 state            :2;
#define BRACKET_NEST_MAX 16
	u8 unclosed_bracket :4;
	u8 bLiteralize      :1; // NOTE: (*1)
	u8 pad11_b6         :1;
	u8 req_args;
	PARAMID id;
	// parsed() gives streamable to input
	u8 count;
	PARAMID params[PARAM_MAX];
} PUSHABLE; // TODO: -> masked()->
static PUSHABLE s_singlton = {
	.tokens = NULL,
	.reserved = 0,
};
static PUSHABLE *my() { return &s_singlton; }
#define heap    my
#define parsing my
#define parsed  my

// joining()
typedef struct {
	char *buf;
	unsigned len;
} JOINING;
static JOINING s_joining = {
	.buf = NULL,
};
static JOINING *joining() { return &s_joining; }

///////////////////////////////////////////////////////////////

static void _push_joining (const char *s, unsigned n)
{
	// memory ensuring (2^N base)
unsigned now
	= joining()->len;
	if (NULL == joining()->buf)
		joining()->buf = (char *)malloc (pow2_ge(now +n +1));
	else if (pow2_ge(now +1) < now +n +1)
		joining()->buf = (char *)realloc (joining()->buf,
	                                     pow2_ge(now +n +1));
	// appending
	memcpy (joining()->buf +joining()->len, s, n);
	joining()->len += n;
	joining()->buf[joining()->len] = '\0';
}

static void _start_literalized_joining ()
{
flow_bug(0 == joining()->len)
	_push_joining ("\"", 1); // NOTE: (*1)
flow_bug(FALSE == parsing()->bLiteralize)
	parsing()->bLiteralize = TRUE;
}

static void _end_literalized_joining ()
{
char tmp[2]
	= { joining()->buf[0], '\0' };
	_push_joining (tmp, 1);
flow_bug(TRUE == parsing()->bLiteralize)
	parsing()->bLiteralize = FALSE;
}

static bool _is_literalized_joining ()
{ return (TRUE == parsing()->bLiteralize) ? true : false; }

/* @func This moves bytes from joined buffer to pulling buffer.
         + Joined buffer gets clear after moving. (joining()->len = 0) */
static const char *_pull_joining (unsigned *joined_len)
{
flow_bug(0 < joining()->len)
	if (joined_len)
		*joined_len = joining()->len;
	joining()->len = 0;
	return joining()->buf;
}

///////////////////////////////////////////////////////////////
// errors that the compiler can't handle (at now version)

void _validate_bitsize ()
{
static bool is_done = false;
	if (is_done)
		return;
	is_done = true;

PUSHABLE test = {
	.state = PRE_MACRO,
	.unclosed_bracket = BRACKET_NEST_MAX -1,
};
bitsize_error(PRE_MACRO == test.state)
bitsize_error(BRACKET_NEST_MAX -1 == test.unclosed_bracket)
}

///////////////////////////////////////////////////////////////

#define ROOT_NEWLINE 1
static u1 _get_macro_parameters (unsigned n
, u8 req_args
, PARAMID *params
)
{
STACK *_stack
	= (STACK *)masked()->resolve_stack;
STACK *p
	= &_stack[n];

	while (1) {
const char *_source; const TOKEN *tok;
		// gather tokens until ',', ')' or EOL
		while (1) {
			tok = p->vtbl->_next (&p->priv, &_source);
			// (root text) newline?
			if (NULL == tok && ROOT_RESOLVE == top()->type)
				return ROOT_NEWLINE;
assert(tok && !IS_NEWLINE(tok))
			// (nested or not) open-bracket?
			if (1 == tok->len && '(' == _source[tok->pos]) {
				if (! (parsing()->unclosed_bracket +1 < BRACKET_NEST_MAX))
tiny_error("Too deep nest of bracket in a parameter passed to macro")
				++parsing()->unclosed_bracket;
			}
			// (nested) close-bracket?
			else if (1 == tok->len && ')' == _source[tok->pos] && 0 < parsing()->unclosed_bracket) {
				if (! (0 < parsing()->unclosed_bracket))
tiny_error("Too many close-bracket in a parameter passed to macro")
				--parsing()->unclosed_bracket;
			}
			// (not nested) comma or close-bracket?
			else if (1 == tok->len && strchr (",)", _source[tok->pos]) && 0 == parsing()->unclosed_bracket)
				break;
			// memory ensuring (2^N base)
unsigned ge
			= pow2_ge(heap()->len +1);
			if (NULL == heap()->tokens) {
				heap()->tokens = (TOKEN *)malloc (sizeof(TOKEN) * ge);
				heap()->reserved = ge;
			}
			else if (heap()->reserved < heap()->len +1) {
				heap()->tokens = (TOKEN *)realloc (heap()->tokens,
				                                   sizeof(TOKEN) * ge);
				heap()->reserved = ge;
			}
			// normal token except for the above
			heap()->tokens[heap()->len] = *tok;
			++heap()->len;
		}
		// if one more parameters exist in gathered tokens, set a mark to each of them
const TOKEN *tokens
		= heap()->tokens;
assert(tok && 0 < heap()->len) // PENDING: multiline support
		if (_is_param_included (tokens, heap()->len)) {
TOKEN *enc
			= (TOKEN *)alloca (sizeof(TOKEN) * heap()->len);
			_encode_tokens (tokens, heap()->len, req_args, enc);
			tokens = enc;
		}
		// append tokens into the parameter-object
		param_write (parsing()->id, _source, tokens, heap()->len);
		// accomplish creating the parameter-object, and accumulate its ID into list
		param_close (parsing()->id);
		if (! (parsed()->count < req_args))
tiny_error("Too many parameters passed to macro.")
		params[parsed()->count++] = parsing()->id;
		// go to next parsing if needed (= next token is ',')
assert(tok) // PENDING: multiline support
		if (! (tok && 1 == tok->len && ',' == _source[tok->pos])) {
assert(tok && 1 == tok->len && ')' == _source[tok->pos]) // PENDING: multiline support
			break;
		}
		// start creating a parameter-object of $2..$N from tokens
		heap()->len = 0;
		parsing()->id = param_open ();
	}
	if (! (req_args == parsed()->count))
tiny_error("Mismatch parameters of macro with defined.")

	return 0;
}

///////////////////////////////////////////////////////////////
#undef PUSHABLE

// start getting tokens of a parameter for a macro
static void _pre_parameter ()
{
	// unshift stack until any token is found NOTE: (*2)
const char *_source; const TOKEN *tok
	= _pop_stack_until_peekable (PARAM_RESOLVE, &_source);

	tok = top()->vtbl->_next (&top()->priv, &_source);
assert(tok && 1 == tok->len && '(' == _source[tok->pos])
	// start creating a parameter-object of $1 from tokens
	parsed()->count = 0;
	parsing()->unclosed_bracket = 0;
	heap()->len = 0;
	parsing()->id = param_open ();
	parsing()->state = PARAMETER;
}

// parsing macro parameter(s) -> $1..$N table
static int _parameter ()
{
assert(0 < parsing()->req_args)
	if (! (parsing()->req_args <= PARAM_MAX))
tiny_error("Too many parameter(s).")
int r
	= _get_macro_parameters (top_depth()
		, parsing()->req_args
		, parsed()->params
	);
	if (! (ROOT_NEWLINE == r))
		parsing()->state = PRE_MACRO;
	return r;
}

// start resolving a macro (stack-shift: ROOT|MACRO|PARAM -> MACRO)
static void _enter_macro ()
{
	top()->macro_id = masked()->found_id;
	top()->vtbl->_reset (top()->priv, usizeof(top()->priv), &top()->macro_id);
	if (0 < parsing()->req_args)
		memcpy (top()->params, parsed()->params, sizeof(PARAMID) * parsing()->req_args);
	top()->params_len = parsing()->req_args;
	parsing()->state = 0;
}

// not literal but macro parameter(s) ($1..$N table)
static void _enter_param (const TOKEN *tok)
{
manage_bug(top()->params)
u16 owner_depth; PARAMID param_id
	= _decode_token (tok, top_depth() -1, &owner_depth);
assert(0 < param_id)
	top()->param_id = param_id;
	top()->owner_stack_depth = owner_depth;
	top()->vtbl->_reset (top()->priv, usizeof(top()->priv), &top()->param_id);
}

///////////////////////////////////////////////////////////////

#define NOTIFY_NEWLINE "\n"
const char *_get_C (unsigned *ret_len) // with preprocessing
{
	while (1) {
		if (PRE_PARAMETER == parsing()->state)
			_pre_parameter ();
		if (PARAMETER == parsing()->state) {
			if (ROOT_NEWLINE == _parameter ())
				break; // end of line
		}
		if (PRE_MACRO == parsing()->state) {
			_push_stack (MACRO_RESOLVE);
			_enter_macro ();
			continue;
		}

const TOKEN *tok; const char *_source;
		tok = top()->vtbl->_next (&top()->priv, &_source);

		if (NULL == tok) {
			if (PARAMETER == parsing()->state)
tiny_error("macro's bracket not closed.") // TODO: (*3)

			if (0 == masked()->stack_depth) {
				break; // end of tokens(= passed argument)
			}

			_pop_stack (); // ROOT|MACRO <- MACRO|PARAM
			continue; // end of called macro
		}

		if (1 == tok->len && '#' == _source[tok->pos]) {
			_start_literalized_joining ();
			continue;
		}

		if (IS_PARAM(tok)) {
			_push_stack (PARAM_RESOLVE); // MACRO -> PARAM
			_enter_param (tok);
			continue;
		}

		if (IS_NEWLINE(tok)) {
			if (ret_len)
				*ret_len = ustrlen (NOTIFY_NEWLINE);
			return NOTIFY_NEWLINE; // newline in macro's declaration
		}

		// token isn't a macro name?
LOOKEDUP _lup; MACROID found_id = 0;
		if (!_is_literalized_joining ())
			found_id = masked()->found_id = _lookup (_source +tok->pos, tok->len, &_lup);
		if (!found_id) {

			// store passed token into caller buffer
			_push_joining (_source +tok->pos, tok->len);

			// Is next token '##'?
			tok = _pop_stack_until_peekable (MACRO_RESOLVE|PARAM_RESOLVE, &_source);
			if (tok && 2 == tok->len
			 && 0 == memcmp ("##", _source +tok->pos, 2)) {
				top()->vtbl->_next (&top()->priv, NULL); // skipping token '##'
				continue; // cannot back to caller yet
			}

			if (_is_literalized_joining ())
				_end_literalized_joining ();
			return _pull_joining (ret_len); // normal token (not macro)

		}
		// token is alias macro? (consisted single symbol)
		if (MACRO_RESOLVE == top()->type
		 && 1 == top()->vtbl->_next_total (&top()->priv)
		 && !top()->vtbl->_peek (&top()->priv, NULL)) {
			// stack down (= macro removing & replacing)
			_pop_stack ();
		}
		// start resloving macro (after getting parameter(s) if needed)
		parsing()->req_args = _lup.req_args;
		parsing()->state = (0 < _lup.req_args) ? PRE_PARAMETER : PRE_MACRO;
	}

	// end of line (root stack)
	return NULL;
}

///////////////////////////////////////////////////////////////

void _get_C_first ()
{
	bottom()->vtbl->_reset (&bottom()->priv, usizeof(bottom()->priv), NULL);
}

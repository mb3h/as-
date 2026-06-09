#include "macro.h"
#define MACROID_MAX (1 << MACROID_BITS)

#include <stdint.h>
#include <stdbool.h>

#include "tokenlist.h"
#include "helper/assert.h"
#include "helper/m2s.h"
#include "helper/vt100.h"

#include <stdio.h>
#include <stddef.h> // offsetof
#include <memory.h>
#include <alloca.h>

typedef  uint8_t u8, u1;
typedef uint16_t u16;

#define usizeof(x) (unsigned)sizeof(x)
#define uoffsetof(x, a) (unsigned)offsetof(x, a)
#define ustrlen(s) (unsigned)strlen(s)
///////////////////////////////////////////////////////////////
// alias

// export
#define _lookup                  macro_lookup
#define _remove                  macro_remove
#define _open                    macro_open
#define _close                   macro_close
#define _write                   macro_write
#define _tokenize_reset          macro_tokenize_reset
#define _tokenize_peek           macro_tokenize_peek
#define _tokenize_next           macro_tokenize_next
#define _tokenize_next_total     macro_tokenize_next_total
#define _tokenize_next_parameter macro_tokenize_next_parameter

// import
#define _create        tokenlist_create
#define _append_tokens tokenlist_append_tokens
#define _destroy       tokenlist_destroy
#define _HEADER        TOKENLIST_HEADER

///////////////////////////////////////////////////////////////
// preprocessor (macro)

/* PENDING:
(*1) memory fragmentation by repeating #undef
(*2) now slow (sequencial seek), should give faster (binary seek etc.)
 */
/* NOTE:
(*3) never uses #0.  ref)_lookup()
(*4) removed ID isn't used again. if an issue happened, it should be fixed.
(*5) If macro has no argument, _open() ignores 'args' and instantly
     starts the definition of 1st line after MACRO_ENTRY::name[].
     Any byte, even 00 (= '\0') isn't written after.
(*6) If macro has no token (= empty macro), _write() never called.
       ex) #define XXX(a,b) <no token>
           #define YYY <no token>
     Any byte, even 00 (= '\0') isn't written after the end of MACRO_ENTRY.
 */
typedef struct {
	_HEADER hdr;
	u8 argc;
	u8 name_len;
	char name[1];
} MACRO_ENTRY; // PENDING: (*1)(*2)

typedef struct {
	MACRO_ENTRY **macros_map;
	unsigned macro_max;
	unsigned macro_used;
} MACRO;

///////////////////////////////////////////////////////////////
/* Checking errors that now C/C++ compilers can't handle.
   TRADEOFF: performance (moment)
   ALTERNATIVE: Testing in Makefile */

static void _conflict_test_once ()
{
	// Done only once
static bool is_done = false;
	if (is_done)
		return;
	is_done = true;

MASKED u;
	// Prepared memory in MASKED is enough?
assert(sizeof(MACRO) <= sizeof(u.macro_work))

	// bitsizeof(.defining_id) == bitsizeof(MACRO_MAX)
	u.defining_id = MACRO_MAX -1;
assert(MACRO_MAX -1 == u.defining_id)
}

///////////////////////////////////////////////////////////////

// @param lup optional (NULL available)
MACROID _lookup (const char *name, unsigned name_len, LOOKEDUP *lup)
{
//	if (!)
//		return -1;
MACRO *m_
	= (MACRO *)masked()->macro_work;
	if (NULL == m_->macros_map)
		return 0;

MACROID id;
	for (id = 1; id < m_->macro_used; ++id) { // NOTE: (*3)(*4)
MACRO_ENTRY *macro;
		if (! (macro = m_->macros_map[id])) // NOTE: (*4)
			continue;
		if (name_len == macro->name_len && 0 == memcmp (name, macro->name, name_len)) {
			if (lup)
				lup->req_args = macro->argc;
			return id; // found
		}
	}
	return 0; // not found
}

void _remove (MACROID id)
{
MACRO *m_
	= (MACRO *)masked()->macro_work;
assert(m_ && m_->macros_map)
assert(id < m_->macro_used)

assert(m_->macros_map[id])
	if (!m_->macros_map[id])
		return; // ideally unexpected condition 

	_destroy (&m_->macros_map[id]->hdr);
	m_->macros_map[id] = NULL;
}

MACROID _open (const char *name, unsigned name_len, unsigned argc, const TOKEN *args, const char *_source)
{
	_conflict_test_once (); // PENDING: Run in application initializer

	if (_lookup (name, name_len, NULL)) {
printf ("%s " VTRR "warn" VTO ": " VTYY "multiply defined" VTO ": '" VTRR "%s" VTO "' still defined, ignored." "\n", whereis(), m2s(name, name_len));
		return 0; // error
	}

MACRO *m_
	= (MACRO *)masked()->macro_work;

	// append entry
u16 need
	= uoffsetof(MACRO_ENTRY, name) +name_len;
MACRO_ENTRY *entrying
	= (MACRO_ENTRY *)alloca (need);
	memset (entrying, 0, sizeof(MACRO_ENTRY)); // safety
	entrying->argc = argc;
	entrying->name_len = name_len;
	memcpy (entrying->name, name, name_len);
	if (0 < argc) { // NOTE: (*5)
unsigned w1, w2;
		w1 = usizeof(TOKEN) * argc;
		w2 = ustrlen (_source) +1;
u8 *work
		= (u8 *)malloc (w1 +w2);
		memcpy (work, args, w1);
		strcpy ((char *)(work +w1), _source);
		masked()->defining_work = work;
	}

unsigned id
	= _create ((_HEADER ***)&m_->macros_map, &entrying->hdr, need, &m_->macro_used, &m_->macro_max, MACROID_MAX);
assert(id < MACROID_MAX)
	if (! (0 < id))
abort_ ("%s " VTRR "macro's id filled out limitation %d" VTO " [" VTCC "%s" VTO "]" "\n", whereis(), MACROID_MAX, __func__);

assert(m_->macros_map[id]->hdr.size == need)

	return id;
}

// NOTE: (*6)
void _write (MACROID id, const char *text, const TOKEN *tokens, unsigned tokens_len, u1 /*flags*/)
{
MACRO *m_
	= (MACRO *)masked()->macro_work;
assert(0 < id && id < m_->macro_used)

MACRO_ENTRY *entried
	= m_->macros_map[id];
unsigned data_begin
	= uoffsetof(MACRO_ENTRY, name) +entried->name_len;

	if (0 < entried->argc) {
const TOKEN *args
		= (const TOKEN *)masked()->defining_work;
const char *_source
		= (const u8 *)&args[entried->argc];
TOKEN *fixed
		= (TOKEN *)alloca (sizeof(TOKEN *) * tokens_len);
unsigned  n, i;
		for (n = 0; n < tokens_len; ++n) {
			fixed[n] = tokens[n];
			// assign $1..$N into .len if token matches with symbol of $1..$N
			for (i = 0; i < entried->argc; ++i)
				if (tokens[n].len == args[i].len
				 && 0 == memcmp (text +tokens[n].pos, _source +args[i].pos, tokens[n].len)
				)
					break;
			if (i < entried->argc) // matched with keyword of parameter $1..$N
				fixed[n].len = (1 << MACROID_BITS) -(i +1); // identifier of $1..$N
		}
		tokens = fixed;
	}
	m_->macros_map[id] = (MACRO_ENTRY *)_append_tokens (&entried->hdr, data_begin, text, tokens, tokens_len);
}

void _close (MACROID mid)
{
}

///////////////////////////////////////////////////////////////
/* NOTE: (*1)
Rules of lifetime about a pointer (*source):
- Ensures same value while returning a token of same line
  registered by _write().
- Ensures 'not dead' even when returning NEWLINE or 0
  (end of defined lines) at later calling _next() or _peek(),
  doesn't write nothing into (*source).
- Allows 'dead' when returning a token of next line.

These rules are designed for the use:
- After repeatly calling until NEWLINE (or 0), refers (*source)
  once only.
  Then, repeats same about next line.
 */

typedef struct {
	MACROID macro_id;
	u16 text_offset;
	TOKEN_POS text_len;
	TOKENS_CNT token_index;
	u16 got_total :15;
	u16 is_peeked :1;
	u8 padding[0]; // (64bits alignment)
} TOKENIZE_ITERATOR;
void _tokenize_reset (void *priv, unsigned len, const void *data)
{
TOKENIZE_ITERATOR *_it = (TOKENIZE_ITERATOR *)priv;

assert(sizeof(TOKENIZE_ITERATOR) <= len)
MACRO *m_
	= (MACRO *)masked()->macro_work;
MACROID id
	= *(MACROID *)data;
MACRO_ENTRY *entried
	= m_->macros_map[id];

	// resetting tokenizer (id=lookedup, ..., 1st of defined lines and 1st token)
	_it->macro_id = id;
	_it->got_total = 0;
	_it->text_offset = uoffsetof(MACRO_ENTRY, name) +entried->name_len; // 1st line
	if (! (_it->text_offset < entried->hdr.size))
		_it->text_len = 0; // nothing line (symbol only #define)
	else
		_it->text_len = ustrlen ((const char *)entried +_it->text_offset); // 1st line
	_it->token_index = 0; // 1st token
}

enum TYPE { NEXT = 0, PEEK };
static const TOKEN *_tokenize_peek_or_next (void *priv, const char **source, enum TYPE type)
{
TOKENIZE_ITERATOR *_it = (TOKENIZE_ITERATOR *)priv;

MACRO *m_
	= (MACRO *)masked()->macro_work;
MACRO_ENTRY *entried
	= m_->macros_map[_it->macro_id];
	if (! (_it->text_offset < entried->hdr.size))
		return NULL; // for nothing line (symbol only #define)

const char *_text
	= (const char *)entried +_it->text_offset;
const TOKEN *_tokens
	= (const TOKEN *)(_text +_it->text_len +1);

const TOKEN *tok
	= _tokens +_it->token_index;
	// can get token from line? (not end of line)
	if (0 < tok->len) {
		if (NEXT == type)
			++_it->token_index, ++_it->got_total;
		if (source)
			*source = _text; // NOTE: (*1)
		return tok;
	}
	// continued to next line?
	if ((const u8 *)(tok +1) < (const u8 *)entried +entried->hdr.size) {
		// prevents going to the next line for _peek()
		if (PEEK == type)
			return &NEWLINE; // NOTE: (*1)
		++tok;
		// <N>th of defined lines
assert((const u8 *)tok < (const u8 *)entried +(1 << 16)) // 16 = bitsizeof(TOKENLIST::size)
		_it->text_offset = (u16)((const u8 *)tok - (const u8 *)entried);
		_it->text_len    = (u8)ustrlen ((const char *)entried +_it->text_offset);
		// 1st token
		_it->token_index = 0;
		// notify newline between defined lines
		return &NEWLINE; // NOTE: (*1)
	}
	return NULL; // end of defined lines NOTE: (*1)
}
const TOKEN *_tokenize_peek (void *priv, const char **source)
{ return _tokenize_peek_or_next (priv, source, PEEK); }
const TOKEN *_tokenize_next (void *priv, const char **source)
{ return _tokenize_peek_or_next (priv, source, NEXT); }
unsigned _tokenize_next_total (void *priv)
{ return ((TOKENIZE_ITERATOR *)priv)->got_total; }

// @func gets one by one parameter consisted on one more tokens while calling macro.
const TOKEN *_tokenize_next_parameter (void *priv, const char **source, unsigned *params_len)
{
TOKENIZE_ITERATOR *_it = (TOKENIZE_ITERATOR *)priv;

MACRO *m_
	= (MACRO *)masked()->macro_work;
MACROID id
	= _it->macro_id;
MACRO_ENTRY *entried
	= m_->macros_map[id];

const char *_text
	= (const char *)entried +_it->text_offset;
const TOKEN *_tokens
	= (const TOKEN *)(_text +_it->text_len +1);

u8 index0 = _it->token_index;
const TOKEN *tok
	= _tokens +_it->token_index;
	for (; 0 < tok->len; ++tok, ++_it->token_index) {
assert(! (1 == tok->len && '(' == _text[tok->pos])) // PENDING: complecated parameter (yet cannot include '(')
		if (1 == tok->len && (')' == _text[tok->pos] || ',' == _text[tok->pos])) {
assert(source && params_len)
			*source = _text;
			*params_len = _it->token_index -index0;
			return &_tokens[index0];
		}
	}
assert(("unexpected end of line", 0)) // PENDING: multiline support
	return NULL;
}

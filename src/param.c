#include "param.h"
#define PARAMID_MAX (1 << PARAMID_BITS)

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
#define _remove         param_remove
#define _open           param_open
#define _write          param_write
#define _close          param_close
//#define _entry          param_entry
#define _tokenize_reset param_tokenize_reset
#define _tokenize_peek  param_tokenize_peek
#define _tokenize_next  param_tokenize_next

// import
#define _create        tokenlist_create
#define _append_tokens tokenlist_append_tokens
#define _destroy       tokenlist_destroy
#define _HEADER        TOKENLIST_HEADER

///////////////////////////////////////////////////////////////
// preprocessor (macro)

/* PENDING:
(*1) memory fragmentation by repeating
   NOTE:
(*2) we can't set empty into any parameter of a macro.
     ex)  MMM(,,)
          MMM(a,,c)
   PENDING:
(*3) tradeoff getting slow for a cost of implementing '.text_len'
 */
typedef struct {
	_HEADER hdr;
} PARAM_ENTRY; // PENDING: (*1)

typedef struct {
	PARAM_ENTRY **params_map;
	unsigned param_max;
	unsigned param_used;
	u8 is_id_recycled :1;
	u8 pad            :7;
	u8 padding[7]; // (64bits alignment)
} PARAM;

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

	// Prepared memory in MASKED is enough?
MASKED u;
assert(sizeof(PARAM) <= sizeof(u.param_work))
}

///////////////////////////////////////////////////////////////

void _remove (PARAMID id)
{
PARAM *m_
	= (PARAM *)masked()->param_work;
assert(m_ && m_->params_map)
assert(id < m_->param_used)

assert(m_->params_map[id])
	if (!m_->params_map[id]) {
		return; // ideally unexpected condition 
	}

	_destroy (&m_->params_map[id]->hdr);
	m_->params_map[id] = NULL;
}

PARAMID _open ()
{
	_conflict_test_once (); // PENDING: Run in application initializer

PARAM *m_
	= (PARAM *)masked()->param_work;

	// append entry
PARAM_ENTRY entrying = {0}; u16 need = usizeof(PARAM_ENTRY);

	if (! (m_->param_used < m_->param_max || m_->param_max < PARAMID_MAX || m_->is_id_recycled))
		m_->is_id_recycled = TRUE;
unsigned id
	= _create ((_HEADER ***)&m_->params_map, &entrying.hdr, need, &m_->param_used, &m_->param_max, PARAMID_MAX);
assert(id < PARAMID_MAX)
	if (! (0 < id))
abort_ ("%s " VTRR "macro's param_id filled out limitation %d" VTO " [" VTCC "%s" VTO "]" "\n", whereis(), PARAMID_MAX, __func__);
	return id;
}

void _write (PARAMID id, const char *text, const TOKEN *tokens, unsigned tokens_len)
{
PARAM *m_
	= (PARAM *)masked()->param_work;
assert(0 < id && id < m_->param_used)
PARAM_ENTRY *entried
	= m_->params_map[id];
assert(entried)

unsigned data_begin = usizeof(PARAM_ENTRY);
	m_->params_map[id] = (PARAM_ENTRY *)_append_tokens (&entried->hdr, data_begin, text, tokens, tokens_len);
}

void _close (PARAMID mid)
{
}

///////////////////////////////////////////////////////////////

typedef struct {
	PARAMID param_id;
	TOKENS_CNT token_index;
} TOKENIZE_ITERATOR;

void _tokenize_reset (void *priv, unsigned len, const void *data)
{
assert(sizeof(TOKENIZE_ITERATOR) <= len)
TOKENIZE_ITERATOR *m_ = (TOKENIZE_ITERATOR *)priv;
	m_->param_id = *(const PARAMID *)data;
	m_->token_index = 0;
}

enum TYPE { NEXT = 0, PEEK };
static const TOKEN *_tokenize_peek_or_next (void *priv, const char **source, enum TYPE type)
{
TOKENIZE_ITERATOR *_it = (TOKENIZE_ITERATOR *)priv;

PARAM *m_
	= (PARAM *)masked()->param_work;
PARAMID id
	= _it->param_id;
PARAM_ENTRY *entried
	= m_->params_map[id];
assert(entried)

	if (! (usizeof(PARAM_ENTRY) < entried->hdr.size)) // NOTE: (*2)
abort_ ("%s " VTRR "missing macro parameter" VTO "." "\n", whereis());

const char *_text
	= (const char *)entried +usizeof(PARAM_ENTRY);
const TOKEN *_tokens
	= (const TOKEN *)(_text +strlen (_text) +1); // PENDING: (*3)

const TOKEN *tok
	= _tokens +_it->token_index;
	if (0 < tok->len) {
		if (NEXT == type)
			++_it->token_index;
		if (source)
			*source = _text;
		return tok;
	}
	return NULL;
}
const TOKEN *_tokenize_peek (void *priv, const char **source)
{ return _tokenize_peek_or_next (priv, source, PEEK); }
const TOKEN *_tokenize_next (void *priv, const char **source)
{ return _tokenize_peek_or_next (priv, source, NEXT); }

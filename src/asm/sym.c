/*
PENDING: (*1) scaling step by step through observing a running.
PENDING: (*2) Now simply linear seeking.
              If getting a wait over being able to put up with,
              replacing into sorted inserting and binary seeking,
              or hash seeking with trading off a coding time.
PENDING: (*3) return (s,n) not (s) for speed-up?
 */
#include "asm/sym.h"

#include "as-.h" // TOKEN FALSE TRUE
#include "helper/pow2_ge.h"
#include "helper/vt100.h"
#include "helper/assert.h"
#define   flow_bug assert
#define  input_bug assert
#define caller_bug assert
#define unexpected assert
#define unimpl_yet assert
#include "helper/m2s.h" // debug only TODO: remove

#include <stdbool.h>
#include <stddef.h> // offsetof
#include <string.h>
#include <memory.h>

typedef uint32_t u32;
typedef  uint8_t u8, u1;
typedef uint16_t u10, u12, u14;

#define ustrlen(x) (unsigned)strlen(x)
///////////////////////////////////////////////////////////////
// export

#define _reset                 sym_reset
#define _set_value             sym_set_value
#define _set_type              sym_set_type
#define _set_bind              sym_set_bind
#define _set_fn_len            sym_set_fn_len
#define _get_count             sym_get_count
#define _enum_first            sym_enum_first
#define _enum_next             sym_enum_next
#define _lookup                sym_lookup

#define TYPE_NUM    VALUE_TYPE_NUM
#define TYPE_OFFSET VALUE_TYPE_OFFSET

///////////////////////////////////////////////////////////////
// imports

///////////////////////////////////////////////////////////////
// constants

#define INITHEAP_STRPOOL_MAX 0x4000 // text store (16KB)
#define INITHEAP_SYMBOLS_MAX 128 // lines
#define SYMBOL_ID_ORIGIN 1 // 1st number of SYMBOL_ID

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

typedef struct {
	VALUE val;
#define STRTBL_POS_BITS  14 // PENDING: (*1)
	u32 pos         :14;
#define STR_LEN_BITS      8 // PENDING: (*1)
	u32 len         :8;
	u32 padding     :32 -(1 +14 +8);
} LOOKUP;

typedef u14 STRTBL_POS;

#define STRTBL_POS_MAX ((1 << STRTBL_POS_BITS) -1)
#define    STR_LEN_MAX ((1 <<    STR_LEN_BITS) -1)

typedef struct {
	// lookup table
	LOOKUP *begin;
	unsigned cnt;
	unsigned reserved;
	// string pool
	char *cstr_pool;
	unsigned cstr_used;
	unsigned cstr_max;
	// for _enum_XXX() only
	unsigned enum_i;
	unsigned enum_name_max;
	char *enum_name;
	unsigned *sorted;
	unsigned sorted_reserved;
} SINGLTON;
static SINGLTON s_singlton = {0};
static SINGLTON *sym() { return &s_singlton; }

///////////////////////////////////////////////////////////////

unsigned _get_count () { return sym()->cnt; }

void _reset ()
{
unsigned ge;
	// string pool (ensure memory 2^N base)
	ge = pow2_ge(INITHEAP_STRPOOL_MAX);
	if (NULL == sym()->cstr_pool) {
		sym()->cstr_pool = (char *)malloc (ge);
		sym()->cstr_max = ge;
	}
	else if (sym()->cstr_max < ge) {
		sym()->cstr_pool = (char *)realloc (sym()->cstr_pool, ge);
		sym()->cstr_max = ge;
	}
	sym()->cstr_used = 0;

	// lookup table (ensure memory 2^N base)
	ge = pow2_ge(INITHEAP_SYMBOLS_MAX);
	if (NULL == sym()->begin) {
		sym()->begin = (LOOKUP *)malloc (sizeof(LOOKUP) * ge);
		sym()->reserved = ge;
	}
	else if (sym()->reserved < ge) {
		sym()->begin = (LOOKUP *)realloc (sym()->begin,
		                                  sizeof(LOOKUP) * ge);
		sym()->reserved = ge;
	}
	sym()->cnt = 0;
}

static LOOKUP *_lookup0 (const char *s, unsigned n)
{
LOOKUP *p = sym()->begin, *end = p +sym()->cnt;
	if (NULL == p)
		return NULL; // nothing registered

	// lookup PENDING: (*2)
	for (; p < end; ++p) {
		if (! (n == p->len))
			continue;
		if (! (0 == memcmp (s, sym()->cstr_pool +p->pos, p->len)))
			continue;
		break;
	}
	if (! (p < end))
		return NULL; // not found

	return p; // found
}

static LOOKUP *_new_entry (const char *s, unsigned n)
{
unsigned ge;
	// string pool (ensure memory 2^N base)
	ge = pow2_ge(sym()->cstr_used +n +1);
	if (NULL == sym()->cstr_pool) {
		sym()->cstr_pool = (char *)malloc (ge);
		sym()->cstr_max = ge;
	}
	else if (sym()->cstr_max < ge) {
		sym()->cstr_pool = (char *)realloc (sym()->cstr_pool, ge);
		sym()->cstr_max = ge;
	}

	// string pool (write)
STRTBL_POS pos = sym()->cstr_used;
	memcpy (sym()->cstr_pool +pos, s, n);
	sym()->cstr_pool[pos +n] = '\0';

	sym()->cstr_used += n +1;

	// lookup table (ensure memory 2^N base)
	ge = pow2_ge(sym()->cnt +1);
	if (NULL == sym()->begin) {
		sym()->begin = (LOOKUP *)malloc (sizeof(LOOKUP) * ge);
		sym()->reserved = ge;
	}
	else if (sym()->reserved < ge) {
		sym()->begin = (LOOKUP *)realloc (sym()->begin,
		                                  sizeof(LOOKUP) * ge);
		sym()->reserved = ge;
	}

	// lookup table (write)
LOOKUP *p
	= &sym()->begin[sym()->cnt];
	p->pos  = pos;
	p->len  = n;
	memset (&p->val, 0, sizeof(p->val));

	++sym()->cnt;

	return p;
}

static LOOKUP *_ensure_entry (const char *s, unsigned n)
{
flow_bug(sym()->begin) // _reset() not called
caller_bug(s)
	n = (0 < n) ? n : ustrlen (s);
caller_bug(0 < n)
caller_bug(n <= STR_LEN_MAX)
unimpl_yet(sym()->cstr_used +n +1 <= STRTBL_POS_MAX)

LOOKUP *p;
	if (NULL == (p = _lookup0 (s, n)))
		p = _new_entry (s, n);

	return p;
}

static LOOKUP *_set_value0 (const char *s, unsigned n, const VALUE *src)
{
LOOKUP *p
	= _ensure_entry (s, n);

	if (TYPE_OFFSET == src->type)
		p->val.addr_id = src->addr_id;
	else /*if (TYPE_NUM == src->type)*/
		p->val.number = src->number;

	p->val.type = src->type;

	return p;
}
void _set_value (const char *s, unsigned n, const VALUE *src)
{ _set_value0 (s, n, src); }

// @retval 1 origin
SYMBOL_ID _set_type (const char *s, unsigned n, u8 type)
{
caller_bug(0 == (~SYM_TYPE_MASK & type))
LOOKUP *p
	= _ensure_entry (s, n);
	p->val.attr &= ~SYM_TYPE_MASK;
	p->val.attr |= SYM_TYPE_MASK & type;

unexpected(p +SYMBOL_ID_ORIGIN < sym()->begin +(1 << SYMBOL_ID_BITS))
SYMBOL_ID sym_id
	= (SYMBOL_ID)(p -sym()->begin +SYMBOL_ID_ORIGIN);
	return sym_id;
}

void _set_bind (const char *s, unsigned n, u8 bind)
{
caller_bug(0 == (~SYM_BIND_MASK & bind))
LOOKUP *p
	= _ensure_entry (s, n);
	p->val.attr &= ~SYM_BIND_MASK;
	p->val.attr |= SYM_BIND_MASK & bind;
}

// @param id 1 origin
void _set_fn_len (SYMBOL_ID id, u32 fn_len)
{
caller_bug(0 < id)
caller_bug(id -SYMBOL_ID_ORIGIN < sym()->cnt)
flow_bug(sym()->begin)

LOOKUP *p
	= sym()->begin +(id -SYMBOL_ID_ORIGIN);
	p->val.fn_len = fn_len;
}

/*
.Lerrno         [rsi]    (.Lerrno           = 0x20)
.Lerraux        [rsi][2] (.Lerraux          = 0x21)
.Lreg_b         [rsi]    (.Lreg_b           = 0x07)
.Llpfn_pre_lea_r[rsi]    (.Llpfn_pre_lea_r  = 0x48)
.Litem_ptr[rdi+iMEM*sizeofITEM] (.Litem_ptr = 0x00
                                 iMEM       = 1
                                 sizeofITEM = 16
 */
// @retval 1 origin
SYMBOL_ID _lookup (const char *s, unsigned n, VALUE *_val)
{
caller_bug(s)
	n = (0 < n) ? n : ustrlen (s);
	if (STR_LEN_MAX < n)
		return 0; // not found

LOOKUP *p;
	if (NULL == (p = _lookup0 (s, n)))
		return 0; // not found

	if (_val)
		*_val = p->val;
unexpected(p +SYMBOL_ID_ORIGIN < sym()->begin +(1 << SYMBOL_ID_BITS))
SYMBOL_ID sym_id
	= (SYMBOL_ID)(p -sym()->begin +SYMBOL_ID_ORIGIN);
	return sym_id; // found
}

// @param id 1 origin
const char *
//void
 sym_id2val (SYMBOL_ID id, VALUE *_val)
{
caller_bug(0 < id)
caller_bug(id -SYMBOL_ID_ORIGIN < sym()->cnt)
flow_bug(sym()->begin)

const LOOKUP *p
	= sym()->begin +(id -SYMBOL_ID_ORIGIN);
	if (_val)
		*_val = p->val;
	return &sym()->cstr_pool[p->pos];
}

const char *_enum_next (VALUE *_val)
{
	if (! (sym()->enum_i < sym()->cnt))
		return NULL;
LOOKUP *p
	= sym()->begin +sym()->sorted[sym()->enum_i];

	// memory ensuring (2^N base) PENDING: (*3)
unsigned ge
	= pow2_ge(p->len +1);
	if (NULL == sym()->enum_name) {
		sym()->enum_name = (char *)malloc (ge);
		sym()->enum_name_max = ge;
	}
	else if (sym()->enum_name_max < ge) {
		sym()->enum_name = (char *)realloc (sym()->enum_name, ge);
		sym()->enum_name_max = ge;
	}
	// PENDING: (*3)
	memcpy (sym()->enum_name, sym()->cstr_pool +p->pos, p->len);
	sym()->enum_name[p->len] = '\0';

	++sym()->enum_i;
	if (_val)
		*_val = p->val;
	return sym()->enum_name;
}
const char *_enum_first (VALUE *_val)
{
	if (0 < sym()->cnt) {
		// memory ensuring (2^N base)
unsigned ge
		= pow2_ge(sym()->cnt);
		if (NULL == sym()->sorted) {
			sym()->sorted = (unsigned *)malloc (sizeof(unsigned) * ge);
			sym()->sorted_reserved = ge;
		}
		else if (sym()->sorted_reserved < ge) {
			sym()->sorted = (unsigned *)realloc (sym()->sorted,
			                                     sizeof(unsigned) * ge);
			sym()->sorted_reserved = ge;
		}

		// sorting: (not global) -> (global)
unsigned dst = 0, i;
		for (i = 0; i < sym()->cnt; ++i) {
LOOKUP *p
			= &sym()->begin[i];
			if (! (128 == i)) // TODO: stop fake
				sym()->sorted[dst++] = i;
		}
		for (i = 0; i < sym()->cnt; ++i) {
LOOKUP *p
			= &sym()->begin[i];
			if (128 == i) // TODO: stop fake
				sym()->sorted[dst++] = i;
		}
	}
	sym()->enum_i = 0;
	return _enum_next (_val);
}

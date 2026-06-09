#include "asm/fragments.h"

#include "helper/pow2_ge.h"
#define ASSERT_PREFIX_FUNC asm_whereis()
#include "helper/assert.h"
#define caller_bug assert
#define   flow_bug assert
#define  input_bug assert
#define unexpected assert

#include <stdbool.h>
#include <string.h>
#include <memory.h>
#include <stddef.h> // offsetof

typedef uint32_t u32;
typedef  uint8_t u8, u1, u2, u3;
typedef uint64_t u64;

#define usizeof(x) (unsigned)sizeof(x)
#define uoffsetof(x, m) (unsigned)offsetof(x, m)
///////////////////////////////////////////////////////////////
// exports

#define _reset                  fragments_reset
//#define _ensure_appending       fragments_ensure_appending
#define _end_appending          fragments_end_appending
#define _begin_appending        fragments_begin_appending
#define _push_code              fragments_push_code
#define _push_rel8_capable_jmp  fragments_push_rel8_capable_jmp
#define _get_appending          fragments_get_appending
#define _push_end               fragments_push_end
#define _enum_first             fragments_enum_first
#define _enum_next              fragments_enum_next
#define _get                    fragments_get
#define _set_ofs_type           fragments_set_ofs_type
#define _set_seg_id             fragments_set_seg_id
#define _set_seg_addr           fragments_set_seg_addr
#define _has_rel8_capable_jmp   fragments_has_rel8_capable_jmp
#define _get_tokens             fragments_get_tokens
#define _write_rel8_capable_jmp fragments_write_rel8_capable_jmp

//#define REL32    TYPE_REL32
#define REL8     TYPE_REL8
//#define REL64    TYPE_REL64
//#define REL8or32 TYPE_REL8or32

#define UNSET SEGMENT_POS_UNSET

#define ID_ORIGIN FRAGMENT_ID_ORIGIN

///////////////////////////////////////////////////////////////
// imports

///////////////////////////////////////////////////////////////
// constants

#define INITHEAP_FRAGMENTS_MAX 128
static const TOKEN EOT = { .pos = 0, .len = 0 };

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

typedef struct {
	FRAGMENT **pieces;
	unsigned pieces_len;
	unsigned pieces_max;
	// for _enum_XXX() only
	unsigned enum_i;
	SEGMENT_ID enum_seg_id;
	// for automatically changing fragment
	SEGMENT_ID last_seg_id;
} SINGLTON;
static SINGLTON s_singlton = {0};
static SINGLTON *my() { return &s_singlton; }

static FRAGMENT **_back ()
{
flow_bug(my()->pieces)
flow_bug(0 < my()->pieces_len)
	return &my()->pieces[my()->pieces_len -1];
}

static const FRAGMENT *_get_appending0 ()
{ return *_back(); }

FRAGMENT_ID _get_appending ()
{
flow_bug(my()->pieces)
flow_bug(0 < my()->pieces_len)

FRAGMENT_ID frag_id
	= my()->pieces_len -1 +ID_ORIGIN;
	if (NULL == my()->pieces[frag_id -ID_ORIGIN])
		return INVALID_FRAGMENT_ID;
	return frag_id;
}

///////////////////////////////////////////////////////////////
// utility (-> ias.c?)

const char *_get_tokens (const TOKEN *t1, unsigned *_len)
{
const TOKEN *t = t1;
	// getting the end of tokens
	while (! (EOT.pos == t->pos && EOT.len == t->len))
		++t;
	// text (refered by tokens)
const char *_source
	= (t1 < t) ? (const char *)(t +1) : NULL;
	// length of tokens
	if (_len)
		*_len = (unsigned)(t -t1);
	return _source;
}

///////////////////////////////////////////////////////////////
// interface (reading only)

const FRAGMENT *_get (FRAGMENT_ID frag_id)
{
caller_bug(frag_id < my()->pieces_len)
flow_bug(my()->pieces)
caller_bug(my()->pieces[frag_id])

	if (! (frag_id < my()->pieces_len))
		return NULL;
FRAGMENT *frag
	= my()->pieces[frag_id];
	return frag;
}

const FRAGMENT *_enum_next (FRAGMENT_ID *_frag_id)
{
	while (my()->enum_i < my()->pieces_len) {
FRAGMENT *next;
		if (NULL == (next = my()->pieces[my()->enum_i]))
			break;
		++my()->enum_i;
		if (ALLSEG == my()->enum_seg_id
		 || my()->enum_seg_id == next->seg_id) {
			if (_frag_id)
				*_frag_id = my()->enum_i -1;
			return next;
		}
	}
	return NULL;
}
const FRAGMENT *_enum_first (SEGMENT_ID seg_id, FRAGMENT_ID *_frag_id)
{
	my()->enum_seg_id = seg_id;
	my()->enum_i = 0;
	return _enum_next (_frag_id);
}

static bool _has_rel8_capable_jmp0 (const FRAGMENT *frag)
{
	if (0 == frag->jmp8_len || 0 == frag->jmp32_len) {
unexpected(0 == frag->src_lineno)
		return false;
	}
	return true;
}
bool _has_rel8_capable_jmp (FRAGMENT_ID frag_id)
{ return _has_rel8_capable_jmp0 (_get(frag_id)); }

/* @retval > 0  written bytes
           = 0  error (not enough memory) */
unsigned _write_rel8_capable_jmp (FRAGMENT_ID frag_id,
                                  u8 *obj,
                                  unsigned obj_max)
{
const FRAGMENT *frag
	= _get(frag_id);
flow_bug(_has_rel8_capable_jmp0 (frag))

unsigned obj_len
	= (REL8 == frag->ofs_type) ? frag->jmp8_len : frag->jmp32_len;
	if (obj_max < obj_len)
		return 0; // not enough memory

	if (obj)
		if (REL8 == frag->ofs_type) {
			if (1 < obj_len)
				memcpy (obj, &frag->jmp8_opcode, obj_len -1);
			*(obj +obj_len -1) = 0x00;
		}
		else {
			if (4 < obj_len)
				memcpy (obj, &frag->jmp32_opcode, obj_len -4);
			memset (obj +obj_len -4, 0x00, 4);
		}
	return obj_len;
}

///////////////////////////////////////////////////////////////
// interface (with writing)

void _reset ()
{
	// reset fragments
	my()->pieces_len = 0;

	// ensure memory (2^N base)
flow_bug(NULL == my()->pieces)
unsigned ge
	= pow2_ge(INITHEAP_FRAGMENTS_MAX);
	my()->pieces = (FRAGMENT **)malloc (sizeof(FRAGMENT *) * ge);
	my()->pieces_max = ge;

	// new fragment (not opened)
	my()->pieces[my()->pieces_len++] = NULL;
}

void _end_appending ()
{
flow_bug(0 < my()->pieces_len)
FRAGMENT_ID old_frag_id
	= my()->pieces_len -1;

flow_bug(my()->pieces && my()->pieces[old_frag_id])
FRAGMENT *old_frag
	= my()->pieces[old_frag_id];

	// new fragment (ensure memory 2^N base)
unsigned ge
	= pow2_ge(my()->pieces_len +1);
flow_bug(my()->pieces)
	if (my()->pieces_max < ge) {
		my()->pieces = (FRAGMENT **)realloc (my()->pieces,
		                                     sizeof(FRAGMENT *) * ge);
		my()->pieces_max = ge;
	}
	// new fragment (not opened)
	my()->pieces[my()->pieces_len] = NULL;
	// commit last fragment
	++my()->pieces_len;
}

FRAGMENT_ID _begin_appending ()
{
flow_bug(0 < my()->pieces_len)
flow_bug(my()->pieces)
flow_bug(NULL == my()->pieces[my()->pieces_len -1])

	// initialize data
unsigned required
	= uoffsetof(FRAGMENT, to.expr)
	+ usizeof(TOKEN) * 1 // 1=EOT
	;
FRAGMENT *init
	= (FRAGMENT *)malloc (required);
	memset (init, 0, required);
	init->reserved = required;
	init->seg_addr = UNSET;
	*init->to.expr = EOT;

FRAGMENT_ID new_frag_id
	= my()->pieces_len -1;
	my()->pieces[new_frag_id] = init;

	return new_frag_id;
}

static void _push_obj (FRAGMENT *apd,
                       const void *obj, unsigned len)
{
const unsigned LEN_MAX
	= (1 << FRAGMENT_OBJLEN_BITS) -1;
	if (! (apd->obj_len +len <= LEN_MAX))
abort_(VTRR "internal error" VTO ": too large fragments (> %d bytes)" "\n", LEN_MAX);

	// ensuring memory (2^N base)
unsigned ge
	= pow2_ge(apd->obj_len +len);
	if (NULL == apd->obj)
		apd->obj = (u8 *)malloc (ge);
	else if (pow2_ge(apd->obj_len) < ge)
		apd->obj = (u8 *)realloc (apd->obj, ge);
	// set data
	memcpy (apd->obj +apd->obj_len, obj, len);
	// commit
	apd->obj_len += len;
}

void _push_code (FRAGMENT_ID frag_id,
                 u64 packed_code, unsigned len)
{
caller_bug(0 < len)
FRAGMENT *apd
	= (FRAGMENT *)_get (frag_id);
caller_bug(apd)
flow_bug(false == _has_rel8_capable_jmp0 (apd))

	_push_obj (apd, &packed_code, len);
}

void _push_rel8_capable_jmp (FRAGMENT_ID frag_id,
                             const TOKEN *t1, unsigned n1,
                             const char *_source,
                             REL_TYPE ofs_type,
                             u32 jmp8_opcode, u3 jmp8_opcode_len,
                             u32 jmp32_opcode, u3 jmp32_opcode_len,
                             unsigned src_lineno)
{
caller_bug(jmp8_opcode_len +1 < (1 << 3))
caller_bug(jmp32_opcode_len +4 < (1 << 3))
	// is first writing into top fragment?
	// ensure memory
unsigned required
	= uoffsetof(FRAGMENT, to.expr)
	+ usizeof(TOKEN) * (n1 +1)
	+ strlen (_source) +1
	;
FRAGMENT *apd
	= (FRAGMENT *)_get (frag_id);
caller_bug(apd)
	if (apd->reserved < required) {
		apd = (FRAGMENT *)realloc (apd, required);
		apd->reserved = required;
		*_back() = apd;
	}
	// tokens
	if (0 < n1)
		memcpy (apd->to.expr, t1, sizeof(TOKEN) * n1);
	// end of tokens
	memcpy (&apd->to.expr[n1], &EOT, sizeof(TOKEN));
	// text (refered from tokens)
	strcpy ((char *)&apd->to.expr[n1 +1], _source);
	// source line number
	apd->src_lineno = src_lineno;
	// ambiguous length
	apd->ofs_type     = ofs_type;
	apd->jmp8_opcode  = jmp8_opcode;
	apd->jmp8_len     = jmp8_opcode_len +1;
	apd->jmp32_opcode = jmp32_opcode;
	apd->jmp32_len    = jmp32_opcode_len +4;
}

void _push_end ()
{
	if (NULL == _get_appending0 ())
		return;
	_end_appending ();
}

static void _register (SEGMENT_ID seg_id, const u8 *obj, unsigned obj_len,
                       const TOKEN *t1, unsigned n1, const char *_source,
                       u3 ofs_type,
                       u32 jmp8_opcode, u3 jmp8_len,
                       u32 jmp32_opcode, u3 jmp32_len,
                       unsigned src_lineno)
{
flow_bug(NULL == _get_appending0 ())
flow_bug(0 < obj_len || 0 < n1)
FRAGMENT_ID new_frag_id
	= _begin_appending ();
FRAGMENT *apd
	= my()->pieces[new_frag_id];
	apd->seg_id = seg_id;
	if (0 < obj_len)
		_push_obj (apd, obj, obj_len);
	if (0 < n1)
		_push_rel8_capable_jmp (new_frag_id, t1, n1, _source
		, ofs_type
		, jmp8_opcode, jmp8_len -1
		, jmp32_opcode, jmp32_len -4
		, src_lineno);

	_end_appending ();
}

void _set_ofs_type (FRAGMENT_ID frag_id, REL_TYPE ofs_type)
{
caller_bug(frag_id < my()->pieces_len)
flow_bug(my()->pieces)
caller_bug(my()->pieces[frag_id])

FRAGMENT *frag
	= my()->pieces[frag_id];
	frag->ofs_type = ofs_type;
}

void _set_seg_id (FRAGMENT_ID frag_id, SEGMENT_ID seg_id)
{
caller_bug(frag_id < my()->pieces_len)
flow_bug(my()->pieces)
caller_bug(my()->pieces[frag_id])

FRAGMENT *frag
	= my()->pieces[frag_id];
	frag->seg_id = seg_id;
}

void _set_seg_addr (FRAGMENT_ID frag_id, SEGMENT_POS seg_addr)
{
caller_bug(frag_id < my()->pieces_len)
flow_bug(my()->pieces)
caller_bug(my()->pieces[frag_id])

FRAGMENT *frag
	= my()->pieces[frag_id];
	frag->seg_addr = seg_addr;
}

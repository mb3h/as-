/*
NOTE: (*1) When a label gives jump offset into zero,
           the label is always placed in a next fragment.
NOTE: (*2) sym_id about '.symtab' is 1 origin
NOTE: (*3) _resolve_cfi() must be called after _resolve_fn_len().
           because '.size' must be associated with sym_id of syms
           by sym_set_fn_len().
 */

#define _POSIX_C_SOURCE 200809L /* POSIX.1-2008 strdup vdprintf
#define _POSIX_C_SOURCE 200112L    POSIX.1-2001 strtoul
#define _POSIX_C_SOURCE 199309L    POSIX.1b-1993 nanosleep */
//#define _FILE_OFFSET_BITS 64
#include <features.h> // __BYTE_ORDER
//#include <endian.h>

#include "as-.h"
#include "asm/addrs.h" // ADDR_ID
#include "asm/sym.h"
#include "asm/calc.h"
#include "asm/reftbl.h"
#include "asm/fragments.h"
#include "asm/procs.h"
#include "helper/pow2_ge.h"
#define ASSERT_PREFIX_FUNC asm_whereis()
#include "helper/assert.h"
#define   caller_bug assert
#define   coding_bug assert
#define     flow_bug assert
#define    input_bug assert
#define overflow_bug assert
#define   unimpl_yet assert
#define     spec_bug assert
#include "helper/m2s.h" // debug only TODO: remove
#include "helper/t2s.h" // debug only TODO: remove

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>

typedef uint32_t u32;
typedef  uint8_t u8, u1, u2, u3, u4;
typedef uint16_t u16, u12;
typedef uint64_t u64;
typedef  int32_t s32;
typedef   int8_t s2;
typedef  int16_t s16;

#ifndef min
# define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#define arraycountof(x) (sizeof(x)/sizeof(x[0]))
#define usizeof(x) (unsigned)sizeof(x)
#define ustrlen(x) (unsigned)strlen(x)
#define PADDING(a,x) ((a) -1 - ((x) +(a) -1)%(a)) // = ALIGN(a,x) - (x)
///////////////////////////////////////////////////////////////
// constants

#define VERSION_STRING "AS-: 0.0.0"

enum {
	IDS_UNKNOWN           = 0,
	IDS_TEXT              = 1,
	IDS_RELA_TEXT         = 2,
	IDS_RODATA            = 3,
	IDS_RELA_RODATA       = 4,
	IDS_COMMENT           = 5,
	IDS_NOTE_GNU_PROPERTY = 6,
	IDS_EH_FRAME          = 7,
	IDS_RELA_EH_FRAME     = 8,
	IDS_SYMTAB            = 9,
	IDS_STRTAB            = 10,

	IDS_MAX               = 10,
};
typedef u4 IDS;

static const char *str_from_ids(unsigned id)
{
	switch (id) {
	case IDS_TEXT             : return ".text"             ;
	case IDS_RELA_TEXT        : return ".rela.text"        ;
	case IDS_RODATA           : return ".rodata"           ;
	case IDS_RELA_RODATA      : return ".rela.rodata"      ;
	case IDS_COMMENT          : return ".comment"          ;
	case IDS_NOTE_GNU_PROPERTY: return ".note.gnu.property";
	case IDS_EH_FRAME         : return ".eh_frame"         ;
	case IDS_RELA_EH_FRAME    : return ".rela.eh_frame"    ;
	case IDS_SYMTAB           : return ".symtab"           ;
	case IDS_STRTAB           : return ".strtab"           ;
	default: return "";
	}
}

extern unsigned idstr_from(const char *s, unsigned n)
{
	n = (0 < n) ? n : ustrlen (s);
	if ( 5 == n && 0 == memcmp (".text"             , s, n)) return IDS_TEXT             ;
	if (10 == n && 0 == memcmp (".rela.text"        , s, n)) return IDS_RELA_TEXT        ;
	if ( 7 == n && 0 == memcmp (".rodata"           , s, n)) return IDS_RODATA           ;
	if (12 == n && 0 == memcmp (".rela.rodata"      , s, n)) return IDS_RELA_RODATA      ;
	if ( 8 == n && 0 == memcmp (".comment"          , s, n)) return IDS_COMMENT          ;
	if (18 == n && 0 == memcmp (".note.gnu.property", s, n)) return IDS_NOTE_GNU_PROPERTY;
	if ( 9 == n && 0 == memcmp (".eh_frame"         , s, n)) return IDS_EH_FRAME         ;
	if (14 == n && 0 == memcmp (".rela.eh_frame"    , s, n)) return IDS_RELA_EH_FRAME    ;
	if ( 7 == n && 0 == memcmp (".symtab"           , s, n)) return IDS_SYMTAB           ;
	if ( 7 == n && 0 == memcmp (".strtab"           , s, n)) return IDS_STRTAB           ;
	return IDS_UNKNOWN;
}

///////////////////////////////////////////////////////////////
// import

#define _tok2id    calc_tok2id
#define _expr2n    calc_expr2n
#define _opr2n     calc_opr2n
#define _opr2seg   calc_opr2seg
#define _sym2n     sym_lookup
#define TYPE_NUM    VALUE_TYPE_NUM
#define TYPE_OFFSET VALUE_TYPE_OFFSET

// assemble: pass1
#define _reset_fragments        fragments_reset
#define _push_code0             fragments_push_code
#define _push_rel8_capable_jmp0 fragments_push_rel8_capable_jmp
#define _get_appending          fragments_get_appending
#define _end_appending          fragments_end_appending
#define _begin_appending        fragments_begin_appending
#define _push_end               fragments_push_end
#define _reset_reftbl           reftbl_reset
#define _append_reftbl          reftbl_append
#define _reset_sym              sym_reset
#define _push_cfi0              procs_push_cfi
// assemble: pass2
#define _enum_first             fragments_enum_first
#define _enum_next              fragments_enum_next
#define _get                    fragments_get
#define _get_tokens             fragments_get_tokens
#define _write_rel8_capable_jmp fragments_write_rel8_capable_jmp
#define _enum_first_reftbl      reftbl_enum_first
#define _enum_next_reftbl       reftbl_enum_next

#define ONE      FRAGMENTS_ONE
#define TWO      FRAGMENTS_TWO
#define THREE    FRAGMENTS_THREE
#define FOUR     FRAGMENTS_FOUR

#define REL32    TYPE_REL32
#define REL8     TYPE_REL8
#define REL64    TYPE_REL64
#define REL8or32 TYPE_REL8or32
#define FN_LEN   TYPE_FN_LEN

#define UNSET    SEGMENT_POS_UNSET

///////////////////////////////////////////////////////////////
// portab.c

static void store_le32 (void *dst_, u32 val)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	*(u32 *)dst_ = val;
#else // __BIG_ENDIAN
u8 *p = (u8 *)dst_;
	*p++ = (u8)val;
	*p++ = (u8)(0xFF & val >> 8);
	*p++ = (u8)(0xFF & val >> 16);
	*p++ = (u8)(0xFF & val >> 24);
#endif
}

static void store_le16 (void *dst_, u16 val)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	*(u16 *)dst_ = val;
#else // __BIG_ENDIAN
u8 *p = (u8 *)dst_;
	*p++ = (u8)val;
	*p++ = (u8)(0xFF & val >> 8);
#endif
}

static void store_le64 (void *dst_, u64 val)
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
	*(u64 *)dst_ = val;
#else // __BIG_ENDIAN
u8 *p = (u8 *)dst_;
	*p++ = (u8)val;
	*p++ = (u8)(0xFF & val >> 8);
	*p++ = (u8)(0xFF & val >> 16);
	*p++ = (u8)(0xFF & val >> 24);
	*p++ = (u8)(0xFF & val >> 32);
	*p++ = (u8)(0xFF & val >> 40);
	*p++ = (u8)(0xFF & val >> 48);
	*p++ = (u8)(0xFF & val >> 56);
#endif
}

///////////////////////////////////////////////////////////////
// base utility (TODO: -> ias.c?)

static TOKEN *_tokchr (const TOKEN *begin, size_t n, int c, const char *_source)
{
const TOKEN *t = begin, *end = begin +n;
	for (; t < end; ++t)
		if (1 == t->len && c == _source[t->pos])
			break;
	if (! (t < end))
		return NULL;
	return /*const_cast*/(TOKEN *)t;
}

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

// .state
#define INITIAL 0

typedef struct {
	// _assemble
	u8 *cod;
	unsigned cod_max;
	//
	u32 state :3 ;
	u32 pad   :29;
	unsigned line;
} STREAMING;
static STREAMING s_streaming = {
	0
};
static STREAMING *my() { return &s_streaming; }
#define heap    my
#define parsing my
#define parsed  my

const char *asm_whereis()
{
static char s_whereis[12+1 +3]; // 12='(asm):ddddd:'
	sprintf (s_whereis, "(asm):%d:", min(99999, my()->line));
	return s_whereis;
}

// pass1
typedef struct {
	u32 struct_addr;
	SEGMENT_ID seg_id;
	PROC_ID proc_id;
	ADDR *addrs;
	unsigned addrs_len;
} PASS1;
static PASS1 s_pass1 = {0};
static PASS1 *pass1() { return &s_pass1; }

// wrapper for readability
static FRAGMENT_ID _ensure_appending ()
{
SEGMENT_ID seg_id
	= pass1()->seg_id;
FRAGMENT_ID frag_id
	= _get_appending();
const FRAGMENT *frag = NULL;
	// now appending same segment?
	if (INVALID_FRAGMENT_ID != frag_id) {
		frag = _get(frag_id);
		if (seg_id == frag->seg_id)
			return frag_id;
	}

	// close current fragment if needed
	if (frag && 0 < frag->obj_len) {
		_end_appending ();
		frag = NULL;
	}

	// create new fragment if needed
	if (! (frag && 0 == frag->obj_len)) {
flow_bug(NULL == frag)
		frag_id = _begin_appending ();
		frag = _get(frag_id);
	}

	// modify segment if nothing in a fragment
	if (! (seg_id == frag->seg_id)) {
flow_bug(0 == frag->obj_len)
		fragments_set_seg_id (frag_id, seg_id);
	}
flow_bug(false == fragments_has_rel8_capable_jmp (frag_id)) // safety

	return frag_id;
}
static void _push_code (u64 packed_code, unsigned len)
{
FRAGMENT_ID frag_id
	= _ensure_appending ();
//flow_bug(false == fragments_has_rel8_capable_jmp (frag_id))

   _push_code0 (frag_id, packed_code, len);
}
static void _push_rel8_capable_jmp (const TOKEN *t1, unsigned n, const char *_source,
                                    REL_TYPE ofs_type,
                                    uint32_t jmp8_opcode, /*u3*/uint8_t jmp8_opcode_len,
                                    uint32_t jmp32_opcode, /*u3*/uint8_t jmp32_opcode_len,
                                    unsigned src_lineno)
{
FRAGMENT_ID frag_id
	= _ensure_appending ();

	_push_rel8_capable_jmp0 (frag_id, t1, n, _source,
	                         ofs_type,
	                         jmp8_opcode, jmp8_opcode_len,
	                         jmp32_opcode, jmp32_opcode_len,
	                         src_lineno);
}
static void _push_cfi (u16 codes, u2 len)
{
input_bug(0 < pass1()->proc_id)

FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());
	_push_cfi0 (pass1()->proc_id,
	            frag_id,
	            frag->obj_len,
	            codes, len);
}

///////////////////////////////////////////////////////////////
// addrs.o

#define ADDR_ID_ORIGIN 1

const ADDR *addrs_get (ADDR_ID addr_id)
{
flow_bug(pass1()->addrs)
caller_bug(ADDR_ID_ORIGIN <= addr_id)
caller_bug(addr_id < pass1()->addrs_len)
const ADDR *ret
	= &pass1()->addrs[addr_id];
	return ret;
}

ADDR_ID addrs_append (FRAGMENT_ID frag_id, FRAGMENT_POS frag_ofs)
{
ADDR_ID addr_id
	= max(ADDR_ID_ORIGIN, pass1()->addrs_len);

	// variable length
unsigned m
	= pow2_ge(addr_id +1);
	if (pow2_ge(pass1()->addrs_len) < m)
		pass1()->addrs = (ADDR *)realloc (pass1()->addrs, m * sizeof(ADDR));
	pass1()->addrs_len = addr_id +1;

	// new item
ADDR *a
	= &pass1()->addrs[addr_id];
	memset (a, 0, sizeof(ADDR));
	a->frag_id  = frag_id;
	a->frag_ofs = frag_ofs;
	return addr_id;
}

static void addrs_resolve_all ()
{
flow_bug(pass1()->addrs)
ADDR_ID i;
	for (i = ADDR_ID_ORIGIN; i < pass1()->addrs_len; ++i) {
ADDR *a
		= &pass1()->addrs[i];
flow_bug(FALSE == a->is_resolved)

const FRAGMENT *f
		= _get(a->frag_id);
flow_bug(f && !(UNSET == f->seg_addr))
		a->seg_ofs = f->seg_addr +a->frag_ofs;
		a->seg_id  = f->seg_id;
		a->is_resolved = TRUE;
	}
}

///////////////////////////////////////////////////////////////
// nop

//  73e:          90  -  nop
static void _nop (const TOKEN * /*t1*/, unsigned n, const char * /*_source*/)
{
input_bug(0 == n)
	_push_code (ONE(0x90));
}

///////////////////////////////////////////////////////////////
// mov movzx lea xchg

//   c1:    4c 8b 4e 40           -  mov r9,qword ptr .Lioctl[rsi]   (.Lioctl = 0x40)
//  171:    49 8b 7c d1 08        -  mov rdi,qword ptr .Lcb_infos[r9+rdx*sizeofCBINFOdiv2][.Lcb_this] (.Lcb_infos = 0 = 0x0000, sizeofCBINFOdiv2 = 8, .Lcb_this = 8)
//  1b3:    4d 8b 0c d1           -  mov r9,qword ptr .Lcb_infos[r9+rdx*sizeofCBINFOdiv2][.Lcb_lpfn] (sizeofCBINFOdiv2 = 8, .Lcb_lpfn = 0)
// <r64> , qword ptr ___
//  3e6:    48 8b 4f 20           -  mov rcx,qword ptr .Litem_ptr[rdi+iIO*sizeofITEM] (= 0x20)
// 39c5:    48 8b 3c 24           -  mov rdi,qword ptr [rsp][0]
static void _mov_r64_from_ptr (OPR_CTRL *dst, OPR_CTRL *src, u32 offset)
{
	if (RAXtoRDI == dst->reg_type) {
		// <r64> , qword ptr ... [ <r64>[+<imm>] ]
		if (RAXtoRDI == src->reg_type) {
unimpl_yet(!src->is_scaling)
			if (0 == offset) {
				_push_code (FOUR(0x48, 0x8b
				, 0000|dst->reg<<3|src->reg
				, 0044 // PENDING: ???
				));
				return;
			}
			else {
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
				_push_code (FOUR(0x48, 0x8b
				, 0100|dst->reg<<3|src->reg
				, (u8)offset
				));
				return;
			}
		}
		// <r64> , qword ptr ... [ <r64(r8..r15)>+<r64>*<scale>[+<imm>] ]
		if (R8toR15 == src->reg_type) {
unimpl_yet(src->is_scaling)
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
			_push_code (TWO  (0x49, 0x8b));
			_push_code (THREE(
			  0104|dst->reg<<3
			, src->scale_lv<<6|src->scale_reg<<3|src->reg
			, (u8)offset
			));
			return;
		}
unimpl_yet(0 == 3)
	}
	if (R8toR15 == dst->reg_type) {
		if (RAXtoRDI == src->reg_type) {
unimpl_yet(!src->is_scaling)
			_push_code (FOUR (0x4c, 0x8b
			, 0100|dst->reg<<3|src->reg
			, (u8)offset
			));
			return;
		}
		if (R8toR15 == src->reg_type) {
unimpl_yet(src->is_scaling)
			_push_code (FOUR (0x4d, 0x8b
			, 0004|dst->reg<<3
			, src->scale_lv<<6|src->scale_reg<<3|src->reg
			));
			return;
		}
unimpl_yet(0 == 2)
	}
unimpl_yet(0 == 1)
}

//   c7: 41 8a 8c 51 01 06 00 00  -  mov cl,byte ptr .Lout_types[r9+rdx*sizeofOUTTYPE][.Lout_type] (.Lout_types = 0x600, sizeofOUTTYPE = 2, .Lout_type = 1)
//  438:    8a 89 64 06 00 00     -  mov cl,byte ptr .Lout_types[rcx+0x32*sizeofOUTTYPE][.Lout_last] (= 0x664)
//  724:    8a 11                 -  mov dl,byte ptr [rcx]
//  77a: 41 8a 50 ff              -  mov dl,byte ptr [r8-1]
//  be3:    8a 4e 02              -  mov cl,byte ptr .Lreg_b[rsi] (= 0x02)
// <r8> , byte ptr ___
static void _mov_r8_from_ptr (OPR_CTRL *dst, OPR_CTRL *src, u32 offset)
{
unimpl_yet(ALtoBH == dst->reg_type)
	if (RAXtoRDI == src->reg_type) {
		if (0 == offset) {
unimpl_yet(!src->is_scaling)
			_push_code (TWO(0x8a
			, 0000|dst->reg<<3|src->reg
			));
			return;
		}
		else if (-128 <= (s32)offset && (s32)offset <= 127) {
unimpl_yet(!src->is_scaling)
			_push_code (TWO(0x8a
			, 0100|dst->reg<<3|src->reg
			));
			_push_code (offset, 1);
			return;
		}
		else {
unimpl_yet(!src->is_scaling)
			_push_code (TWO(0x8a
			, 0200|dst->reg<<3|src->reg
			));
			_push_code (offset, 4);
			return;
		}
	}
	else if (R8toR15 == src->reg_type) {
		if (0 == offset) {
unimpl_yet(!src->is_scaling)
			_push_code (THREE(0x41, 0x8a
			, 0000|dst->reg<<3|src->reg
			));
			return;
		}
		else if (-128 <= (s32)offset && (s32)offset <= 127) {
unimpl_yet(!src->is_scaling)
			_push_code (FOUR(0x41, 0x8a
			, 0100|dst->reg<<3|src->reg
			, (u8)offset
			));
			return;
		}
		else {
unimpl_yet(src->is_scaling)
			_push_code (FOUR(0x41, 0x8a
			, 0204|dst->reg<<3
			, src->scale_lv<<6|src->scale_reg<<3|src->reg
			));
			_push_code (offset, 4);
			return;
		}
	}
	else
unimpl_yet(0)
}

//  1be:      66    8b 56 36  -  mov dx,word ptr .Lok_clk[rsi] (.Lok_clk = 0x36)
//  848:      66 41 8b 50 fe  -  mov dx,word ptr [r8-2]
// <r16> , word ptr ___
static void _mov_r16_from_ptr (OPR_CTRL *dst, OPR_CTRL *src, u32 offset)
{
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
unimpl_yet(AXtoDI == dst->reg_type)
unimpl_yet(!src->is_scaling)
	if (RAXtoRDI == src->reg_type) {
		_push_code (TWO(0x66, 0x8b));
		if (0 == offset) {
			_push_code (ONE(
			  0000|dst->reg<<3|src->reg
			));
		}
		else {
			_push_code (TWO(
			  0100|dst->reg<<3|src->reg
			, (u8)offset
			));
		}
		return;
	}
	else if (R8toR15 == src->reg_type) {
		_push_code (THREE(0x66, 0x41, 0x8b));
		_push_code (TWO(
		  0100|dst->reg<<3|src->reg
		, (u8)offset
		));
		return;
	}
	else
unimpl_yet(0 == 1)
}

//    0:    40 88 7e 20           -  mov byte ptr .Lerrno[rsi],dil    (.Lerrno  = 0x20)
//   b1:       88 4e 23           -  mov byte ptr .Lerraux[rsi][2],cl (.Lerraux = 0x21)
//   f5: 41 88 8c 51 00 06 00 00  -  mov byte ptr .Lout_types[r9+rdx*sizeofOUTTYPE][.Lout_last],cl (.Lout_types = 0x600, sizeofOUTTYPE = 2, .Lout_last = 0)
//   f5: 41 88 8c 51 00 06 00 00  -  mov byte ptr .Lout_types[r9+rdx*sizeofOUTTYPE][.Lout_last],cl (.Lout_types = 0x600, sizeofOUTTYPE = 2, .Lout_last = 0)
//  aa0:       88 4e 07           -  mov byte ptr .Lreg_b[rsi],cl     (.Lreg_b  = 0x07)
// byte ptr ___ , <r8>
static void _mov_ptr_from_r8 (OPR_CTRL *dst, u32 offset, OPR_CTRL *src)
{
unimpl_yet(ALtoBH == src->reg_type)
	if (RAXtoRDI == dst->reg_type) {
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
unimpl_yet(!dst->is_scaling)
		if (0 == offset) {
			_push_code (TWO(0x88
			, 0000|src->reg<<3|dst->reg
			));
		}
		else /*if (-128 <= (s32)offset && (s32)offset <= 127)*/ {
			_push_code (THREE(0x88
			, 0100|src->reg<<3|dst->reg
			, (u8)offset
			));
		}
	}
	else if (R8toR15 == dst->reg_type) {
unimpl_yet(dst->is_scaling)
		_push_code (FOUR (0x41, 0x88
		, 0204|src->reg<<3
		, dst->scale_lv<<6|dst->scale_reg<<3|dst->reg
		));
		_push_code (offset, 4);
	}
	else
unimpl_yet(0)
}

// word ptr ___ , <r16>
static void _mov_ptr_from_r16 (OPR_CTRL *dst, u32 offset, OPR_CTRL *src)
{
unimpl_yet(AXtoDI == src->reg_type)
unimpl_yet(RAXtoRDI == dst->reg_type)
unimpl_yet(!dst->is_scaling)
	if (0 == offset) {
		_push_code (THREE(0x66, 0x89
		, 0000|src->reg<<3|dst->reg
		));
		return;
	}
	else {
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
		_push_code (FOUR(0x66, 0x89
		, 0100|src->reg<<3|dst->reg
		, (u8)offset
		));
		return;
	}
unimpl_yet(0)
}

//   95:    48 89 4e 28           -  mov qword ptr .Lerrptr[rsi],rcx (.Lerrptr = 0x28)
// 395e:    48 89 3c 24           -  mov qword ptr [rsp][0],rdi (= 0x00)
// qword ptr ___ , <r64>
static void _mov_ptr_from_r64 (OPR_CTRL *dst, u32 offset, OPR_CTRL *src)
{
unimpl_yet(RAXtoRDI == src->reg_type)
	if (RAXtoRDI == dst->reg_type) {
unimpl_yet(!dst->is_scaling)
		if (0 == offset) {
			_push_code (FOUR (0x48, 0x89
			, 0000|src->reg<<3|dst->reg
			, 0044 // PENDING: ???
			));
			return;
		}
		else {
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
			_push_code (FOUR (0x48, 0x89
			, 0100|src->reg<<3|dst->reg
			, (u8)offset
			));
			return;
		}
	}
unimpl_yet(0)
}

//   8a: c6 46 20 fc              -  mov byte ptr .Lerrno[rsi],EASSERT
// byte ptr ___ , <imm8>
static void _mov_ptr_from_imm8 (OPR_CTRL *dst, u32 offset,
                                const TOKEN *p, unsigned n, const char *_source)
{
unimpl_yet(RAXtoRDI == dst->reg_type)
unimpl_yet(-128 <= (s32)offset && (s32)offset <= 127)
VALUE imm8; s2 r
	= _expr2n (p, n, _source, &imm8);
unimpl_yet(TYPE_NUM == r)
	_push_code (FOUR (0xc6
	, 0100|dst->reg
	, (u8)offset
	, (u8)imm8.number
	));
}

//   23:    66 89 d3              -  mov bx,dx
//   26:    48 89 da              -  mov rdx,rbx
//   3f:    66 89 da              -  mov dx,bx
//   52:       88 ef              -  mov bh,ch
//   7c:    49 89 c8              -  mov r8,rcx
//   f3:       88 e9              -  mov cl,ch
//  11e:       88 f1              -  mov cl,dh
//  3f7:          b0 50           -  mov al,EFAULT_ERAM
//  44f:          b9 00 60 00 00  -  mov ecx,(ofsN88ROM-0x0000) << (16 -8) | 0x0000 >> (10 -2) | 0x6000 >> (10 -10)
//  7c4:       89 da              -  mov edx,ebx
// 134e:    66    ba 00 00        -  mov dx,0x00
// 3738:    41 88 c9              -  mov r9b,cl
// 38ac:    48 c7 c2 00 00 00 00  -  mov rdx,exINT_ACK (= 0x0)
// 3a4b:       c6 46 21 71        -  mov byte ptr .Lerraux[rsi][0],1+EFAULT_RAM (= 0x21, 0x71)
static void _mov (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

	// [byte|word|qword] ptr ___ , <r8|16|64>|<imm8|16|64>
	if (1 == (t1 +comma_i -1)->len && ']' == *(_source +(t1 +comma_i -1)->pos)) {
input_bug(5 <= comma_i)
input_bug(ID_PTR == _tok2id (_source +(t1 +1)->pos, (t1 +1)->len, NULL))

		// destination operand
OPR_CTRL dst; VALUE ofs; s2 r
		= _opr2n (t1 +2, comma_i -2, _source, &dst, &ofs);
unimpl_yet(TYPE_NUM == r)

u32 val;
		switch (src0.reg_type) {
		// byte ptr ___ , <imm8>
		case SYMNUM:
input_bug(ETC == dst0.reg_type && ETC_BYTE == dst0.reg)
			_mov_ptr_from_imm8 (&dst, ofs.number, t1 +src0_i, n -src0_i, _source);
			break;
		// byte ptr ___ , <r8>
		case ALtoBH:
unimpl_yet(src0_i +1 == n)
input_bug(ETC == dst0.reg_type && ETC_BYTE == dst0.reg)
			_mov_ptr_from_r8 (&dst, ofs.number, &src0);
			break;
		case SPLtoDIL:
unimpl_yet(src0_i +1 == n)
input_bug(ETC == dst0.reg_type && ETC_BYTE == dst0.reg)
unimpl_yet(!dst.is_scaling)
unimpl_yet(RAXtoRDI == dst.reg_type)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
			_push_code (FOUR (0x40, 0x88
			, 0100|src0.reg<<3|dst.reg
			, (u8)ofs.number
			));
			break;
		// word ptr ___ , <r16>
		case AXtoDI:
unimpl_yet(src0_i +1 == n)
input_bug(ETC == dst0.reg_type && ETC_WORD == dst0.reg)
			_mov_ptr_from_r16 (&dst, ofs.number, &src0);
			break;
		// qword ptr ___ , <r64>
		case RAXtoRDI:
unimpl_yet(src0_i +1 == n)
input_bug(ETC == dst0.reg_type && ETC_QWORD == dst0.reg)
			_mov_ptr_from_r64 (&dst, ofs.number, &src0);
			break;
		default:
unimpl_yet(0 == 5)
			break;
		}
		return;
	}
input_bug(1 == comma_i)

	switch (src0.reg_type) {
	// <r32> , <r32>
	case EAXtoEDI:
input_bug(3 == n)
input_bug(EAXtoEDI == dst0.reg_type)
		_push_code (TWO(0x89
		, 0300|src0.reg<<3|dst0.reg
		));
		return;
	case ALtoBH:
input_bug(3 == n)
		// <r8> , <r8>
		if (ALtoBH == dst0.reg_type) {
			_push_code (TWO  (0x88
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
		// <r8(r8b..r15b)> , <r8>
		if (R8BtoR15B == dst0.reg_type) {
			_push_code (THREE(0x41, 0x88
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 4)
	// <r16> , <r16>
	case AXtoDI:
input_bug(3 == n)
input_bug(AXtoDI == dst0.reg_type)
		_push_code (THREE(0x66, 0x89
		, 0300|src0.reg<<3|dst0.reg
		));
		return;
	case RAXtoRDI:
		// <r64> , <r64>
		if (RAXtoRDI == dst0.reg_type) {
			_push_code (THREE(0x48, 0x89
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
		// <r64(r8..r15)> , <r64>
		if (R8toR15 == dst0.reg_type) {
			_push_code (THREE(0x49, 0x89
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 3)
	default:
		break;
	}

	if (1 == (t1 +n -1)->len && ']' == *(_source +(t1 +n -1)->pos)) {
input_bug(src0_i +5 <= n)
input_bug(ID_PTR == _tok2id (_source +(t1 +src0_i +1)->pos, (t1 +src0_i +1)->len, NULL))
OPR_CTRL src; VALUE ofs; s2 r
		= _opr2n (t1 +src0_i +2, n -(src0_i +2), _source, &src, &ofs);
unimpl_yet(TYPE_NUM == r)
OPR_CTRL dst = {0};

		switch (dst0.reg_type) {
		default:
			break;
		// <r64> , qword ptr ___
		case RAXtoRDI:
input_bug(ETC == src0.reg_type && ETC_QWORD == src0.reg)
			_mov_r64_from_ptr (&dst0, &src, ofs.number);
			return;
		case R8toR15:
input_bug(ETC == src0.reg_type && ETC_QWORD == src0.reg)
			_mov_r64_from_ptr (&dst0, &src, ofs.number);
			return;
		// <r8> , byte ptr ___
		case ALtoBH:
input_bug(ETC == src0.reg_type && ETC_BYTE == src0.reg)
			_mov_r8_from_ptr (&dst0, &src, ofs.number);
			return;
		// <r16> , word ptr ___
		case AXtoDI:
input_bug(ETC == src0.reg_type && ETC_WORD == src0.reg)
			_mov_r16_from_ptr (&dst0, &src, ofs.number);
			return;
		}
unimpl_yet(0 == 2)
	}

input_bug(SYMNUM == src0.reg_type)
VALUE val; s2 r
	= _expr2n (t1 +src0_i, n -src0_i, _source, &val);
unimpl_yet(TYPE_NUM == r)
	// <r32> , <imm32>
	if (EAXtoEDI == dst0.reg_type) {
unimpl_yet(! (-128 <= (s32)val.number && (s32)val.number <= 127))
		_push_code (ONE(
		  0270|dst0.reg
		));
		_push_code (val.number, 4);
		return;
	}
	// <r8> , <imm8>
	if (ALtoBH == dst0.reg_type) {
		_push_code (TWO(
		  0260|dst0.reg
		, (u8)val.number
		));
		return;
	}
	// <r16> , <imm16>
	if (AXtoDI == dst0.reg_type) {
unimpl_yet(-65536 <= (s32)val.number && (s32)val.number <= 65535)
		_push_code (TWO(0x66
		, 0270|dst0.reg
		));
		_push_code ((u16)val.number, 2);
		return;
	}
	// <r64> , <imm64>
	if (RAXtoRDI == dst0.reg_type) {
		_push_code (THREE(0x48, 0xc7
		, 0300|dst0.reg
		));
		_push_code (val.number, 4);
		return;
	}
unimpl_yet(0 == 1)
}

//  124:    0f b6 d2              -    movzx edx,dl
//  16a: 48 0f b6 d3              -    movzx rdx,bl
//  49b:    0f b6 89 e0 06 00 00  -    movzx ecx,byte ptr .Lout_types[rcx+0x70*sizeofOUTTYPE][.Lout_last] (= 0x6e0)
//  562: 48 0f b7 ca              -    movzx rcx,dx
//  a8c: 48 0f b6 56 06           -    movzx rdx,byte ptr .Lreg_c[rsi] (= 0x06)
// 32bf: 48 0f b7 4e 06           -    movzx rcx,word ptr .Lreg_bc[rsi] (= 0x06)
// 3750: 49 0f b6 c9              -    movzx rcx,r9b
// 39ae: 49 0f b6 10              -    movzx rdx,byte ptr [r8]
static void _movzx (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

	switch (src0.reg_type) {
	case ALtoBH:
input_bug(3 == n)
		switch (dst0.reg_type) {
		default:
			break;
		// <r32> , <r8>
		case EAXtoEDI:
			_push_code (THREE(      0x0f, 0xb6
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		// <r64> , <r8>
		case RAXtoRDI:
			_push_code (FOUR (0x48, 0x0f, 0xb6
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		}
unimpl_yet(0 == 5)

	case R8BtoR15B:
input_bug(3 == n)
		switch (dst0.reg_type) {
		default:
			break;
		// <r64> , <r8(r8b..r15b)>
		case RAXtoRDI:
			_push_code (FOUR (0x49, 0x0f, 0xb6
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		}
unimpl_yet(0 == 4)

	case AXtoDI:
input_bug(3 == n)
		switch (dst0.reg_type) {
		default:
			break;
		// <r64> , <r16>
		case RAXtoRDI:
			_push_code (FOUR (0x48, 0x0f, 0xb7
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		}
unimpl_yet(0 == 3)
	}

	if (1 == (t1 +n -1)->len && ']' == *(_source +(t1 +n -1)->pos)) {
input_bug(src0_i +5 <= n)
input_bug(ID_PTR == _tok2id (_source +(t1 +src0_i +1)->pos, (t1 +src0_i +1)->len, NULL))
OPR_CTRL src; VALUE ofs; s2 r
		= _opr2n (t1 +src0_i +2, n -(src0_i +2), _source, &src, &ofs);
unimpl_yet(TYPE_NUM == r)

		switch (dst0.reg_type) {
		default:
			break;
		// <r32> , byte ptr ___
		case EAXtoEDI:
input_bug(ETC == src0.reg_type && ETC_BYTE == src0.reg)
unimpl_yet(RAXtoRDI == src.reg_type)
unimpl_yet(! (-128 <= (s32)ofs.number && (s32)ofs.number <= 127))
unimpl_yet(!src.is_scaling)
			_push_code (THREE(0x0f, 0xb6
			, 0200|dst0.reg<<3|src.reg
			));
			_push_code (ofs.number, 4);
			return;
		case RAXtoRDI:
unimpl_yet(!src.is_scaling)
unimpl_yet(ETC == src0.reg_type)
			// <r64> , byte ptr ___
			if (ETC_BYTE == src0.reg) {
				if (0 == ofs.number) {
					_push_code (FOUR(0x49, 0x0f, 0xb6
					, 0000|dst0.reg<<3|src.reg
					));
				}
				else {
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
					_push_code (FOUR(0x48, 0x0f, 0xb6
					, 0100|dst0.reg<<3|src.reg
					));
					_push_code ((u8)ofs.number, 1);
				}
				return;
			}
			// <r64> , word ptr ___
			if (ETC_WORD == src0.reg) {
				_push_code (FOUR(0x48, 0x0f, 0xb7
				, 0100|dst0.reg<<3|src.reg
				));
				_push_code ((u8)ofs.number, 1);
				return;
			}
			break;
		}
unimpl_yet(0 == 2)
	}

	// <r64> , <r16>
unimpl_yet(0 == 1)
}

//  c21: 48 0f be d2              -    movsx rdx,dl
// 19d4: 66 0f be d2              -    movsx dx,dl
static void _movsx (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

	if (ALtoBH == src0.reg_type) {
input_bug(3 == n)
		switch (dst0.reg_type) {
		default:
			break;
		// <r32> , <r8>
		// <r16> , <r8>
		case AXtoDI:
			_push_code (FOUR(0x66, 0x0f, 0xbe
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		// <r64> , <r8>
		case RAXtoRDI:
			_push_code (FOUR(0x48, 0x0f, 0xbe
			, 0300|dst0.reg<<3|src0.reg
			));
			return;
		}
	}
unimpl_yet(0 == 1)
}

//   99: 4c 8d 05 60 ff ff ff     -    lea r8,qword ptr .Lrva_base[rip] (.Lrva_base = -0xa0?)
//   b4: 48 8d 0d 7d 39 00 00     -    lea rcx,qword ptr .Labort_rsiCPU[rip] (.Labort_rsiCPU = 0xf9)
// 33c6: 48 8d 4a 01              -    lea rcx, [rdx+1]
static void _lea (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

input_bug(src0_i +2 < n)
input_bug(1 == (t1 +n -1)->len && ']' == _source[(t1 +n -1)->pos])
input_bug(2 == src0_i)
OPR_CTRL src1;
	_tok2id (_source +(t1 +src0_i +1)->pos, (t1 +src0_i +1)->len, &src1);
unsigned opr0_i = src0_i;
	if (ETC == src0.reg_type && ETC_QWORD == src0.reg
	 && ETC == src1.reg_type && ETC_PTR   == src1.reg)
		opr0_i += 2;

	// source operand
OPR_CTRL src; VALUE ofs; s2 r
	= _opr2n (t1 +opr0_i, n -opr0_i, _source, &src, &ofs);
input_bug(RIP == src.reg_type || RAXtoRDI == src.reg_type)

	// destination operand
OPR_CTRL dst;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst);
input_bug(RAXtoRDI == dst.reg_type || R8toR15 == dst.reg_type)

	switch (src.reg_type) {
	// <r64> , qword ptr ___ [ <r64> ___ ]
	case RAXtoRDI:
unimpl_yet(TYPE_NUM == r)
unimpl_yet(! (4 == src.reg || 5 == src.reg)) // except for RBP RSP
		if (R8toR15 == dst.reg_type) {
unimpl_yet(0 == ofs.number)
			_push_code (THREE(0x4c, 0x8d
			, 0000|dst.reg<<3|src.reg
			));
			return;
		}
		if (RAXtoRDI == dst.reg_type) {
unimpl_yet(-128 <= (s32)ofs.number || (s32)ofs.number <= 127)
			_push_code (FOUR(0x48, 0x8d
			, 0100|dst.reg<<3|src.reg
			, (u8)ofs.number
			));
			return;
		}
		break;
	case RIP:
		// <r64> , qword ptr ___ [ rip ___ ]
		if (RAXtoRDI == dst.reg_type) {
unimpl_yet(CALC_ANY_REPLACED_ZERO == r)
FRAGMENT_ID frag_id; const FRAGMENT *frag
			= _get (frag_id = _ensure_appending ());

			_push_code (THREE(0x48, 0x8d
			, 0005|dst.reg<<3
			));
			_push_code (0x00000000, 4);
REFTBL_OPR opr32 = {
			.type    = REL32,
			.addr_id = addrs_append (frag_id, frag->obj_len),
			.sym_id  = 0,
};
			_append_reftbl (&opr32, t1 +opr0_i, n -opr0_i, _source);
			return;
		}
		// <r64(r8..r15)> , qword ptr ___ [ rip ___ ]
		else if (R8toR15 == dst.reg_type) {
input_bug(CALC_ANY_REPLACED_ZERO == r)
FRAGMENT_ID frag_id; const FRAGMENT *frag
			= _get (frag_id = _ensure_appending ());

			_push_code (THREE(0x4c, 0x8d
			, 0005|dst.reg<<3
			));
			_push_code (0x00000000, 4);
REFTBL_OPR opr32 = {
			.type    = REL32,
			.addr_id = addrs_append (frag_id, frag->obj_len),
			.sym_id  = 0,
};
			_append_reftbl (&opr32, t1 +opr0_i, n -opr0_i, _source);
			return;
		}
		break;
	}
unimpl_yet(0 == 1)
}

// 3923:    66 87 da              -  xchg dx,bx
static void _xchg (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

	// <r16> , <r16>
	if (AXtoDI == src0.reg_type) {
input_bug(3 == n)
input_bug(AXtoDI == dst0.reg_type)
		_push_code (THREE(0x66, 0x87
		, 0300|src0.reg<<3|dst0.reg
		));
		return;
	}
unimpl_yet(0 == 1)
}

///////////////////////////////////////////////////////////////
// inc dec

typedef enum {
	INC = 0,
	DEC,
} INCDEC;
//   59: 66 ff cb                 -  dec bx
//   cf:    fe c9                 -  dec cl
//  636: 48 ff 46 68              -  inc qword ptr .LobsvGVRAM[rsi] (= 0x68)
//  774: 49 ff c0                 -  inc r8
//  777: 66 ff c3                 -  inc bx
//  878: 66 ff ca                 -  dec dx
//  87b: 48 ff c9                 -  dec rcx
// 12a0:    fe 4e 07              -  dec byte ptr .Lreg_b[rsi] (= 0x07)
// 32bb: 66 ff 46 02              -  inc word ptr .Lreg_hl[rsi] (= 0x02)
static void _incdec (const TOKEN *t1, unsigned n, const char *_source, INCDEC op)
{
OPR_CTRL dst0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	// <r8>
	if (ALtoBH == dst0.reg_type) {
input_bug(1 == n)
		_push_code (TWO  (      0xfe
		, 0300|op<<3|dst0.reg
		));
		return;
	}
	// <r16>
	if (AXtoDI == dst0.reg_type) {
input_bug(1 == n)
		_push_code (THREE(0x66, 0xff
		, 0300|op<<3|dst0.reg
		));
		return;
	}
	// <r64>
	if (RAXtoRDI == dst0.reg_type) {
input_bug(1 == n)
		_push_code (THREE(0x48, 0xff
		, 0300|op<<3|dst0.reg
		));
		return;
	}
	// <r64(r8..r15)>
	if (R8toR15 == dst0.reg_type) {
input_bug(1 == n)
		_push_code (THREE(0x49, 0xff
		, 0300|op<<3|dst0.reg
		));
		return;
	}
input_bug(1 == (t1 +n -1)->len && ']' == _source[(t1 +n -1)->pos])
input_bug(5 <= n)
input_bug(ID_PTR == _tok2id (_source +(t1 +1)->pos, (t1 +1)->len, NULL))
OPR_CTRL dst; VALUE ofs; s2 r
	= _opr2n (t1 +2, n -2, _source, &dst, &ofs);
unimpl_yet(TYPE_NUM == r)
	if (RAXtoRDI == dst.reg_type) {
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
unimpl_yet(!dst.is_scaling)
input_bug(ETC == dst0.reg_type)
		// byte ptr ___
		if (ETC_BYTE == dst0.reg) {
			_push_code (THREE(0xfe
			, 0100|op<<3|dst.reg
			, (u8)ofs.number
			));
			return;
		}
		// word ptr ___
		if (ETC_WORD == dst0.reg) {
			_push_code (FOUR(0x66, 0xff
			, 0100|op<<3|dst.reg
			, (u8)ofs.number
			));
			return;
		}
		// qword ptr ___
		if (ETC_QWORD == dst0.reg) {
			_push_code (FOUR(0x48, 0xff
			, 0100|op<<3|dst.reg
			, (u8)ofs.number
			));
			return;
		}
	}
unimpl_yet(0 == 1)
}

static void _inc (const TOKEN *t1, unsigned n, const char *_source)
{ _incdec (t1, n, _source, INC); }

static void _dec (const TOKEN *t1, unsigned n, const char *_source)
{ _incdec (t1, n, _source, DEC); }

///////////////////////////////////////////////////////////////
// add adc sub sbc and xor or cmp

typedef enum {
	ADD = 0,
	OR,
	ADC,
	SBB,
	AND,
	SUB,
	XOR,
	CMP,
} ALU;
//    9: 66 41 29 d8              -  sub r8w,bx
//    f:    49 81 e8 00 00 01 00  -  sub r8,0x10000
//   16: 66 41 01 d0              -  add r8w,dx
//   1c:    49 81 c0 00 00 01 00  -  add r8,0x10000
//   2d:       21 d2              -  and edx,edx
//   31: 66    39 d3              -  cmp bx,dx
//   4a: 66    81 e3 fc 00        -  and bx,0xfc
//   54: 66    81 e3 00 fc        -  and bx,0xFC00
//   66:    48 81 e1 00 ff 03 00  -  and rcx,ofsMASK
//   6d: 66    01 d1              -  add cx,dx
//   72:       81 c1 00 00 01 00  -  add ecx,0x10000
//   78:    48 03 4f 10           -  add rcx,qword ptr .Litem_ptr[rdi+iMEM*sizeofITEM]
//   d7:       80 f9 15           -  cmp cl,CB_MAX -iCB2 +1
//  1c2: 66    2b 4e 36           -  sub cx,word ptr .Lok_clk[rsi] (.Lok_clk = 0x36)
//  1c6: 66    83 f9 14           -  cmp cx,(16+3)+1
//  4d6:          3c 03           -  cmp al,3
//  6a1:       28 f5              -  sub ch,dh
//  72b:       80 4e 21 01        -  or byte ptr .Lerraux[rsi][0],1 (= 0x21)
//  c9b:    49 01 d0              -  add r8,rdx
// 1180: 66    83 46 0c 02        -  add word ptr .Lreg_sp[rsi],2 (= 0x0c)
// 1461:       02 56 06           -  add dl,byte ptr .Lreg_c[rsi] (= 0x06)
// 3364:    49 83 e8 02           -  sub r8,2
// 3775:          05 00 00 00 02  -  add eax,0x02000000
// 39d1:    48 3b 3c 24           -  cmp rdi,qword ptr [rsp][0]
// 3a15:       30 46 19           -  xor byte ptr .Lreg_r[rsi],al (= 0x19)
static void _alu (const TOKEN *t1, unsigned n, const char *_source, ALU op)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

	if (5 <= comma_i
	 && 1 == (t1 +comma_i -1)->len && ']' == *(_source +(t1 +comma_i -1)->pos)) {
input_bug(ID_PTR == _tok2id (_source +(t1 +1)->pos, (t1 +1)->len, NULL))
OPR_CTRL dst; VALUE ofs; s2 r
		= _opr2n (t1 +2, comma_i -2, _source, &dst, &ofs);
unimpl_yet(TYPE_NUM == r)

		// byte ptr ___ , <r8>
		if (ALtoBH == src0.reg_type) {
unimpl_yet(RAXtoRDI == dst.reg_type)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
			_push_code (THREE(
			  0000|op<<3
			, 0100|src0.reg<<3|dst.reg
			, (u8)ofs.number
			));
			return;
		}
		if (SYMNUM == src0.reg_type) {
VALUE val; s2 r
			= _expr2n (t1 +src0_i, n -src0_i, _source, &val);
unimpl_yet(TYPE_NUM == r)
input_bug(ETC == dst0.reg_type)
			// dword ptr ___ , <imm32>
			// byte ptr ___ , <imm8>
			if (ETC_BYTE == dst0.reg) {
unimpl_yet(RAXtoRDI == dst.reg_type)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
unimpl_yet(-256 <= (s32)val.number && (s32)val.number <= 255)
				_push_code (FOUR (0x80
				, 0100|op<<3|dst.reg
				, (u8)ofs.number
				, (u8)val.number
				));
				return;
			}
			// word ptr ___ , <imm16>
			if (ETC_WORD == dst0.reg) {
unimpl_yet(RAXtoRDI == dst.reg_type)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
unimpl_yet(-256 <= (s32)val.number && (s32)val.number <= 255)
				_push_code (TWO(0x66, 0x83
				));
				_push_code (THREE(
				  0100|op<<3|dst.reg
				, (u8)ofs.number
				, (u8)val.number
				));
				return;
			}
			// qword ptr ___ , <imm64>
		}
unimpl_yet(0 == 8)
	}

	if (EAXtoEDI == src0.reg_type) {
		// <r32> , <r32>
		if (EAXtoEDI == dst0.reg_type) {
			_push_code (TWO(
			  0001|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 7)
	}
	if (ALtoBH == src0.reg_type) {
		// <r8> , <r8>
		if (ALtoBH == dst0.reg_type) {
			_push_code (TWO(
			  0000|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 6)
	}
	if (AXtoDI == src0.reg_type) {
		// <r16> , <r16>
		if (AXtoDI == dst0.reg_type) {
			_push_code (THREE(0x66
			, 0001|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
		// <r16(r8w..r15w)> , <r16>
		if (R8WtoR15W == dst0.reg_type) {
			_push_code (FOUR (0x66, 0x41
			, 0001|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 5)
	}
	if (R8toR15 == src0.reg_type) {
		// <r64> , <r64(r8..r15)>
		if (RAXtoRDI== dst0.reg_type) {
			_push_code (THREE(0x4c
			, 0001|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 4)
	}
	if (RAXtoRDI== src0.reg_type) {
		// <r64(r8..r15)>, <r64>
		if (R8toR15 == dst0.reg_type) {
			_push_code (THREE(0x49
			, 0001|op<<3
			, 0300|src0.reg<<3|dst0.reg
			));
			return;
		}
unimpl_yet(0 == 3)
	}

	if (src0_i +5 <= n
	 && 1 == (t1 +n -1)->len && ']' == *(_source +(t1 +n -1)->pos)) {
input_bug(ID_PTR == _tok2id (_source +(t1 +src0_i +1)->pos, (t1 +src0_i +1)->len, NULL))
OPR_CTRL src; VALUE ofs; s2 r
		= _opr2n (t1 +src0_i +2, n -(src0_i +2), _source, &src, &ofs);
unimpl_yet(TYPE_NUM == r)

unimpl_yet(RAXtoRDI == src.reg_type)
		// <r8> , byte ptr ___
		if (ALtoBH == dst0.reg_type) {
input_bug(ETC == src0.reg_type && ETC_BYTE == src0.reg)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
			_push_code (THREE(
			  0002|op<<3
			, 0100|dst0.reg<<3|src.reg
			, (u8)ofs.number
			));
			return;
		}
		// <r16> , qword ptr ___
		if (AXtoDI == dst0.reg_type) {
input_bug(ETC == src0.reg_type && ETC_WORD == src0.reg)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
			_push_code (FOUR (0x66
			, 0003|op<<3
			, 0100|dst0.reg<<3|src.reg
			, (u8)ofs.number
			));
			return;
		}
		// <r64> , qword ptr ___
		if (RAXtoRDI == dst0.reg_type) {
input_bug(ETC == src0.reg_type && ETC_QWORD == src0.reg)
			if (0 == ofs.number) {
				_push_code (FOUR (0x48
				, 0003|op<<3
				, 0000|dst0.reg<<3|src.reg
				, 0044 // PENDING: ???
				));
				return;
			}
			else {
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
				_push_code (FOUR (0x48
				, 0003|op<<3
				, 0100|dst0.reg<<3|src.reg
				, (u8)ofs.number
				));
				return;
			}
		}
unimpl_yet(0 == 2)
	}

unimpl_yet(SYMNUM == src0.reg_type)
VALUE val; s2 r
	= _expr2n (t1 +src0_i, n -src0_i, _source, &val);
unimpl_yet(TYPE_NUM == r)

	if (EAXtoEDI == dst0.reg_type) {
		// eax , <imm32>
		if (0 == dst0.reg) { // EAX
			_push_code (ONE(
			  0005|op<<3
			));
			_push_code (val.number, 4);
			return;
		}
		// <r32> , <imm32>
		else {
			if (-128 <= (s32)val.number && (s32)val.number <= 127) {
				_push_code (TWO(0x83
				, 0300|op<<3|dst0.reg
				));
				_push_code ((u8)val.number, 1);
				return;
			}
			else {
				_push_code (TWO(0x81
				, 0300|op<<3|dst0.reg
				));
				_push_code (val.number, 4);
				return;
			}
		}
	}
	else if (ALtoBH == dst0.reg_type) {
unimpl_yet(-256 <= (s32)val.number && (s32)val.number <= 255)
		// al , <imm8>
		if (0 == dst0.reg) {
			_push_code (TWO(
			  0004|op<<3
			, (u8)val.number
			));
			return;
		}
		// <r8> , <imm8>
		else {
			_push_code (THREE(0x80
			, 0300|op<<3|dst0.reg
			, (u8)val.number
			));
			return;
		}
	}
	// <r16> , <imm16>
	else if (AXtoDI == dst0.reg_type) {
		if (-128 <= (s16)val.number && (s16)val.number <= 127) {
			_push_code (THREE(0x66, 0x83
			, 0300|op<<3|dst0.reg
			));
			_push_code ((u8)val.number, 1);
			return;
		} else {
			_push_code (THREE(0x66, 0x81
			, 0300|op<<3|dst0.reg
			));
			_push_code ((u16)val.number, 2);
			return;
		}
	}
	// <r64> , <imm64>
	else if (RAXtoRDI == dst0.reg_type) {
		if (-128 <= (s32)val.number && (s32)val.number <= 127) {
			_push_code (THREE(0x48, 0x83
			, 0300|op<<3|dst0.reg
			));
			_push_code ((u8)val.number, 1);
			return;
		}
		else {
			_push_code (THREE(0x48, 0x81
			, 0300|op<<3|dst0.reg
			));
			_push_code (val.number, 4);
			return;
		}
	}
	// <r64(r8..r15)> , <imm64>
	else if (R8toR15 == dst0.reg_type) {
		if (-128 <= (s32)val.number && (s32)val.number <= 127) {
			_push_code (THREE(0x49, 0x83
			, 0300|op<<3|dst0.reg
			));
			_push_code ((u8)val.number, 1);
			return;
		}
		else {
			_push_code (THREE(0x49, 0x81
			, 0300|op<<3|dst0.reg
			));
			_push_code (val.number, 4);
			return;
		}
	}
unimpl_yet(0 == 1)
}
static void _add (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, ADD); }

static void _or  (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, OR ); }

static void _adc (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, ADC); }

static void _sbb (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, SBB); }

static void _and (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, AND); }

static void _sub (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, SUB); }

static void _xor (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, XOR); }

static void _cmp (const TOKEN *t1, unsigned n, const char *_source)
{ _alu (t1, n, _source, CMP); }

///////////////////////////////////////////////////////////////
// add adc sub sbc and xor or cmp

//  176: 49 f7 04 d1 ff ff ff ff  -  test qword ptr .Lcb_infos[r9+rdx*sizeofCBINFOdiv2][.Lcb_lpfn],-1 (sizeofCBINFOdiv2 = 8, .Lcb_lpfn = 0)
//  3f0:    f6 81 c4 07 00 00 01  -  test byte ptr .Lout_types[rcx+0xE2*sizeofOUTTYPE][.Lout_last],0x01 (= 0x7c4)
//  caf:    f6 c4 40              -  test ah,0x40
// 15f7: 66 f7 c1 ff ff           -  test cx,0xffff
// 3241:    f6 46 1b 04           -  test byte ptr .Lint_control[rsi],IFF1 (= 0x1b, 0x04)
static void _test (const TOKEN *t1, unsigned n, const char *_source)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
input_bug(comma)
unsigned comma_i, src0_i;
	comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i && src0_i < n)

OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);

		// source operand
unimpl_yet(SYMNUM == src0.reg_type)
VALUE val; s2 r
		= _expr2n (t1 +src0_i, n -src0_i, _source, &val);
unimpl_yet(TYPE_NUM == r)

	// <r8> , <imm8>
	if (ALtoBH == dst0.reg_type) {
unimpl_yet(1 == comma_i)
unimpl_yet(-256 <= (s32)val.number && (s32)val.number <= 255)
		_push_code (THREE(0xf6
		, 0300|dst0.reg
		, (u8)val.number
		));
		return;
	}
	// <r16> , <imm16>
	if (AXtoDI == dst0.reg_type) {
unimpl_yet(1 == comma_i)
unimpl_yet(-65536 <= (s32)val.number && (s32)val.number <= 65535)
		_push_code (THREE(0x66, 0xf7
		, 0300|dst0.reg
		));
		_push_code ((u16)val.number, 2);
		return;
	}

unimpl_yet(ETC == dst0.reg_type)
unimpl_yet(ETC_BYTE == dst0.reg || ETC_QWORD == dst0.reg)
input_bug(5 <= comma_i)
		// destination operand
input_bug(1 == (t1 +comma_i -1)->len && ']' == _source[(t1 +comma_i -1)->pos])
input_bug(ID_PTR == _tok2id (_source +(t1 +1)->pos, (t1 +1)->len, NULL))
OPR_CTRL dst; VALUE ofs;
		r = _opr2n (t1 +2, comma_i -2, _source, &dst, &ofs);
unimpl_yet(TYPE_NUM == r)

unimpl_yet(ETC == dst0.reg_type)
	// qword ptr ___ , <imm64>
	if (ETC_QWORD == dst0.reg) {
unimpl_yet(0 == ofs.number)
unimpl_yet(R8toR15 == dst.reg_type)
unimpl_yet(dst.is_scaling)

		_push_code (FOUR (0x49, 0xf7
		, 0004
		, dst.scale_lv<<6|dst.scale_reg<<3|dst.reg
		));
		_push_code (val.number, 4);
		return;
	}
	// byte ptr ___ , <imm8>
	if (ETC_BYTE == dst0.reg) {
unimpl_yet(RAXtoRDI == dst.reg_type)
unimpl_yet(!dst.is_scaling)
		if (-128 <= (s32)ofs.number && (s32)ofs.number <= 127) {
			_push_code (FOUR(0xf6
			, 0100|dst.reg
			, (u8)ofs.number
			, (u8)val.number
			));
			return;
		}
		else {
			_push_code (TWO(0xf6
			, 0200|dst.reg
			));
			_push_code (ofs.number, 4);
			_push_code ((u8)val.number, 1);
			return;
		}
	}
unimpl_yet(0)
}

///////////////////////////////////////////////////////////////
// flag manipulation

//   7f:          f8  -  clc
static void _clc (const TOKEN * /*t1*/, unsigned n, const char * /*_source*/)
{
input_bug(0 == n)
	_push_code (ONE(0xf8));
}

//  569:          f9  -  stc
static void _stc (const TOKEN * /*t1*/, unsigned n, const char * /*_source*/)
{
input_bug(0 == n)
	_push_code (ONE(0xf9));
}

//  96e:          9e  -  sahf
static void _sahf (const TOKEN * /*t1*/, unsigned n, const char * /*_source*/)
{
input_bug(0 == n)
	_push_code (ONE(0x9e));
}

//  974:          9f  -  lahf
static void _lahf (const TOKEN * /*t1*/, unsigned n, const char * /*_source*/)
{
input_bug(0 == n)
	_push_code (ONE(0x9f));
}

///////////////////////////////////////////////////////////////
// not setcc

//  93b:       f6 d0  -  not al
static void _not (const TOKEN * t1, unsigned n, const char * _source)
{
input_bug(1 == n)
OPR_CTRL dst0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	// <r8>
	if (ALtoBH == dst0.reg_type) {
		_push_code (TWO(0xf6
		, 0320|dst0.reg
		));
		return;
	}
unimpl_yet(0 == 1)
}

typedef enum {
	CC_O  = 0, // seto   jo  [objdump]
	CC_NO = 1, // setno  jno
	CC_C  = 2, // setb   jb
	CC_NC = 3, // setae  jae
	CC_Z  = 4, // sete   je
	CC_NZ = 5, // setne  jne
	CC_NA = 6, // setna  jna, jbe
	CC_A  = 7, // seta   ja , jnbe
} COND;

//  a27:    0f 90 c6  -  seto dh
// 1600:    0f 94 c1  -  setz cl
static void _setcc (const TOKEN * t1, unsigned n, const char * _source, COND cc)
{
input_bug(1 == n)
OPR_CTRL dst0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	// <r8>
	if (ALtoBH == dst0.reg_type) {
		_push_code (THREE(0x0f
		, 0220|cc
		, 0300|dst0.reg
		));
		return;
	}
unimpl_yet(0 == 1)
}

///////////////////////////////////////////////////////////////
// rotate / shift

typedef enum {
	ROL = 0,
	ROR,
	RCL,
	RCR,
	SHL,
	SHR,
	UNK_SHIFT,
	SAR,
} ROTSFT;
//   29:    48 c1 ca 10  -  ror rdx,16
//   4f:       c1 e3 18  -  shl ebx,10 -2 +16
//  1ff:       c1 c1 10  -  rol ecx,16
//  266:       c1 e9 10  -  shr ecx,16
//  956:       c0 e2 04  -  shl dl,4
//  96f:       d0 c0     -  rol al,1
// 3523:       d0 e2     -  sal dl,1
static void _rotsft (const TOKEN *t1, unsigned n, const char *_source, ROTSFT op)
{
const TOKEN *comma
	= _tokchr (t1, n, ',', _source);
unsigned comma_i, src0_i;
	if (NULL == comma)
		comma_i = src0_i = n;
	else
		comma_i = (unsigned)(comma -t1), src0_i = comma_i +1;
input_bug(0 < comma_i)

VALUE val;
OPR_CTRL dst0, src0;
	_tok2id (_source +(t1 +0)->pos, (t1 +0)->len, &dst0);
	if (NULL == comma)
		src0.reg_type = SYMNUM, val.number = 1;
	else {
		_tok2id (_source +(t1 +src0_i)->pos, (t1 +src0_i)->len, &src0);
unimpl_yet(SYMNUM == src0.reg_type)
s2 r
			= _expr2n (t1 +src0_i, n -src0_i, _source, &val);
unimpl_yet(TYPE_NUM == r)
		}

	// <r32> , <imm>
	if (EAXtoEDI == dst0.reg_type) {
		_push_code (THREE(0xc1
		, 0300|op<<3|dst0.reg
		, (u8)val.number
		));
		return;
	}
	// <r8> , <imm>
	if (ALtoBH == dst0.reg_type) {
		if (1 == val.number) {
			_push_code (TWO(0xd0
			, 0300|op<<3|dst0.reg
			));
		}
		else {
			_push_code (THREE(0xc0
			, 0300|op<<3|dst0.reg
			, (u8)val.number
			));
		}
		return;
	}
	// <r64> , <imm>
	if (RAXtoRDI == dst0.reg_type) {
		_push_code (FOUR (0x48, 0xc1
		, 0300|op<<3|dst0.reg, (u8)val.number
		));
		return;
	}
unimpl_yet(0 == 1)
}

static void _rol (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, ROL); }

static void _ror (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, ROR); }

static void _rcl (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, RCL); }

static void _rcr (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, RCR); }

static void _shl (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, SHL); }

static void _shr (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, SHR); }

static void _sal (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, SHL); }

static void _sar (const TOKEN *t1, unsigned n, const char *_source)
{ _rotsft (t1, n, _source, SAR); }

///////////////////////////////////////////////////////////////
// label / jmp jcc call ret leave push pop

static void _label (const char *s, unsigned n)
{
	// is first writing into top fragment?
	if (STRUCTSEG == pass1()->seg_id) {
VALUE entry = {
		.type   = TYPE_NUM,
		.number = pass1()->struct_addr
		};
		sym_set_value (s, n, &entry);
		return;
	}
	// 
FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());
VALUE entry = {
	.type    = TYPE_OFFSET,
	.addr_id = addrs_append(frag_id, frag->obj_len),
	};
	sym_set_value (s, n, &entry);
	return;
}

static void _jcc (const TOKEN *t0, unsigned n0, const char *_source, COND cc)
{
input_bug(2 == n0)
	_push_rel8_capable_jmp (t0 +1, n0 -1, _source
	, REL8or32
	, ONE(0160|cc)
	, TWO(0x0f, 0200|cc)
	, my()->line
	);
	_end_appending ();
}

//    4:       e9 2f 3a 00 00  -  jmp .Labort_rsiCPU   (= 3a38)
//   bb:    ff e1              -  jmp rcx
//   ee:       e9 49 39 00 00  -  jmp .Labort_rsiCPU2  (= 3a3c)
//  10c:       eb 22           -  jmp .LBus_Out_callback (= 0x130)
// 3785:    ff 24 d1           -  jmp qword ptr [rcx+rdx*8]
// 3803: 41 ff 24 d1           -  jmp qword ptr [r9+rdx*8]
static void _jmp (const TOKEN *t0, unsigned n0, const char *_source)
{
input_bug(1 +1 <= n0)
OPR_CTRL src1;
	_tok2id (_source +(t0 +1)->pos, (t0 +1)->len, &src1);
	// <r64>
	if (RAXtoRDI == src1.reg_type) {
input_bug(1 +1 == n0)
		_push_code (TWO  (0xff
		, 0340|src1.reg
		));
		return;
	}
	if (1 == (t0 +n0 -1)->len && ']' == *(_source +(t0 +n0 -1)->pos)) {
input_bug(1 +2 +1+1+4+1 <= n0)
OPR_CTRL src; VALUE ofs; s2 r
		= _opr2n (t0 +3, n0 -3, _source, &src, &ofs);
unimpl_yet(TYPE_NUM == r)
		// qword ptr [<expr>] '[' <r64> +<r64>*<scale> [[+-]<expr>] ']'
		if (RAXtoRDI == src.reg_type) {
unimpl_yet(0 == ofs.number)
			_push_code (THREE(0xff, 0x24
			, src.scale_lv<<6|src.scale_reg<<3|src.reg
			));
			return;
		}
		// qword ptr [<expr>] '[' <r64(r8..r15)> +<r64>*<scale> [[+-]<expr>] ']'
		if (R8toR15 == src.reg_type) {
unimpl_yet(0 == ofs.number)
			_push_code (FOUR(0x41, 0xff, 0x24
			, src.scale_lv<<6|src.scale_reg<<3|src.reg
			));
			return;
		}
	}
	// <imm>
	if (SYMNUM == src1.reg_type) {
input_bug(1 +1 == n0)
		_push_rel8_capable_jmp (t0 +1, n0 -1, _source
		, REL8or32
		, ONE(0xeb)
		, ONE(0xe9)
		, my()->line);
		_end_appending ();
		return;
	}
unimpl_yet(0)
}

//   42:    ff 56 48           -  call qword ptr .Llpfn_pre_lea_r[rsi] (= +48)
//  118:       e8 13 00 00 00  -  call .LBus_Out_callback
//  218: 41 ff d1              -  call r9
// 36ae: 41 ff 14 c9           -  call qword ptr [r9+rcx*8]
// 39ce:    ff 14 d1           -  call qword ptr [rcx+rdx*8]
static void _call (const TOKEN *t0, unsigned n0, const char *_source)
{
input_bug(1 +1 <= n0)
OPR_CTRL src1;
	_tok2id (_source +(t0 +1)->pos, (t0 +1)->len, &src1);

OPR_CTRL src; VALUE ofs;
	switch (src1.reg_type) {
	// <r64(r8..r15)>
	case R8toR15:
input_bug(1 +1 == n0)
		_push_code (THREE(0x41, 0xff
		, 0320|src1.reg
		));
		return;
	// qword ptr ___
	case ETC:
input_bug(ETC_QWORD == src1.reg)
input_bug(1 +5 <= n0)
input_bug(ID_PTR == _tok2id (_source +(t0 +2)->pos, (t0 +2)->len, NULL))
input_bug(1 == (t0 +n0 -1)->len && ']' == *(_source +(t0 +n0 -1)->pos))
s2 r
		= _opr2n (t0 +3, n0 -3, _source, &src, &ofs);
unimpl_yet(TYPE_NUM == r)
		// qword ptr [<expr>] '[' <r64(r8..r15)> +<r64>*<scale> [[+-]<expr>] ']'
		if (src.is_scaling) {
unimpl_yet(0 == ofs.number)
			if (RAXtoRDI == src.reg_type) {
				_push_code (THREE(0xff, 0x14
				, src.scale_lv<<6|src.scale_reg<<3|src.reg
				));
				return;
			}
			else {
unimpl_yet(R8toR15 == src.reg_type)
				_push_code (FOUR(0x41, 0xff, 0x14
				, src.scale_lv<<6|src.scale_reg<<3|src.reg
				));
				return;
			}
		}
		// qword ptr [<expr>] '[' <r64> [[+-]<expr>] ']'
		else {
input_bug(RAXtoRDI == src.reg_type)
unimpl_yet(-128 <= (s32)ofs.number && (s32)ofs.number <= 127)
			_push_code (THREE(0xff
			, 0120|src.reg
			, (u8)ofs.number
			));
			return;
		}
		break;
	// rel32
	case SYMNUM:
input_bug(1 +1 == n0)
		_push_rel8_capable_jmp (t0 +1, n0 -1, _source
		, REL32
		, ONE(0xe8)
		, ONE(0xe8)
		, my()->line); // call can't use REL8
		_end_appending ();
		return;
	default:
		break;
	}
unimpl_yet(0 == 1)
}

//   80:          c3  -  ret
static void _ret (const TOKEN *t1, unsigned n, const char *_source)
{
input_bug(0 == n)
	_push_code (ONE(0xc3));
}

// 3a45:          c9  -  leave
static void _leave (const TOKEN *t1, unsigned n, const char *_source)
{
input_bug(0 == n)
	_push_code (ONE(0xc9));
}

typedef enum {
	PUSH = 0,
	POP,
} PUSHPOP;
//  117:          52  -  push rdx
//  11d:          5a  -  pop rdx
//  202:       41 50  -  push r8
static void _pushpop (const TOKEN *t1, unsigned n, const char *_source, PUSHPOP op)
{
input_bug(1 == n)
OPR_CTRL src1;
	_tok2id (_source +t1->pos, t1->len, &src1);
	if (RAXtoRDI == src1.reg_type) {
		_push_code (ONE(
		  0120|op<<3|src1.reg
		));
	}
	else if (R8toR15 == src1.reg_type) {
		_push_code (TWO(0x41
		, 0120|op<<3|src1.reg
		));
	}
	else
input_bug(0)
}

static void _push (const TOKEN *t1, unsigned n, const char *_source)
{ _pushpop (t1, n, _source, PUSH); }

static void _pop (const TOKEN *t1, unsigned n, const char *_source)
{ _pushpop (t1, n, _source, POP); }


///////////////////////////////////////////////////////////////
// .equ .struct .text .globl .type .cfi_startproc .cfi_endproc
// .size

static void _equ (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(2 < n1)
input_bug(1 == (t1 +1)->len && ',' == *(_source +(t1 +1)->pos))
VALUE val; s2 r
	= _expr2n (t1 +2, n1 -2, _source, &val);
	if (! (TYPE_NUM == r))
abort_("%s undefined symbol in %s." "\n", asm_whereis(), t2s(t1, n1, _source, VTYY, T2S_ALL));
unimpl_yet(TYPE_NUM == r)
VALUE entry = {
	.type   = TYPE_NUM,
	.number = val.number,
	};
	sym_set_value (_source +t1->pos, t1->len, &entry);
}

static void _struct (const TOKEN *t1, unsigned n1, const char *_source)
{
VALUE val; s2 r
	= _expr2n (t1, n1, _source, &val);
	if (! (TYPE_NUM == r))
abort_("%s '.struct' has ambiguous value. %s." "\n", asm_whereis(), t2s(t1, n1, _source, VTYY, T2S_ALL));

	pass1()->struct_addr = val.number;
	pass1()->seg_id = STRUCTSEG;
}

static void _section (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(1 == n1)
const char *s = _source +t1->pos; unsigned n = t1->len;

// '.rodata'
	if (IDS_RODATA == idstr_from(s, n)) {
input_bug(0 == pass1()->proc_id)
		pass1()->seg_id = DSEG;

		if (0 < _sym2n (s, n, NULL))
			return;
FRAGMENT_ID frag_id; const FRAGMENT *frag
			= _get (frag_id = _ensure_appending ());
VALUE entry = {
		.type    = TYPE_OFFSET,
		.addr_id = addrs_append(frag_id, 0),
		};
		sym_set_value (s, n, &entry);
		sym_set_type (s, n, SYM_TYPE_d);
		return;
	}

input_bug(0 == 1)
}

static void _text (const TOKEN *t1, unsigned n1, const char *_source)
{
const char *s = str_from_ids(IDS_TEXT); unsigned n = 0;

input_bug(0 == pass1()->proc_id)
	pass1()->seg_id = CSEG;

	if (0 < _sym2n (s, n, NULL))
		return;
FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());
VALUE entry = {
	.type    = TYPE_OFFSET,
	.addr_id = addrs_append(frag_id, 0),
	};
	sym_set_value (s, n, &entry);
	sym_set_type (s, n, SYM_TYPE_d);
}

static void _globl (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(1 == n1)
input_bug(0 < t1->len)
	sym_set_bind (_source +t1->pos, t1->len, SYM_BIND_g);
}

static void _type (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(4 == n1)
input_bug(1 == (t1 +1)->len && ',' == _source[(t1 +1)->pos])
input_bug(1 == (t1 +2)->len && '@' == _source[(t1 +2)->pos])
u8 attr = 0;
	if (8 == (t1 +3)->len && 0 == memcmp ("function", _source +(t1 +3)->pos, 8))
		attr = SYM_TYPE_F;
	else if (6 == (t1 +3)->len && 0 == memcmp ("object", _source +(t1 +3)->pos, 6))
		attr = SYM_TYPE_O;
	else {
input_bug(0 == 1)
	}
	sym_set_type (_source +t1->pos, t1->len, attr);
}

static void _cfi_startproc ()
{
FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());

ADDR_ID addr_id
	= addrs_append (frag_id, frag->obj_len);
input_bug(0 == pass1()->proc_id)
	pass1()->proc_id = procs_append (addr_id);
}

static void _cfi_endproc ()
{
FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());

ADDR_ID addr_id
	= addrs_append (frag_id, frag->obj_len);
input_bug(0 < pass1()->proc_id)
	procs_set_endaddr (pass1()->proc_id, addr_id);
	pass1()->proc_id = 0;
}

static u4 _cfa_reg_id_from (const char *s, unsigned n)
{
static const u4 err = 8;
OPR_CTRL r; OPR id
	= _tok2id (s, n, &r);
	if (! (RAXtoRDI == r.reg_type))
		return err;

	switch (id) {
	case ID_RBP: return 6;
	case ID_RSP: return 7;
//	case ID_RAX: case ID_RCX: case ID_RDX: case ID_RBX:
//	case ID_RSI: case RDI:
	default:
		break;
	}
	return err;
}

#define DIGIT "0123456789"
static void _cfi_offset (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(4 == n1)
input_bug(1 == (t1 +1)->len && ',' == _source[(t1 +1)->pos])
input_bug(1 == (t1 +2)->len && '-' == _source[(t1 +2)->pos])
	// <r64>
u4 reg
	= _cfa_reg_id_from (_source +t1->pos, t1->len);
input_bug(reg < 8)
	// <imm8>
const char *s; unsigned n;
	s = m2s(_source +(t1 +3)->pos, n = (t1 +3)->len);
input_bug(n == strspn (s, DIGIT))
int ofs
	= atoi (s);
input_bug(0 <= ofs && ofs <= 0xff)

//input_bug(pass1()->is_cfi_startproc) // TODO: remove
	_push_cfi (TWO(0x80 +reg, ofs / 8));
}

static void _cfi_restore (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(1 == n1)
	// <r64>
u4 reg
	= _cfa_reg_id_from (_source +t1->pos, t1->len);
input_bug(reg < 8)

//input_bug(pass1()->is_cfi_startproc) // TODO: remove
	_push_cfi (ONE(0xc0 +reg));
}

static void _cfi_def_cfa_offset (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(1 == n1)
	// <imm8>
const char *s; unsigned n;
	s = m2s(_source +t1->pos, n = t1->len);
input_bug(n == strspn (s, DIGIT))
int ofs
	= atoi (s);
input_bug(0 <= ofs && ofs <= 0xff)

//input_bug(pass1()->is_cfi_startproc) // TODO: remove
	_push_cfi (TWO(0x0e, ofs));
}

static void _cfi_def_cfa_register (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(1 == n1)
	// <r64>
u4 reg
	= _cfa_reg_id_from (_source +t1->pos, t1->len);
input_bug(reg < 8)

//input_bug(pass1()->is_cfi_startproc) // TODO: remove
	_push_cfi (TWO(0x0d, reg));
}

static void _size (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(2 < n1)
input_bug(1 == (t1 +1)->len && ',' == _source[(t1 +1)->pos])

FRAGMENT_ID frag_id; const FRAGMENT *frag
	= _get (frag_id = _ensure_appending ());
unsigned sym_id
	= _sym2n (_source +t1->pos, t1->len, NULL);
flow_bug(0 < sym_id)

REFTBL_OPR symtab32 = {
	.type    = FN_LEN,
	.addr_id = addrs_append (frag_id, frag->obj_len),
	.sym_id  = sym_id,
};
	_append_reftbl (&symtab32, t1 +2, n1 -2, _source);
}

static void _byte (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(0 < n1)
const TOKEN *p = t1, *end = t1 +n1;
	while (p < end) {
		// separetes tokens by ','
const TOKEN *q
		= _tokchr (p, end -p, ',', _source);
		if (NULL == q)
			q = end;

		// calculates expression
VALUE val; s2 r
		= _expr2n (p, (unsigned)(q -p), _source, &val);
		if (! (TYPE_NUM == r))
abort_("%s '.byte' includes ambiguous value. %s." "\n", asm_whereis(), t2s(p, (unsigned)(q -p), _source, VTYY, T2S_ALL));

		// registers a byte code
		_push_code ((u8)val.number, 1);

		p = q +1;
	}
}

static void _long (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(0 < n1)
const TOKEN *p = t1, *end = t1 +n1;
	while (p < end) {
		// separetes tokens by ','
const TOKEN *q;
		for (q = p; q < end; ++q)
			if (1 == q->len && ',' == _source[q->pos])
				break;
input_bug(p < q)

		// calculates expression
VALUE val; s2 r
		= _expr2n (p, (unsigned)(q -p), _source, &val);
		if (! (TYPE_NUM == r))
abort_("%s '.long' includes ambiguous value. %s." "\n", asm_whereis(), t2s(p, (unsigned)(q -p), _source, VTYY, T2S_ALL));

		// registers dword code
		_push_code (val.number, 4);

		p = q +1;
	}
}

static void _quad (const TOKEN *t1, unsigned n1, const char *_source)
{
input_bug(! (STRUCTSEG == pass1()->seg_id))
input_bug(0 < n1)
const TOKEN *p = t1, *end = t1 +n1;
	while (p < end) {
		// separeting tokens by ','
const TOKEN *q;
		for (q = p; q < end; ++q)
			if (1 == q->len && ',' == _source[q->pos])
				break;
input_bug(p < q)

		// calculating expression
VALUE val; s2 r
		= _expr2n (p, (unsigned)(q -p), _source, &val);
		// registering qword code
		if (TYPE_NUM == r)
			_push_code (val.number, 8);
		// putting off resolving expression
		else {
flow_bug(CALC_ANY_REPLACED_ZERO == r)
FRAGMENT_ID frag_id; const FRAGMENT *frag
			= _get (frag_id = _ensure_appending ());

			_push_code (0x0000000000000000ULL, 8);
REFTBL_OPR opr64 = {
			.type    = REL64,
			.addr_id = addrs_append (frag_id, frag->obj_len),
			.sym_id  = 0,
};
			_append_reftbl (&opr64, p, (unsigned)(q -p), _source);
		}

		p = q +1;
	}
}

///////////////////////////////////////////////////////////////

typedef enum {
	PSE_ID_EQU = 1,
	PSE_ID_STRUCT,
	PSE_ID_SECTION,
	PSE_ID_TEXT,
	PSE_ID_GLOBL,
	PSE_ID_TYPE,
	PSE_ID_CFI_STARTPROC,
	PSE_ID_CFI_ENDPROC,
	PSE_ID_CFI_OFFSET,
	PSE_ID_CFI_RESTORE,
	PSE_ID_CFI_DEF_CFA_OFFSET,
	PSE_ID_CFI_DEF_CFA_REGISTER,
	PSE_ID_SIZE,
	PSE_ID_BYTE,
	PSE_ID_LONG,
	PSE_ID_QUAD,

	ID_MOV,
	ID_MOVZX,
	ID_MOVSX,
	ID_LEA,
	ID_XCHG,
	ID_INC,
	ID_DEC,
	ID_ADD,
	ID_OR,
	ID_ADC,
	ID_SBB,
	ID_AND,
	ID_SUB,
	ID_XOR,
	ID_CMP,
	ID_TEST,
	ID_CLC,
	ID_STC,
	ID_SAHF,
	ID_LAHF,
	ID_NOT,
	ID_JMP,
	ID_JO,
	ID_JNO,
	ID_JC,
	ID_JNC,
	ID_JZ,
	ID_JNZ,
	ID_JNA,
	ID_JA,
	ID_CALL,
	ID_RET,
	ID_LEAVE,
	ID_PUSH,
	ID_POP,
	ID_NOP,
	ID_SETO,
	ID_SETNO,
	ID_SETC,
	ID_SETNC,
	ID_SETZ,
	ID_SETNZ,
	ID_SETNA,
	ID_SETA,
	ID_ROL,
	ID_ROR,
	ID_RCL,
	ID_RCR,
	ID_SHL,
	ID_SHR,
	ID_SAL,
	ID_SAR,
} MNE;

static MNE _pseudo_instruction (const char *s, unsigned n)
{
	switch (n) {
	default:
		break;
	case 3:
		     if (0 == memcmp ("equ", s, n)) return PSE_ID_EQU;
		break;
	case 4:
		     if (0 == memcmp ("text", s, n)) return PSE_ID_TEXT;
		else if (0 == memcmp ("type", s, n)) return PSE_ID_TYPE;
		else if (0 == memcmp ("size", s, n)) return PSE_ID_SIZE;
		else if (0 == memcmp ("byte", s, n)) return PSE_ID_BYTE;
		else if (0 == memcmp ("long", s, n)) return PSE_ID_LONG;
		else if (0 == memcmp ("quad", s, n)) return PSE_ID_QUAD;
		break;
	case 5:
		     if (0 == memcmp ("globl", s, n)) return PSE_ID_GLOBL;
		break;
	case 6:
		     if (0 == memcmp ("struct", s, n)) return PSE_ID_STRUCT;
		break;
	case 7:
		     if (0 == memcmp ("section", s, n)) return PSE_ID_SECTION;
		break;
	case 10:
		     if (0 == memcmp ("cfi_offset", s, n)) return PSE_ID_CFI_OFFSET;
		break;
	case 11:
		     if (0 == memcmp ("cfi_endproc", s, n)) return PSE_ID_CFI_ENDPROC;
		else if (0 == memcmp ("cfi_restore", s, n)) return PSE_ID_CFI_RESTORE;
		break;
	case 13:
		     if (0 == memcmp ("cfi_startproc", s, n)) return PSE_ID_CFI_STARTPROC;
		break;
	case 18:
		     if (0 == memcmp ("cfi_def_cfa_offset", s, n)) return PSE_ID_CFI_DEF_CFA_OFFSET;
		break;
	case 20:
		     if (0 == memcmp ("cfi_def_cfa_register", s, n)) return PSE_ID_CFI_DEF_CFA_REGISTER;
		break;
	}
	return 0;
}

static MNE _x86_64_instruction (const char *s, unsigned n)
{
	switch (n) {
	case 5:
		     if (0 == memcmp ("movzx", s, n)) return ID_MOVZX;
		else if (0 == memcmp ("movsx", s, n)) return ID_MOVSX;
		else if (0 == memcmp ("setno", s, n)) return ID_SETNO;
		else if (0 == memcmp ("setnc", s, n)) return ID_SETNC;
		else if (0 == memcmp ("setnz", s, n)) return ID_SETNZ;
		else if (0 == memcmp ("setna", s, n)) return ID_SETNA;
		else if (0 == memcmp ("leave", s, n)) return ID_LEAVE;
		break;
	case 4:
		     if (0 == memcmp ("call", s, n)) return ID_CALL;
		else if (0 == memcmp ("push", s, n)) return ID_PUSH;
		else if (0 == memcmp ("xchg", s, n)) return ID_XCHG;
		else if (0 == memcmp ("test", s, n)) return ID_TEST;
		else if (0 == memcmp ("seto", s, n)) return ID_SETO;
		else if (0 == memcmp ("setc", s, n)) return ID_SETC;
		else if (0 == memcmp ("setz", s, n)) return ID_SETZ;
		else if (0 == memcmp ("seta", s, n)) return ID_SETA;
		else if (0 == memcmp ("sahf", s, n)) return ID_SAHF;
		else if (0 == memcmp ("lahf", s, n)) return ID_LAHF;
		break;
	case 3:
		     if (0 == memcmp ("mov", s, n)) return ID_MOV;
		else if (0 == memcmp ("lea", s, n)) return ID_LEA;
		else if (0 == memcmp ("inc", s, n)) return ID_INC;
		else if (0 == memcmp ("dec", s, n)) return ID_DEC;
		else if (0 == memcmp ("add", s, n)) return ID_ADD;
		else if (0 == memcmp ("adc", s, n)) return ID_ADC;
		else if (0 == memcmp ("sbb", s, n)) return ID_SBB;
		else if (0 == memcmp ("and", s, n)) return ID_AND;
		else if (0 == memcmp ("sub", s, n)) return ID_SUB;
		else if (0 == memcmp ("xor", s, n)) return ID_XOR;
		else if (0 == memcmp ("cmp", s, n)) return ID_CMP;
		else if (0 == memcmp ("clc", s, n)) return ID_CLC;
		else if (0 == memcmp ("stc", s, n)) return ID_STC;
		else if (0 == memcmp ("not", s, n)) return ID_NOT;
		else if (0 == memcmp ("jmp", s, n)) return ID_JMP;
		else if (0 == memcmp ("jno", s, n)) return ID_JNO;
		else if (0 == memcmp ("jnc", s, n)) return ID_JNC;
		else if (0 == memcmp ("jnz", s, n)) return ID_JNZ;
		else if (0 == memcmp ("jna", s, n)) return ID_JNA;
		else if (0 == memcmp ("ret", s, n)) return ID_RET;
		else if (0 == memcmp ("pop", s, n)) return ID_POP;
		else if (0 == memcmp ("nop", s, n)) return ID_NOP;
		else if (0 == memcmp ("rol", s, n)) return ID_ROL;
		else if (0 == memcmp ("ror", s, n)) return ID_ROR;
		else if (0 == memcmp ("rcl", s, n)) return ID_RCL;
		else if (0 == memcmp ("rcr", s, n)) return ID_RCR;
		else if (0 == memcmp ("shl", s, n)) return ID_SHL;
		else if (0 == memcmp ("shr", s, n)) return ID_SHR;
		else if (0 == memcmp ("sal", s, n)) return ID_SAL;
		else if (0 == memcmp ("sar", s, n)) return ID_SAR;
		break;
	case 2:
		     if (0 == memcmp ("or", s, n)) return ID_OR;
		else if (0 == memcmp ("jo", s, n)) return ID_JO;
		else if (0 == memcmp ("jc", s, n)) return ID_JC;
		else if (0 == memcmp ("jz", s, n)) return ID_JZ;
		else if (0 == memcmp ("ja", s, n)) return ID_JA;
		break;
	default:
		break;
	}
	return 0;
}

///////////////////////////////////////////////////////////////

static void _assemble_pass1 (const TOKEN *t0, unsigned n, const char *_source)
{
	if (1 < n && 1 == (t0 +1)->len && ':' == _source[(t0 +1)->pos]) {
		_label (_source +t0->pos, t0->len);
		return;
	}
flow_bug(0 < n && 0 < t0->len)
MNE id;
	if ('.' == _source[t0->pos])
		id = _pseudo_instruction (_source +t0->pos +1, t0->len -1);
	else
		id = _x86_64_instruction (_source +t0->pos, t0->len);
	if (! (0 < id))
abort_("%s unknown mnemonic: " VTRR "%s" VTO "\n", asm_whereis(), m2s(_source +t0->pos, t0->len));
	switch (id) {
	case ID_MOV  :   _mov (t0 +1, n -1, _source); break;
	case ID_MOVZX: _movzx (t0 +1, n -1, _source); break;
	case ID_MOVSX: _movsx (t0 +1, n -1, _source); break;
	case ID_LEA  :   _lea (t0 +1, n -1, _source); break;
	case ID_XCHG :  _xchg (t0 +1, n -1, _source); break;
	case ID_INC  :   _inc (t0 +1, n -1, _source); break;
	case ID_DEC  :   _dec (t0 +1, n -1, _source); break;
	case ID_ADD  :   _add (t0 +1, n -1, _source); break;
	case ID_OR   :   _or  (t0 +1, n -1, _source); break;
	case ID_ADC  :   _adc (t0 +1, n -1, _source); break;
	case ID_SBB  :   _sbb (t0 +1, n -1, _source); break;
	case ID_AND  :   _and (t0 +1, n -1, _source); break;
	case ID_SUB  :   _sub (t0 +1, n -1, _source); break;
	case ID_XOR  :   _xor (t0 +1, n -1, _source); break;
	case ID_CMP  :   _cmp (t0 +1, n -1, _source); break;
	case ID_TEST :  _test (t0 +1, n -1, _source); break;
	case ID_CLC  :   _clc (t0 +1, n -1, _source); break;
	case ID_STC  :   _stc (t0 +1, n -1, _source); break;
	case ID_SAHF :  _sahf (t0 +1, n -1, _source); break;
	case ID_LAHF :  _lahf (t0 +1, n -1, _source); break;
	case ID_NOT  :   _not (t0 +1, n -1, _source); break;
	case ID_JMP  :   _jmp (t0 +0, n -0, _source); break;
	case ID_JO   :   _jcc (t0 +0, n -0, _source, CC_O ); break;
	case ID_JNO  :   _jcc (t0 +0, n -0, _source, CC_NO); break;
	case ID_JC   :   _jcc (t0 +0, n -0, _source, CC_C ); break;
	case ID_JNC  :   _jcc (t0 +0, n -0, _source, CC_NC); break;
	case ID_JZ   :   _jcc (t0 +0, n -0, _source, CC_Z ); break;
	case ID_JNZ  :   _jcc (t0 +0, n -0, _source, CC_NZ); break;
	case ID_JNA  :   _jcc (t0 +0, n -0, _source, CC_NA); break;
	case ID_JA   :   _jcc (t0 +0, n -0, _source, CC_A ); break;
	case ID_CALL :  _call (t0 +0, n -0, _source); break;
	case ID_RET  :   _ret (t0 +1, n -1, _source); break;
	case ID_LEAVE: _leave (t0 +1, n -1, _source); break;
	case ID_PUSH :  _push (t0 +1, n -1, _source); break;
	case ID_POP  :   _pop (t0 +1, n -1, _source); break;
	case ID_NOP  :   _nop (t0 +1, n -1, _source); break;
	case ID_SETO : _setcc (t0 +1, n -1, _source, CC_O ); break;
	case ID_SETNO: _setcc (t0 +1, n -1, _source, CC_NO); break;
	case ID_SETC : _setcc (t0 +1, n -1, _source, CC_C ); break;
	case ID_SETNC: _setcc (t0 +1, n -1, _source, CC_NC); break;
	case ID_SETZ : _setcc (t0 +1, n -1, _source, CC_Z ); break;
	case ID_SETNZ: _setcc (t0 +1, n -1, _source, CC_NZ); break;
	case ID_SETNA: _setcc (t0 +1, n -1, _source, CC_NA); break;
	case ID_SETA : _setcc (t0 +1, n -1, _source, CC_A ); break;
	case ID_ROL  :   _rol (t0 +1, n -1, _source); break;
	case ID_ROR  :   _ror (t0 +1, n -1, _source); break;
	case ID_RCL  :   _rcl (t0 +1, n -1, _source); break;
	case ID_RCR  :   _rcr (t0 +1, n -1, _source); break;
	case ID_SHL  :   _shl (t0 +1, n -1, _source); break;
	case ID_SHR  :   _shr (t0 +1, n -1, _source); break;
	case ID_SAL  :   _sal (t0 +1, n -1, _source); break;
	case ID_SAR  :   _sar (t0 +1, n -1, _source); break;

	case PSE_ID_EQU                 :                    _equ (t0 +1, n -1, _source); break;
	case PSE_ID_STRUCT              :                 _struct (t0 +1, n -1, _source); break;
	case PSE_ID_SECTION             :                _section (t0 +1, n -1, _source); break;
	case PSE_ID_TEXT                :                   _text (t0 +1, n -1, _source); break;
	case PSE_ID_GLOBL               :                  _globl (t0 +1, n -1, _source); break;
	case PSE_ID_TYPE                :                   _type (t0 +1, n -1, _source); break;
	case PSE_ID_CFI_STARTPROC       :          _cfi_startproc ()                    ; break;
	case PSE_ID_CFI_ENDPROC         :            _cfi_endproc ()                    ; break;
	case PSE_ID_CFI_OFFSET          :             _cfi_offset (t0 +1, n -1, _source); break;
	case PSE_ID_CFI_RESTORE         :            _cfi_restore (t0 +1, n -1, _source); break;
	case PSE_ID_CFI_DEF_CFA_OFFSET  :     _cfi_def_cfa_offset (t0 +1, n -1, _source); break;
	case PSE_ID_CFI_DEF_CFA_REGISTER:   _cfi_def_cfa_register (t0 +1, n -1, _source); break;
	case PSE_ID_SIZE                :                   _size (t0 +1, n -1, _source); break;
	case PSE_ID_BYTE                :                   _byte (t0 +1, n -1, _source); break;
	case PSE_ID_LONG                :                   _long (t0 +1, n -1, _source); break;
	case PSE_ID_QUAD                :                   _quad (t0 +1, n -1, _source); break;
	default:
unimpl_yet(0)
		break;
	}
}

///////////////////////////////////////////////////////////////

static void _pass1 (const TOKEN *t0, unsigned n, const char *_source)
{
	++my()->line;
	if (! (0 < n && 0 < t0->len))
		return; // unexpected tokens {pos=<any>, len=0}

	_assemble_pass1 (t0, n, _source);
}

///////////////////////////////////////////////////////////////
// I/F for Python

static struct {
	SEGMENT_ID seg_id;
	IDS secstr_id;
} s_segtbl[] = {
	{ CSEG, IDS_TEXT },
	{ DSEG, IDS_RODATA }
};

static unsigned s_sec_id_from[SEGMENT_ID_MAX +1] = {0}; // PENDING: -> pass2()
static const unsigned *sec_id_from_make (const IDS *secstr_id_from, unsigned len)
{
caller_bug(secstr_id_from)
caller_bug(0 < len) // empty list not allowed
unsigned n;
	for (n = 0; n < arraycountof(s_segtbl); ++n) {
flow_bug(s_segtbl[n].seg_id <= SEGMENT_ID_MAX)
unsigned secidx;
		for (secidx = 0; secidx < len; ++secidx)
			if (secstr_id_from[secidx] == s_segtbl[n].secstr_id) {
				s_sec_id_from[s_segtbl[n].seg_id] = secidx;
				break;
			}
	}
	return s_sec_id_from; // TODO: -> pass2()
}
static unsigned sec_id_from(SEGMENT_ID seg_id)
{ return s_sec_id_from[seg_id]; }

///////////////////////////////////////////////////////////////
// (pass 2.1 / 2.4)

static FRAGMENT_POS _fragment_addr_from_expr2n (const TOKEN *t1,
                                                FRAGMENT_ID *_id)
{
flow_bug(t1)
unsigned n1; const char *_source
	= _get_tokens (t1, &n1);

unimpl_yet(1 == n1)
VALUE val; bool found
	= (0 < _sym2n (_source +t1->pos, t1->len, &val));
flow_bug(found)
flow_bug(TYPE_OFFSET == val.type)
flow_bug(!addrs_get(val.addr_id)->is_resolved)

const ADDR *ad
	= addrs_get(val.addr_id);
flow_bug(!ad->is_resolved)
	if (_id)
		*_id = ad->frag_id;
	return ad->frag_ofs;
}

static SEGMENT_POS _segment_addr_from_opr2n (const TOKEN *t1,
                                             SEGMENT_ID *_id)
{
flow_bug(t1)
unsigned n1; const char *_source
	= _get_tokens (t1, &n1);
SEGMENT_ID seg_id; SEGMENT_POS seg_ofs;
	// with offset? '<label>[expr1][expr2]...'
	if (3 <= n1 && 1 == t1[n1 -1].len && ']' == _source[t1[n1 -1].pos]) {
OPR_CTRL dst; s2 r
		= _opr2seg (t1, n1, _source, &dst, &seg_ofs, &seg_id);
flow_bug(TYPE_OFFSET == r && (CSEG == seg_id || DSEG == seg_id))
unimpl_yet(RIP == dst.reg_type)
	}
	// without offset? '<label>'
	else {
unimpl_yet(1 == n1)
VALUE val; bool found
		= (0 < _sym2n (_source +t1->pos, t1->len, &val));
flow_bug(found)
flow_bug(TYPE_OFFSET == val.type)
const ADDR *ad
		= addrs_get(val.addr_id);
flow_bug(ad->is_resolved)
		seg_id  = ad->seg_id;
		seg_ofs = ad->seg_ofs;
	}

	if (_id)
		*_id = seg_id;
	return seg_ofs;
}

///////////////////////////////////////////////////////////////
// (pass 2.5 / 2.6)

static unsigned sym_id_from(SEGMENT_ID seg_id)
{
unsigned symtab_i = 0;
VALUE val; const char *key_name
	= sym_enum_first (&val);
	for (; key_name; key_name = sym_enum_next (&val)) {
		if (0 == memcmp (".L", key_name, 2)
		 || 0 == strcmp (".", key_name)
		)
			continue; // local or pseudo symbol
		++symtab_i; // NOTE: (*2)
		if (! (SYM_TYPE_d == val.attr))
			continue;
		if (! (TYPE_OFFSET == val.type))
			continue;
const ADDR *ad
		= addrs_get(val.addr_id);
flow_bug(ad && ad->is_resolved)
		if (! (seg_id == ad->seg_id))
			continue;
		return symtab_i;
	}
flow_bug(0 == 1) // note found
}

///////////////////////////////////////////////////////////////
// REL32/REL8 list (pass 2.1 -> 2.4)
// - all of [jmp|jcc]
// for giving obvious to 'jmp'|'jcc' size and relative distance

typedef struct {
	// copied from fragments
	struct {
		FRAGMENT_ID  frag_id;
		FRAGMENT_POS obj_len;
	} from; // where to jump from
	u32 jmp8_len    :3; // 2=(jcc)(16X <rel8>)  2=(jmp)(353 <rel8>)  5=(call)(350 <rel32>)
	u32 jmp32_len   :3; // 6=(jcc)(017 20X <rel32>)  5=(jmp)(351 <rel32>)
	u32 pad1        :32 -(3 +3);
	struct {
		FRAGMENT_ID  frag_id;
		FRAGMENT_POS frag_ofs;
	} to; // where to jump
} SHRINK_SRC;

typedef struct {
	u8 *data;
	unsigned len;
	unsigned reserved; // TODO: remove
} SECDATA;

typedef struct {
	SHRINK_SRC *rel8_capable_jmp;
	unsigned cnt;
	unsigned *i_from_frag_id;
	u32 *seg2elf;
	SECDATA secs[IDS_MAX +1];
} PASS2;
static PASS2 s_pass2 = {
	.rel8_capable_jmp = NULL, .cnt = 0,
	.i_from_frag_id = NULL,
	.seg2elf = NULL,
	.secs = {0},
};
static PASS2 *pass2() { return &s_pass2; }

///////////////////////////////////////////////////////////////
// (pass 2.1)

static FRAGMENT_ID _jmp8or32_from (SEGMENT_ID seg_id)
{
//unsigned reserved;
SHRINK_SRC *rel8_capable_jmp = NULL;

FRAGMENT_ID frag_id; const FRAGMENT *pickup
	= _enum_first (seg_id, &frag_id);
unsigned used = 0; FRAGMENT_ID max_frag_id = 0;
	for (; pickup; pickup = _enum_next (&frag_id), ++used) {
		max_frag_id = max(max_frag_id, frag_id +1);

		// new pickup (ensure memory 2^N base)
unsigned ge
		= pow2_ge(used +1);
		if (NULL == rel8_capable_jmp)
			rel8_capable_jmp = (SHRINK_SRC *)malloc (sizeof(SHRINK_SRC) * ge);
		else if (pow2_ge(used) < ge)
			rel8_capable_jmp = (SHRINK_SRC *)realloc (rel8_capable_jmp,
			                                          sizeof(SHRINK_SRC) * ge);
SHRINK_SRC *dst
		= &rel8_capable_jmp[used];

		// new pickup (copied from FRAGMENT)
		dst->from.frag_id = frag_id;
		dst->from.obj_len = pickup->obj_len;

		// nothing REL8/REL32 caused by jmp|jcc?
		if (!fragments_has_rel8_capable_jmp (frag_id)) {
			dst->jmp8_len = dst->jmp32_len = 0; // for _has_rel8_capable_jmp()
			continue;
		}
		dst->jmp8_len  = pickup->jmp8_len;
		dst->jmp32_len = pickup->jmp32_len;
	}
	if (pass2()->rel8_capable_jmp)
		free (pass2()->rel8_capable_jmp);
	pass2()->rel8_capable_jmp = rel8_capable_jmp;
	pass2()->cnt              = used;

	return max_frag_id;
}

static bool _has_rel8_capable_jmp (unsigned i_from)
{
SHRINK_SRC *jmp
	= &pass2()->rel8_capable_jmp[i_from];
	if (0 == jmp->jmp8_len || 0 == jmp->jmp32_len)
		return false;
	return true;
}

static void _jmp8or32_optimize_startup (SEGMENT_ID seg_id)
{
FRAGMENT_ID max_frag_id
	= _jmp8or32_from (seg_id);

	// tiny parse (only handing jmp|jcc <label>)
unsigned i_from;
	for (i_from = 0; i_from < pass2()->cnt; ++i_from) {
		if (!_has_rel8_capable_jmp (i_from))
			continue;
SHRINK_SRC *jmp
		= &pass2()->rel8_capable_jmp[i_from];
const FRAGMENT *pickup
		= _get(jmp->from.frag_id);
		jmp->to.frag_ofs = _fragment_addr_from_expr2n (pickup->to.expr, &jmp->to.frag_id);
	}

	// prepare reverse-lookup
unsigned *lookup
	= (unsigned *)malloc (max_frag_id * sizeof(unsigned));
//unsigned i_from;
	for (i_from = 0; i_from < pass2()->cnt; ++i_from) {
SHRINK_SRC *jmp
		= &pass2()->rel8_capable_jmp[i_from];
		lookup[jmp->from.frag_id] = i_from;
	}
	if (pass2()->i_from_frag_id)
		free (pass2()->i_from_frag_id);
	pass2()->i_from_frag_id = lookup;
}

static void _cseg_calc_distance (u12 start,
                                 u12 end,
                                 unsigned *_total_min,
                                 unsigned *_total_max)
{
unsigned total_min = 0, total_max = 0;
const SHRINK_SRC *p, *end_ptr;
	p       = &pass2()->rel8_capable_jmp[start];
	end_ptr = &pass2()->rel8_capable_jmp[end];

	for (; p < end_ptr; ++p) {
		total_min += p->from.obj_len;
		total_max += p->from.obj_len;
		switch (_get(p->from.frag_id)->ofs_type) {
		case REL32:
			total_min += p->jmp32_len;
			total_max += p->jmp32_len;
			break;
		case REL8or32:
			total_min += p->jmp8_len;
			total_max += p->jmp32_len;
			break;
	//	case REL8:
		default:
			total_min += p->jmp8_len;
			total_max += p->jmp8_len;
			break;
		}
	}

	if (_total_min)
		*_total_min = total_min;
	if (_total_max)
		*_total_max = total_max;
}

static void _jmp8or32_optimize ()
{
	// reducing ambiguity as possible loop by loop
unsigned before, after = pass2()->cnt; // ambiguity count
	do {
		before = after, after = 0;
unsigned i_from;
		for (i_from = 0; i_from < pass2()->cnt; ++i_from) {
			// nothing REL8/REL32 caused by jmp|jcc?
			if (!_has_rel8_capable_jmp (i_from))
				continue;
SHRINK_SRC *jmp
			= &pass2()->rel8_capable_jmp[i_from];

			// still resolved?
			if (! (REL8or32 == _get(jmp->from.frag_id)->ofs_type))
				continue;

u1 min_need_rel32 = FALSE, max_need_rel32 = FALSE;
			// minus jump
			if (jmp->to.frag_id <= i_from) {
flow_bug(! (jmp->to.frag_id == i_from && jmp->from.obj_len < jmp->to.frag_ofs)) // NOTE: (*1)
unsigned i_to
				= pass2()->i_from_frag_id[jmp->to.frag_id];
unsigned min_, max_;
				_cseg_calc_distance (i_to, i_from, &min_, &max_);
				if ((s32)(jmp->to.frag_ofs -(min_ +jmp->from.obj_len +jmp->jmp8_len)) < -128)
					min_need_rel32 = TRUE;
				if ((s32)(jmp->to.frag_ofs -(max_ +jmp->from.obj_len +jmp->jmp8_len)) < -128)
					max_need_rel32 = TRUE;
				// Does unresolved ambiguities affect REL8/REL32?
				if (! (min_need_rel32 == max_need_rel32)) {
					++after;
					continue; // can't decide REL8/REL32 (expecting next loop)
				}
				fragments_set_ofs_type (jmp->from.frag_id, (min_need_rel32) ? REL32 : REL8);
				continue;
			}
			// plus jump NOTE: (*1)
			else /*if (i_from < jmp->to.frag_id)*/ {
flow_bug(jmp->to.frag_id <= pass2()->cnt)
unsigned i_to
				= pass2()->i_from_frag_id[jmp->to.frag_id];
unsigned min_, max_;
				_cseg_calc_distance (i_from +1, i_to, &min_, &max_);
				if (127 < (s32)(min_ +jmp->to.frag_ofs))
					min_need_rel32 = TRUE;
				if (127 < (s32)(max_ +jmp->to.frag_ofs))
					max_need_rel32 = TRUE;
				// Does unresolved ambiguities affect REL8/REL32?
				if (! (min_need_rel32 == max_need_rel32)) {
					++after;
					continue; // can't decide REL8/REL32 (expecting next loop)
				}
				fragments_set_ofs_type (jmp->from.frag_id, (min_need_rel32) ? REL32 : REL8);
			}
		}
	} while (after < before);
flow_bug(after == before)
}

// deciding offset (handling an ambiguity that was given up as REL32)
static void _jmp8or32_to_reftbl ()
{
unsigned next_seg_addr, i_from;
	for (next_seg_addr = 0, i_from = 0; i_from < pass2()->cnt; ++i_from) {
SHRINK_SRC *jmp
		= &pass2()->rel8_capable_jmp[i_from];
		// assign an offset address to each fragment
		fragments_set_seg_addr (jmp->from.frag_id, next_seg_addr);
		next_seg_addr += jmp->from.obj_len;

		// fragment ended with REL8/REL32 jmp|jcc?
		if (_has_rel8_capable_jmp (i_from)) {
			if (REL8 == _get(jmp->from.frag_id)->ofs_type) {
REFTBL_OPR opr8 = {
				.type    = REL8,
				.addr_id = addrs_append (jmp->from.frag_id,
				                         jmp->from.obj_len +jmp->jmp8_len),
				.sym_id  = 0,
};
				_append_reftbl (&opr8, _get(jmp->from.frag_id)->to.expr, 0, NULL);
				next_seg_addr += jmp->jmp8_len;
			}
			else {
REFTBL_OPR opr32 = {
				.type    = REL32,
				.addr_id = addrs_append (jmp->from.frag_id,
				                         jmp->from.obj_len +jmp->jmp32_len),
				.sym_id  = 0,
};
				_append_reftbl (&opr32, _get(jmp->from.frag_id)->to.expr, 0, NULL);
				next_seg_addr += jmp->jmp32_len;
			}
		}
	}
}

///////////////////////////////////////////////////////////////
// (pass 2.2)

static unsigned _seg_commit (IDS secstr_id)
{
caller_bug(secstr_id <= IDS_MAX)
SECDATA *seg
	= &pass2()->secs[secstr_id];
flow_bug(NULL == seg->data && 0 == seg->len)

unsigned reserved, commit_len, i_from;
	for (commit_len = i_from = 0; i_from < pass2()->cnt; ++i_from) {
SHRINK_SRC *jmp
		= &pass2()->rel8_capable_jmp[i_from];

		// ensure memory for codes (2^N base)
unsigned needs
		= commit_len +jmp->from.obj_len +jmp->jmp32_len;
		if (NULL == seg->data) {
			seg->data = (u8 *)malloc (pow2_ge(needs));
			reserved = pow2_ge(needs);
		}
		if (pow2_ge(commit_len) < pow2_ge(needs)) {
			seg->data = (u8 *)realloc (seg->data, pow2_ge(needs));
			reserved = pow2_ge(needs);
		}
		// output codes except for (REL8/REL32 jcc|jmp)
		if (0 < jmp->from.obj_len) {
coding_bug(commit_len <= reserved)
			memcpy (seg->data +commit_len,
			        _get(jmp->from.frag_id)->obj,
			        jmp->from.obj_len);
			commit_len += jmp->from.obj_len;
		}
		// output codes of (REL8/REL32 jcc|jmp) (*)REL8/REL32=0 yet
		if (_has_rel8_capable_jmp (i_from)) {
flow_bug(commit_len < reserved)
unsigned obj_len
			= _write_rel8_capable_jmp (jmp->from.frag_id,
                                    seg->data +commit_len,
                                    reserved -commit_len);
flow_bug(0 < obj_len) // PENDING: error handling (not enough memory)
			commit_len += obj_len;
		}
	}
	seg->len = commit_len;
	return commit_len;
}

///////////////////////////////////////////////////////////////
// (pass 2.4)

static void _resolve_fn_len ()
{
	// SECDATA * <- secstr_id <- seg_id (*)readability, expandable
struct {
	SECDATA *codes;
	SECDATA *relocs;
} section_from[SEGMENT_ID_MAX +1] = {0};
	section_from[CSEG].codes  = &pass2()->secs[IDS_TEXT       ];
	section_from[DSEG].codes  = &pass2()->secs[IDS_RODATA     ];
	section_from[CSEG].relocs = &pass2()->secs[IDS_RELA_TEXT  ];
	section_from[DSEG].relocs = &pass2()->secs[IDS_RELA_RODATA];

const REFTBL *p = _enum_first_reftbl (); unsigned i = 0;
	for (; p; p = _enum_next_reftbl (), ++i) {
		if (! (FN_LEN == p->opr.type))
			continue;
const ADDR *ad
		= addrs_get(p->opr.addr_id);
flow_bug(ad->is_resolved)

		// '.size' of '.type=@function' for '.symtab' '.eh_frame' ...
VALUE entry = {
		.type    = TYPE_OFFSET,
		.addr_id = p->opr.addr_id,
		};
		sym_set_value (".", 1, &entry);

VALUE val; const char *sym_name
		= sym_id2val (p->opr.sym_id, &val);
flow_bug(TYPE_OFFSET == val.type)
flow_bug(addrs_get(val.addr_id)->is_resolved)

unsigned n1; const char *_source
		= _get_tokens (p->ref.expr, &n1);
VALUE fn_len; s2 r
		= _expr2n (p->ref.expr, n1, _source, &fn_len);
input_bug(TYPE_NUM == r)
		sym_set_fn_len (p->opr.sym_id, fn_len.number);
	}

PROC_ID proc_id; const PROC *proc
	= procs_enum_first (&proc_id);
	for (; proc; proc = procs_enum_next (&proc_id)) {
const ADDR *st, *en;
		st = addrs_get(proc->start_id);
		en = addrs_get(proc->end_id);
flow_bug(st->is_resolved && en->is_resolved)
input_bug(st->seg_id == en->seg_id)
unsigned fn_len
		= en->seg_ofs - st->seg_ofs;
		procs_set_fn_len (proc_id, fn_len);
	}
}

///////////////////////////////////////////////////////////////
// (pass 2.5)

#define CFA_NOP     0x00
#define CFA_DEF_CFA 0x0c
#define CFA_OFFSET  0x80
#define CFA_RSP     7
#define CFA_RIP     16

// NOTE: (*3)
static void _resolve_cfi ()
{
SECDATA *codes, *relocs;
	codes  = &pass2()->secs[IDS_EH_FRAME     ];
	relocs = &pass2()->secs[IDS_RELA_EH_FRAME];
flow_bug(0 == codes->len && NULL == codes->data)
flow_bug(0 == relocs->len && NULL == relocs->data)

SYMBOL_ID sym_id_from_[SEGMENT_ID_MAX +1];
	sym_id_from_[CSEG] = sym_id_from(CSEG);
	sym_id_from_[DSEG] = sym_id_from(DSEG);

	// '.eh_frame' header
	{
static const u8 HDR[] = {
		  1                            // Version:
		, 'z', 'R', '\0'               // Augmentation:
		, 1                            // Code align factor:  1
		, 0x7f & -8                    // Data align factor:  -8
		, 16                           // Return addr column: 16
		, 1, 0x1b                      // Augmentation data:  1b
		, CFA_DEF_CFA, CFA_RSP, 8
		, CFA_OFFSET +CFA_RIP, -8 / -8
};
unsigned PAD_LEN, PACKED_LEN;
		PAD_LEN = PADDING(4, usizeof(HDR));
		PACKED_LEN = 4 +4 +usizeof(HDR) +PAD_LEN;

		// ensure memory
		codes->data = (u8 *)malloc (pow2_ge(PACKED_LEN)); // PENDING: free()
		codes->len = PACKED_LEN;
u8 *p
		= codes->data;

		// header size
		store_le32 (p, PACKED_LEN -4);
		// position
		store_le32 (p +4, 0x00000000); // SPEC: this position = 4, but 0
		// header description
		memcpy (p +8, HDR, sizeof(HDR));
		if (0 < PAD_LEN)
			memset (p +8 +sizeof(HDR), CFA_NOP, PAD_LEN);
	}

	// '.eh_frame' '.rela.eh_frame'
unsigned cfa_max = 0; u8 *cfa = (u8 *)malloc (cfa_max +1);
const PROC *proc
	= procs_enum_first (NULL);
	for (; proc; proc = procs_enum_next (NULL)) {
unsigned total = 0;
		cfa[total++] = 0x00;
const ADDR *st
		= addrs_get(proc->start_id);
		if (0 < proc->cfi_len) {
const ADDR *en
			= addrs_get(proc->end_id);
flow_bug(st->is_resolved && en->is_resolved)
SEGMENT_POS prev_ofs = st->seg_ofs; unsigned i;
			for (i = 0; i < proc->cfi_len; ++i) {
const ADDR *a
				= addrs_get(proc->cfi[i].addr_id);
flow_bug(a->is_resolved)
flow_bug(st->seg_id == a->seg_id)
unsigned m
				= pow2_ge(total +2 +proc->cfi[i].codes_len);
				if (pow2_ge(cfa_max +1) < m)
					cfa = (u8 *)realloc (cfa, m), cfa_max = m -1;

				// advance
				if (prev_ofs < a->seg_ofs) {
SEGMENT_POS delta
					= (SEGMENT_POS)(a->seg_ofs -prev_ofs);
					if (delta < 0x80 -0x40)
						cfa[total++] = 0x40 +(u8)delta;
					else {
unimpl_yet(delta < 0x100)
						cfa[total++] = 0x02;
						cfa[total++] = (u8)delta;
					}
				}
				// cfa body
flow_bug(1 == proc->cfi[i].codes_len || 2 == proc->cfi[i].codes_len)
				memcpy (cfa +total, &proc->cfi[i].codes, proc->cfi[i].codes_len);
				// next
				total += proc->cfi[i].codes_len;
				prev_ofs = a->seg_ofs;
			}
		}

unsigned m;
		// '.rela.eh_frame'
		m = pow2_ge(relocs->len +0x18);
		if (pow2_ge(relocs->len) < m)
			relocs->data = (u8 *)realloc (relocs->data, m);
u8 *r
		= relocs->data +relocs->len;
		store_le64 (r +0x00, codes->len +4 +4);
		store_le32 (r +0x08, 2); // R_X86_64_PC32
		store_le32 (r +0x0c, sym_id_from_[st->seg_id]);
		store_le64 (r +0x10, (u64)st->seg_ofs);
		relocs->len += 0x18;

		// '.eh_frame'
unsigned pad; u8 *p;
		pad = PADDING(4, total);
		// ensure memory
		m = pow2_ge(codes->len +4 +12 +total +pad);
		if (pow2_ge(codes->len) < m)
			codes->data = (u8 *)realloc (codes->data, m);

		p = codes->data +codes->len;
		store_le32 (p, 12 +total +pad);
		codes->len += 4, p += 4;
		store_le32 (p +0, codes->len); // this position
		store_le32 (p +4, 0x00000000); // for RELOC
		store_le32 (p +8, proc->codes_len); // '.cfi_startproc' to '.cfi_endproc'
spec_bug(0 < total)
		memcpy (p +12, cfa, total);
		if (0 < pad)
			memset (p +12 +total, CFA_NOP, pad);
		codes->len += 12 +total +pad;
	}
	free (cfa);
}

///////////////////////////////////////////////////////////////
// (pass 2.6)

// RELOCATION RECORDS <- reftbl[], fragments[]
static void _resolve_reftbl ()
{
	// SECDATA * <- secstr_id <- seg_id (*)readability, expandable
struct {
	SECDATA *codes;
	SECDATA *relocs;
} section_from[SEGMENT_ID_MAX +1] = {0};
	section_from[CSEG].codes  = &pass2()->secs[IDS_TEXT       ];
	section_from[DSEG].codes  = &pass2()->secs[IDS_RODATA     ];
	section_from[CSEG].relocs = &pass2()->secs[IDS_RELA_TEXT  ];
	section_from[DSEG].relocs = &pass2()->secs[IDS_RELA_RODATA];

const REFTBL *p = _enum_first_reftbl (); unsigned i = 0;
	for (; p; p = _enum_next_reftbl (), ++i) {
		if (FN_LEN == p->opr.type)
			continue;

const ADDR *from
		= addrs_get(p->opr.addr_id);
flow_bug(from->is_resolved)

		// entry for '.text' '.rodata' ... (segment binary)
flow_bug(pass2()->seg2elf)
SEGMENT_ID to_seg_id; SEGMENT_POS to_seg_ofs
		= _segment_addr_from_opr2n (p->ref.expr, &to_seg_id);
		// over segments: '.quad'(REL64) or 'jcc|jmp|call'(REL32)
		if (! (to_seg_id == from->seg_id)) {
flow_bug(REL32 == p->opr.type || REL64 == p->opr.type)
			// RELOCATION RECORDS '.rela.text' '.rela.rodata' ...
SECDATA *relocs
			= section_from[from->seg_id].relocs;
			// ensuring memory (2^N base)
unsigned ge
			= pow2_ge(relocs->len +0x18);
			if (NULL == relocs->data)
				relocs->data = (u8 *)malloc (ge);
			else if (pow2_ge(relocs->len) < ge)
				relocs->data = (u8 *)realloc (relocs->data, ge);
			// writing
u8 *r
			= relocs->data +relocs->len;
			memset (r, 0, 0x18);
unsigned sym_id
			= sym_id_from(to_seg_id); // syms# of '.text', '.rodata', ...
			if (REL64 == p->opr.type) {
				store_le64 (r +0x00, from->seg_ofs -8);
				store_le32 (r +0x08, 1); // R_X86_64_64
				store_le32 (r +0x0c, sym_id);
				store_le32 (r +0x10, to_seg_ofs);
			}
			else if (REL32 == p->opr.type) {
				store_le64 (r +0x00, from->seg_ofs -4);
				store_le32 (r +0x08, 2); // R_X86_64_PC32
				store_le32 (r +0x0c, sym_id);
				store_le64 (r +0x10, (u64)to_seg_ofs -4);
			}
			relocs->len += 0x18;
			continue;
		}

u32 rel32
		= to_seg_ofs -from->seg_ofs;
overflow_bug(from->seg_id <= SEGMENT_ID_MAX)
SECDATA *codes
		= section_from[from->seg_id].codes;
overflow_bug(from->seg_ofs <= codes->len)

		// within segments: 'jcc|jmp|call'(REL8)
		if (REL8 == p->opr.type) {
flow_bug(-128 <= (s32)rel32 && (s32)rel32 <= 127)
			*(codes->data +from->seg_ofs -1) = (u8)rel32;
		}
		// within segments: 'jcc|jmp|call'(REL32)
		else /*if (REL32 == p->opr.type)*/ {
flow_bug(REL32 == p->opr.type)
			store_le32 (codes->data +from->seg_ofs -4, rel32);
		}
	}
}

///////////////////////////////////////////////////////////////
// (pass2)

static void _pass2 (const char *outpath)
{
unsigned seg2elf_reserved = 0;
unsigned segs_len = 0;
unsigned i;
	for (i = 0; i < arraycountof(s_segtbl); ++i) {
		// 2.1
		_jmp8or32_optimize_startup (s_segtbl[i].seg_id);
		_jmp8or32_optimize ();
		_jmp8or32_to_reftbl ();

		// 2.2
		// ensuring memory (2^N base)
unsigned ge
		= pow2_ge(s_segtbl[i].seg_id +1);
		if (NULL == pass2()->seg2elf) {
			pass2()->seg2elf = (u32 *)malloc (sizeof(u32) * ge);
			seg2elf_reserved = ge;
		}
		else if (seg2elf_reserved < ge) {
			pass2()->seg2elf = (u32 *)realloc (pass2()->seg2elf,
			                                   sizeof(u32) * ge);
			seg2elf_reserved = ge;
		}
		// appending
		pass2()->seg2elf[s_segtbl[i].seg_id] = segs_len;

		segs_len += _seg_commit (s_segtbl[i].secstr_id);
	}

	// 2.3 _pass2_resolve_address_of_symbols
	addrs_resolve_all ();

	// 2.4
	_resolve_fn_len ();

	// 2.5
	_resolve_cfi (); // NOTE: (*3)

	// 2.6
	_resolve_reftbl ();
}

///////////////////////////////////////////////////////////////
// fragments/sym/reftbl -> sections

static void _symtab_strtab_ctor ()
{
unsigned symtab_len
	= (1 +sym_get_count ()) * 0x18;
unsigned strtab_len = 0, symtab_i = 0;

u8 *heap
	= (u8 *)malloc (pow2_ge(symtab_len +strtab_len +1));
	heap[symtab_len +strtab_len++] = '\0';
	memset (heap +0x18 * symtab_i++, 0, 0x18);

	// '.symtab' '.strtab'
VALUE val; const char *key_name
	= sym_enum_first (&val);
	for (; key_name; key_name = sym_enum_next (&val)) {
		if (0 == memcmp (".L", key_name, 2)
		 || 0 == strcmp (".", key_name)
		)
			continue; // local or pseudo symbol
u8 attr; u16 sec_id; u32 value, fn_len;
		attr   = val.attr; // .global .type (b7654: 1=g  b3210: 2=F 3=d 4=df)
		fn_len = val.fn_len;
		if (TYPE_OFFSET == val.type) {
flow_bug(addrs_get(val.addr_id))
const ADDR *ad
			= addrs_get(val.addr_id);
flow_bug(ad && ad->is_resolved)
			sec_id = sec_id_from(ad->seg_id);
			value  = ad->seg_ofs;
		}
		else /*if (TYPE_NUM == val.type)*/ {
			sec_id = 0xfff1; // *ABS*
			value  = val.number;
		}
u8 *sym
		= heap +0x18 * symtab_i;
		memset (sym, 0, 0x18);
u16 name_ofs
		= (SYM_TYPE_d == val.attr) ? 0x00 : strtab_len;
		store_le16 (sym +0x00, name_ofs);
		sym[0x04] = attr;
		store_le16 (sym +0x06, sec_id);
		store_le32 (sym +0x08, value);  // .equ | <label offset>
		store_le32 (sym +0x10, fn_len); // .size
		++symtab_i;

		if ('.' == key_name[0])
			continue; // give nothing to '.strtab': '.text' '.rodata' ...
unsigned key_len
		= ustrlen (key_name);
		// ensuring memory (2^N base)
unsigned now
		= symtab_len +strtab_len;
		if (pow2_ge(now) < pow2_ge(now +key_len +1))
			heap = (u8 *)realloc (heap, pow2_ge(now +key_len +1));
		// appending
		memcpy (heap +symtab_len +strtab_len, key_name, key_len +1);
		strtab_len += key_len +1;
	}
	// truncate waste space
	if (0x18 * symtab_i < symtab_len) {
		memmove (heap +0x18 * symtab_i, heap +symtab_len, strtab_len);
		symtab_len = symtab_i * 0x18;
	}

SECDATA *a, *b;
	a = &pass2()->secs[IDS_SYMTAB];
	b = &pass2()->secs[IDS_STRTAB];
	a->data = heap            , a->len = symtab_len;
	b->data = heap +symtab_len, b->len = strtab_len;
}

///////////////////////////////////////////////////////////////
// sections (static)

static void _comment_ctor ()
{
static const char comm[]
	= "\0" VERSION_STRING;
SECDATA *a
	= &pass2()->secs[IDS_COMMENT];
	a->data = (u8 *)comm, a->len = arraycountof(comm), a->reserved = 0;
}

///////////////////////////////////////////////////////////////
//  sections (specification not made sure)

static void _note_gnu_property_ctor ()
{
static u8 prop[32] = {0};
	// initialize needed?
	if (! (4 == prop[0])) {
		store_le32 (prop +0x00, 4);
		store_le32 (prop +0x04, 16);
		store_le32 (prop +0x08, 5);
		store_le32 (prop +0x0c, 'G'|'N'<<8|'U'<<16);
		store_le32 (prop +0x10, 0xC0000002U);
		store_le32 (prop +0x14, 4);
		store_le32 (prop +0x18, 3);
		store_le32 (prop +0x1c, 0);
	}

SECDATA *a
	= &pass2()->secs[IDS_NOTE_GNU_PROPERTY];
	a->data = (u8 *)prop, a->len = arraycountof(prop), a->reserved = 0;
}

///////////////////////////////////////////////////////////////
// library interface

extern void as_pass1_new (const char *filename)
{
	_reset_fragments ();
	_reset_sym ();
	_reset_reftbl ();
	pass1()->proc_id = 0; // safety

VALUE entry = {
	.type   = TYPE_NUM,
	.number = 0
	};
	sym_set_value (filename, ustrlen(filename), &entry);
	sym_set_type (filename, ustrlen(filename), SYM_TYPE_df);
}

extern void as_pass1_x86_64_intel (const TOKEN *t0, unsigned n, const char *_source)
{
	_pass1 (t0, n, _source);
}

extern void as_pass2 (const IDS *secstr_id_from, unsigned len)
{
	// is the end of fragment not closed?
	_push_end ();

caller_bug(secstr_id_from)
	sec_id_from_make (secstr_id_from, len);
	_pass2 ("a.out");
}

extern const void *as_pass2_result (const char *secname, u32 *_len)
{
IDS secstr_id
	= idstr_from(secname, 0);
	if (0 == secstr_id)
		return NULL;
caller_bug(secstr_id < IDS_MAX +1)

	// initialize needed?
	if (NULL == pass2()->secs[secstr_id].data) {
		switch (secstr_id) {
		case IDS_COMMENT:
			_comment_ctor ();
			break;
		case IDS_NOTE_GNU_PROPERTY:
			_note_gnu_property_ctor ();
			break;
		case IDS_SYMTAB:
		case IDS_STRTAB:
			_symtab_strtab_ctor ();
			break;
		default:
flow_bug(0 == 1)
			return NULL;
		}
	}

SECDATA *r
	= &pass2()->secs[secstr_id];
	if (_len)
		*_len = r->len;
	return r->data;
}

/*
NOTE:    (*1) 
PENDING: (*2) Expression to return segment offset doesn't work correctly yet.
              That's happened only when calc_expr2n() will be called with
              expression that has resolved symbol. ref)_parse_token() */
#include "asm/calc.h"

#include "as-.h" // TOKEN FALSE TRUE
#include "asm/sym.h"
#include "helper/mtoi.h"
#include "helper/memtoul.h"
#include "helper/memspn.h"
#include "helper/vt100.h"
#define ASSERT_PREFIX_FUNC asm_whereis()
#include "helper/assert.h"
#define  input_bug assert
#define unimpl_yet assert
#define caller_bug assert
#define   flow_bug assert
#define unexpected assert
#include "helper/m2s.h"
#define giveup_calc(tok, len, func) \
	abort_("%s giveup: '" VTRR "%s" VTO "' [%s]" "\n", asm_whereis(), m2s(tok, len), func)

#include <ctype.h> // isdigit
#include <memory.h>
#include <alloca.h>

typedef uint32_t u32;
typedef  uint8_t u8, u1, u2, u3, u4;
typedef   int8_t s2, s3;

#define min(a,b) ((a) < (b) ? (a) : (b))
#define usizeof(x) (unsigned)sizeof(x)
#define arraycountof(x) (sizeof(x)/sizeof(x[0]))
///////////////////////////////////////////////////////////////

// export
#define _tok2id   calc_tok2id
//#define _expr2n   calc_expr2n
#define _opr2n    calc_opr2n
#define _opr2seg  calc_opr2seg

// import
#define _sym2n         sym_lookup
#define TYPE_NUM    VALUE_TYPE_NUM
#define TYPE_OFFSET VALUE_TYPE_OFFSET
#define TYPE_BITS   VALUE_TYPE_BITS

#define BINARY_OP_LEN1 "*+-/|"

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

typedef enum {
	CALC_NUM     = 0,
	CALC_SEGMENT = 1,
} VALUE0_TYPE;

typedef struct {
	union {
		uint32_t number;
		struct {
			SEGMENT_ID  seg_id;
			SEGMENT_POS seg_ofs;
		};
	};
	VALUE0_TYPE type;
} VALUE0;

typedef struct {
	u32 bin_op;
	u32 una_op;
	VALUE0 val;
} STACK;

typedef enum {
	ACCEPT_UNARY_OP_OR_VALUE = 0,
	ACCEPT_VALUE,
	ACCEPT_BINARY_OP,
//	ROUND_BRACKETS,
} STATE;

typedef struct {
	STACK stack[8];
	unsigned depth;
	unsigned unclosed_bracket;
	STATE state;
	u1 is_any_replaced_zero;
} PUSHABLE;
static PUSHABLE s_singlton = {0};
static PUSHABLE *my() { return &s_singlton; }
static STACK *top() { return &my()->stack[my()->depth]; }

#define calc my // readability

///////////////////////////////////////////////////////////////
// stack operation

// for passing by value char[4] as uint32_t
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define     ONE(b1)         ((b1) | '\0' << 8 | '\0' << 16 | '\0' << 24)
#define     TWO(b1, b2)     ((b1) | (b2) << 8 | '\0' << 16 | '\0' << 24)
#define   THREE(b1, b2, b3) ((b1) | (b2) << 8 | (b3) << 16 | '\0' << 24)
#endif

///////////////////////////////////////////////////////////////
// readability

#define DIGIT "0123456789"
#define UHEX  "ABCDEF"
#define LHEX  "abcdef"
#define UPPER UHEX "GHIJKLMNOPQRSTUVWXYZ"
#define LOWER LHEX "ghijklmnopqrstuvwxyz"

///////////////////////////////////////////////////////////////

typedef enum {
	NOERR = 0,
	UNRESOLVED_SYMBOL = -1,
	UNDEF_SYMBOL = -2,
	INVALID_CHAR = -3,
	SEGMENT_CANNOT_UNARY_OP = -4,
} ERR;

static ERR _parse_token (const char *s, unsigned n, VALUE0 *_val)
{
caller_bug(0 < n)
caller_bug(_val)

char c = s[0];
	// hex decimal
	if (2 < n && '0' == c && 'x' == s[1]) {
		if (! (n -2 == memspn (s +2, n -2, DIGIT UHEX LHEX)))
abort_("'%s': cannot be accepted as hex decimal." "\n", m2s(s +2, n -2));
		_val->type   = CALC_NUM;
		_val->number = (u32)memtoul (s +2, n -2, NULL, 16);
		return NOERR;
	}
	// decimal
	if (strchr ("123456789", c) || 1 == n && '0' == c) {
		if (! (1 == n || n -1 == memspn (s +1, n -1, DIGIT)))
abort_("'%s': cannot be accepted as decimal." "\n", m2s(s, n));
		_val->type   = CALC_NUM;
		_val->number = (u32)mtoi (s, n);
		return NOERR;
	}

	// symbol
	if (isalpha (c) || strchr ("._", c)) {
VALUE val;
		if (! (0 < _sym2n (s, n, &val)))
			return UNDEF_SYMBOL; // undefined symbol
flow_bug(TYPE_NUM == val.type || TYPE_OFFSET == val.type)
		if (TYPE_NUM == val.type) {
			_val->type   = CALC_NUM;
			_val->number = val.number;
		}
		else /*if (TYPE_OFFSET == val.type)*/ {
const ADDR *ad
			= addrs_get(val.addr_id);
flow_bug(ad)
			if (!ad->is_resolved)
				return UNRESOLVED_SYMBOL;
			_val->type    = CALC_SEGMENT;
			_val->seg_id  = ad->seg_id ;
			_val->seg_ofs = ad->seg_ofs;
		}
		return NOERR;
	}

	return INVALID_CHAR; // invalid character
}

static u8 _prior(u32 bin_op)
{
	switch (bin_op) {
	case ONE('\0'):                              return 0; break;
	case ONE('('):                               return 1; break;
	case ONE('|'):                               return 2; break;
	case TWO('<', '<'): case TWO('>', '>'):      return 3; break;
	case ONE('+'): case ONE('-'):                return 4; break;
	case ONE('*'): case ONE('/'): case ONE('%'): return 5; break;
	default:
unimpl_yet(0 == 1)
	}
}

static void _pop_stack (u32 *_bin_op, VALUE0 *_val)
{
flow_bug(0 < calc()->depth || 0 < top()->bin_op)

STACK old
	= *top();

	if (0 < calc()->depth)
		--calc()->depth;

	if (_bin_op)
		*_bin_op = old.bin_op;
	if (_val)
		*_val = old.val;
}

static void _push_stack (u32 bin_op)
{
flow_bug(0 < bin_op || 0 == calc()->depth)
	if (0 < bin_op) {
unimpl_yet(calc()->depth +1 < arraycountof(calc()->stack))
		++calc()->depth;
	}
	top()->bin_op = bin_op;
	top()->una_op = 0;
	top()->val.type   = CALC_NUM; // safety
	top()->val.number = 0; // safety

	calc()->state = ACCEPT_UNARY_OP_OR_VALUE;
}

static void _unary_op (u32 una_op, VALUE0 *_val)
{
caller_bug(_val)
	if (CALC_SEGMENT == _val->type) {
input_bug(ONE('+') == una_op || 0 == una_op)
		return;
	}

caller_bug(CALC_NUM == _val->type)
	switch (una_op) {
	case ONE('-'):             _val->number = -_val->number; break;
	case ONE('+'): case 0:                                   break;
	case THREE('n', 'o', 't'): _val->number = ~_val->number; break;
	default:
unimpl_yet(0 == 1)
	}
}

// <val(num|sym)>|'('
static ERR _on_value (const char *s, unsigned n)
{
caller_bug(0 < n)
char c = s[0];
	if ('(' == c && 1 == n) {
		_push_stack (ONE('(')); // NOTE: (*1)
		++calc()->unclosed_bracket;
		return NOERR;
	}
unimpl_yet(strchr ("." DIGIT UPPER "_" LOWER, c))
VALUE0 found; ERR err
	= _parse_token (s, n, &found);

	if (UNRESOLVED_SYMBOL == err || UNDEF_SYMBOL == err) {
		found.number = 0x00000000;
		found.type   = CALC_NUM;
		calc()->is_any_replaced_zero = TRUE;
	}
	else if (! (NOERR == err))
		return err;

	if (CALC_NUM == found.type)
		_unary_op (top()->una_op, &found);
	else /*if (CALC_SEGMENT == found.type)*/ {
		if (! (0 == top()->una_op
		    || ONE('+') == top()->una_op))
			return SEGMENT_CANNOT_UNARY_OP;
	}
	top()->val      = found;
	top()->una_op   = 0;
	calc()->state   = ACCEPT_BINARY_OP;

	return NOERR;
}

// <unary op>|<val(num|sym)>|'('
static ERR _on_unary_op_or_value (const char *s, unsigned n)
{
caller_bug(0 < n)
u32 una_op = 0;
	switch (n) {
	case 1:
		if (strchr ("+-", s[0]))
			una_op = ONE(s[0]);
		break;
	case 3:
		if (0 == memcmp ("not", s, 3))
			una_op = THREE(s[0], s[1], s[2]);
		break;
	}
	if (una_op) {
		top()->una_op = una_op;
		calc()->state = ACCEPT_VALUE;
		return NOERR;
	}
	return _on_value (s, n);
}

static void _binary_op (VALUE0 *_val1, u32 bin_op, const VALUE0 *_val2)
{
caller_bug(_val1 && _val2)

	if (CALC_SEGMENT == _val1->type
	 && CALC_SEGMENT == _val2->type) {
input_bug(ONE('-') == bin_op)
input_bug(_val1->seg_id == _val2->seg_id)
		_val1->type   = CALC_NUM;
		_val1->number = _val1->seg_ofs - _val2->seg_ofs;
		return;
	}
	else if (CALC_SEGMENT == _val1->type) {
input_bug(ONE('+') == bin_op)
flow_bug(CALC_NUM == _val2->type)
		_val1->seg_ofs += _val2->number;
		return;
	}
	else if (CALC_SEGMENT == _val2->type) {
input_bug(ONE('+') == bin_op)
flow_bug(CALC_NUM == _val1->type)
		_val1->seg_ofs = _val1->number + _val2->seg_ofs;
		_val1->type    =  CALC_SEGMENT;
		_val1->seg_id  =  _val2->seg_id;
		return;
	}

flow_bug(CALC_NUM == _val1->type && CALC_NUM == _val2->type)
	switch (bin_op) {
	case ONE('+'): _val1->number += _val2->number; break;
	case ONE('-'): _val1->number -= _val2->number; break;
	case ONE('*'): _val1->number *= _val2->number; break;
	case ONE('/'):
		if (0 == _val2->number)
abort_ ("%s division by zero" "\n", asm_whereis());
		_val1->number /= _val2->number; break;
	case TWO('<', '<'): _val1->number <<= _val2->number; break;
	case TWO('>', '>'): _val1->number >>= _val2->number; break;
	case ONE('|'): _val1->number |= _val2->number; break;
	default:
unimpl_yet(0 == 1)
	}
}

static void _reduce_stack (u32 bin_op, const VALUE0 *_val2)
{
VALUE0 *_val1 = &top()->val;
	_binary_op (_val1, bin_op, _val2);
}

// <binary op>|')'
static void _on_binary_op (const char *s, unsigned n)
{
caller_bug(0 < n)
	if (1 == n && ')' == s[0]) {
flow_bug(0 < calc()->unclosed_bracket)
		--calc()->unclosed_bracket;
		// reduce until '('
u32 old_op = 0;
		while (0 < calc()->depth) {
u32 old_una_op; VALUE0 val2;
			_pop_stack (&old_op, &val2);
			if (ONE('(') == old_op) {
flow_bug(CALC_NUM == top()->val.type)
flow_bug(0 == top()->val.number)
				_unary_op (top()->una_op, &val2);
				top()->val = val2; // NOTE: (*1)
				top()->una_op = 0;
				break;
			}
			_reduce_stack (old_op, &val2);
		}
input_bug(ONE('(') == old_op)
		return;
	}

	// accept an operator
input_bug(1 == n || 2 == n)
u32 new_op
	= (1 == n) ? ONE(s[0])
	: (2 == n) ? TWO(s[0], s[1])
	: ONE('\0')
	;
unimpl_yet(1 == n && strchr (BINARY_OP_LEN1, s[0])
        || TWO('<', '<') == new_op
        || TWO('>', '>') == new_op)

	// push or reduce?
u32 old_op = top()->bin_op;
	if (_prior(old_op) < _prior(new_op)) {
		_push_stack (new_op);
	}
	// reduce
	else {
VALUE0 val2;
		_pop_stack (/*&old_op*/ NULL, &val2);
		_reduce_stack (old_op, &val2);
		_push_stack (new_op);
	}
}

static ERR _expr2n (const TOKEN *t0, unsigned n, const char *_source, VALUE0 *_val)
{
	calc()->depth = 0;
	top()->bin_op = 0;
	_push_stack (ONE('\0'));

const TOKEN *tok;
	for (tok = t0; tok < t0 +n; ++tok) {
ERR err = NOERR;
		switch (calc()->state) {
		case ACCEPT_UNARY_OP_OR_VALUE:
			err = _on_unary_op_or_value (_source +tok->pos, tok->len);
			break;
		case ACCEPT_VALUE:
			err = _on_value (_source +tok->pos, tok->len);
			break;
		case ACCEPT_BINARY_OP:
			_on_binary_op (_source +tok->pos, tok->len);
			break;
		default:
giveup_calc(_source +tok->pos, tok->len, __func__);
		}
		if (! (NOERR == err))
			return err;
	}
input_bug(ACCEPT_BINARY_OP == calc()->state)
	// reduce (all)
	while (0 < calc()->depth) {
u32 old_op; VALUE0 val2;
		_pop_stack (&old_op, &val2);
input_bug(! (ONE('(') == old_op))
		_reduce_stack (old_op, &val2);
	}
input_bug(ONE('\0') == top()->bin_op)
	if (_val)
		*_val = top()->val;
	return NOERR;
}
s2 calc_expr2n (const TOKEN *t0, unsigned n, const char *_source, VALUE *_val)
{
	calc()->is_any_replaced_zero = FALSE;

VALUE0 val; ERR err
	= _expr2n (t0, n, _source, &val);
	if (! (NOERR == err))
		return CALC_ABORT;

flow_bug(CALC_NUM == val.type || CALC_SEGMENT == val.type)
u2 ret;
	if (CALC_NUM == val.type)
		ret = TYPE_NUM;
	else /*if (CALC_SEGMENT == val.type)*/
		ret = TYPE_OFFSET;
	if (_val) {
		_val->type = ret;
		if (CALC_NUM == val.type)
			_val->number = val.number;
		else /*if (CALC_SEGMENT == val.type)*/ {
#if 0 // PENDING: (*2)
			_val->segment_id  = val.seg_id ;
			_val->segment_ofs = val.seg_ofs;
#else
unimpl_yet(0 == 1)
#endif
		}
	}
	if (calc()->is_any_replaced_zero)
		return CALC_ANY_REPLACED_ZERO;
flow_bug((TYPE_NUM == ret || TYPE_OFFSET == ret) && ret < 2)
	return (s2)ret;
}

///////////////////////////////////////////////////////////////

typedef struct {
	const char *name;
	OPR id;
	u3 reg_type;
	OPR base_id;
} LOOKUP;

static const LOOKUP g_tok2id_5[] = {
	{ "qword", ID_QWORD, ETC, ID_BYTE },
	{ NULL, 0, UNK, 0 },
};

static const LOOKUP g_tok2id_4[] = {
	{ "byte", ID_BYTE, ETC, ID_BYTE },
	{ "word", ID_WORD, ETC, ID_BYTE },
	{ "r10b", ID_R10B, R8BtoR15B, ID_R8B },
	{ "r11b", ID_R11B, R8BtoR15B, ID_R8B },
	{ "r12b", ID_R12B, R8BtoR15B, ID_R8B },
	{ "r13b", ID_R13B, R8BtoR15B, ID_R8B },
	{ "r14b", ID_R14B, R8BtoR15B, ID_R8B },
	{ "r15b", ID_R15B, R8BtoR15B, ID_R8B },
	{ NULL, 0, UNK, 0 },
};

static const LOOKUP g_tok2id_3[] = {
	{ "ptr", ID_PTR, ETC, ID_BYTE },
	{ "rax", ID_RAX, RAXtoRDI, ID_RAX },
	{ "rcx", ID_RCX, RAXtoRDI, ID_RAX },
	{ "rdx", ID_RDX, RAXtoRDI, ID_RAX },
	{ "rbx", ID_RBX, RAXtoRDI, ID_RAX },
	{ "rsp", ID_RSP, RAXtoRDI, ID_RAX },
	{ "rbp", ID_RBP, RAXtoRDI, ID_RAX },
	{ "rsi", ID_RSI, RAXtoRDI, ID_RAX },
	{ "rdi", ID_RDI, RAXtoRDI, ID_RAX },
	{ "rip", ID_RIP, RIP, 0 },
	{ "eax", ID_EAX, EAXtoEDI, ID_EAX },
	{ "ecx", ID_ECX, EAXtoEDI, ID_EAX },
	{ "edx", ID_EDX, EAXtoEDI, ID_EAX },
	{ "ebx", ID_EBX, EAXtoEDI, ID_EAX },
	{ "esp", ID_ESP, EAXtoEDI, ID_EAX },
	{ "ebp", ID_EBP, EAXtoEDI, ID_EAX },
	{ "esi", ID_ESI, EAXtoEDI, ID_EAX },
	{ "edi", ID_EDI, EAXtoEDI, ID_EAX },
	{ "spl", ID_SPL, SPLtoDIL, ID_SPL -4 },
	{ "bpl", ID_BPL, SPLtoDIL, ID_SPL -4 },
	{ "sil", ID_SIL, SPLtoDIL, ID_SPL -4 },
	{ "dil", ID_DIL, SPLtoDIL, ID_SPL -4 },
	{ "r10", ID_R10, R8toR15, ID_R8  },
	{ "r11", ID_R11, R8toR15, ID_R8  },
	{ "r12", ID_R12, R8toR15, ID_R8  },
	{ "r13", ID_R13, R8toR15, ID_R8  },
	{ "r14", ID_R14, R8toR15, ID_R8  },
	{ "r15", ID_R15, R8toR15, ID_R8  },
	{ "r8b", ID_R8B, R8BtoR15B, ID_R8B },
	{ "r9b", ID_R9B, R8BtoR15B, ID_R8B },
	{ "r8w", ID_R8W, R8WtoR15W, ID_R8W },
	{ NULL, 0, UNK, 0 },
};

static const LOOKUP g_tok2id_2[] = {
	{ "al", ID_AL, ALtoBH, ID_AL },
	{ "cl", ID_CL, ALtoBH, ID_AL },
	{ "dl", ID_DL, ALtoBH, ID_AL },
	{ "bl", ID_BL, ALtoBH, ID_AL },
	{ "ah", ID_AH, ALtoBH, ID_AL },
	{ "ch", ID_CH, ALtoBH, ID_AL },
	{ "dh", ID_DH, ALtoBH, ID_AL },
	{ "bh", ID_BH, ALtoBH, ID_AL },
	{ "ax", ID_AX, AXtoDI, ID_AX },
	{ "cx", ID_CX, AXtoDI, ID_AX },
	{ "dx", ID_DX, AXtoDI, ID_AX },
	{ "bx", ID_BX, AXtoDI, ID_AX },
	{ "sp", ID_SP, AXtoDI, ID_AX },
	{ "bp", ID_BP, AXtoDI, ID_AX },
	{ "si", ID_SI, AXtoDI, ID_AX },
	{ "di", ID_DI, AXtoDI, ID_AX },
	{ "r8", ID_R8, R8toR15, ID_R8 },
	{ "r9", ID_R9, R8toR15, ID_R8 },
	{ NULL, 0, UNK, 0 },
};

OPR _tok2id (const char *s, unsigned n, OPR_CTRL *_ctl)
{
caller_bug(0 < n)

OPR retval = 0; u4 reg_type; u3 reg;
	if (2 < n && 0 == memcmp ("0x", s, 2))
		retval = ID_NUM, reg_type = SYMNUM;
	else if (0 < n && isdigit (s[0]))
		retval = ID_NUM, reg_type = SYMNUM;
	else {
const LOOKUP *token = NULL;
		switch (n) {
		case 5: case 4: case 3: case 2:
			token = (5 == n) ?   g_tok2id_5 :
			        (4 == n) ?   g_tok2id_4 :
			        (3 == n) ?   g_tok2id_3 :
			      /*(2 == n) ?*/ g_tok2id_2 ;
			for (; token->name; ++token)
				if (0 == memcmp (token->name, s, n)) {
					retval   = token->id;
					reg_type = token->reg_type;
					reg      = (u3)(token->id -token->base_id);
					break;
				}
			break;
		case 1:
			if (strchr ("(+-", s[0]))
				retval = ID_NUM, reg_type = SYMNUM;
			break;
		default:
			break;
		}
	}
	if (0 == retval) {
char c = s[0];
		if (strchr ("[]", c))
			retval = ID_SQUARE_BRACKET, reg_type = BRACKET, reg = SQUARE;
		else if (isalpha (c) || strchr ("._", c)) {
			if (1 == n || n -1 == memspn (s +1, n -1, "." DIGIT UPPER "_" LOWER))
				retval = ID_SYM, reg_type = SYMNUM;
		}
		else
unimpl_yet(1 == n && strchr (BINARY_OP_LEN1, c))
	}

	if (0 == retval)
		return 0;
	if (_ctl) {
		_ctl->reg_type = reg_type;
		_ctl->reg      = reg     ;
	}
	return retval;
}

// ([byte|qword] ptr) [<offset>] [<r64>[+<r64>*<scale_lv>][+<expr>]]{1,}
static s2 _opr2val (const TOKEN *t0, unsigned n0, const char *_source,
                    OPR_CTRL *_ctl, VALUE0 *_val)
{
	calc()->is_any_replaced_zero = FALSE;
VALUE0 retval = {0}; OPR_CTRL ctl = {0};
input_bug(2 < n0)

OPR reg_id = 0; const TOKEN *q;
	for (q = t0 +n0; t0 +2 < q;) {
		if (! (1 == (q -1)->len && ']' == *(_source +(q -1)->pos)))
			break;
const TOKEN *p;
		for (p = q -1; t0 < p; --p)
			if (1 == (p -1)->len && '[' == *(_source +(p -1)->pos))
				break;
input_bug(t0 < p) // '[' not found
input_bug(p < q -1) // nothing between '[' and ']'
unsigned n
		= (unsigned)(q -1 -p);
		q = p -1; // next loop

		// resolve '[' [<r64>] ... ']'
		if (0 == reg_id && (2 == p->len || 3 == p->len) && 'r' == _source[p->pos]) {
			reg_id = _tok2id (_source +p->pos, p->len, NULL);
			if (ID_RAX <= reg_id && reg_id <= ID_RDI) {
				++p, --n;
				ctl.reg = reg_id -ID_RAX;
				ctl.reg_type = RAXtoRDI;
			}
			else if (ID_R8 <= reg_id && reg_id <= ID_R15) {
				++p, --n;
				ctl.reg = reg_id -ID_R8;
				ctl.reg_type = R8toR15;
			}
			else if (ID_RIP == reg_id) {
				++p, --n;
				ctl.reg_type = RIP;
			}
			else if (ID_NUM == reg_id || ID_SYM == reg_id)
				reg_id = 0; // cancelling
			else
unimpl_yet(0 == 2)
		}
		// resolve '[' [+<r64>[*<scale_lv>]] ... ']'
		if (0 < reg_id
		 && 1 < n && 1 == p->len && '+' == _source[p->pos]) {
OPR scale_reg_id
			= _tok2id (_source +(p +1)->pos, (p +1)->len, NULL);
			switch (scale_reg_id) {
			default:
unimpl_yet(0 == 1)
			case ID_NUM: case ID_SYM:
				scale_reg_id = 0; // canceling
				break;
			case ID_RAX: case ID_RCX: case ID_RDX: case ID_RBX:
			case ID_RSP: case ID_RBP: case ID_RSI: case ID_RDI:
				p += 2, n -= 2;
				ctl.is_scaling = TRUE;
				ctl.scale_reg = scale_reg_id -ID_RAX;
				if (! (1 < n && 1 == p->len && '*' == _source[p->pos]))
					break;
VALUE0 val; ERR err
				= _parse_token (_source +(p +1)->pos, (p +1)->len, &val);
input_bug(0 == err)
unimpl_yet(CALC_NUM == val.type || CALC_SEGMENT == val.type)
u32 num
				= val.number;
input_bug(1 == num || 2 == num || 4 == num || 8 == num)
				p += 2, n -= 2;
				ctl.scale_lv = (8 == num) ? 3
				             : (4 == num) ? 2
				                          : num -1;
				break;
			}
		}
		// resolve '[' ... <expr> ']'
		if (0 < n) {
VALUE0 val; ERR err
			= _expr2n (p, n, _source, &val);
unimpl_yet(NOERR == err)
			_binary_op (&retval, ONE('+'), &val);
		}
	}
	if (t0 < q) {
VALUE0 val; ERR err
		= _expr2n (t0, (unsigned)(q -t0), _source, &val);
		if (! (NOERR == err))
			return CALC_ABORT;
		_binary_op (&retval, ONE('+'), &val);
	}
	if (ID_RIP == reg_id) {
		// TODO: especially handling
	}

	if (_ctl)
		*_ctl = ctl;
	if (_val)
		*_val = retval;
	if (calc()->is_any_replaced_zero)
		return CALC_ANY_REPLACED_ZERO;
flow_bug(CALC_NUM == retval.type || CALC_SEGMENT == retval.type)
u2 ret
	= (TYPE_NUM == retval.type) ? TYPE_NUM : TYPE_OFFSET;
flow_bug(ret < 2)
	return (s2)ret;
}
s2 _opr2n (const TOKEN *t0, unsigned n0, const char *_source,
           OPR_CTRL *_ctl, VALUE *_val)
{
VALUE0 val; s2 ret
	= _opr2val (t0, n0, _source, _ctl, &val);
	if (_val) {
flow_bug(CALC_NUM == val.type)
		_val->type   = TYPE_NUM;
		_val->number = val.number;
	}
	return ret;
}
s2 _opr2seg (const TOKEN *t0, unsigned n0, const char *_source,
             OPR_CTRL *_ctl, SEGMENT_POS *_ofs, SEGMENT_ID *_id)
{
VALUE0 val; s2 ret
	= _opr2val (t0, n0, _source, _ctl, &val);
flow_bug(TYPE_OFFSET == ret)
	if (_ofs)
		*_ofs = val.seg_ofs;
	if (_id)
		*_id = val.seg_id;
	return ret;
}

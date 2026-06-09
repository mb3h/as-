#include "as-.h"
#include "helper/assert.h"

#include <stdbool.h>
#include <ctype.h> // isdigit ...
#include <memory.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define usizeof(x) (unsigned)sizeof(x)
#define arraycountof(x) (unsigned)(sizeof(x)/sizeof(x[0]))
///////////////////////////////////////////////////////////////
// error handling

#define tiny_error(msg) assert((msg, 0))

#include "helper/m2s.h"
#define giveup_skipping(s, n, func) \
	abort_("%s '%s' [%s]" "\n", whereis(), m2s(s, n), func)

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

typedef enum {
	INITIAL = 0,
	TYPEDEF,
	STRUCT,
	CURLY_BRACKETS,
	ROUND_BRACKETS,
	DECLARE,
} STATE;

typedef struct {
	char tok1st[8];
	STATE state;
	unsigned unclosed_bracket;
} STACK;

typedef struct {
	STACK stack[4];
	unsigned depth;
} PUSHABLE;
static PUSHABLE s_singlton = {0};
static PUSHABLE *my() { return &s_singlton; }
static STACK *top() { return &my()->stack[my()->depth]; }

#define skipping my // readability

///////////////////////////////////////////////////////////////
// transparency

static const char *sp(unsigned depth)
{
// 64: maximum length, 2: parallel count
static char s_text[2][96]; // 64= 4+2 +7+8+4 +1+3*6+1+7+1+4+1 +1 +margin
static unsigned s_i = 0;
unsigned i
	= ++s_i % 2;

unsigned n; STACK *stk;
	n = skipping()->depth, stk = &skipping()->stack[depth];
	if (depth < n +1)
		sprintf (s_text[i], "%4d: " VTYY "%8s" VTO " %*s[" "%s" "%d" VTO "]"
			, masked()->line
			, stk->tok1st
			, depth * 3, ""
			, (INITIAL == stk->state) ? VTMM : VTO
			, depth
			);
	else
		*s_text[i] = '\0';

	return s_text[i];
}

///////////////////////////////////////////////////////////////

static void pop_stack ()
{
unsigned n; STACK *stk;
	n = skipping()->depth, stk = &skipping()->stack[n];

	if (0 < skipping()->depth) {
		--skipping()->depth;
		return;
	}
	top()->state = INITIAL;
	top()->tok1st[0] = '\0'; // safety
}

static void up_stack ()
{
unsigned n
	= skipping()->depth;

	if (! (n +1 < arraycountof(skipping()->stack))) {
tiny_error("cannot push_stack() (overflow).")
	}

	++skipping()->depth;
}

static void modify_stack (STATE state, const char *s, unsigned n)
{
	top()->state = state;
	top()->unclosed_bracket = 0;
	strncpy (top()->tok1st, s, min(n +1, usizeof(top()->tok1st)));
	top()->tok1st[usizeof(top()->tok1st) -1] = '\0';
}

static void push_stack (STATE state, const char *s, unsigned n)
{
	up_stack ();
	modify_stack (state, s, n);
}

static void _round_brackets (const char *s, unsigned n)
{
	if (! (1 == n))
		return;

	if ('(' == *s)
		++top()->unclosed_bracket;
	else if (')' == *s && 0 < top()->unclosed_bracket)
		--top()->unclosed_bracket;
	else if (')' == *s /*&& 0 == top()->unclosed_bracket*/)
		pop_stack ();
	else if (strchr ("*,;", *s))
		return;
	else
giveup_skipping(s, n, __func__);
}

static void _curly_brackets (const char *s, unsigned n)
{
	if (! (1 == n))
		return;
int c = *s;

	if ('{' == c)
		++top()->unclosed_bracket;
	else if ('}' == c && 0 < top()->unclosed_bracket)
		--top()->unclosed_bracket;
	else if ('}' == c /*&& 0 == top()->unclosed_bracket*/)
		pop_stack ();
	else if (strchr ("()*,;[]", c))
		return;
	else if (isdigit (c) || isalpha (c))
		return;
	else
giveup_skipping(s, n, __func__);
}

static void _declare (const char *s, unsigned n)
{
	if (! (1 == n))
		return;

	if (';' == *s)
		pop_stack ();
	else if ('(' == *s)
		push_stack (ROUND_BRACKETS, s, n);
	else
giveup_skipping(s, n, __func__);
}

static void _struct (const char *s, unsigned n)
{
	if (! (1 == n))
		return;

	if (';' == *s)
		pop_stack ();
	else if ('{' == *s)
		push_stack (CURLY_BRACKETS, s, n);
	else
giveup_skipping(s, n, __func__);
}

static void _typedef (const char *s, unsigned n)
{
	if (6 == n && 0 == memcmp ("struct", s, 6))
		modify_stack (STRUCT, s, n);
	else if (! (1 == n))
		return;
	else if (';' == *s)
		pop_stack ();
	else if ('(' == *s)
		push_stack (ROUND_BRACKETS, s, n);
	else
giveup_skipping(s, n, __func__);
}

static void _dispatch (const char *s, unsigned n)
{
	switch (n) {
	default:
		break;
	case 7:
		if (0 == memcmp ("typedef", s, n)) {
			modify_stack (TYPEDEF, s, n);
			return;
		}
		break;
	case 1:
giveup_skipping(s, n, __func__);
	}
	modify_stack (DECLARE, s, n);
}

extern bool skip_statement_C (const char *s, unsigned n)
{
	switch (top()->state) {
	case INITIAL:
		_dispatch (s, n);
		break;
	case TYPEDEF:
		_typedef (s, n);
		break;
	case STRUCT:
		_struct (s, n);
		break;
	case CURLY_BRACKETS:
		_curly_brackets (s, n);
		break;
	case ROUND_BRACKETS:
		_round_brackets (s, n);
		break;
	case DECLARE:
		_declare (s, n);
		break;
	default:
giveup_skipping(s, n, __func__);
	}
	return (INITIAL == top()->state) ? true : false;
}

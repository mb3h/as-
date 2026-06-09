#include "asm/reftbl.h"

#include "as-.h" // asm_whereis
#include "asm/fragments.h"
#include "helper/pow2_ge.h"
#define ASSERT_PREFIX_FUNC asm_whereis()
#include "helper/assert.h"
#define caller_bug assert
#define   flow_bug assert
#define  input_bug assert
#define unexpected assert

#include <stdint.h>

#include <stddef.h> // offsetof
#include <string.h>
#include <memory.h>
#include <malloc.h>

#define ustrlen(x) (unsigned)strlen(x)
#define usizeof(x) (unsigned)sizeof(x)
#define uoffsetof(x, m) (unsigned)offsetof(x, m)
///////////////////////////////////////////////////////////////
// REL32/REL8 queue (pass 2.1 -> 2.4)
// - all of [jcc|jmp|call]
// - some of [lea(RIP base)]

///////////////////////////////////////////////////////////////
// exports

#define _reset         reftbl_reset
#define _append        reftbl_append
#define _enum_first    reftbl_enum_first
#define _enum_next     reftbl_enum_next

#define FN_LEN TYPE_FN_LEN

///////////////////////////////////////////////////////////////
// imports

#define _get_tokens    fragments_get_tokens

///////////////////////////////////////////////////////////////
// constants

#define INITHEAP_TXTPOOL_MAX 0x4000 // text store (16KB)
#define MAX_FORMULA_LEN 255

static const TOKEN EOT = { .pos = 0, .len = 0 };

///////////////////////////////////////////////////////////////

typedef struct {
	REFTBL **list;
	unsigned cnt;
	// for _enum_XXX() only
	unsigned enum_i;
} SINGLTON;
static SINGLTON s_relopr_singlton = {0};
static SINGLTON *my() { return &s_relopr_singlton; }

void _reset ()
{
unsigned i;
	for (i = 0; i < my()->cnt; ++i) {
flow_bug(my()->list[i])
		free (my()->list[i]);
		my()->list[i] = NULL; // safety
	}
	my()->cnt = 0;
}

static void _register (const REFTBL_OPR *opr,
                       const TOKEN *t1, unsigned n1, const char *_source)
{
caller_bug(opr)
	// ensure memory 2^N base (list)
unsigned ge
	= pow2_ge(my()->cnt +1);
	if (NULL == my()->list)
		my()->list = (REFTBL **)malloc (sizeof(REFTBL *) * ge);
	else if (pow2_ge(my()->cnt) < ge)
		my()->list = (REFTBL **)realloc (my()->list,
		                                 sizeof(REFTBL *) * ge);
	my()->list[my()->cnt] = NULL;
	++my()->cnt;

	// ensure memory (list item)
	if (NULL == _source) {
caller_bug(0 == n1)
		_source = _get_tokens (t1, &n1);
	}
caller_bug(0 < n1)
caller_bug(memchr (_source, '\0', MAX_FORMULA_LEN +1))
unsigned required
	= uoffsetof(REFTBL, ref.expr)
	+ usizeof(TOKEN) * (n1 +1) // +1=EOT
	+ ustrlen(_source) +1      // +1='\0'
	;
REFTBL *p
	= (REFTBL *)malloc (required);

	// refering address
	memcpy (p->ref.expr, t1, sizeof(TOKEN) * n1);
	memcpy (&p->ref.expr[n1], &EOT, sizeof(TOKEN));
	strcpy ((char *)&p->ref.expr[n1 +1], _source);

	// writing address
	p->opr.type    = opr->type;
	p->opr.addr_id = opr->addr_id;
	p->opr.sym_id  = (FN_LEN == opr->type) ? opr->sym_id : 0;

	my()->list[my()->cnt -1] = p;
}
void _append (const REFTBL_OPR *opr,
              const TOKEN *t1, unsigned n1, const char *_source)
{
	_register (opr,
	           t1, n1, _source);
}

const REFTBL *_enum_next ()
{
	if (! (my()->enum_i < my()->cnt))
		return NULL;
REFTBL *next;
	if (NULL == (next = my()->list[my()->enum_i]))
		return NULL;
	++my()->enum_i;
	return next;
}
const REFTBL *_enum_first ()
{
	my()->enum_i = 0;
	return _enum_next ();
}

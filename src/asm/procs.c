#include "asm/procs.h"

#include "as-.h" // dbg
#include "helper/pow2_ge.h"
#include "helper/assert.h"
#define caller_bug assert
#define   flow_bug assert

#include <memory.h>
#include <malloc.h>

typedef uint32_t u32;
typedef  uint8_t u2;
typedef uint16_t u16;

#ifndef max
# define max(a, b) ((a) > (b) ? (a) : (b))
#endif
///////////////////////////////////////////////////////////////
// exports

#define _append        procs_append
#define _push_cfi      procs_push_cfi
#define _set_endaddr   procs_set_endaddr
#define _set_fn_len    procs_set_fn_len
#define _get           procs_get
#define _enum_first    procs_enum_first
#define _enum_next     procs_enum_next

#define UNSET SEGMENT_POS_UNSET

#define ID_ORIGIN 1

///////////////////////////////////////////////////////////////
// imports

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

typedef struct {
	PROC *begin;
	unsigned len;
	// for _enum_XXX() only
	unsigned enum_id;
} SINGLTON;
static SINGLTON s_singlton = {0};
static SINGLTON *procs() { return &s_singlton; }

static void _reset_array ()
{
	procs()->len = 0;
	if (procs()->begin)
		free (procs()->begin);
	procs()->begin = NULL;
}

///////////////////////////////////////////////////////////////
// interface (reading only)

const PROC *_get (PROC_ID proc_id)
{
flow_bug(procs()->begin)
caller_bug(ID_ORIGIN <= proc_id)
	if (! (proc_id < procs()->len))
		return NULL;

PROC *proc
	= &procs()->begin[proc_id];
	return proc;
}

const PROC *_enum_next (PROC_ID *_proc_id)
{
flow_bug(procs()->begin)
	if (! (procs()->enum_id < procs()->len))
		return NULL;

	if (_proc_id)
		*_proc_id = procs()->enum_id;
PROC *proc
	= &procs()->begin[procs()->enum_id++];
	return proc;
}
const PROC *_enum_first (PROC_ID *_proc_id)
{
	procs()->enum_id = ID_ORIGIN;
	return _enum_next (_proc_id);
}

///////////////////////////////////////////////////////////////
// interface (with writing)

PROC_ID _append (ADDR_ID start_id)
{
PROC_ID new_proc_id
	= max(ID_ORIGIN, procs()->len);

	// variable length
unsigned m
	= pow2_ge(new_proc_id +1);
	if (pow2_ge(procs()->len) < m)
		procs()->begin = (PROC *)realloc (procs()->begin, m * sizeof(PROC));
	procs()->len = new_proc_id +1;

	// new item
PROC *proc
	= &procs()->begin[new_proc_id];
	memset (proc, 0, sizeof(PROC));
	proc->start_id = start_id;

	return new_proc_id;
}

void _push_cfi (PROC_ID proc_id,
                FRAGMENT_ID frag_id,
                FRAGMENT_POS frag_ofs,
                u16 codes, u2 len)
{
caller_bug(1 == len || 2 == len)
flow_bug(procs()->begin)
caller_bug(ID_ORIGIN <= proc_id)
	if (! (proc_id < procs()->len))
		return;
PROC *proc
	= &procs()->begin[proc_id];


unsigned m
		= pow2_ge(proc->cfi_len +1);
		if (pow2_ge(proc->cfi_len) < m)
			proc->cfi = (CFI_ENTRY *)realloc (proc->cfi, m * sizeof(CFI_ENTRY));
CFI_ENTRY *cfi
		= &proc->cfi[proc->cfi_len];
		cfi->addr_id = addrs_append (frag_id, frag_ofs);
		memcpy (&cfi->codes, &codes, len);
		cfi->codes_len = len;
		++proc->cfi_len;
}

void _set_fn_len (PROC_ID proc_id, u32 fn_len)
{
flow_bug(procs()->begin)
caller_bug(ID_ORIGIN <= proc_id)
	if (! (proc_id < procs()->len))
		return;
PROC *proc
	= &procs()->begin[proc_id];

	proc->codes_len = fn_len;
}

void _set_endaddr (PROC_ID proc_id, ADDR_ID end_id)
{
flow_bug(procs()->begin)
caller_bug(ID_ORIGIN <= proc_id)
	if (! (proc_id < procs()->len))
		return;
PROC *proc
	= &procs()->begin[proc_id];

	proc->end_id = end_id;
}

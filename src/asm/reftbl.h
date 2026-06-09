#ifndef REFTBL_H_INCLUDED__
#define REFTBL_H_INCLUDED__

#include "as-.h" // TOKEN
#include "asm/addrs.h" // ADDR_ID
#include "asm/sym.h" // SYMBOL_ID

typedef struct {
	ADDR_ID addr_id;
	SYMBOL_ID sym_id;
	REL_TYPE type;
} REFTBL_OPR;

typedef struct {
	// writing address
	REFTBL_OPR opr;
	// formula of refering address
	struct {
		TOKEN expr[1];
	} ref;
} REFTBL; // relative reference from operand

void reftbl_reset ();
void reftbl_append (const REFTBL_OPR *opr,
                    const TOKEN *t1, unsigned n1, const char *_source);
const REFTBL *reftbl_enum_next ();
const REFTBL *reftbl_enum_first ();

#endif  //ndef REFTBL_H_INCLUDED__

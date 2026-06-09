#ifndef PROCS_H_INCLUDED__
#define PROCS_H_INCLUDED__

//#define OLD_26215 // TODO: remove (ADDR_ID)
#ifdef OLD_26215 // TODO: remove (ADDR_ID)
#include "asm/fragments.type.h" // FRAGMENT_ID FRAGMENT_POS
#endif
#include "asm/addrs.h" // ADDR_ID
#include <stdint.h>

typedef struct {
	ADDR_ID addr_id;
#define CFI_CODES_BITS 16
	/*u16*/uint16_t codes;
#define CFI_CODES_LEN_BITS 2
	/*u2*/uint8_t codes_len;
} CFI_ENTRY;

#define PROC_ID_BITS 16
typedef /*u16*/uint16_t PROC_ID;

typedef struct {
	ADDR_ID start_id, end_id;
	CFI_ENTRY *cfi;
	uint8_t cfi_len;
	uint32_t codes_len;
} PROC;

const PROC *procs_get (PROC_ID proc_id);
const PROC *procs_enum_first (PROC_ID *_proc_id);
const PROC *procs_enum_next (PROC_ID *_proc_id);

PROC_ID procs_append (ADDR_ID start_id);
void procs_push_cfi (PROC_ID proc_id,
                     FRAGMENT_ID frag_id,
                     FRAGMENT_POS frag_ofs,
                     uint16_t codes, /*u2*/uint8_t len);
void procs_set_fn_len (PROC_ID proc_id, uint32_t fn_len);
void procs_set_endaddr (PROC_ID proc_id, ADDR_ID end_id);

#endif  //ndef PROCS_H_INCLUDED__

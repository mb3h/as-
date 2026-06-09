#ifndef FRAGMENTS_H_INCLUDED__
#define FRAGMENTS_H_INCLUDED__

#include "as-.h" // TOKEN
#include "asm/sym.h" // SYMBOL_ID
#include "asm/fragments.type.h" // FRAGMENT_ID FRAGMENT_POS SEGMENT_ID ...
#include <stdint.h>

typedef struct {
	unsigned reserved;
	// (pass2 sets)
	SEGMENT_POS seg_addr;
	REL_TYPE ofs_type;
	uint8_t pad1;
	// (pass1 sets) fixed native code
	FRAGMENT_POS obj_len;
	uint8_t *obj;
	SEGMENT_ID seg_id;
	// (pass1 sets) variable native code
	uint16_t jmp8_len  :3; // 2=(jcc)(16X <rel8>)  2=(jmp)(353 <rel8>)  5=(call)(350 <rel32>)
	uint16_t jmp32_len :3; // 6=(jcc)(017 20X <rel32>)  5=(jmp)(351 <rel32>)
	uint16_t pad3      :16 -(3 +3);
	uint32_t jmp8_opcode;
	uint32_t jmp32_opcode;
	unsigned src_lineno;
	struct {
		TOKEN expr[1];
	} to;
} FRAGMENT;

void fragments_reset ();

// for pass1
void fragments_end_appending ();
FRAGMENT_ID fragments_begin_appending ();
void fragments_push_code (FRAGMENT_ID frg_id,
                          uint64_t packed_code,
                          unsigned len);
void fragments_push_rel8_capable_jmp (FRAGMENT_ID frg_id,
                                      const TOKEN *t1, unsigned n, const char *_source,
                                      REL_TYPE ofs_type,
                                      uint32_t jmp8_opcode, /*u3*/uint8_t jmp8_opcode_len,
                                      uint32_t jmp32_opcode, /*u3*/uint8_t jmp32_opcode_len,
                                      unsigned src_lineno);
FRAGMENT_ID fragments_get_appending ();
void fragments_push_end ();

// for pass2
const FRAGMENT *fragments_enum_first (SEGMENT_ID seg_id, FRAGMENT_ID *frag_id);
const FRAGMENT *fragments_enum_next (FRAGMENT_ID *frag_id);
const FRAGMENT *fragments_get (FRAGMENT_ID frag_id);
void fragments_set_ofs_type (FRAGMENT_ID frag_id, REL_TYPE ofs_type);
void fragments_set_seg_id (FRAGMENT_ID frag_id, SEGMENT_ID seg_id);
void fragments_set_seg_addr (FRAGMENT_ID frag_id, SEGMENT_POS seg_addr);
_Bool fragments_has_rel8_capable_jmp (FRAGMENT_ID frag_id);
const char *fragments_get_tokens (const TOKEN *t0, unsigned *_len);
unsigned fragments_write_rel8_capable_jmp (FRAGMENT_ID frag_id,
                                           uint8_t *obj,
                                           unsigned obj_max);

// helper macro for passing by value char[4] as uint32_t
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define   FRAGMENTS_ONE(b1)             ((b1)                                      ), 1
#define   FRAGMENTS_TWO(b1, b2)         ((b1) | (b2) << 8                          ), 2
#define FRAGMENTS_THREE(b1, b2, b3)     ((b1) | (b2) << 8 | (b3) << 16             ), 3
#define  FRAGMENTS_FOUR(b1, b2, b3, b4) ((b1) | (b2) << 8 | (b3) << 16 | (b4) << 24), 4
#else
	// PENDING:
#endif

#endif  //ndef FRAGMENTS_H_INCLUDED__

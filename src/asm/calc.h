#ifndef CALC_H_INCLUDED__
#define CALC_H_INCLUDED__

#include "as-.h" // TOKEN
#include "asm/sym.h" // VALUE
#include <stdint.h>

#define ETC_BYTE  0
#define ETC_WORD  1
#define ETC_DWORD 2
#define ETC_QWORD 3
#define ETC_PTR   4
#define SQUARE    0

typedef enum {
	ID_NUM = 1,
	ID_SYM,
	ID_BYTE,
	ID_WORD  = ID_BYTE +ETC_WORD,
	ID_DWORD = ID_BYTE +ETC_DWORD,
	ID_QWORD = ID_BYTE +ETC_QWORD,
	ID_PTR   = ID_BYTE +ETC_PTR,
	ID_SQUARE_BRACKET = ID_PTR +1,

	ID_RIP,

	ID_EAX,
	ID_ECX,
	ID_EDX,
	ID_EBX,
	ID_ESP,
	ID_EBP,
	ID_ESI,
	ID_EDI,

	ID_AL,
	ID_CL,
	ID_DL,
	ID_BL,
	ID_AH,
	ID_CH,
	ID_DH,
	ID_BH,

	ID_SPL,
	ID_BPL,
	ID_SIL,
	ID_DIL,

	ID_AX,
	ID_CX,
	ID_DX,
	ID_BX,
	ID_SP,
	ID_BP,
	ID_SI,
	ID_DI,

	ID_RAX,
	ID_RCX,
	ID_RDX,
	ID_RBX,
	ID_RSP,
	ID_RBP,
	ID_RSI,
	ID_RDI,

	ID_R8,
	ID_R9,
	ID_R10,
	ID_R11,
	ID_R12,
	ID_R13,
	ID_R14,
	ID_R15,

	ID_R8B,
	ID_R9B,
	ID_R10B,
	ID_R11B,
	ID_R12B,
	ID_R13B,
	ID_R14B,
	ID_R15B,

	ID_R8W,
} OPR;

typedef enum {
	UNK = 0,
	RAXtoRDI,
	R8toR15,
	EAXtoEDI,
	ALtoBH,
	AXtoDI,
	RIP,

	SPLtoDIL,
	R8BtoR15B,
	R8WtoR15W,
	SYMNUM,
	ETC,
	BRACKET,
} REG_TYPE;

typedef struct {
	uint16_t reg         :3;
	uint16_t reg_type    :4;
	uint16_t is_scaling  :1;
	uint16_t scale_reg   :3;
	uint16_t scale_lv    :2;

	uint16_t padding     :3;
} OPR_CTRL;

#define CALC_ANY_REPLACED_ZERO -1
#define CALC_ABORT             -2

OPR calc_tok2id (const char *s, unsigned n, OPR_CTRL *_ctl);
/*s2*/int8_t calc_expr2n (const TOKEN *t0, unsigned n, const char *_source,
                          VALUE *_val);
/*s2*/int8_t calc_opr2n (const TOKEN *t0, unsigned n, const char *_source,
                         OPR_CTRL *_ctl, VALUE *_val);
/*s2*/int8_t calc_opr2seg (const TOKEN *t0, unsigned n0, const char *_source,
                           OPR_CTRL *_ctl, SEGMENT_POS *_ofs, SEGMENT_ID *_id);

#endif  //ndef CALC_H_INCLUDED__

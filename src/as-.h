#ifndef IAS_H__INCLUDED__
# define IAS_H__INCLUDED__

#include <stdint.h>

///////////////////////////////////////////////////////////////
// constants

#define MACROID_BITS    12
#define MACROID /*u12*/ uint16_t

#define MACRO_PARAM_MAX 16

// .bDirectiveContinued etc.
#define FALSE 0
#define TRUE  1

///////////////////////////////////////////////////////////////
// tradeoff concurrency (for readability, flexibility)

#define MACRO_WORK 256
typedef struct {
	const char *text;
	const char *fname;
	unsigned line;
	// (define.c)
	uint8_t *defining_work;
	uint32_t defining_id    :MACROID_BITS; // 0-4095
	uint32_t pad_0C_b4      :31 -MACROID_BITS;
	uint32_t defining_is1st :1;
	// (tokenize.c)
	void *resolve_stack;
	uint16_t stack_max;
	uint16_t stack_depth;
	const void *root_tokenized;
	uint8_t root_tokens_len;
	MACROID found_id;
	// (directive.c)
	uint32_t directive_if_disabler; // b10, b32, ...: depth=0, 1, ...
	uint32_t bDirectiveContinued :1;
	uint32_t directive_id        :4;
	uint32_t directive_if_depth  :5;
	uint32_t pad                 :22;
	// (macro.c)(tokenize.c)
	uint8_t macro_work[16];
	uint8_t param_work[24];
} MASKED;

// .directive_if_depth .directive_if_disabler
#define DIRECTIVE_STATE_PER_IF_BITS 2

// .directive_id
#define DIRECTIVE_ID_UNDEF      1
#define DIRECTIVE_ID_DEFINE     2
#define DIRECTIVE_ID_INCLUDE    3
#define DIRECTIVE_ID_ENDIF      4
#define DIRECTIVE_ID_IF         5
#define DIRECTIVE_ID_ELSE       6
#define DIRECTIVE_ID_IFDEF      7
#define DIRECTIVE_ID_IFNDEF     8
#define DIRECTIVE_ID_ELIF       9
#define DIRECTIVE_ID_MAX        9

MASKED *masked();
const char *whereis();
const char *asm_whereis();

///////////////////////////////////////////////////////////////
// integrated log (with filtering)

void dbg (unsigned mask, unsigned level, const char *format, ...);
_Bool is_dbg (unsigned mask, unsigned level);

///////////////////////////////////////////////////////////////
// core designed model

#define TOKEN_POS_BITS 8
#define TOKEN_POS /*u8*/ uint8_t
#define TOKEN_LEN_BITS 8
#define TOKEN_LEN /*u8*/ uint8_t
typedef struct {
	TOKEN_POS pos;
	TOKEN_LEN len;
} TOKEN;

#define TOKENS_CNT_BITS 8
#define TOKENS_CNT /*u8*/ uint8_t

extern const TOKEN NEWLINE;
/* PENDING: 'extern' needs '-fPIC' option for exporting an
            actual data not a pointer (at gcc-13.3.0).
            It's ambiguous that how this way affects the
            portability. */
#define IS_NEWLINE(p) \
	(NEWLINE.pos == (p)->pos && NEWLINE.len == (p)->len)

#endif //def IAS_H__INCLUDED__

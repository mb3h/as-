#ifndef SYM_H_INCLUDED__
#define SYM_H_INCLUDED__

#include "asm/addrs.h" // ADDR_ID
#include <stdint.h>

typedef struct {
	union {
		uint32_t number;
		ADDR_ID addr_id;
	};
#define VALUE_TYPE_BITS   1
#define VALUE_TYPE_NUM    0
#define VALUE_TYPE_OFFSET 1
	uint8_t type :VALUE_TYPE_BITS;
	uint8_t pad1 :8 -VALUE_TYPE_BITS;
	uint8_t attr;
	uint8_t pad2[2];
	uint32_t fn_len;
} VALUE;

#define SYM_TYPE_MASK 0x0f
#define SYM_TYPE_O    0x01 // PENDING: '_O' not good name
#define SYM_TYPE_F    0x02 // PENDING: '_F' not good name
#define SYM_TYPE_d    0x03 // PENDING: '_d' not good name
#define SYM_TYPE_df   0x04 // PENDING: '_df' not good name
#define SYM_BIND_MASK 0xf0
#define SYM_BIND_g    0x10 // PENDING: '_g' not good name

#define SYMBOL_ID_BITS 16
typedef /*u16*/uint16_t SYMBOL_ID;

void sym_reset ();
// @note CAUTION: set_value() ignores 'VALUE::attr' and 'VALUE::fn_len'
void sym_set_value (const char *s, unsigned n, const VALUE *src);
SYMBOL_ID sym_set_type (const char *s, unsigned n, uint8_t type);
void sym_set_bind (const char *s, unsigned n, uint8_t bind);
void sym_set_fn_len (SYMBOL_ID id, uint32_t fn_len);
unsigned sym_get_count ();
SYMBOL_ID sym_lookup (const char *s, unsigned n, VALUE *_val);
const char *sym_id2val (SYMBOL_ID id, VALUE *_val);

const char *sym_enum_first (VALUE *_val);
const char *sym_enum_next (VALUE *_val);

#endif  //ndef SYM_H_INCLUDED__

#ifndef ADDRS_H_INCLUDED__
#define ADDRS_H_INCLUDED__
#include <stdint.h>
#include "asm/fragments.type.h" // FRAGMENT_ID FRAGMENT_POS ...

#define ADDR_ID_BITS 16
typedef /*u16*/uint16_t ADDR_ID;

typedef struct {
	struct {
		FRAGMENT_ID  frag_id;
		FRAGMENT_POS frag_ofs;
	};
	struct {
		SEGMENT_ID  seg_id;
		SEGMENT_POS seg_ofs;
	};
	uint8_t is_resolved :1;
	uint8_t pad         :7;
} ADDR;

const ADDR *addrs_get (ADDR_ID addr_id);
ADDR_ID addrs_append (FRAGMENT_ID frag_id, FRAGMENT_POS frag_ofs);

#endif  //ndef ADDRS_H_INCLUDED__

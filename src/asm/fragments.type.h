#ifndef FRAGMENTS_TYPE_H_INCLUDED__
#define FRAGMENTS_TYPE_H_INCLUDED__

#include <stdint.h>

/*
PENDING: (*1) scaling step by step through observing a running.
 */

#define FRAGMENT_ID_BITS     10 // PENDING: (*1)
typedef /*u10*/uint16_t FRAGMENT_ID;
#define INVALID_FRAGMENT_ID  (uint16_t)((1 << FRAGMENT_ID_BITS)-1) // TODO: -> 0
#define FRAGMENT_ID_ORIGIN   0 // TODO: -> 1

#define FRAGMENT_OBJLEN_BITS 14 // PENDING: (*1)
typedef /*u14*/uint16_t FRAGMENT_POS;

#define FRAGMENT_CFILEN_BITS  8 // PENDING: (*1)
typedef /*u8*/uint8_t CFI_POS;

#define SEGMENT_ID_BITS       2 // PENDING: (*1)
typedef /*u2 */uint8_t SEGMENT_ID;
#define      STRUCTSEG 0 // '.struct'
#define           CSEG 1 // '.text'
#define           DSEG 2 // '.rodata'
#define SEGMENT_ID_MAX 2
#define    ALLSEG STRUCTSEG // for _enum_first()
#define SEGMENT_SIZE_BITS  16 // PENDING: (*1)
typedef /*u16*/uint16_t SEGMENT_POS;
#define SEGMENT_POS_UNSET ((1 << SEGMENT_SIZE_BITS) -1)
#define SEGMENT_POS_MAX   ((1 << SEGMENT_SIZE_BITS) -2)

typedef enum {
	TYPE_REL32     = 0,
	TYPE_REL8      = 1,
	TYPE_REL64     = 2,
	TYPE_REL8or32  = 3,
	TYPE_FN_LEN    = 4,
} REL_TYPE;
#define REL_TYPE_BITS 3

#endif  //ndef FRAGMENTS_TYPE_H_INCLUDED__

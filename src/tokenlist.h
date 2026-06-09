#ifndef TOKENLIST_H__INCLUDED__
# define TOKENLIST_H__INCLUDED__

#include "as-.h" // TOKEN
#include <stdint.h>

///////////////////////////////////////////////////////////////
// list of tokens

/* layout of TOKENLIST
	---- +0
	.size
	.lines
	----
	(owner-area)
	---- +body_offset cf)_append_tokens()
	(source-text#1) + '\0'
	  .pos .len (TOKEN#1-1)
	  .pos .len (TOKEN#1-2)
	  ...
	  .pos .len (TOKEN#1-n)
	  .pos=0 .len=0 (endmark)
	(source-text#2) + '\0'
	  .pos .len (TOKEN#2-1)
	  ...
	  .pos=0 .len=0 (endmark)
	...
	(source-text#M) + '\0'
	  .pos .len (TOKEN#M-1)
	  ...
	  .pos=0 .len=0 (endmark)
	---- +.size
 */
/* NOTE: (*4) never uses #0
 */

typedef struct {
	uint16_t size;
	uint16_t lines;
	uint8_t owner[0];
} TOKENLIST_HEADER;

unsigned tokenlist_create (TOKENLIST_HEADER ***entried_list, TOKENLIST_HEADER *data_header, unsigned body_offset, unsigned *used_, unsigned *reserved_, unsigned reserve_max);
TOKENLIST_HEADER *tokenlist_append_tokens (TOKENLIST_HEADER *entried, unsigned body_offset, const char *text, const TOKEN *tokens, unsigned tokens_len);
unsigned tokenlist_get_lines (TOKENLIST_HEADER *entried, unsigned body_offset);
void tokenlist_destroy (TOKENLIST_HEADER *entried);

#endif //ndef TOKENLIST_H__INCLUDED__

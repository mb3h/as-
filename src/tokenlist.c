#include "tokenlist.h"

#include "helper/assert.h"
#include <stdint.h>
#include <memory.h>

typedef  uint8_t u8;
typedef uint16_t u16;

#define usizeof(s) (unsigned)sizeof(s)
#define ustrlen(s) (unsigned)strlen(s)
///////////////////////////////////////////////////////////////
// alias

// export
#define _create        tokenlist_create
#define _append_tokens tokenlist_append_tokens
#define _get_lines     tokenlist_get_lines
#define _destroy       tokenlist_destroy
#define _HEADER        TOKENLIST_HEADER

///////////////////////////////////////////////////////////////
// constants

#define LIST_INITMAX 128

///////////////////////////////////////////////////////////////

void _destroy (_HEADER *entried)
{ free (entried); }

unsigned _create (_HEADER ***entried_list, _HEADER *data_header, unsigned body_offset, unsigned *used_, unsigned *reserved_, unsigned reserve_max)
{
static unsigned NEED = 1; // (for redesigning)
assert(entried_list && used_ && reserved_ && 0 < NEED)
_HEADER **m_ = *entried_list; unsigned used = *used_, n = *reserved_;
	if (NULL == m_)
		used = 1, n = LIST_INITMAX; // NOTE: (*4)
	while (n < used +NEED && n * 2 <= reserve_max)
		n *= 2;

	// initialize entries (when needed)
	if (NULL == m_) {
		m_ = (_HEADER **)malloc (n * sizeof(_HEADER *));
		*reserved_ = n;
		m_[0] = NULL; // safety NOTE: (*4)
	}
	// expand entries memory (when needed)
	else if (*reserved_ < n) {
		m_ = (_HEADER **)realloc (m_, n * sizeof(_HEADER *));
		*reserved_ = n;
	}

_HEADER *entried
	= (_HEADER *)malloc (body_offset);
	memcpy (entried, data_header, body_offset);
	entried->size  = body_offset;
	entried->lines = 0;

	// can assign previous +1 id? (bits limitation) PENDING: managing link list of available id
unsigned index = 0;
	do {
		if (used +NEED <= n) {
			*used_ = used +NEED;
			*entried_list = m_;
			index = used; // new entry allocated
			break;
		}
		// cannot assign previous +1 index, looking for available index (perhaps getting slow)
		for (index = 1; index < n; ++index) // NOTE: (*4)
			if (NULL == m_[index])
				break; // recycled entry allocated
		if (index < n)
			break;

		return 0; // entry not allcated (any not empty)
	} while (0);

	m_[index] = entried;
	return index;
}

static unsigned _test_end (_HEADER *entried, unsigned begin, unsigned stop, unsigned *lines)
{
unsigned tail = begin, line = 0;
	while (tail < stop) {
unsigned text_len
		= ustrlen ((const u8 *)entried +tail);
		++line;
assert(text_len < (1 << 8))
		tail += text_len +1;
const TOKEN *tok
		= (const TOKEN *)((const u8 *)entried +tail);
		while (0 < tok->len)
			++tok;
		tail = (unsigned)((const u8 *)(tok +1) - (const u8 *)entried);
	}
	if (lines)
		*lines = line;
	return tail;
}

_HEADER *_append_tokens (_HEADER *entried, unsigned body_offset, const char *text, const TOKEN *tokens, unsigned tokens_len)
{
unsigned data_end
	= entried->size;
#if 1 // validates for safety (it might be getting slow)
unsigned actual_end
	= _test_end (entried, body_offset, data_end, NULL);
assert(data_end == actual_end)
#endif
u16 i1
	= ustrlen (text) +1;
u16 i2
	= usizeof(TOKEN) * tokens_len;
u16 new_size
	= data_end +i1 +i2 +usizeof(TOKEN);
	entried = (_HEADER *)realloc (entried, new_size);
	strcpy ((char *)((u8 *)entried +data_end), text);
	if (0 < i2)
		memcpy ((u8 *)entried +data_end +i1, tokens, i2);
TOKEN endmark
	= { .pos = 0, .len = 0 };
	memcpy ((u8 *)entried +data_end +i1 +i2, &endmark, usizeof(TOKEN));

	entried->size = new_size;
	++entried->lines;
	return entried;
}

unsigned _get_lines (_HEADER *entried, unsigned body_offset)
{
#if 1 // validates for safety (it might be getting slow)
unsigned data_end
	= entried->size;
unsigned actual_lines;
	_test_end (entried, body_offset, data_end, &actual_lines);
assert(actual_lines == entried->lines)
	return actual_lines;
#else
	return entried->lines;
#endif
}

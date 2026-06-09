from libif import LTOKENIZE_PY
from libif import LV4
from libif import sizeofTOKEN
from libif import dbg, sizeofTOKEN
from constants import VTO, VTCC

# /////////////////////////////////////////////////////////////
# // helper

def utf8len(s: str, start: int, end: int) -> int:
	utf8 = s[start:end].encode('utf-8') # bytes
	return len(utf8)

def tokens_splice(t: bytes, a: int = 0, b: int = 0) -> bytes:
	if 0 == b:
		return t[a * sizeofTOKEN() :]
	else:
		return t[a * sizeofTOKEN() : b * sizeofTOKEN()]

def tokens_count(t: bytes) -> int:
	return int(len(t) / sizeofTOKEN())

def tokens_pos(t: bytes, i: int) -> int:
	if not i < tokens_count(t):
		return -1
	return t[i*sizeofTOKEN()]

def tokens_get(t: bytes, s: str, i: int) -> int:
	if not i < tokens_count(t):
		return ''
	pos = t[i*sizeofTOKEN()]; leng = t[i*sizeofTOKEN() +1]
	utf8 = s.encode('utf-8') # bytes
	retval = utf8[pos:pos+leng].decode('utf-8')
	return retval

def tokens_to_list(t: bytes, s: str) -> list:
	tokens = []
	i = 0
	for b in range(0, len(t), sizeofTOKEN()):
		pos = t[b]; tok = t[b+1]
		tokens.append(s[pos:pos+tok])
	return tokens

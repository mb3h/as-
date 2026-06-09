from libif import LTOKENIZE_PY
from libif import LV3
from libif import dbg, sizeofTOKEN
from helper import utf8len
from constants import VTO, VTRR, VTGG, VTCC

import re
# /////////////////////////////////////////////////////////////
# // main value

global dbg_prefix; dbg_prefix = ''

global directive_bComment; directive_bComment = False
def directive_tokenize(s: str, i: int) -> tuple:
	global directive_bComment

	i0 = i
	# finds the beginning of available token
	while True: # control not lost in repeating of /* ... */
		# /* ... */ completed?
		if directive_bComment:
			# looks for '*/'
			j = -1
			while i < len(s):
				j = s.find('*', i, len(s))
				if -1 == j:
					break # not found
				if j +1 < len(s) and '/' == s[j+1:j+2]:
					break # found
				i = j +1
			# not found?
			if -1 == j:
				# '\\' on tail is handled as a token under '/*' (not handled so under '//')
				if 0 < len(s) and '\\' == s[-1:]:
					spc = len(s) -i0 -1
					return (spc, 1) # next line with linkning
				else:
					spc = len(s) -i0
					return (spc, 0) # next line
			i = j+2; directive_bComment = False
			dbg(LTOKENIZE_PY, LV3, '[{1}.py{0}] {3}: bComment <- {2}False{0}'.format(VTO, VTCC, VTGG, dbg_prefix))
		# one more space?
		if i < len(s):
			o = re.search('^([ \t]*)', s[i:])
			i += len(o[1])
		if not i < len(s):
			spc = len(s) -i0
			return (spc, 0) # next line
		# /* ... */ beginned?
		if '/' == s[i:i+1] and i+1 < len(s) and '*' == s[i+1:i+2]:
			i += 2; directive_bComment = True
			dbg(LTOKENIZE_PY, LV3, '[{1}.py{0}] {3}: bComment <- {2}True{0}'.format(VTO, VTCC, VTRR, dbg_prefix))
			continue
		break
	spc = utf8len(s, i0, i); c = s[i:i+1]

	# finds the bound of available token
	if '"' == c:
		j = s.find('"', i+1, len(s))
		if (-1 == j): # NOTE: super omitting: doesn't handle unclosed "
			unexpected_abort('\'"\' not closed')
		if '\\' == s[j-1:j]: # NOTE: super omitting: doesn't handle unclosed \"
			unexpected_abort('\'\\"\' not available yet')
		leng = utf8len(s, i, j+1); i += leng
	elif '#' == c:
		o = re.search('^(#+)', s[i:])
		leng = len(o[1]); i += leng
	elif -1 < '\'()*,;[]'.find(c):
		leng = 1; i += leng
	elif '/' == c and i+1 < len(s) and '/' == s[i+1:i+2]:
		return (spc, 0) # next line
	elif '\\' == c:
		return (spc, 1) # next line with linkning
	else:
		o = re.search('^([^ \t"#\'()*,/;\\[\\]]+)', s[i:])
		tok = '' if not o else o[1]
		leng = len(tok); i += leng
	return (spc, leng)

def x86_64_intel_tokenize(s: str, i: int) -> tuple:
	i0 = i
# finds the beginning of available token
	# one more space?
	if i < len(s):
		o = re.search('^([ \t]*)', s[i:])
		i += len(o[1])
	if not i < len(s):
		spc = len(s) -i0
		return (spc, 0) # end of line
	spc = i -i0

# finds the bound of available token
	c = s[i:i+1]
	if -1 < '#()*+,-/:@[]|'.find(c):
		leng = 1; i += leng
		return (spc, leng)
	if -1 < '._'.find(c) or 'A' <= c and c <= 'Z' or 'a' <= c and c <= 'z':
		o = re.search('^([.0-9A-Z_a-z]+)', s[i:])
		tok = o[1]
		leng = len(tok); i += leng
		return (spc, leng)
	if '0' == c and 'x' == s[i+1:i+2]:
		o = re.search('^([0-9A-Fa-f]+)', s[i+2:])
		tok = o[1]
		leng = 2 +len(tok); i += leng
		return (spc, leng)
	if -1 < '0123456789'.find(c):
		o = re.search('^([0-9]+)', s[i:])
		tok = o[1]
		leng = len(tok); i += leng
		return (spc, leng)
	if -1 < '<>'.find(c):
		if i+1 < len(s) and c == s[i+1:i+2]:
			leng = 2; i += leng
			return (spc, leng)
	dbg(0, 0, 'unexpected character: \'{1}{2}{0}\''.format(VTO, VTRR, c))
	exit(-1)

# /////////////////////////////////////////////////////////////
# // export

def directive_tokenize_list(s: str) -> bytes:
	r = bytes(); i = 0; leng = 0
	while i < len(s):
		(spc, leng) = directive_tokenize(s, i)
		if not 0 < leng:
			break # next line
		r += bytes((i +spc, leng))
		if (2 < sizeofTOKEN()):
			r += bytes([0x00] * (sizeofTOKEN() -2))
		i += spc +leng
	return r

def directive_tokenize_set_dbg_prefix(s: str) -> None:
	global dbg_prefix
	dbg_prefix = s

def x86_64_intel_tokenize_list(s: str) -> bytes:
	r = bytes(); i = 0; leng = 0

	while i < len(s):
		(spc, leng) = x86_64_intel_tokenize(s, i)
		if not 0 < leng:
			break # end of line
		if 1 == leng and '#' == s[i+spc:i+spc+1]:
			break # comment later
		r += bytes((i +spc, leng))
		if (2 < sizeofTOKEN()):
			r += bytes([0x00] * (sizeofTOKEN() -2))
		i += spc +leng

	return r

# /////////////////////////////////////////////////////////////
# // Now for debugging only

def bytes_cmp(lhs: bytes, rhs: bytes) -> bool:
	if not len(lhs) == len(rhs):
		return False
	for i in range(0, len(lhs)):
		if not lhs[i] == rhs[i]:
			return False
	return True

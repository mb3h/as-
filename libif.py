#from libif import dbg
from constants import VTO, VTRR, VTYY, VTMM

# /////////////////////////////////////////////////////////////
# // constants

LRESOLVE     = 1
LDEFINE      = 2
LTOKENIZE    = 4
LASSEMBLE    = 8
LTOKENIZE_PY = 128

LV0 = 0
LV1 = 1
LV2 = 2
LV3 = 3
LV4 = 4

# /////////////////////////////////////////////////////////////
# // shared library (interface)

from ctypes import c_ubyte, c_ushort, c_uint, c_ulong, c_int, c_bool, CDLL, CFUNCTYPE, POINTER, Structure, c_void_p, Union, c_char_p, create_string_buffer, c_char # , addressof

global LIB_NAME; LIB_NAME = 'libas-.so'

# /////////////////////////////////////////////////////////////
# // constants depending on behavior of shared library

global _sizeofTOKEN; _sizeofTOKEN = 2

def sizeofTOKEN() -> int:
	global _sizeofTOKEN
#	if 0 == _sizeofTOKEN:
#		_sizeofTOKEN = sizeofTOKEN_get()
	return _sizeofTOKEN

# /////////////////////////////////////////////////////////////
# // shared library

def dbg(mask: int, level: int, s: str) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_uint, c_uint, c_char_p)
	paramflags = (1, 'mask', 0), (1, 'level', 0), (1, 'text', 0)
	lpfn = prototype(('dbg0', iaslib), paramflags = paramflags)

	mk = c_uint(mask)
	lv = c_uint(level)
	text = create_string_buffer(s.encode('utf-8'))
	lpfn(mk, lv, text)

def dbg_set_mask(mask: int) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_uint)
	paramflags = (1, 'mask', 0)
	lpfn = prototype(('dbg_set_mask', iaslib), paramflags = paramflags)
	lpfn(c_uint(mask))

def dbg_set_level(level: int) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_uint)
	paramflags = (1, 'level', 0)
	lpfn = prototype(('dbg_set_level', iaslib), paramflags = paramflags)
	lpfn(c_uint(level))

def validate_bitsize() -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None)
	lpfn = prototype(('as_validate_bitsize', iaslib), paramflags = None)
	lpfn()

def is_disable_by_if() -> bool:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(bool)
	paramflags = (1, 'is_disable', 0)
	lpfn = prototype(('directive_is_disable_by_if', iaslib), paramflags = paramflags)

	r = lpfn()
	return r

def parse_directive_line(s: str, t: bytes) -> int:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(c_ubyte, c_char_p, POINTER(c_ubyte), c_uint)
	paramflags = (1, 'parsed_count', 0), (1, 'text', 0), (1, 'tokens', 0), (1, 'tokens_count', 0)
	lpfn = prototype(('as_directive_parse_line', iaslib), paramflags = paramflags)

	tokens = (c_ubyte * len(t))(*t)
	tokens_count = c_uint(int(len(t) / sizeofTOKEN()))
	text = create_string_buffer(s.encode('utf-8'))

	r = lpfn(text, tokens, tokens_count)
	return r

def tokenize_init(s: str) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_char_p)
	paramflags = (1, 'pre_defined_symbols', 0)
	lpfn = prototype(('as_tokenize_init', iaslib), paramflags = None)

	text = create_string_buffer(s.encode('utf-8'))
	lpfn(text)

# with preprocessing
global s_pp_text, s_pp_tokens, s_pp_tokens_count # for keeping memory
s_pp_text = None; s_pp_b = None
def tokenize_get_C_first(s: str, t: bytes) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_char_p, POINTER(c_ubyte), c_uint)
	paramflags = (1, 'text', 0), (1, 'tokens', 0), (1, 'tokens_count', 0)
	lpfn = prototype(('as_tokenize_get_C_first', iaslib), paramflags = paramflags)

	global s_pp_text, s_pp_tokens, s_pp_tokens_count # for keeping memory
	s_pp_tokens = (c_ubyte * len(t))(*t)
	s_pp_tokens_count = int(len(t) / sizeofTOKEN())
	if (255 < s_pp_tokens_count):
		dbg(0, LV1, '{1}error{0}: {2}(): tokens_count cannot be over {3}, buf {4}'.format(VTO, VTRR, 'as_tokenize_get_C', 255, s_pp_tokens_count))
		exit(-1)
	s_pp_text = create_string_buffer(s.encode('utf-8'))

	lpfn(s_pp_text, s_pp_tokens, s_pp_tokens_count)

def tokenize_get_C() -> str:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(c_uint, POINTER(c_char), c_uint)
	paramflags = (1, 'got_len', 0), (2, 'got_buf', 0), (1, 'got_buf_max', 0)
	lpfn = prototype(('as_tokenize_get_C', iaslib), paramflags = paramflags)

	got_buf = create_string_buffer(256)

	got_len = lpfn(got_buf, 256)
	tmp = bytes(got_buf[0:got_len])
	nulls = [b for b in tmp if 0x00 == b]
	if 0 < len(nulls):
		dbg(0, 0, '{1}error{0}: as_tokenize_get_C() returns {1}{2}{0} byte(s) of {1}0x00{0} (in {3} bytes)'.format(VTO, VTRR, len(nulls), len(tmp)))
		dbg(0, 0, '({0}) {1}'.format(len(tmp), ' '.join(['{0:02X}'.format(b) for b in tmp])))
		exit(-1)
	return tmp.decode('utf-8')

def skip_statement_C(tok: str) -> bool:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(c_bool, c_char_p, c_uint)
	paramflags = (1, 'completed', 0), (1, 'tok', 0), (1, 'len', 0)
	lpfn = prototype(('skip_statement_C', iaslib), paramflags = paramflags)

	s = create_string_buffer(tok.encode('utf-8'))
	return lpfn(s, c_uint(len(s) -1)) # except for '\0'

def as_pass1_new(filename: str) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_char_p)
	paramflags = (1, 'filename', 0)
	lpfn = prototype(('as_pass1_new', iaslib), paramflags = paramflags)

	cstr = create_string_buffer(filename.encode('utf-8'))
	lpfn(cstr)

def as_pass1_x86_64_intel(s: str, t: bytes) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, POINTER(c_ubyte), c_uint, c_char_p)
	paramflags = (1, 'tokens', 0), (1, 'tokens_count', 0), (1, 'text', 0)
	lpfn = prototype(('as_pass1_x86_64_intel', iaslib), paramflags = paramflags)

	tokens = (c_ubyte * len(t))(*t)
	tokens_count = c_uint(int(len(t) / sizeofTOKEN()))
	text = create_string_buffer(s.encode('utf-8'))

	lpfn(tokens, tokens_count, text)

def as_pass2(secstr_id_from_secidx: (c_ubyte)) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, POINTER(c_ubyte))
	paramflags = (1, 'secstr_id_from_secidx', 0), (1, 'secstr_id_from_secidx_len', 0)
	lpfn = prototype(('as_pass2', iaslib), paramflags = paramflags)
	lpfn(secstr_id_from_secidx, c_uint(len(secstr_id_from_secidx)))

global s_pp_fname # for keeping memory
s_pp_fname = None
def dbg_set_line(fname: str, line: int, line_text: str) -> None:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(None, c_uint, c_char_p)
	paramflags = (1, 'line', 0), (1, 'fname', 0)
	lpfn = prototype(('dbg_set_line', iaslib), paramflags = paramflags)

	global s_pp_fname
	s_pp_fname = create_string_buffer(fname.encode('utf-8'))
	lpfn(c_uint(line), s_pp_fname)

	# python side
	global g_TEXT; g_TEXT = line_text

# ELF helper
def strtab_id_from(strtab: POINTER(c_ubyte), strtab_len: int, needle: str) -> int:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(c_ulong, POINTER(c_ubyte), c_ulong, c_char_p)
	paramflags = (1, 'ret', 0), (1, 'strtab', 0), (1, 'strtab_len', 0), (1, 'str', 0)
	lpfn = prototype(('strtab_id_from', iaslib), paramflags = paramflags)

	cstr = create_string_buffer(needle.encode('utf-8'))
	str_id = lpfn(strtab, strtab_len, cstr)
	return str_id

def idstr_from(s: str) -> int:
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(c_uint, c_char_p, c_uint)
	paramflags = (1, 'idstr', 0), (1, 's', 0), (1, 'n', 0)
	lpfn = prototype(('idstr_from', iaslib), paramflags = paramflags)

	cstr = create_string_buffer(s.encode('utf-8'))
	ids = lpfn(cstr, 0)
	return ids

from ctypes import cast, byref, pointer
def pass2_result(needle: str) -> (c_ubyte):
	global LIB_NAME; iaslib = CDLL('./' + LIB_NAME)

	prototype = CFUNCTYPE(POINTER(c_ubyte), c_char_p, POINTER(c_uint))
	paramflags = (1, 'got', 0), (1, 'secname', 0), (2, 'len', 0)
	lpfn = prototype(('as_pass2_result', iaslib), paramflags = paramflags)

	cstr = create_string_buffer(needle.encode('utf-8'))
	got_len = c_uint()
	cptr = lpfn(cstr, byref(got_len))
	got = cast(cptr, POINTER((c_ubyte * got_len.value)))
	return got.contents

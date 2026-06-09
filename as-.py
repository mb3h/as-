#!/usr/bin/env python3.8

# /* NOTE: (*1)
#    takes care that the preprocessor needs to check
#    how many '#if' are even if it's under '#if 0'.
#    ex)
#    #if 0 <- (a)
#    # if 1 <- (b)
#    # endif <- not helps (a) but (b)
#    #endif <- just helps (a)
#    TODO: remove: (*2) 
#    It's a bad and not obvious flow.
#
#    However, now sample source includes '#include <...>',
#    and it's hard to skip all of complicated C/C++ syntax.
#    But comparing with output of GCC is needed in
#    the development, so the coding can't ignore simply for GCC.
#    Therefore, the coding bypasses only when '-E' option spedified.
#    
#    It's needed to remove.
#  */

# /////////////////////////////////////////////////////////////
# // imports

from libif import LRESOLVE, LASSEMBLE, LTOKENIZE_PY
from libif import LV0, LV1, LV2, LV3, LV4
from libif import sizeofTOKEN
from libif import dbg, dbg_set_mask, dbg_set_level, dbg_set_line
from libif import validate_bitsize, is_disable_by_if, parse_directive_line, tokenize_init, tokenize_get_C_first, tokenize_get_C, skip_statement_C
from libif import as_pass1_new, as_pass1_x86_64_intel, as_pass2
from scanner import x86_64_intel_tokenize_list, directive_tokenize_list, directive_tokenize_set_dbg_prefix
from helper import tokens_splice, tokens_count, tokens_pos, tokens_get, tokens_to_list, utf8len
from utility import read_text_file
from constants import VTO, VTRR, VTGG, VTCC

import sys
# /////////////////////////////////////////////////////////////
# tradeoff concurrency (for readability, flexibility)

global g_sources, g_TEXT
g_sources = list(); g_TEXT = ''

def unexpected_abort(msg: str, a: int = 0, b: int = 0) -> None:
	global g_TEXT
	s = g_TEXT if 0 == a and 0 == b else g_TEXT[a:] if 0 == b else g_TEXT[a:b]
	dbg(0, 0, '{3}: [{2}.py{0}] {1}{4}{0}: {5}'.format(VTO, VTRR, VTCC, whereis(), msg, s))
	exit(-1)

# /////////////////////////////////////////////////////////////
# // preprocess

INITIAL = 0
DIRECTIVE_CONTINUED = 1
DIRECTIVE_BYPASSED = 2
TOKENIZE_CONTINUED = 3 # newline happened while expanding macro

# tradeoff concurrency (for readability, flexibility)
global s_pp_state, s_pp_text2
s_pp_state = INITIAL; s_pp_text2 = ''

def preprocess_first(s: str) -> None:
	global s_pp_text2
	s_pp_text2 = s

class Input:
	# @public fname: str, lineno: int
	# @private _line_texts: list, _target_lines: list, _index: int
	def __init__(self, fname: str, target_lines: list = None):
		self.fname = fname
		t = read_text_file(fname)
		if not t:
			exit(-1)
		self._line_texts = t.split('\n')
		self._target_lines = target_lines
		self._index = 0
	def enumerate(self) -> str:
		if None == self._target_lines:
			if not self._index < len(self._line_texts):
				return None
			self.lineno = self._index +1
		else:
			if not self._index < len(self._target_lines):
				return None
			self.lineno = self._target_lines[self._index]
		self._index += 1
		return self._line_texts[self.lineno -1]

def whereis() -> str:
	reader = g_sources[len(g_sources) -1]
	fname = reader.fname if 1 < len(g_sources) else ''
	s = '{0}:{1}'.format(fname, reader.lineno)
	return s

PP_FOR_GCC = 1
def preprocess(flags: int) -> list | None:
	global s_pp_state, s_pp_text2
	s = s_pp_text2 # alias

	# empty line
	if not 0 < len(s):
		return None # empty line (len=0) [directive | C/C++]

	# end of bypassing? TODO: remove: (*2)
	if DIRECTIVE_BYPASSED == s_pp_state:
		s_pp_state = INITIAL
		return None # end of bypassing

	# lines of directive (#define, #undef, ...)
	bDirectiveStart = True if (INITIAL == s_pp_state and '#' == s[0]) else False
	bNotRun = True
	while bNotRun:
		bNotRun = False # NOTE: Ideally wants 'do {} while (0)' available on C/C++
		if bDirectiveStart or DIRECTIVE_CONTINUED == s_pp_state:
			dbg(LTOKENIZE_PY, LV1, '[{1}.py{0}] {2}: {3} {4}'.format(VTO, VTCC, whereis(), '>' if INITIAL == s_pp_state else '.', s))
			directive_tokenize_set_dbg_prefix(whereis())
			t = directive_tokenize_list(s)
			if bDirectiveStart:
				cmd = tokens_get(t, s, 1) if 1 < tokens_count(t) else ''
				if is_disable_by_if() and not ('if' == cmd or 'elif' == cmd or 'else' == cmd or 'endif' == cmd or 'ifdef' == cmd or 'ifndef' == cmd):
					return None # ignored by '#if' [directive]
				if 'include' == cmd:
					path = tokens_get(t, s, 2) if 2 < tokens_count(t) else ''
					if 2 < len(path) and '<' == path[0] and '>' == path[-1]:
						if PP_FOR_GCC & flags:
							s_pp_state = DIRECTIVE_BYPASSED # TODO: remove: (*2)
							tokens = ['#', 'include', path]
							return tokens # bypassing '#include <...>' to GCC
						return None # ignoring
					elif 2 < len(path) and '"' == path[0] and '"' == path[-1]:
						path = path[1:-1]
						g_sources.append(Input(path))
					else:
						dbg(0, 0, '{0}: {1}: argument of #include must be "PATH" or <PATH>'.format(whereis(), path))
						exit(-1)
					return None # correctly accepted line [directive(#include)]
			cnt = parse_directive_line(s, t)
			if '\\' == s[len(s)-1]:
				t = tokens_splice(t, 0, -1)
				s_pp_state = DIRECTIVE_CONTINUED
			else:
				if DIRECTIVE_CONTINUED == s_pp_state:
					dbg(LTOKENIZE_PY, LV4, '[{1}.py{0}] {2}: directive: {1}missing{0} \'\\\', {1}multi-line ended{0}.'.format(VTO, VTCC, whereis()))
				s_pp_state = INITIAL
			if cnt < tokens_count(t):
				unexpected_abort('waste tokens', tokens_pos(t, cnt))
		#	if PP_FOR_GCC & flags:
		#		if 'define' == tokens_get(t, s, 1):
		#			s_pp_state = DIRECTIVE_BYPASSED # TODO: remove: (*2)
		#			return tokens_to_list(t, s)
			return None # correctly accepted line [directive]

		# ignored by '#if'
		elif is_disable_by_if():
			return None # ignored by '#if' [directive]

		# linse of C/C++ (ignores all except for '__asm__ ( ... )')
		elif INITIAL == s_pp_state and '//' == s[0:2]: # comment
			return None # empty line with '^//' [C/C++]

	# resolving macro
	if INITIAL == s_pp_state:
		dbg(LTOKENIZE_PY, LV1, '[{1}.py{0}] {2}: > {3}'.format(VTO, VTCC, whereis(), s))
		directive_tokenize_set_dbg_prefix(whereis())
		t = directive_tokenize_list(s) # TODO: need new function just for C/C++ not for macro
		tokenize_get_C_first(s, t)

	tokens = []
	while True:
		got = tokenize_get_C()
		if '\n' == got: # accepts newline in macro's declaration
			s_pp_state = TOKENIZE_CONTINUED
			break
		dbg(LTOKENIZE_PY, LV1, '[{1}.py{0}] \'{2}{3}{0}\''.format(VTO, VTCC, VTGG, got))
		if '' == got and 0 < len(tokens):
			s_pp_state = TOKENIZE_CONTINUED # last tokens
			break
		if '' == got:
			s_pp_state = INITIAL # actual end
			break
		tokens.append(got)
	if 0 == len(tokens) and INITIAL == s_pp_state:
		return None # end of line [C/C++]
	return tokens # 1 more token(s) [C/C++] | no token [in macro]

# /////////////////////////////////////////////////////////////
# // limited C/C++ compile

def unexpected_token_abort(expected: str, actual: str) -> None:
	dbg(0, 0, '{1}error{0}: {2}: must be \'{3}\', but \'{4}\'.'.format(VTO, VTRR, whereis(), expected, actual))
	exit(-1)

# tradeoff concurrency (for readability, flexibility)
global s_cc_state, s_cc_remainder
s_cc_state = 0; s_cc_remainder = ''
global s_cc_tokens, s_cc_i
s_cc_tokens = None; s_cc_i = 0

def cc_reset(tokens: list) -> None:
	global s_cc_tokens, s_cc_i

	s_cc_tokens = tokens; s_cc_i = 0

def cc() -> str | None:
	global s_cc_state, s_cc_remainder
	global s_cc_tokens, s_cc_i
	# alias (not modified)
	tokens = s_cc_tokens

	joined = s_cc_remainder; s_cc_remainder = ''
	while s_cc_i < len(tokens):
		t = tokens[s_cc_i]; s_cc_i += 1
		if 0 == s_cc_state:
			if not '__asm__' == t:
				s_cc_i -= 1
				s_cc_state = 4
				continue
			s_cc_state = 1
			continue
		if 1 == s_cc_state:
			if not '(' == t:
				unexpected_token_abort('(', t)
			s_cc_state = 2
			continue
		if 2 == s_cc_state:
			if ')' == t:
				s_cc_state = 3
				continue
			if not ('"' == t[:1] and '"' == t[-1:]):
			#	unexpected_token_abort('" ... "', t)
				unexpected_abort('{0}unexpected symbol \'{1}{2}{0}\''.format(VTO, VTRR, t))
			token = t[1:-1]
			if '\\n' == token:
				return joined # might be '' (when '\n' only)
			elif '\\t' == token:
				joined += '\t'
			else:
				joined += token
			continue
		if 3 == s_cc_state:
			if not ';' == t:
				unexpected_token_abort(';', t)
			s_cc_state = 0

		if 4 == s_cc_state:
			completed = skip_statement_C(t)
			if completed:
				s_cc_state = 0
			continue
	s_cc_remainder = joined
	return None

# /////////////////////////////////////////////////////////////
# // command-line parameter

# usage: [-m <log-mask>(default: 8)] [-d[d[d[...]]]] [-E|-S] [--no-line|-nl]

# pass1 control
PREPROCESS   = 1
COMPILE      = 2
ASSEMBLE     = 4
# log control
SHOW_LINE    = 8

LMASK_DEFAULT = LASSEMBLE

class Options: # using as struct
	def __init__(self):
		self.flags = 0
		self.log_level = 0
		self.log_mask = 0
		self.output_path = 'a.out'
		self.cmdv = []

def parse_cmdline(argv: list) -> Options:
	ret = Options()
	ret.flags = PREPROCESS|COMPILE|ASSEMBLE|SHOW_LINE
	ret.log_level = 0
	ret.log_mask = LMASK_DEFAULT

	# cmdline parsing
	ret.cmdv = []
	i = 0
	while i +1 < len(sys.argv):
		i += 1
		opt = sys.argv[i]
		if not '-' == opt[:1]:
			ret.cmdv.append(opt)
			continue
		# output object path
		if '-o' == opt and i +1 < len(sys.argv):
			i += 1
			ret.output_path = sys.argv[i];
		# Preprocess only; do not compile or assemble.
		elif '-E' == opt:
			ret.flags &= ~(ASSEMBLE|COMPILE)
		# Compile only; do not assemble.
		elif '-S' == opt:
			ret.flags &= ~ASSEMBLE
		# Hide line number
		elif '--no-line' == opt or '-nl' == opt:
			ret.flags &= ~SHOW_LINE
		# log level
		elif '-d' == opt[0:2]:
			ret.log_level = len(opt) -1
		# log mask
		elif '-m' == opt[:2]:
			if 2 < len(opt):
				ret.log_mask = int(opt[2:])
			elif i +1 < len(sys.argv):
				i += 1
				ret.log_mask = int(sys.argv[i])
			else:
				dbg(0, 0, '{1}invalid option{0}: \'-m\' usage: \'-m<mask>\' or \'-m <mask>\''.format(VTO, VTRR))
				exit(-1)
			if not 0 < ret.log_level:
				ret.log_level = 2 # = '-dd'
		else:
			dbg(0, 0, '{1}invalid option{0}: \'{2}\''.format(VTO, VTRR, opt))
			exit(-1)

	return ret

# /////////////////////////////////////////////////////////////
# // pass1

PASS1_BREAK    = 0
PASS1_CONTINUE = 1
def pass1(flags: int) -> int:
	# getting 1 line
	reader = g_sources[len(g_sources) -1] # front source
	s = reader.enumerate()
	if None == s:
		if 0 == len(g_sources) -1:
			return PASS1_BREAK
		del g_sources[len(g_sources) -1]
		return PASS1_CONTINUE
	fname = reader.fname if 1 < len(g_sources) else ''
	dbg_set_line(fname, reader.lineno, s)

	# preprocess
	preprocess_first(s)
	while True:
		f = PP_FOR_GCC if not COMPILE & flags else 0
		tokens = preprocess(f)
		if None == tokens: # to next line
			break
		if 0 == len(tokens): # no token [in macro]
			continue
		prefix = '{0}: '.format(reader.lineno) if SHOW_LINE & flags else ''
		postfix = '' # '({1}{3}{0}) <- {2}pp{0}()'.format(VTO, VTGG, VTCC, len(tokens))
		if not COMPILE & flags:
			dbg(0, 0, prefix +' '.join(tokens) +postfix)
			continue
	#	# TODO: remove: (*2)
	#	if 0 < len(tokens) and '#' == tokens[0]:
	#		continue

		# compile
		cc_reset(tokens)
		while True:
			s = cc()
			if None == s:
				break
			if not ASSEMBLE & flags:
				dbg(0, 0, s)
				continue

			# assemble
			t = x86_64_intel_tokenize_list(s)
			if 0 == len(t):
				t = bytes((0, 0, 0, 0)) # dummy, but needed for right my()->line in x86_64.as.c
			as_pass1_x86_64_intel(s, t)
	return PASS1_CONTINUE

# /////////////////////////////////////////////////////////////
# // pass2

from elf import elf_start, elf_append, elf_commit
from libif import pass2_result, idstr_from
from ctypes import c_ubyte

from utility import write_binary_file
def pass2(output_path: str) -> None:
	# ELF index order
	elf_index_order = [
		''                  ,
		'.text'             ,
		'.rela.text'        ,
		'.data'             ,
		'.bss'              ,
		'.rodata'           ,
		'.rela.rodata'      ,
		'.comment'          ,
		'.note.GNU-stack'   ,
		'.note.gnu.property',
		'.eh_frame'         ,
		'.rela.eh_frame'    ,
	]
	b = [ c_ubyte(idstr_from(s)) for s in elf_index_order ]
	sec_str_id_from = (c_ubyte * len(b))(*b)

	# '.text' '.rela.text' '.rodata' '.rela.rodata' '.eh_frame' '.rela.eh_frame'
	as_pass2(sec_str_id_from)

	# '.shstrtab'
	s  = '' + '\0'
	s += '.symtab' + '\0'
	s += '.strtab' + '\0'
	s += '.shstrtab' + '\0'
	s += '.rela' + '.text' + '\0'
	s += '.data' + '\0'
	s += '.bss' + '\0'
	s += '.rela' + '.rodata' + '\0'
	s += '.comment' + '\0'
	s += '.note.GNU-stack' + '\0'
	s += '.note.gnu.property' + '\0'
	s += '.rela'
	s += '.eh_frame' + '\0'
	b = s.encode('utf-8')
	shstrtab = (c_ubyte * len(b))(*b)

	elf_start()

	k = ''                  ; elf_append(None           , 0,    0,  0,  0, 0,  0, k)
	k = '.text'             ; elf_append(pass2_result(k), 1,    6,  0,  0, 1,  0, k)
	k = '.rela.text'        ; elf_append(pass2_result(k), 4, 0x40, 12,  1, 8, 24, k)
	k = '.data'             ; elf_append(None           , 1,    3,  0,  0, 1,  0, k)
	k = '.bss'              ; elf_append(None           , 8,    3,  0,  0, 1,  0, k)
	k = '.rodata'           ; elf_append(pass2_result(k), 1,    2,  0,  0, 1,  0, k)
	k = '.rela.rodata'      ; elf_append(pass2_result(k), 4, 0x40, 12,  5, 8, 24, k)
	k = '.comment'          ; elf_append(pass2_result(k), 1, 0x30,  0,  0, 1,  1, k)
	k = '.note.GNU-stack'   ; elf_append(None           , 1,    0,  0,  0, 1,  0, k)
	k = '.note.gnu.property'; elf_append(pass2_result(k), 7,    2,  0,  0, 8,  0, k)
	k = '.eh_frame'         ; elf_append(pass2_result(k), 1,    2,  0,  0, 8,  0, k)
	k = '.rela.eh_frame'    ; elf_append(pass2_result(k), 4, 0x40, 12, 10, 8, 24, k)
	k = '.symtab'           ; elf_append(pass2_result(k), 2,    0, 13, 73, 8, 24, k)
	k = '.strtab'           ; elf_append(pass2_result(k), 3,    0,  0,  0, 1,  0, k)
	k = '.shstrtab'         ; elf_append(shstrtab       , 3,    0,  0,  0, 1,  0, k)

	elf_commit(output_path)

# /////////////////////////////////////////////////////////////
# // entry point

if __name__ == '__main__':
	opt = parse_cmdline(sys.argv)
	if not 1 == len(opt.cmdv):
		dbg(0, 0, '{1}cmdline error{0}: source file not specified.'.format(VTO, VTRR))
		exit(-1)
	output_path = opt.output_path
	input_path = opt.cmdv[0]
	i = input_path.rfind('/')
	obj_fname = input_path[i +1 if -1 < i else 0:]

	if 0 < opt.log_level:
		dbg_set_mask(opt.log_mask)
		dbg_set_level(opt.log_level)
		dbg(0, 0, 'LV{0} debug (mask: {1}) (\'-{2}\', \'-m{1}\')'.format(opt.log_level, opt.log_mask, 'dddd'[:opt.log_level]))
		dbg(0, 0, '')

	validate_bitsize()
	tokenize_init('')

	if True:
		g_sources.append(Input(input_path))

	if True:
		if ASSEMBLE & opt.flags:
			as_pass1_new(obj_fname)
		r = PASS1_CONTINUE
		while PASS1_CONTINUE == r:
			r = pass1(opt.flags)

	if ASSEMBLE & opt.flags:
		pass2(output_path)

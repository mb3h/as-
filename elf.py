
from ctypes import c_ubyte, c_ulong, create_string_buffer
from utility import read_binary_file, write_binary_file
from libif import dbg, strtab_id_from

# /////////////////////////////////////////////////////////////
# // assertion

def assert_(expr: bool, msg: str):
	if (not expr):
		dbg(0, 0, msg)
		exit(-1)
def flow_bug(expr: bool, msg: str): assert_(expr, msg)
def spec_bug(expr: bool, msg: str): assert_(expr, msg)

# /////////////////////////////////////////////////////////////
# // utility

def store_le32(b: (c_ubyte), addr: int, val: c_ulong) -> None:
	b[addr +0] = 255 & val
	b[addr +1] = 255 & val >> 8
	b[addr +2] = 255 & val >> 16
	b[addr +3] = 255 & val >> 24

# /////////////////////////////////////////////////////////////
# // class (using as struct)

class ElfSection:
	def __init__(self, data: (c_ubyte), type: int, attr: int, link: int, info: int, align: int, recsiz: int, idstr: str):
		self.type    = type
		self.attr    = attr
		self.begin   = 0
		self.size    = 0
		self.link    = link
		self.info    = info
		self.align   = align
		self.recsiz  = recsiz
		self.idstr   = idstr

		self.data    = data

# /////////////////////////////////////////////////////////////
# // tradeoff concurrency (for readability, flexibility)

global g_sections; g_sections = []

# /////////////////////////////////////////////////////////////
# // I/F

def elf_start() -> None:
	pass
	# PENDING:

def elf_append(data: (c_ubyte), type: int, attr: int, link: int, info: int, align: int, recsiz: int, idstr: str) -> None:
	# header
	sect = ElfSection(data, type, attr, link, info, align, recsiz, idstr)

	# register
	g_sections.append(sect)

IDSTR_SHSTRTAB = '.shstrtab'
TYPE_NULL = 0 # ELF header (psedou section)
TYPE_RELA = 4 # RELOCATION RECORDS
HDRLEN = 0x0040

def elf_get_section_order() -> list:
	# sorting: (normal) -> TYPE_RELA -> '.shstrtab'
	normal = []; rela = []; last = []
	for index_i in range(len(g_sections)):
		sect = g_sections[index_i]
		if TYPE_RELA == sect.type:
			rela.append(index_i)
		elif IDSTR_SHSTRTAB == sect.idstr:
			last.append(index_i)
		else:
			normal.append(index_i)
	dat2idx = normal + rela + last # data# -> index#
	return dat2idx

def elf_commit(path: str) -> None:
	dat2idx = elf_get_section_order()

	# '.shstrtab' section (-> ELF footer)
	shstrtab = None
	for sect in g_sections:
		if '.shstrtab' == sect.idstr:
			shstrtab = sect.data
			break
	flow_bug(shstrtab, '.shstrtab not found.')

	# ELF body - all sections
	body = bytes(); offset = 0
	for data_i in range(len(dat2idx)):
		sect = g_sections[dat2idx[data_i]]
		# placement of section (-> ELF footer)
		sect.begin = offset
		sect.size = len(sect.data) if sect.data else 0

		# appending section binary
		if sect.data:
			body += sect.data
			offset += len(sect.data)

		# alignment of next section
		if not data_i +1 < len(g_sections):
			align = 8 # end of sections
		else:
			align = g_sections[dat2idx[data_i +1]].align
			align = max(1, align) # safety (division by zero)
		# skip address (for ELF header)
		if '' == sect.idstr: # psedou section
			spec_bug(0 == data_i, '')
			spec_bug(0 == sect.align, '')
			spec_bug(TYPE_NULL == sect.type, '')
			padlen = HDRLEN
		# padding between sections
		else:
			padlen = align -1 -((len(body) +align -1) % align)
			body += (c_ubyte * padlen)(*[0x00] * padlen)
		offset += padlen

	# ELF footer - index of sections
	footer = bytes()
	for sect in g_sections:
		# don't use (c_ulong * 0x10) for avoiding endian-issue
		idx = (c_ubyte * 0x40)(*[0x00] * 0x40)
		name_id = strtab_id_from(shstrtab, len(shstrtab), sect.idstr)
		store_le32(idx, 0x00, name_id    )
		store_le32(idx, 0x04, sect.type  )
		store_le32(idx, 0x08, sect.attr  )
		store_le32(idx, 0x18, sect.begin )
		store_le32(idx, 0x20, sect.size  )
		store_le32(idx, 0x28, sect.link  )
		store_le32(idx, 0x2c, sect.info  )
		store_le32(idx, 0x30, sect.align )
		store_le32(idx, 0x38, sect.recsiz)
		footer += idx

	# ELF header
	header = (c_ubyte * HDRLEN)(*[0x00] * HDRLEN)
	header[0:4] = b'\177ELF'
	store_le32(header, 0x04, 0x010102)
	store_le32(header, 0x10, 0x3e0001)
	store_le32(header, 0x14, 1)
	store_le32(header, 0x28, HDRLEN +len(body))
	store_le32(header, 0x34, 0x0040)
	store_le32(header, 0x38, 0x400000)
	store_le32(header, 0x3c, 0x0e000f)

	elf_obj = bytes(header) + body + footer
	write_binary_file(path, elf_obj)

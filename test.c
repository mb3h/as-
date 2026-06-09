/* PENDING:
	(*1) .................... IFF1 .... Z80 Manual .... IFF2 ................
	(*2) LD (IX+d),n .................... (4,4,3,5,3,3) ...... (4,4,3,5,3) ........
	     ............ Z80 CPU User's Manual ........................................
	(*3) DD/FD CB d xx .. R ....(OP................) .... 3......2......................
 */
/* NOTE:
	(*1) M88 ........................M88 .... ExecXXX .......... reg.af .. b30-b24 ........
	     R ..................../.............. reg.r7 ................................
	     reg.af ..32.......................... reg.r7 ..........................
	    (...... Z80_WORDREG_IN_INT ...................... ExecXXX ........ reg.hl .. b14-b8 ..
	     R .................................................... R ..........................
	     ................................ M88 ................ ( ......) )
 */
#ifndef Z80_C_MENTIONED__

#include <features.h>
#include <endian.h>

#include <stdint.h>
#include <stdbool.h>

#if 1 // ........SP..............................
# define SP_LOAD 
# define SP_PUSH 
#endif
#define MINIMIZED_WORK
///////////////////////////////////////////////////////////////
// inner macros

#define LF "\n"
#define NL LF "\t"
#define LC ".L"
#define OP ".L"
#define  M ".L"
#define _(x) #x

///////////////////////////////////////////////////////////////
// assertion (*)for inline assembler

#if 0 // high performance mode
# define assertnc(/*n,*/operation_bytes, operation) 
# define assertc(/*n,*/operation_bytes, operation) 
# define assertnz(/*n,*/operation_bytes, operation) 
# define assertz(/*n,*/operation_bytes, operation) 
# define assertge(/*n,*/operation_bytes, operation) 
# define assertle(/*n,*/operation_bytes, operation) 
#else // low performance (bug check) mode
__asm__ (
	".text" LF
LC "rva_base:" NL // NOTE: must write before all sorts of '.text' section
);

#  define assert_with_rva_calculated \
		"lea " r8 "," PTR64 LC "rva_base" "[rip]" NL /* 4C 8D 05 xx xx xx xx */ \
		"sub rcx," r8 NL                             /* 4C 29 C1 */ \
		"add rcx,0x74b0 +0x9b92" NL /* NOTE: (*1) */ /* 48 81 C1 xx xx xx xx */
		/* NOTE: (*1) can't support modifying and rebuild.
		              A value of 'readelf -S <this.so> | grep .text' must be reflected.
		              Is it no way to get this value under executing ... ?  */

# define assert_post(/*n,*/operation_bytes) \
		"mov " cpuERRNO "," _(EASSERT) NL                    /* C6 46 2C FF */ \
	/*	"lea rcx," PTR64 LOCAL "_assert" #n "[rip]" */       ".byte 0x48,0x8D,0x0D" NL \
                                                           ".long -7 -4 -2 - " #operation_bytes NL \
		"mov " PTR64 cpuERRPTR ",rcx" NL                     /* 48 89 4E xx */ \
		assert_with_rva_calculated                           /* (0x11)[NOT_SHARED_LIBRARY] or (0x11) */ \
		"mov " PTR16 cpuERRAUX "[0],cx" NL                   /* 66 89 4E xx */ \
		"shr ecx,16" NL                                      /* C1 E9 10 */ \
		"mov " PTR8 cpuERRAUX "[2],cl" NL                    /* 88 4E xx */ \
	/*	"jmp " LC "abort_rsiCPU" LF */                       /* E9 xx xx xx xx | EB xx (NOTE: can't choose) */ \
		"lea rcx," PTR64 LC "abort_rsiCPU" "[rip]" NL        /* 48 8D 0D xx xx xx xx */ \
		"jmp rcx" NL                                         /* FF E1 */
#  define LOCAL_true "0x33" // (0F+11+13)

# define assertnc(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jc " LOCAL "_true" #n */ ".byte 0x73," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/

# define assertc(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jc " LOCAL "_true" #n */ ".byte 0x72," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/

# define assertnz(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jnz " LOCAL "_true" #n */ ".byte 0x75," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/

# define assertz(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jz " LOCAL "_true" #n */ ".byte 0x74," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/

# define assertge(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jge " LOCAL "_true" #n */ ".byte 0x7D," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/

# define assertle(/*n,*/operation_bytes, operation) \
/*	LOCAL "_assert" #n ":" NL*/ \
		operation NL \
	/*	"jle " LOCAL "_true" #n */ ".byte 0x7E," LOCAL_true NL \
		assert_post/*(n)*/(operation_bytes)
/*	LOCAL "_true" #n ":" NL*/
#endif

///////////////////////////////////////////////////////////////
// constants

__asm__ (
	".equ " _(EIO)          ", 5"   NL
#define      EIO              5
#define      EWOULDBLOCK      11
#define      ENOMEM           12
	".equ " _(EFAULT)       ", 14"  NL
	".equ " _(ECPU)         ", 251" NL
#endif //ndef Z80_C_MENTIONED__
#define      ECPU             251
#ifndef Z80_C_MENTIONED__
	".equ " _(EASSERT)      ", 252" NL
#define      EASSERT          252
	".equ " _(EASSERTC)     ", 253" NL
#define      EASSERTC         253
#define      EOPN             254
#define      EFDC             255

	".equ " _(EIO_IN)            ", 0" NL
#define      EIO_IN                0
	".equ " _(EIO_OUT)           ", 4" NL
#define      EIO_OUT               4
#define      EFAULT_SIZE           3
	".equ " _(EFAULT_WRITE)      ", 4" NL
#define      EFAULT_WRITE          4
#define      EFAULT_TYPEMASK       0xF0
	".equ " _(EFAULT_N88EROM)    ", 0xA0" NL
#define      EFAULT_N88EROM        0xA0
	".equ " _(EFAULT_GVRAM)      ", 0x90" NL
#define      EFAULT_GVRAM          0x90
	".equ " _(EFAULT_ALU)        ", 0x80" NL
#define      EFAULT_ALU            0x80
	".equ " _(EFAULT_RAM)        ", 0x70" NL
#define      EFAULT_RAM            0x70
	".equ " _(EFAULT_TVRAM)      ", 0x60" NL
#define      EFAULT_TVRAM          0x60
	".equ " _(EFAULT_ERAM)       ", 0x50" NL
#define      EFAULT_ERAM           0x50
	".equ " _(EFAULT_DICTIONARY) ", 0x40" NL
#define      EFAULT_DICTIONARY     0x40
	".equ " _(EFAULT_CDBIOS)     ", 0x30" NL
#define      EFAULT_CDBIOS         0x30
	".equ " _(EFAULT_EROM)       ", 0x20" NL
#define      EFAULT_EROM           0x20
	".equ " _(EFAULT_N80ROM)     ", 0x10" NL
#define      EFAULT_N80ROM         0x10
	".equ " _(EFAULT_UNKNOWN)    ", 0x00" NL
#define      EFAULT_UNKNOWN        0x00
);

__asm__ (
//	".equ " _(iREAD_x2)   ", 0 * 2" NL // obsolete
//	".equ " _(iWRITE_x2)  ", 1 * 2" NL // obsolete

	// cpu1 (and cpu2)
#define      iIN_32      2
#define      iIN_40      3
#define      iIN_44      4
#define      iIN_45      5
#define      iIN_5C      6
#define      iIN_70      7
#define      iIN_71      8
#define      iIN_E3      9
#define      iIN_E8     10
#define      iIN_E9     11
#define      iIN_EC     12
#define      iIN_ED     13
#define      iIN_F8     14
#define      iIN_FA     15
#define      iIN_FB     16
#define      iIN_ACK    17
#define     iOUT_45     18
#define     iOUT_50     19
#define     iOUT_51     20
#define     iOUT_54     21
#define     iOUT_55     22
#define     iOUT_56     23
#define     iOUT_57     24
#define     iOUT_58     25
#define     iOUT_59     26
#define     iOUT_5A     27
#define     iOUT_5B     28
#define     iOUT_5C     29
#define     iOUT_5D     30
#define     iOUT_5E     31
#define     iOUT_5F     32
#define     iOUT_E8     33
#define     iOUT_E9     34
#define     iOUT_EC     35
#define     iOUT_ED     36
#define     iOUT_FB     37
#define     iOUT_FC     38
#define     iOUT_FD     39
#define     iOUT_FE     40
#define     iOUT_FF     41
	".equ "   _(iCB2) ", 44" NL
#endif //ndef Z80_C_MENTIONED__
#define        iCB2     44
#ifndef Z80_C_MENTIONED__
#define     iOUT_00     44
#define     iOUT_01     45
#define     iOUT_02     46
#define     iOUT_31     47
#define     iOUT_32     48
#define     iOUT_34     49
#define     iOUT_35     50
#define     iOUT_44     51
#define     iOUT_64     52
#define     iOUT_65     53
#define     iOUT_68     54
#define     iOUT_70     55
#define     iOUT_71     56
#define     iOUT_E2     57
#define     iOUT_E3     58
#define     iOUT_E4     59
#define     iOUT_E6     60
#define      iIN_FC     61
#define      iIN_FD     62
#define      iIN_FE     63 // CB_MAX -1 [test.h]
	// cpu2 only
#define     iOUT_F7      2
);

#define LATEST_IO_CLK "(16+3)" // (11+2)OUT(n),A  (12+2)OUT(C),r  (16+3)OUTI

///////////////////////////////////////////////////////////////
// gcc

#define sizeofPTR "8"
#ifdef MINIMIZED_WORK // minimized work > rapid execution
# define sizeofPAD "0"
#else // rapid execution > minimized work
# define sizeofPAD "2"
#endif

//#ifdef _X86_64
#define RELOC ".quad " // x86_64
//#else
//#define RELOC ".long " // i386
//#endif

#define DECL_SHARED_ASM
#include "test.h"
///////////////////////////////////////////////////////////////
// iCPU (#0)

__asm__ (
	".struct 0" LF
M "reg_fa:" LF
M "reg_a:" NL
	".struct " M "reg_a +1" LF
M "reg_f:" NL
	".struct " M "reg_f +1 +" sizeofPAD LF

M "reg_hl:" LF
M "reg_l:" NL
	".struct " M "reg_l +1" LF
M "reg_h:" NL
	".struct " M "reg_h +1 +" sizeofPAD LF

M "reg_de:" LF
M "reg_e:" NL
	".struct " M "reg_e +1" LF
M "reg_d:" NL
	".struct " M "reg_d +1 +" sizeofPAD LF

M "reg_bc:" LF
M "reg_c:" NL
	".struct " M "reg_c +1" LF
M "reg_b:" NL
	".struct " M "reg_b +1 +" sizeofPAD LF

M "reg_ix:" LF
M "reg_xl:" NL
	".struct " M "reg_xl +1" LF
M "reg_xh:" NL
	".struct " M "reg_xh +1 +" sizeofPAD LF

M "reg_iy:" LF
M "reg_yl:" NL
	".struct " M "reg_yl +1" LF
M "reg_yh:" NL
	".struct " M "reg_yh +1 +" sizeofPAD LF

M "reg_sp:" LF
M "reg_spl:" NL
	".struct " M "reg_spl +1" LF
M "reg_sph:" NL
	".struct " M "reg_sph +1 +" sizeofPAD LF

M "reg_af_r:" NL
	".struct " M "reg_af_r +2 +" sizeofPAD LF
M "reg_hl_r:" NL
	".struct " M "reg_hl_r +2 +" sizeofPAD LF
M "reg_de_r:" NL
	".struct " M "reg_de_r +2 +" sizeofPAD LF
M "reg_bc_r:" NL
	".struct " M "reg_bc_r +2 +" sizeofPAD LF
M "reg_pc:" NL
	".struct " M "reg_pc +2 +" sizeofPAD LF

M "reg_i:" NL
	".struct " M "reg_i +1" LF
M "reg_r:" NL
	".struct " M "reg_r +1" LF
M "intmode:" NL
	".struct " M "intmode +1" LF
M "int_control:" NL
	".struct " M "int_control +1" LF
M "state_control:" NL
	".struct " M "state_control +1" NL
	".equ " _(STATE_HALT) ",0x01" LF
//--- TRACE#log-OPCODE.workarea.0z .
#ifdef FULLTRACE_ENABLED
	".equ " _(STATE_OPCODE_TRACE) ",0x02" LF
#endif

M "pad8:" NL
	".struct " M "pad8 +3" LF
M "errno:" NL
	".struct " M "errno +1" LF
M "erraux:" NL
	".struct " M "erraux +3" LF
M "pad9:" NL
	".struct " M "pad9 +4" LF
M "errptr:" NL
	".struct " M "errptr +" sizeofPTR LF

M "debug:" NL
	".struct " M "debug +4" LF
M "goal_clk:" NL
	".struct " M "goal_clk +2" LF
M "ok_clk:" NL
	".struct " M "ok_clk +2" LF
M "eh_rsp:" NL
	".struct " M "eh_rsp +8" LF
#if 1 // TODO: ..Z80....................(......)
M "ioctl:" NL
	".struct " M "ioctl +" sizeofPTR LF
M "lpfn_pre_lea_r:" NL
	".struct " M "lpfn_pre_lea_r +" sizeofPTR LF
M "lpfn_pre_lea_w:" NL
	".struct " M "lpfn_pre_lea_w +" sizeofPTR LF
#endif
#if 0
M "inst:" NL
	".struct " M "inst +" sizeofPTR LF
M "pc_sub_rip:" NL // NOTE: ExecXXX ............OpTable............(..................)
	".struct " M "pc_sub_rip +" sizeofPTR LF
// renew per page-breaking
M "riplim:" NL
	".struct " M "riplim +" sizeofPTR LF
M "instpage:" NL
	".struct " M "instpage +" sizeofPTR LF
#endif
M "ripwait:" NL
	".struct " M "ripwait +4" LF

#if 0
M "ioflags:" NL // link of IOBus::flags [w]PC88::ConnectDevices()
	".struct " M "ioflags +" sizeofPTR LF
M "portmax:" NL
	".struct " M "portmax +4" LF
M "intr:" NL // MODE2 interrupt # // TODO: ...... .int_control ................
	".struct " M "intr +1" LF
#endif
M "flag_b5b3:" NL // b5=F.b5(undef) b3=F.b3(undef) b764210:(ignored)
	".struct " M "flag_b5b3 +1" LF
#if 0
M "eshift:" NL // 1(x2):when CPU1 boost 0(x1):(none) [w]Z80_x86::ExecDual2()
	".struct " M "eshift +1" LF
M "execcount:" NL
	".struct " M "execcount +4" LF
#endif
M "pad11:" NL
	".struct " M "pad11 +3" LF
#if 1 // observe ALU write count
M "obsvALU:" NL
	".struct " M "obsvALU +8" LF
M "obsvGVRAM:" NL
	".struct " M "obsvGVRAM +8" LF
#endif
#ifdef FULLTRACE_ENABLED
M "opcode_trace_chunked:" NL
	".struct " M "opcode_trace_chunked +4" LF
M "opcode_trace_total:" NL
	".struct " M "opcode_trace_total +4" LF
#endif
);
#endif //ndef Z80_C_MENTIONED__

///////////////////////////////////////////////////////////////
// iMEM (#1)

__asm__ (
	".equ " _(ofsN88ROM)  ", 0x00000" NL // 0000-7FFF
//	".equ " _(ofsN80ROM)  ", 0x08000" NL // 6000-7FFF
	".equ " _(ofsN88EROM) ", 0x0C000" NL // 6000-7FFF x4
	".equ " _(ofsDISKROM) ", 0x14000" NL // 0000-1FFF
#define      ofsDISKROM     0x14000
//	".equ " _(ofsN80ROM0) ", 0x16000" NL // 0000-5FFF
	".equ " _(ofsRAM2)    ", 0x1C000" NL // 4000-7FFF
#define      ofsRAM2        0x1C000
	".equ " _(ofsRAM1)    ", 0x20000" NL // 0000-FFFF
#define      ofsRAM1        0x20000
	".equ " _(ofsGVRAM0)  ", 0x30000" NL // 16KB (#5C)
#define      ofsGVRAM0      0x30000
	".equ " _(ofsGVRAM1)  ", 0x34000" NL // 16KB (#5D)
#define      ofsGVRAM1      0x34000
	".equ " _(ofsGVRAM2)  ", 0x38000" NL // 16KB (#5E)
#define      ofsGVRAM2      0x38000
	".equ " _(ofsTVRAM)   ", 0x3C000" NL // 4KB
	".equ " _(ofsNORAM2)  ", 0x3D000" NL // 4KB
#define      ofsNORAM2      0x3D000
#define      MEMSIZE        0x3E000
	".equ " _(ofsMASK)    ", 0x3FF00" NL
		/* WARNING: ........ offset ........ offset - begin ....
		   ..............0x3C000 ...... 0x3E000 ......
		   (0xC000 - 0x6000 = 0x6000 : ofsN88EROM)
		   ......8000-83FF ........................ 0x3FF00 ....
		   ..................
		*/
);

///////////////////////////////////////////////////////////////
// (#0-) control

#define ioctl_item(items) \
	PTR64 M "item_ptr[" items "+" _(iIO ) "*" _(sizeofITEM) "]"
#define memory_item(items) \
	PTR64 M "item_ptr[" items "+" _(iMEM) "*" _(sizeofITEM) "]"

///////////////////////////////////////////////////////////////
// ..................

#define  PTR32 "dword ptr "
#define   PTR8 "byte ptr "
#define  PTR16 "word ptr "
#define  PTR64 "qword ptr "
#ifdef MINIMIZED_WORK
# define Z16PTR PTR16
#else
# define Z16PTR PTR32
#endif

#define rdiITEMS                          "rdi"
#define rsiCPU                            "rsi"

#define  alA                               "al"
#define  ahF                               "ah"
#define  ahA                               "ah" // at 'rol eax,8'
#define  alR                               "al" // at 'rol eax,8'
#define  clF                               "cl"
#define  dlF                               "dl"
#define rax____R_FA                       "rax"
#define cpuB              PTR8 M  "reg_b[" rsiCPU "]"
#define cpuC              PTR8 M  "reg_c[" rsiCPU "]"
#define cpuD              PTR8 M  "reg_d[" rsiCPU "]"
#define cpuE              PTR8 M  "reg_e[" rsiCPU "]"
#define cpuH              PTR8 M  "reg_h[" rsiCPU "]"
#define cpuL              PTR8 M  "reg_l[" rsiCPU "]"
#define cpuXH             PTR8 M "reg_xh[" rsiCPU "]"
#define cpuXL             PTR8 M "reg_xl[" rsiCPU "]"
#define cpuYH             PTR8 M "reg_yh[" rsiCPU "]"
#define cpuYL             PTR8 M "reg_yl[" rsiCPU "]"
#define cpuSPH           PTR8 M "reg_sph[" rsiCPU "]"
#define cpuSPL           PTR8 M "reg_spl[" rsiCPU "]"
#define cpuFA           Z16PTR M "reg_fa[" rsiCPU "]"
#define cpuBC           Z16PTR M "reg_bc[" rsiCPU "]"
#define cpuBC16          PTR16 M "reg_bc[" rsiCPU "]"
#define cpuDE           Z16PTR M "reg_de[" rsiCPU "]"
#define cpuDE16          PTR16 M "reg_de[" rsiCPU "]"
#define cpuHL           Z16PTR M "reg_hl[" rsiCPU "]"
#define cpuHL16          PTR16 M "reg_hl[" rsiCPU "]"
#define cpuIX           Z16PTR M "reg_ix[" rsiCPU "]"
#define cpuIX16          PTR16 M "reg_ix[" rsiCPU "]"
#define cpuIY           Z16PTR M "reg_iy[" rsiCPU "]"
#define cpuIY16          PTR16 M "reg_iy[" rsiCPU "]"
#define cpuSP           Z16PTR M "reg_sp[" rsiCPU "]"
#define cpuSP16          PTR16 M "reg_sp[" rsiCPU "]"
#define cpuREVAF      Z16PTR M "reg_af_r[" rsiCPU "]"
#define cpuREVHL      Z16PTR M "reg_hl_r[" rsiCPU "]"
#define cpuREVDE      Z16PTR M "reg_de_r[" rsiCPU "]"
#define cpuREVBC      Z16PTR M "reg_bc_r[" rsiCPU "]"
#define cpuPC           Z16PTR M "reg_pc[" rsiCPU "]"
#define cpuPC16          PTR16 M "reg_pc[" rsiCPU "]"
#define cpuI               PTR8 M "reg_i[" rsiCPU "]"
#define cpuR8              PTR8 M "reg_r[" rsiCPU "]" // b7: upd/ref at LD R,A / LD A,R only. b6-b0: activate on eax.b30-b24 in Exec0
#define cpuINT       PTR8 M "int_control[" rsiCPU "]"
#define cpuSTATE   PTR8 M "state_control[" rsiCPU "]"
#define cpu__X_X___  PTR8 M   "flag_b5b3[" rsiCPU "]" // F........................
#define cpuEHRSP         PTR64 M "eh_rsp[" rsiCPU "]"
#define cpuERRNO          PTR8 M  "errno[" rsiCPU "]" // TODO: ..Z80....................(......)
#define cpuERRAUX              M "erraux[" rsiCPU "]" // TODO: ..Z80....................(......)
#define cpuERRPTR              M "errptr[" rsiCPU "]" // TODO: ..Z80....................(......)
#define cpuIOCTL           PTR64 M      "ioctl[" rsiCPU "]"
#define cachePRE_LEA_R_dxADDR  PTR64 M "lpfn_pre_lea_r[" rsiCPU "]"
#define cachePRE_LEA_W_dxADDR  PTR64 M "lpfn_pre_lea_w[" rsiCPU "]"
#define  bpCLK                        "bp"

#define r8                           "r8"
#define r8eaPC                       "r8"
#define r8eaPCw                      "r8w"
#define rcxRIP                       "rcx"
#define rdxPC                        "rdx"
#define  dxPC                         "dx"
#define rbx__MNMXPC                  "rbx"
#define ebxMXPC                      "ebx"
#define  bxPC                         "bx"
#define edxMXPC                      "edx"
#define  dxBANKBEGIN                  "dx"
#define  dxBANKEND                    "dx"
#define  dxSP                         "dx"
#define rcxSP                        "rcx"
#define rdxADDR                      "rdx"
#define edxADDR                      "edx"
#define  dxADDR                       "dx"
#define rcxADDR                      "rcx"
#define  cxADDR                       "cx"
#define rbxADDR                      "rbx"
#define  bxADDR                       "bx"
#define edxNN                        "edx"
#define  dxNN                         "dx"
#define edxN                         "edx"
#define rcxIX                        "rcx"
#define  cxIX                         "cx"
#define rcxIY                        "rcx"
#define  cxIY                         "cx"
#define  cxBC                         "cx"
#define cpuGOAL_CLK   Z16PTR M  "goal_clk[" rsiCPU "]"
#define cpuOK_CLK     Z16PTR M    "ok_clk[" rsiCPU "]"

#define   clT         "cl"
#define   chS         "ch"
#define  cxST         "cx"
#define rcxST        "rcx"
#define   dlV         "dl"
#define   dlVs        "dl"
#define   dhU         "dh"
#define edxUV        "edx"
#define  dxUV         "dx"
#define rdxUV        "rdx"
#define  rdxVs       "rdx"
#define   dxVs        "dx"
#define ebxPQ        "ebx"
#define  bxPQ         "bx"
#ifdef MINIMIZED_WORK
# define aFA          "ax"
# define dUV          "dx"
# define cST          "cx"
# define bPQ          "bx"
# define dSP          "dx"
# define cSP          "cx"
#else
# define aFA         "eax"
# define dUV         "edx"
# define cST         "ecx"
# define bPQ         "ebx"
# define dSP         "edx"
# define cSP         "ecx"
#endif
#ifdef MINIMIZED_WORK
# define movzx_from_cpu(r16, cpu16) "movzx r" r16 "," cpu16 NL
# define   movzx_to_cpu(cpu16, r16)   "mov " cpu16 "," r16 /*NL*/
#elif defined(__i386__)
# define movzx_from_cpu(r16, cpu32)   "mov e" r16 "," cpu32 NL
# define   movzx_to_cpu(cpu32, r16) "movzx " cpu32 "," r16 /*NL*/
#elif defined(__x86_64__)
# define movzx_from_cpu(r16, cpu32)   "xor r" r16 ",r" r16 NL "mov r" r16 "," cpu32 NL
# define   movzx_to_cpu(cpu32, r16) "movzx " cpu32 "," r16 /*NL*/
#endif

#define CF          "0x01"
#define NF          "0x02"
#define PF          "0x04"
#define HF          "0x10"
#define ZF          "0x40"
#define SF          "0x80"
#define __X_X___    "(0x20 + 0x08)"
#define SZ____N_    "(" SF "+" ZF               "+" NF        ")"
#define S_______        SF
#define _Z______               ZF
#define ___H_PN_    "("               HF "+" PF "+" NF        ")"
#define ___H__NC    "("               HF        "+" NF "+" CF ")"
#define ___H__N_    "("               HF        "+" NF        ")"
#define ___H___C    "("               HF               "+" CF ")"
#define ___H____                      HF
#define _____P__                             PF
#define _____P_C    "("                      PF        "+" CF ")"
#define _____PN_    "("                      PF "+" NF        ")"
#define ______N_                                    NF
#define _______C                                           CF

///////////////////////////////////////////////////////////////
// ........

#define mov_SZHPC_ahF    "sahf"
#define mov_ahF_SZHPC    "lahf"
#define MOVdl(dst8, src8) "mov dl," src8 NL \
                          "mov " dst8 ",dl" NL
#define SETIM(n) \
	"and " cpuINT ",not " _(INT_MODE) NL \
	"or " cpuINT "," #n NL

#define M1W      // 8801, etc.
#define CLKm1(n, m)      "add " bpCLK "," #n " -(0 " M1W ")" NL
#define CLK0(n, m)       "add " bpCLK "," #n NL
#define CLK1(n, m)       "add " bpCLK "," #n M1W NL
#define CLK1LF(n, m)     "add " bpCLK "," #n M1W LF
#define CLK2(n, m)       "add " bpCLK "," #n M1W M1W NL
#define CLK2LF(n, m)     "add " bpCLK "," #n M1W M1W LF

///////////////////////////////////////////////////////////////
// PC ....

#define incPC \
	"inc " r8eaPC NL \
	"inc " bxPC NL \
	"mov rdx," rbx__MNMXPC NL \
	"shr rdx,16" NL \
	"cmp " bxPC "," dxBANKEND NL \
/*	"jbe " LOCAL "_same_page" #n */ ".byte 0x76,0x05" NL \
	"call " LC "SetPCbx" /* E8 xx xx xx xx */ \
/*LOCAL "_same_page" #n ":" NL */

#define decPCret \
	"dec " r8eaPC NL \
	"dec " bxPC NL \
	"mov rdx," rbx__MNMXPC NL \
	"shr rdx,32" NL \
	"cmp " bxPC "," dxBANKBEGIN NL \
	"jc " LC "SetPCbx" NL \
	"ret" NL

#define rbxRIPsub2ret "sub " r8eaPC ",2" NL \
                      "sub " bxPC ",2" NL \
                   /* JPIFPGBRK(1)*/ \
                      "ret" LF /* C3 */ \
                   /* "add " INST "," INST2PC_NEG NL*/ \
                   /* "jmp SetPC" LF*/

#define JPdxADDR "jmp " LC "SetPCdx" LF

#ifdef FULLTRACE_ENABLED
# define FTRACE_set_flags(n, val) \
								"test " cpuSTATE "," _(STATE_OPCODE_TRACE) NL \
							/*	"jz " LOCAL "_not_ftrace"*/ ".byte 0x74,0x05" NL \
								"or " PTR8 "[" r10FTRACE " -" #n " -1]," #val /* 41 80 4A xx 01 */ NL \
						/*	LOCAL "_not_ftrace:" NL*/
#else
# define FTRACE_set_flags(n, val) 
#endif
#define JRrdxVs     assertge(4, "cmp " rdxVs ",-128") /* 48 83 FA 80 */ \
                    assertle(4, "cmp " rdxVs  ",127") /* 48 83 FA 7F */ \
								FTRACE_set_flags(2, 0x01) \
                        "add " r8eaPC "," rdxVs NL /*b63-b16..............*/ \
                        "add " bxPC "," dxVs NL \
                        "jmp " LC "SetPCbx" LF

///////////////////////////////////////////////////////////////

#define GFNSTART(label) \
	__asm__ ( \
		"\t" ".text" NL \
		".globl " "z80_" #label NL \
		".type " "z80_" #label ",@function" LF \
	"z80_" #label ":" NL \
		".cfi_startproc" NL
#define GFNEND0(label) \
		".cfi_endproc" NL \
		".size " "z80_" #label ",.-" "z80_" #label LF LF "#--------------------------------------------------------------" LF \
	);

#define FNSTART(label) \
	__asm__ ( \
		"\t" ".text" NL \
		".type " #label ",@function" LF \
	#label ":" NL \
		".cfi_startproc" NL
#define FNEND(label) \
		"ret" NL \
		FNEND0(label)
#define FNEND0(label) \
		".cfi_endproc" NL \
		".size " #label ",.-" #label LF LF "#--------------------------------------------------------------" LF \
	);
#define LCFNSTART(label) \
	__asm__ ( \
		"\t" ".text" NL \
		".type " LC #label ",@function" LF \
	LC #label ":" NL \
		".cfi_startproc" NL
#define LCFNEND(label) \
		"ret" NL \
		LCFNEND0(label)
#define LCFNEND0(label) \
		".cfi_endproc" NL \
		".size " LC #label ",.-" LC #label LF LF "#--------------------------------------------------------------" LF \
	);

#define OPFUNC(label) \
	__asm__ ( \
		"\t" ".text" NL \
		".type " OP #label ",@function" LF \
	OP #label ":" NL \
		".cfi_startproc" NL
#define  OPEND(label) \
		"ret" NL \
		OPEND0(label)
#define OPEND0(label) \
		".cfi_endproc" NL \
		".size " OP #label ",.-" OP #label LF LF "#--------------------------------------------------------------" LF \
	);

///////////////////////////////////////////////////////////////
// assertion (*)for C/C++ (2/2)

GFNSTART(abortC_thunk)
#define LOCAL LC _(abortC_thunk)
#define dilERRNO "dil"
//#define rsiCPU   "rsi"
//	"mov rsi," rsiCPU NL
	"mov " cpuERRNO "," dilERRNO NL
	"jmp " LC "abort_rsiCPU" NL
#undef LOCAL
#undef dilERRNO
//#undef rsiCPU
GFNEND0(abortC_thunk)
#ifndef Z80C_GOAL
# define abortC_thunk z80_abortC_thunk
#else //def Z80C_GOAL
# define abortC_thunk(errno_, cpu) \
	cpu->err.no = errno_;
#endif

///////////////////////////////////////////////////////////////
// .................... PC ....

LCFNSTART(SetPCdx)
#define LOCAL LC _(SetPCdx)
   "sub " r8eaPCw "," bxPC NL
   "jnc " LOCAL "_sub_nc" NL
   "sub " r8eaPC ",0x10000" LF
LOCAL "_sub_nc:" NL
   "add " r8eaPCw "," dxADDR NL
   "jnc " LOCAL "_add_nc" NL
   "add " r8eaPC ",0x10000" LF
LOCAL "_add_nc:" NL
   "mov " bxPC "," dxADDR LF
#undef LOCAL

LC _(SetPCbx) ":" NL
#define LOCAL LC _(SetPCbx)
#if 0
	"mov dx," bxPC NL
	"call " LC "cpuLEA_R_dxADDR" NL
	"mov " r8 "," rcxRIP NL

#else
#define rcxeaPC      "rcx"
	"mov rdx," rbx__MNMXPC NL
	"ror rdx,16" NL
	"and edx,edx" NL
	"jz " LOCAL "_out_of_range" NL
	"cmp " bxPC "," dxBANKEND NL
	"ja " LOCAL "_out_of_range" NL
	"ror rdx,16" NL
	"cmp " bxPC "," dxBANKBEGIN NL
	"jnc " LOCAL "_in_range" LF

LOCAL "_out_of_range:" NL
	"mov dx," bxPC NL
	"call " cachePRE_LEA_R_dxADDR NL
	"jc " LOCAL "_false" NL

	"mov bx,cx" NL
	"and bx,0x00FC" NL
	"shl ebx,10 -2 +16" NL // BANKBEGIN = (c.b7-2<<10) -> b:MN----
	"mov bh,ch" NL
	"and bx,0xFC00" NL
//	"shl bx,10 -10" NL
	"dec bx" NL // BANKEND = (c.b15-10<<10) -1 -> b:--MX--
	"shl rbx,16" NL
	"mov bx," dxPC NL
	"shr ecx,16 -8" NL
	"and rcx," _(ofsMASK) NL // ofsXXX = (c.b25-16<<8)
	"add cx," dxPC NL // +PC
	"jnc " LOCAL "_add_nc" NL
	"add ecx,0x10000" LF
LOCAL "_add_nc:" NL
	"add rcx," memory_item(rdiITEMS) NL
	"mov r8," rcxeaPC LF

LOCAL "_in_range:" NL
	"clc" LF
LOCAL "_false:" NL
#endif
#undef rcxeaPC
#undef LOCAL
LCFNEND(SetPCbx)

///////////////////////////////////////////////////////////////

#define mov_ra54_from(r16) \
	"ror rax,32" NL \
	"mov ax," r16 NL \
	"rol rax,32" NL
#define mov_ra54_to(r16) \
	"ror rax,32" NL \
	"mov " r16 ",ax" NL \
	"rol rax,32" NL
///////////////////////////////////////////////////////////////
// Bus I/O
// @note (*a)........PRES........OUT............................
//           ..................................................C......................
//           IIOBus::Out................Z80_x86::Bus_Out................
// CAUTION: (*1) ............................ sprintf() ................................................
//               /lib/x86_64-linux-gnu/libc.so.6::sprintf ........ movaps ..................(!)
//               ...... sprintf() ................

#define cb_this(ioctl, i_x2) PTR64 M "cb_infos[" ioctl "+" i_x2 "*" _(sizeofCBINFOdiv2) "][" M "cb_this]"
#define cb_lpfn(ioctl, i_x2) PTR64 M "cb_infos[" ioctl "+" i_x2 "*" _(sizeofCBINFOdiv2) "][" M "cb_lpfn]"
#define in_type(ioctl, port)  PTR8 M "in_types[" ioctl "+" port "*" _(sizeofINTYPE) "][" M "in_type]"
#define in_fixed(ioctl, port) PTR8 M "in_types[" ioctl "+" port "*" _(sizeofINTYPE) "][" M "in_fixed]"
#define in_aux(ioctl, port)   PTR8 M "in_types[" ioctl "+" port "*" _(sizeofINTYPE) "][" M "in_aux]"
#define in_last(ioctl, port)  PTR8 M "in_types[" ioctl "+" port "*" _(sizeofINTYPE) "][" M "in_last]"
#define out_type(ioctl, port) PTR8 M "out_types[" ioctl "+" port "*" _(sizeofOUTTYPE) "][" M "out_type]"
#define out_last(ioctl, port) PTR8 M "out_types[" ioctl "+" port "*" _(sizeofOUTTYPE) "][" M "out_last]"
#define out_aux(ioctl, port)  PTR8 M "out_types[" ioctl "+" port "*" _(sizeofOUTTYPE) "][" M "out_aux]"
#define out_5x(ioctl)         PTR8 M    "out_5x[" ioctl "]"

FNSTART(Bus_Out)
#define LOCAL LC _(Bus_Out)
#define  r9IOCTL   "r9"
#define rdxPORT   "rdx"
#define  dlPORT    "dl"
#define  clN       "cl"
#define  chN       "ch"
#define  dhN       "dh"
#define  clTYPE    "cl"

#define  blCBi     "bl"
#define edxCBi    "edx"
#define rdxCBi_x2 "rdx"
#define  cxCLK     "cx"
#define r9lpfn     "r9"
assertc(7,
	"cmp " rdxPORT "," _(PORT_MAX)) /* 48 81 FA 00 01 00 00 */
	"shl rbx,8" NL

	"mov r9," cpuIOCTL NL
	"mov ch," clN NL
	"mov cl," out_type(r9IOCTL, rdxPORT) NL
	"dec " clTYPE NL
	"jz " LOCAL "_store_only" NL
	"dec " clTYPE NL
	"jz " LOCAL "_aux_callback" NL
	"cmp " clTYPE "," _(CB_MAX) " -" _(iCB2) " +1" NL
	"jc " LOCAL "_store_and_callback" NL

//LOCAL "_not_launched:" NL
	"shr rbx,8" NL
	"mov " cpuERRNO "," _(EIO) NL
	"mov " PTR8 cpuERRAUX "[0]," _(EIO_OUT) NL
	"mov " PTR8 cpuERRAUX "[1]," dlPORT NL
	"mov " PTR8 cpuERRAUX "[2]," chN NL
#if 0 // ......
	"mov dx," bxPC NL
	"call " cachePRE_LEA_R_dxADDR NL
	"mov " PTR8 cpuERRAUX "[3],cl" NL
#endif
	"jmp " LC "abort_rsiCPU2" LF // NOTE: cannot skip because modified bpCLK must not be accepted

LOCAL "_store_only:" NL
	"mov cl," chN NL
	"mov " out_last(r9IOCTL, rdxPORT) "," clN NL
	"shr rbx,8" NL
	"ret" LF

LOCAL "_aux_callback:" NL
	"mov bl," out_aux(r9IOCTL, rdxPORT) NL
	"mov cl," chN NL
	"jmp " LOCAL "_callback" LF

LOCAL "_store_and_callback:" NL
	"mov bl," clTYPE NL
	"mov cl," chN NL
	"add bl," _(iCB2) " -1" NL
// not 'jump' but 'call' for hander getting old .out_last
	"mov dh," clN NL
	"push rdx" NL // (-0x38)

	"call " LOCAL "_callback" NL // (-0x40)

	"pop rdx" NL
	"mov cl," dhN NL
	"mov r9," cpuIOCTL NL
	"movzx edx," dlPORT NL
	"mov " out_last(r9IOCTL, rdxPORT) "," clN NL
	"ret" LF

LOCAL "_callback:" NL
assertc(3,
	"cmp " blCBi "," _(CB_MAX)) /* 80 FB xx */
	"mov ch," dlPORT NL
	"movzx rdx," blCBi NL
	"add " edxCBi "," edxCBi NL
	"push rdi" NL     // (-0x38) PENDING: r8..r15 ............
	"mov rdi," cb_this(r9IOCTL, rdxCBi_x2) NL
assertnz(8,
	"test " cb_lpfn(r9IOCTL, rdxCBi_x2) ",-1") /* 49 F7 04 D1 FF FF FF FF */
	"mov r9," cb_lpfn(r9IOCTL, rdxCBi_x2) NL
	// arg1(rdi)=.this_  arg2(rsi)=cpu  arg3(dx)=ok_clk arg4(c2:ch:cl)=I/Ocycle-end:port:data
	"push rsi" NL     // (-0x40)
	"ror ecx,16" NL
	"mov cx," bpCLK NL
	"mov " dUV "," cpuOK_CLK NL
	"sub " cxCLK "," cpuOK_CLK NL
assertc(4,
	"cmp cx," LATEST_IO_CLK "+1") /* 66 83 F9 xx */
	"rol ecx,16" NL
	"push " r8eaPC NL // (-0x48)
	"shr rbx,8" NL
	mov_ra54_from(bxPC) /* TODO: ...................... rbx ............ MXMN ................
	                             Exec0 .... __MXMN__.broken_check ......................
	                             ........ rbx ........................................
	                             ........ bxPC ................................ */
	"mov " cpuPC16 "," bxPC NL //
	"push rax" NL     // (-0x50) ahF amR .... (..)__stdcall .................. // PENDING: r8..r15 ............
	"call r9" NL      // CAUTION: may cause SIGSERV out of 16 bytes alignment (*1)
	"pop rax" NL
	mov_ra54_to(bxPC)
	"pop " r8eaPC NL
	"pop rsi" NL
	"pop rdi" NL
#undef LOCAL
#undef  r9IOCTL
#undef rdxPORT
#undef  dlPORT
#undef  clN
#undef  chN
#undef  dhN
#undef  clTYPE

#undef  blCBi
#undef edxCBi
#undef rdxCBi_x2
#undef  cxCLK
FNEND(Bus_Out)

#define OUT_rdxPORT_clN "call Bus_Out" NL // (-0x30)

///////////////////////////////////////////////////////////////
// @note (*a)Bus_Out................PIACK PIAC2......IN............................
//           ........................................O_INTR............
//           ....Z80_x86::Bus_In....................
// CAUTION: (*1) ............................ sprintf() ................................................
//               /lib/x86_64-linux-gnu/libc.so.6::sprintf ........ movaps ..................(!)
//               ...... sprintf() ................

FNSTART(Bus_In)
#define LOCAL LC _(Bus_In)
#define  r9IOCTL   "r9"
#define rdxPORT   "rdx"
#define  dlPORT    "dl"
#define  alPORT    "al"
#define  clTYPE    "cl"
#define  clN       "cl"

#define  chPORT    "ch"
#define  blCBi     "bl"
#define edxCBi    "edx"
#define rdxCBi_x2 "rdx"
#define  cxCLK     "cx"
	"mov r9," cpuIOCTL NL
	"jnc " LOCAL "_normal" NL
	"add " r9IOCTL "," M "ex_in_types" " - " M "in_types" LF
LOCAL "_normal:" NL

assertc(7,
	"cmp " rdxPORT "," _(PORT_MAX)) /* 48 81 FA 00 01 00 00 */
	"shl rbx,8" NL

	"mov cl," in_type(r9IOCTL, rdxPORT) NL
	"dec " clTYPE NL
	"jz " LOCAL "_fixed_value" NL
	"dec " clTYPE NL
	"jz " LOCAL "_callback_only" NL
	"cmp " clTYPE "," _(CB_MAX) " -" _(iCB2) " +1" NL
	"jc " LOCAL "_callback_and_store" NL

//LOCAL "_not_launched:" NL
	"shr rbx,8" NL
	"mov " cpuERRNO "," _(EIO) NL
	"mov " PTR8 cpuERRAUX "[0]," _(EIO_IN) NL
	"mov " PTR8 cpuERRAUX "[1]," dlPORT NL
#if 0 // ......
	"mov dx," bxPC NL
	"call " cachePRE_LEA_R_dxADDR NL
	"mov " PTR8 cpuERRAUX "[3],cl" NL
#endif
	"jmp " LC "abort_rsiCPU2" LF // NOTE: cannot skip because modified bpCLK must not be accepted

LOCAL "_fixed_value:" NL
	"mov " clN "," in_fixed(r9IOCTL, rdxPORT) NL
	"shr rbx,8" NL
	"ret" LF

LOCAL "_callback_and_store:" NL
	"mov bl," clTYPE NL
	"sub rsp,8" NL
	"add bl," _(iCB2) " -1" NL
	"mov al," dlPORT NL
	"call " LOCAL "_callback" NL
	"mov r9," cpuIOCTL NL
	"movzx rdx," alPORT NL
	"add rsp,8" NL
	"mov " in_last(r9IOCTL, rdxPORT) "," clN NL
	"ret" LF

LOCAL "_callback_only:" NL
	"mov bl," in_aux(r9IOCTL, rdxPORT) LF
LOCAL "_callback:" NL
assertc(3,
	"cmp " blCBi "," _(CB_MAX)) /* 80 FB xx */
	"mov ch," dlPORT NL
	"movzx rdx," blCBi NL
	"add " edxCBi "," edxCBi NL
	"push rdi" NL     // (-0x38) PENDING: r8..r15 ............
	"mov r9," cpuIOCTL NL
	"mov rdi," cb_this(r9IOCTL, rdxCBi_x2) NL
assertnz(8,
	"test " cb_lpfn(r9IOCTL, rdxCBi_x2) ",-1") /* 49 F7 04 D1 FF FF FF FF */
	"mov r9," cb_lpfn(r9IOCTL, rdxCBi_x2) NL
	// arg1(rdi)=.this_  arg2(rsi)=cpu  arg3(dx)=ok_clk arg4(c2:ch:cl)=I/Ocycle-begin:port:data
	"push rsi" NL     // (-0x40)
	"ror ecx,16" NL
	"mov cx," bpCLK NL
	"mov " dUV "," cpuOK_CLK NL
	"sub " cxCLK "," cpuOK_CLK NL
assertc(4,
	"cmp cx," LATEST_IO_CLK "+1") /* 66 83 F9 xx */
//	"sub cx,4" NL // NOTE: Bus_In() .. I/O......................................
	"rol ecx,16" NL
	"push " r8eaPC NL // (-0x48)
	"shr rbx,8" NL
	mov_ra54_from(bxPC)
	"mov " cpuPC16 "," bxPC NL //
	"push rax" NL     // (-0x50) ahF amR .... (..)__stdcall .................. // PENDING: r8..r15 ............
	"call r9" NL      // CAUTION: may cause SIGSERV out of 16 bytes alignment (*1)
	"mov cl,al" NL
	"pop rax" NL
	mov_ra54_to(bxPC)
	"pop " r8eaPC NL
	"pop rsi" NL
	"pop rdi" NL
#undef LOCAL
#undef  r9IOCTL
#undef rdxPORT
#undef  dlPORT
#undef  alPORT
#undef  clTYPE
#undef  clN

#undef  chPORT
#undef  blCBi
#undef edxCBi
#undef rdxCBi_x2
#undef  cxCLK
FNEND(Bus_In)

#define IN_clN_rdxN \
	"clc" NL \
	"call Bus_In" NL // (-0x30)

///////////////////////////////////////////////////////////////
// iMEM (#1) - cpu1 (*)PC-8801 ....

#if 1 // TODO: ..............................
LCFNSTART(cpu1_getinfo_R_dxADDR)
#define LOCAL LC _(cpu1_getinfo_R_dxADDR)
#define rcxIOCTL    "rcx"
	"rol eax,16" NL
	"cmp " dxADDR ",0x8000" NL
	"mov rcx," ioctl_item(rdiITEMS) NL
	"jnc " LOCAL "_8000" NL
	/*  (24KB)     (8KB)
	     0000      6000        #E2      #31      #99      #71      #32
	     ERAM        ..   -------1 --------
	      RAM        ..   -------0 ------1- 
	  CDBIOS1        ..         .. -----00- ---1----
	  CDBIOS2        ..         .. -----10-       ..
	   N88ROM        ..         .. -----00- ---0---- 11111111 # ...... ............
	   N88ROM    EROM#N         ..       ..       .. NNNNNNN1 (N=0-6 ..................)
	   N88ROM N88EROM#N         ..       ..       .. -------0 ------NN
	   N80ROM        ..         .. -----10-       .. 
	 */
	"test " out_last(rcxIOCTL, "0xE2") ",0x01" NL
	"mov al," _(EFAULT_ERAM) NL
	"jnz " LOCAL "_false" NL
	"test " out_last(rcxIOCTL, "0x31") ",0x02" NL
	"jnz " LOCAL "_RAM_0000" NL
	"test " out_last(rcxIOCTL, "0x99") ",0x10" NL
	"mov al," _(EFAULT_CDBIOS) NL
	"jnz " LOCAL "_false" NL
	"test " out_last(rcxIOCTL, "0x31") ",0x04" NL
	"mov al," _(EFAULT_N80ROM) NL
	"jnz " LOCAL "_false" NL
	"cmp " dxADDR ",0x6000" NL
	"jc " LOCAL "_ROM_0000" NL
	"test " out_last(rcxIOCTL, "0x71") ",0x01" NL
	"jz " LOCAL "_N88EROM" NL
#if 0 // PENDING: 1111111- e[1-7].rom ..................
	"cmp " out_last(rcxIOCTL, "0x71") ",0xFF" NL
	"mov al," _(EFAULT_EROM) NL
	"jnz " LOCAL "_false" NL
#endif
	"jmp " LOCAL "_ROM_6000" LF

LOCAL "_N88EROM:" NL
#if 0
	"mov al," out_last(rcxIOCTL, "0x32") NL
	"and al,3" NL
	"mov al," _(EFAULT_N88EROM) NL
	"jnz " LOCAL "_false" NL
//LOCAL "_N88ROM_0:" NL
	"mov ecx,(" _(ofsN88EROM) "-" _(0x6000) ") << (16 -8) | 0x6000 >> (10 -2) | 0x8000 >> (10 -10)" NL // 6000-7FFF
#else
	"mov cl," out_last(rcxIOCTL, "0x32") NL
	"and ecx,3" NL
	"shl ecx,13 +8" NL // x 0x2000 << (16 -8)
	"add ecx,(" _(ofsN88EROM) "-" _(0x6000) ") << (16 -8) | 0x6000 >> (10 -2) | 0x8000 >> (10 -10)" NL // 6000-7FFF
#endif
	"jmp " LOCAL "_true" LF

LOCAL "_ROM_0000:" NL
	"mov ecx,(" _(ofsN88ROM) "-" _(0x0000) ") << (16 -8) | 0x0000 >> (10 -2) | 0x6000 >> (10 -10)" NL // 0000-5FFF
	"jmp " LOCAL "_true" LF
LOCAL "_ROM_6000:" NL
	"mov ecx,(" _(ofsN88ROM) "-" _(0x0000) ") << (16 -8) | 0x6000 >> (10 -2) | 0x8000 >> (10 -10)" NL // 6000-7FFF
	"jmp " LOCAL "_true" LF

LOCAL "_RAM_0000:" NL
	"cmp " dxADDR ",0x6000" NL
	"jnc " LOCAL "_RAM_6000" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x0000 >> (10 -2) | 0x6000 >> (10 -10)" NL // 0000-5FFF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_6000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x6000 >> (10 -2) | 0x8000 >> (10 -10)" NL // 6000-7FFF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_8000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x8000 >> (10 -2) | 0x8400 >> (10 -10)" NL // 8000-83FF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_8400:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x8400 >> (10 -2) | 0xC000 >> (10 -10)" NL // 8400-BFFF
	"jmp " LOCAL "_true" LF

LOCAL "_8000_83FF:" NL
	"test " out_last(rcxIOCTL, "0x31") ",0x06" NL
	"jnz " LOCAL "_RAM_8000" NL // TODO: TXTorWND .... OFF
	"movzx ecx," out_last(rcxIOCTL, "0x70") NL
	"cmp cl,0xFD" NL
	"jnc " LOCAL "_false" NL // TODO: FD00-, FE00-, FF00- ............ (*)SetPC .... and FC00, and 00FC ........
	"shl ecx,8 +(16 -8)" NL
	"add ecx,(" _(ofsRAM1) "-" _(0x8000) ") << (16 -8) | 0x8000 >> (10 -2) | 0x8400 >> (10 -10)" NL // 8000-83FF
	"jmp " LOCAL "_true" LF

LOCAL "_8000:" NL
	"cmp " dxADDR ",0x8400" NL
	"jc " LOCAL "_8000_83FF" NL
	"cmp " dxADDR ",0xC000" NL
	"jc " LOCAL "_RAM_8400" LF
//	"jmp " LOCAL "_C000" NL

	/*  (12KB) (4KB)
	     C000  F000       #F1  IN(31h)      #32      #35      #F0
	   ....#N    ..  -------0          -------- -------- ---NNNNN
	      ALU    ..  -------1          -1------ 1-XX----          (*1)(*2)
	   GVRAM0    ..        ..          -0------ (*)OUT #5C ..........
	   GVRAM1    ..        ..                .. (*)OUT #5D ..
	   GVRAM2    ..        ..                .. (*)OUT #5E ..
	      RAM TVRAM        .. -1------ -0-0---- (*)OUT #5F .. ...... ............
	       ..   RAM        .. -X------ -0-X---- (*)..
	      RAM TVRAM        .. -1------ -1-0---- 0-------         
	       ..   RAM        .. -X------ -1-X---- 0-------         
	 (*1) #32.b6=1..GVRAM....(OUT(5[C-E]))................=0............OUT....(M88....)
	 (*2) #35.b54..ALUwr.......... (*3)
	 (*3) ......0....#34..RESET/SET/INVERT....
	      OUT(34) -BBB-AAA   BA=00:RESET 01:SET 10:INVERT 11:PASS
	               210 210  (GVRAM#)
	 */
LOCAL "_C000:" NL
	// TODO: check ....#N
	"test " out_last(rcxIOCTL, "0x32") ",0x40" NL
	"jnz " LOCAL "_ALU_or_RAM" NL
	"mov al," out_5x(rcxIOCTL) NL
	"cmp al,3" NL
	"jz " LOCAL "_TVRAM_or_RAM" LF

LOCAL "_GVRAM:" NL
assertc(2,
	"cmp al,3") /* 3C 03 */
	"movzx ecx,al" NL
	"shl ecx,14 +(16 -8)" NL // x 0x4000
	"add ecx,(" _(ofsGVRAM0) "-" _(0xC000) ") << (16 -8) | 0xC000 >> (10 -2) | 0x0000 >> (10 -10)" NL // C000-FFFF
	"jmp " LOCAL "_true" LF

LOCAL "_ALU_or_RAM:" NL
	"test " out_last(rcxIOCTL, "0x35") ",0x80" NL
	"jz " LOCAL "_TVRAM_or_RAM" NL
//LOCAL "_ALU:" NL
	"mov al,0" NL          // TODO: ALU ................ (*)GVRAM#0 ..........
	"jmp " LOCAL "_GVRAM" LF

LOCAL "_TVRAM_or_RAM:" NL
	"cmp " dxADDR ",0xF000" NL
	"jc " LOCAL "_RAM_C000" NL
	"mov al," _(EFAULT_UNKNOWN) NL
	"test " out_last(rcxIOCTL, "0x32") ",0x10" NL
	"jnz " LOCAL "_RAM_F000" NL
	"test " in_fixed(rcxIOCTL, "0x31") ",0x40" NL
	"jz " LOCAL "_RAM_F000" NL

//LOCAL "_TVRAM_C000:" NL
#if 0
	// TODO: TVRAM
	"mov al," _(EFAULT_TVRAM) NL
	"jmp " LOCAL "_false" NL
#else
 	"mov ecx,(" _(ofsTVRAM) "-" _(0xF000) ") << (16 -8) | 0xF000 >> (10 -2) | 0x0000 >> (10 -10)" NL // F000-FFFF
	"jmp " LOCAL "_true" LF
#endif

LOCAL "_RAM_C000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0xC000 >> (10 -2) | 0xF000 >> (10 -10)" NL // C000-EFFF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_F000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0xF000 >> (10 -2) | 0x0000 >> (10 -10)" LF // F000-FFFF
//	"jmp " LOCAL "_true" NL
LOCAL "_true:" NL
	"ror eax,16" NL
	"clc" NL
	"ret" LF

LOCAL "_false:" NL
	"mov " PTR8 cpuERRAUX "[0],al" NL
	"movzx rcx," dxADDR NL
	"ror eax,16" NL
	"stc" NL
#undef LOCAL
#undef rcxIOCTL
LCFNEND(cpu1_getinfo_R_dxADDR)
#endif

#if 1 // TODO: ..............................
LCFNSTART(cpu1_getinfo_W_dxADDR)
#define LOCAL LC _(cpu1_getinfo_W_dxADDR)
#define rcxIOCTL "rcx"
	"rol eax,16" NL
	"mov rcx," ioctl_item(rdiITEMS) NL
	"cmp " dxADDR ",0x8000" NL
	"jc " LOCAL "_0000" NL
	"cmp " dxADDR ",0xC000" NL
	"jnc " LOCAL "_C000" NL
	"cmp " dxADDR ",0x8400" NL
	"jnc " LOCAL "_RAM_8400" NL
#if 0
	// TODO:
	"mov al," _(EFAULT_UNKNOWN) NL
	"jmp " LOCAL "_false" NL
#else

//LOCAL "_8000_83FF:" NL
	"test " out_last(rcxIOCTL, "0x31") ",0x06" NL
	"jnz " LOCAL "_RAM_8000" NL
//LOCAL "_WND_8000:" NL
	"movzx ecx," out_last(rcxIOCTL, "0x70") NL
	"cmp cl,0xFD" NL
	"jnc " LOCAL "_false" NL // TODO: FD00-, FE00-, FF00- ............ (*)SetPC .... and FC00, and 00FC ........
	"shl ecx,8 +(16 -8)" NL
	"add ecx,(" _(ofsRAM1) "-" _(0x8000) ") << (16 -8) | 0x8000 >> (10 -2) | 0x8400 >> (10 -10)" NL // 8000-83FF
	"jmp " LOCAL "_true" LF
#endif

LOCAL "_0000:" NL
	"test " out_last(rcxIOCTL, "0xE2") ",0x10" NL
	"mov al," _(EFAULT_ERAM) NL
	"jnz " LOCAL "_false" NL
//LOCAL "_RAM_0000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x0000 >> (10 -2) | 0x8000 >> (10 -10)" NL // 0000-7FFF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_8000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x8000 >> (10 -2) | 0x8400 >> (10 -10)" NL // 8000-83FF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_8400:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0x8400 >> (10 -2) | 0xC000 >> (10 -10)" NL // 8400-BFFF
	"jmp " LOCAL "_true" LF

	/*  (12KB) (4KB)
	     C000  F000       #F1  IN(31h)      #32      #35      #F0
	   ALUSet    ..  -------1          -1------ 1-----00
	   ALURGB    ..        ..                .. 1-----01
	   ALUB      ..        ..                .. 1-----10
	   ALUR      ..        ..                .. 1-----11
	   RAM/G(T)VRAM        .. -X------ -0-X---- (READ......)
	             ..  -------0 -X------ -0-X---- #F1.b0=1......(....#N............)
	 */
LOCAL "_C000:" NL
	"mov al," _(EFAULT_ALU) NL
	"test " out_last(rcxIOCTL, "0x32") ",0x40" NL
	"jz " LOCAL "_C000_not_ALU" NL
	"test " out_last(rcxIOCTL, "0x35") ",0x80" NL
	"mov al,0" NL          // TODO: ALU ................ (*)GVRAM#0 ..........
	"jnz " LOCAL "_ALU" LF // TODO: ALU ................

LOCAL "_C000_not_ALU:" NL
	"mov al," out_5x(rcxIOCTL) NL
	"cmp al,3" NL
	"jz " LOCAL "_C000_not_GVRAM" LF

LOCAL "_GVRAM_C000:" NL
assertc(2,
	"cmp al,3") /* 3C 03 */
#if 1 // observe GVRAM write count
	"inc " PTR64 M _(obsvGVRAM) "[" rsiCPU "]" NL
#endif
	"movzx ecx,al" NL
	"shl ecx,14 +(16 -8)" NL // x 0x4000
	"add ecx,(" _(ofsGVRAM0) "-" _(0xC000) ") << (16 -8) | 0xC000 >> (10 -2) | 0x0000 >> (10 -10)" NL // C000-FFFF
	"jmp " LOCAL "_true" LF

LOCAL "_C000_not_GVRAM:" NL
	"cmp " dxADDR ",0xF000" NL
	"jnc " LOCAL "_F000" NL

//LOCAL "_RAM_C000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0xC000 >> (10 -2) | 0xF000 >> (10 -10)" NL // C000-EFFF
	"jmp " LOCAL "_true" LF
LOCAL "_RAM_F000:" NL
	"mov ecx,(" _(ofsRAM1) "-" _(0x0000) ") << (16 -8) | 0xF000 >> (10 -2) | 0x0000 >> (10 -10)" NL // F000-FFFF
	"jmp " LOCAL "_true" LF


LOCAL "_F000:" NL
	"mov al," _(EFAULT_UNKNOWN) NL
	"test " out_last(rcxIOCTL, "0x32") ",0x10" NL
	"jz " LOCAL "_TVRAM_F000" NL
	"test " in_fixed(rcxIOCTL, "0x31") ",0x40" NL
	"jnz " LOCAL "_RAM_F000" LF

LOCAL "_TVRAM_F000:" NL
#if 0
	// TODO: TVRAM
	"mov al," _(EFAULT_TVRAM) NL
	"jmp " LOCAL "_false" NL
#else
 	"mov ecx,(" _(ofsTVRAM) "-" _(0xF000) ") << (16 -8) | 0xF000 >> (10 -2) | 0x0000 >> (10 -10)" NL // F000-FFFF
	"jmp " LOCAL "_true" LF
#endif

LOCAL "_ALU:" NL
	// TODO: ALU
#if 1 // observe ALU write count
	"inc " PTR64 M _(obsvALU) "[" rsiCPU "]" NL
#endif
	"jmp " LOCAL "_GVRAM_C000" LF

LOCAL "_false:" NL
	"mov " PTR8 cpuERRAUX "[0],al" NL
	"ror eax,16" NL
	"stc" NL
	"ret" LF

LOCAL "_true:" NL
	"ror eax,16" NL
	"clc" NL
#undef LOCAL
#undef rcxIOCTL
LCFNEND(cpu1_getinfo_W_dxADDR)
#endif

///////////////////////////////////////////////////////////////
// iMEM (#1) - cpu2 (*)PC-8801 ....

LCFNSTART(cpu2_getinfo_R_dxADDR)
#define LOCAL LC _(cpu2_getinfo_R_dxADDR)
	"cmp " dxPC ",0x2000" NL
	"jc " LOCAL "_ROM_0000" NL
	"call " LC _(cpu2_getinfo_W_dxADDR) NL
	"jnc " LOCAL "_true" NL

	"mov ecx," _(ofsNORAM2) NL
	"and dh,0xF0" NL // 4KB cf)#define ofsNORAM2
	"sub ch,dh" NL
	"jnc " LOCAL "_nc" NL
	"sub ecx,0x10000" LF
LOCAL "_nc:" NL
	"shl ecx," "(16 -8)" NL // _(ofsNORAM2) - (0xFF00 & dxPC) << 16 -8
	"mov cl,dh" NL
	"mov ch,dh" NL // (0xFC00 & dxPC) >> 10 -2
	"add cl,0x10" NL // (0xFC00 & dxPC) +0x400 >> 10 -10
	"ret" LF

LOCAL "_ROM_0000:" NL
	"mov ecx,(" _(ofsDISKROM) "-" _(0x0000) ") << (16 -8) | 0x0000 >> (10 -2) | 0x2000 >> (10 -10)" LF // 0000-1FFF
LOCAL "_true:" NL
	"clc" NL
#undef LOCAL
LCFNEND(cpu2_getinfo_R_dxADDR)

LCFNSTART(cpu2_getinfo_W_dxADDR)
#define LOCAL LC _(cpu2_getinfo_W_dxADDR)
	"cmp " dxPC ",0x4000" NL
	"jc " LOCAL "_false" NL
	"cmp " dxPC ",0x8000" NL
	"jc " LOCAL "_RAM_4000" LF
LOCAL "_false:" NL
	"mov cl," _(EFAULT_UNKNOWN) NL
	"mov " PTR8 cpuERRAUX "[0],cl" NL
	"movzx rcx," dxPC NL
	"stc" NL
	"ret" LF

LOCAL "_RAM_4000:" NL
	"mov ecx,(" _(ofsRAM2) "-" _(0x4000) ") << (16 -8) | 0x4000 >> (10 -2) | 0x8000 >> (10 -10)" LF // 4000-7FFF
LOCAL "_true:" NL
	"clc" NL
#undef LOCAL
LCFNEND(cpu2_getinfo_W_dxADDR)

///////////////////////////////////////////////////////////////
// iMEM (#1)

LCFNSTART(cpuLEA_R_dxADDR)
#define LOCAL LC _(cpuLEA_R_dxADDR)
	"call " cachePRE_LEA_R_dxADDR NL
	"jc " LOCAL "_pending" NL
	"shr ecx,16 -8" NL
	"and rcx," _(ofsMASK) NL
	"add cx," dxADDR NL
	"jnc " LOCAL "_nc" NL
	"add ecx,0x10000" LF
LOCAL "_nc:" NL
	"add rcx," memory_item(rdiITEMS) NL
	"clc" LF // (not needed, but) safety
LOCAL "_pending:" NL
#undef LOCAL
LCFNEND(cpuLEA_R_dxADDR)

LCFNSTART(cpuLEA_W_dxADDR)
#define LOCAL LC _(cpuLEA_W_dxADDR)
	"call " cachePRE_LEA_W_dxADDR NL
	"jc " LOCAL "_pending" NL
	"shr ecx,16 -8" NL
	"and rcx," _(ofsMASK) NL
	"add cx," dxADDR NL
	"jnc " LOCAL "_nc" NL
	"add ecx,0x10000" LF
LOCAL "_nc:" NL
	"add rcx," memory_item(rdiITEMS) NL
	"clc" LF // (not needed, but) safety
LOCAL "_pending:" NL
#undef LOCAL
LCFNEND(cpuLEA_W_dxADDR)


///////////////////////////////////////////////////////////////
// ................

FNSTART(Reader8dlN_dxADDR)
#define LOCAL LC _(Reader8dlN_dxADDR)
#define rcxPTR  "rcx"
#if 1 // TODO: ..............................
	"call " LC "cpuLEA_R_dxADDR" NL
	"jc " LOCAL "_other_page" NL
#endif
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov dl," PTR8 "[" rcxPTR "]" NL
	"ret" LF

LOCAL "_other_page:" NL
#if 1 // TODO: ..............................
	"mov " cpuERRNO "," _(EFAULT) NL
	"or " PTR8 cpuERRAUX "[0],1" NL
	"mov " PTR16 cpuERRAUX "[1]," dxADDR NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
#undef rcxPTR
FNEND(Reader8dlN_dxADDR)

///////////////////////////////////////////////////////////////
// ....................

FNSTART(Fetcher8dlV)
#define LOCAL LC _(Fetcher8dlV)
#if 0 // TODO: ..............................
	// TODO: bxPC ................ cachePRE_LEA_R_dxADDR
	"cmp " bxPC ",0x6000" NL
	"jnc " LOCAL "_other_page" NL
#else
	"call " LC "SetPCbx" NL
assertnc(1,
	"nop") /* 90 */
#endif
	"inc " r8eaPC NL
	"inc " bxPC NL
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov dl," PTR8 "[" r8eaPC "-1]" NL
#if 0 // TODO: ..............................
	"ret" LF

LOCAL "_other_page:" NL
	"mov " cpuERRNO "," _(EFAULT) NL
	"mov " PTR8 cpuERRAUX "[0],1+" _(EFAULT_RAM) NL
	"mov " PTR16 cpuERRAUX "[1]," bxPC NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
FNEND(Fetcher8dlV)

///////////////////////////////////////////////////////////////
// ....................(2)

FNSTART(Fetcher8dlVs)
#define LOCAL LC _(Fetcher8dlVs)
#if 0 // TODO: ..............................
	// TODO: bxPC ................ cachePRE_LEA_R_dxADDR
	"cmp " bxPC ",0x6000" NL
	"jnc " LOCAL "_other_page" NL
#else
	"call " LC "SetPCbx" NL
assertnc(1,
	"nop") /* 90 */
#endif
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov dl," PTR8 "[" r8eaPC "]" NL
	"inc " r8eaPC NL
	"inc " bxPC NL
#if 0 // TODO: ..............................
//	"ret" LF

LOCAL "_other_page:" NL
	"mov " cpuERRNO "," _(EFAULT) NL
	"mov " PTR8 cpuERRAUX "[0],1+" _(EFAULT_RAM) NL
	"mov " PTR16 cpuERRAUX "[1]," bxPC NL
	"jmp " LC "abort_rsiCPU" NL
#elif 0
LOCAL "_other_page:" NL
	"mov ecx," INSTLIM NL
	"and ecx,ecx" NL
	"jz " LOCAL "_other_page" NL
	"mov edx," INST NL
	"add edx," INST2PC_NEG NL // ebx .. PC ......
	"lea ecx,[edx-1]" NL
	"xor ecx,edx" NL
	"and ecx,0xffff ^ " LS "PGADRMASK" NL
	"jnz " LOCAL "_other_page" NL
//LOCAL "_mmap_iofn:" NL
	"shr edx," LS "log2ofPAGESIZE - " LS "log2ofPAGEHDR" NL
	"and edx," LS "PGNUMMASK << " LS "log2ofPAGEHDR" NL
	"mov ecx," M "iofn + " RDPAGES "[edx]" NL
	"mov " M "inst[" CPU "]," INST NL // GetPC()....
	"jmp " LOCAL "_call_fn" LF

LOCAL "_other_page:" NL
	"add " INST "," INST2PC_NEG NL // ebx .. PC ......
	"call SetPC" NL
	"jz " LOCAL "_rwfn" NL

	"add " CLOCKCOUNT "," INSTWAIT NL
	"movsx edx," PTR8 "[" INST "]" NL
	"inc " INST NL
	"ret" LF

LOCAL "_rwfn:" NL
	"shld edx," INST ",32-(" LS "log2ofPAGESIZE-" LS "log2ofPAGEHDR)" NL
	"and " INST ",0xffff" NL
	"and edx," LS "PGNUMMASK << " LS "log2ofPAGEHDR" NL
	"mov ecx," M "raw_or_rwfn + " RDPAGES "[edx]" LF
LOCAL "_call_fn:" NL
	"add " CLOCKCOUNT "," INSTWAIT NL
	"push eax" NL
	"mov edx," M "lParam + " RDPAGES "[edx]" NL
	"push " INST NL // ADDR
	"and ecx,not " LS "IDBIT" NL
	"inc " INST NL
	"push edx" NL // INST
	"call ecx" NL
	"movsx edx,al " NL
	"pop eax" NL
#endif
#undef LOCAL
FNEND(Fetcher8dlVs)

///////////////////////////////////////////////////////////////
// ....................
// TODO: WAITTBL(PC-8801....), page break support

FNSTART(Fetcher16dxUV)
#define LOCAL LC _(Fetcher16dxUV)
#if 0 // TODO: ..............................
	"inc " r8eaPC NL
	"inc " bxPC NL
	// TODO: bxPC ................ cachePRE_LEA_R_dxADDR
	"cmp " bxPC ",0x7FFF" NL
	"jnc " LOCAL "_other_page" NL
#else
	"mov edx," ebxMXPC NL
	"shr " edxMXPC ",16" NL
assertnz(3,
	"cmp " bxPC "," dxBANKEND) /* 66 39 D3 */
	"inc " r8eaPC NL
	"inc " bxPC NL
	"call " LC "SetPCbx" NL
assertnc(1,
	"nop") /* 90 */
#endif
	"inc " r8eaPC NL
	"inc " bxPC NL
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov " dxUV "," PTR16 "[" r8eaPC "-2]" NL
#if 0 // TODO: ..............................
//	"ret" LF

LOCAL "_other_page:" NL
	"mov " cpuERRNO "," _(EFAULT) NL
	"dec " bxPC NL
	"mov " PTR8 cpuERRAUX "[0],2+" _(EFAULT_RAM) NL
	"mov " PTR16 cpuERRAUX "[1]," bxPC NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
FNEND(Fetcher16dxUV)

///////////////////////////////////////////////////////////////
// ................

FNSTART(Reader16dxNN_dxADDR)
#define LOCAL LC _(Reader16dxNN_dxADDR)
#define rcxPTR  "rcx"
#if 1 // TODO: ..............................
# if 1 // TODO: ................2................
	"cmp " dxADDR ",0x5FFF" NL
	"jz " LOCAL "_other_page" NL
	"cmp " dxADDR ",0x7FFF" NL
	"jz " LOCAL "_other_page" NL
	"cmp " dxADDR ",0xBFFF" NL
	"jz " LOCAL "_other_page" NL
	"cmp " dxADDR ",0xEFFF" NL
	"jz " LOCAL "_other_page" NL
	"cmp " dxADDR ",0xFFFF" NL
	"jz " LOCAL "_other_page" NL
# endif
	"inc " dxADDR NL
	"call " LC "cpuLEA_R_dxADDR" NL
	"dec " dxADDR NL
	"dec " rcxADDR NL
	"jc " LOCAL "_other_page" NL
#endif
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov " dxNN "," PTR16 "[" rcxPTR "]" NL
	"ret" LF

LOCAL "_other_page:" NL
#if 1 // TODO: ..............................
	"mov " cpuERRNO "," _(EFAULT) NL
	"or " PTR8 cpuERRAUX "[0],2" NL
	"mov " PTR16 cpuERRAUX "[1]," dxADDR NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
#undef rcxPTR
FNEND(Reader16dxNN_dxADDR)

#define READ16dxNN_dxADDR "call Reader16dxNN_dxADDR" NL

///////////////////////////////////////////////////////////////
// ................

FNSTART(Writer8d2N_dxADDR)
#define LOCAL LC _(Writer8d2N_dxADDR)
#define  dlN     "dl"
#define rcxPTR  "rcx"
#if 1 // TODO: ..............................
	"call " LC "cpuLEA_W_dxADDR" NL
	"jc " LOCAL "_other_page" NL
	"shr edx,16" NL
#endif
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov " PTR8 "[" rcxPTR "]," dlN NL
	"ret" LF

LOCAL "_other_page:" NL
#if 1 // TODO: ..............................
	"mov " cpuERRNO "," _(EFAULT) NL
	"or " PTR8 cpuERRAUX "[0],1+" _(EFAULT_WRITE) NL
	"mov " PTR16 cpuERRAUX "[1]," dxADDR NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
#undef  dlN
#undef rcxPTR
FNEND(Writer8d2N_dxADDR)

#define READ8dlN_dxADDR \
	"call Reader8dlN_dxADDR" NL

#define WRITE8d2N_dxADDR \
	"call Writer8d2N_dxADDR" NL

///////////////////////////////////////////////////////////////
// ................

FNSTART(Writer16dwNN_dxADDR)
#define LOCAL LC _(Writer16dwNN_dxADDR)
#define rcxPTR  "rcx"
assertc(4,
	"cmp " dxADDR ",0xFFFF") /* 66 83 FF FF */
	"inc " dxADDR NL // TODO: support FFFF -> 0000 in here
#if 1 // TODO: ..............................
	"call " LC "cpuLEA_W_dxADDR" NL
	"jc " LOCAL "_other_page" NL
	"shr edx,16" NL
#endif
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"mov " PTR16 "[" rcxPTR "-1]," dxNN NL
	"ret" LF

LOCAL "_other_page:" NL
#if 1 // TODO: ..............................
	"mov " cpuERRNO "," _(EFAULT) NL
	"dec " rcxADDR NL
	"or " PTR8 cpuERRAUX "[0],2+" _(EFAULT_WRITE) NL
	"mov " PTR16 cpuERRAUX "[1]," dxADDR NL
	"jmp " LC "abort_rsiCPU" NL
#endif
#undef LOCAL
#undef rcxPTR
FNEND(Writer16dwNN_dxADDR)

#define WRITE16dwNN_dxADDR \
	"call Writer16dwNN_dxADDR" NL

///////////////////////////////////////////////////////////////
// PENDING: page break .................... call ..........................................

#define FETCH8dlV     "call Fetcher8dlV" NL
#define FETCH8rdxV    "call Fetcher8dlV" NL \
                      "movzx rdx," dlV NL

#define FETCH8dlVs   "call Fetcher8dlVs" NL
#define FETCH8dxVs   "call Fetcher8dlVs" NL \
                     "movsx dx," dlVs NL
#define FETCH8rdxVs  "call Fetcher8dlVs" NL \
                     "movsx rdx," dlVs NL

#ifdef MINIMIZED_WORK
# define FETCH16dUV    "call Fetcher16dxUV" NL
#else // PENDING: ...... Fetcher16edxUV ......................................
# define FETCH16dUV    "xor edx,edx" NL \
                       "call Fetcher16dxUV" NL
#endif
#define FETCH16dxUV   "call Fetcher16dxUV" NL
#define FETCH16rdxUV  "xor rdx,rdx" NL \
                      "call Fetcher16dxUV" NL
#define FETCH16rcxST  "call Fetcher16dxUV" NL \
                      "movzx rcx,dx" NL


///////////////////////////////////////////////////////////////
//  ............

#define PUSHdxNN "shl " edxNN ",16" NL \
                 "mov dx," cpuSP16 NL \
                 "sub " dxSP ",2" NL \
                 "mov " cpuSP16 "," dxSP NL \
                 WRITE16dwNN_dxADDR
#define POPdxNN "mov " dUV "," cpuSP NL \
                READ16dxNN_dxADDR \
                "add " cpuSP ",2" NL

///////////////////////////////////////////////////////////////
// IX+d / IY+d ............ -> ST

#define mov_dw(cpu16) \
	"ror edx,16" NL \
	"mov " dxUV "," cpu16 NL \
	"rol edx,16" NL
#define cpuBCto_dxADDR "mov dx," cpuBC16 NL
#define cpuDEto_dxADDR "mov dx," cpuDE16 NL
#define cpuHLto_dxADDR "mov dx," cpuHL16 NL
#define cpuHLto_dADDR  "mov " dUV "," cpuHL NL
#define cpuIXrbxRIPto_dxADDR FETCH8dxVs "mov " cST "," cpuIX NL "add " dxVs "," cxIX NL /* "and rdx,0xFFFF" NL */
#define cpuIYrbxRIPto_dxADDR FETCH8dxVs "mov " cST "," cpuIY NL "add " dxVs "," cxIY NL /* "and rdx,0xFFFF" NL */

///////////////////////////////////////////////////////////////
// ............(....)
// UV <-> AF, A <- R

//	AF->UV
#define dl__X_X___ "dl"
//#define LOADUVAF // TODO: ..................................
#define mov_dlA_dhSZ_H_PNC_cpu__X_X___to_dxAF "mov dl," cpu__X_X___ NL \
                                              "mov dh," alA NL \
                                              "xor " dl__X_X___ "," ahF NL \
                                              "and " dl__X_X___ "," __X_X___ NL \
                                              "xor " dl__X_X___ "," ahF NL

#define mov_alA7_amR7 "rol eax,8" NL \
                      "ror " alR NL \
                      "xor " ahA "," alR NL \
                      "and " ahA ",0x80" NL \
                      "xor " ahA "," alR NL \
                      "rol " alR NL \
                      "ror eax,8" NL

#define mov_amR7_alA7 "rol eax,8" NL \
                      "ror " alR NL \
                      "xor " alR "," ahA NL \
                      "and " alR ",0x80" NL \
                      "xor " alR "," ahA NL \
                      "rol " alR NL \
                      "ror eax,8" NL

#define mov_amR7_cpuR7 "rol eax,7" NL \
                       "xor al," cpuR8 NL \
                       "and al,0x80" NL \
                       "xor al," cpuR8 NL \
                       "ror eax,7" NL

#define mov_cpuR7_amR7 "rol eax,7" NL \
                       "xor " cpuR8 "," alR NL \
                       "and " cpuR8 ",0x80" NL \
                       "xor " cpuR8 "," alR NL \
                       "ror eax,7" NL

// UV->AF	
#define mov_dxAFto_dlA_dhSZ_H_PNC_cpu__X_X___ "mov ah," dlV NL \
                                              "mov " cpu__X_X___"," dlV NL \
                                              "mov " alA "," dhU NL

#define inc_amR "add eax,0x02000000" NL

///////////////////////////////////////////////////////////////
// ..........

#define mov_ahF_____P0_ "seto dh" NL \
                        "and " ahF ",not " _____PN_ NL \
                        "shl dh,2" NL \
                        "or " ahF ",dh" NL

#define mov_ahF_____P1_ "seto dh" NL \
                        "and " ahF ",not " _____P__ NL \
                        "shl dh,2" NL \
                        "or " ahF "," ______N_ NL \
                        "or " ahF ",dh" NL

// A ........ IFF .... ............ ...... PENDING: (*1)
#define mov_ah___0_P0_IFF1 mov_SZHPC_ahF NL \
                           "inc " alA NL \
                           "dec " alA NL \
                           "mov " cpu__X_X___ "," alA NL \
                           mov_ahF_SZHPC NL \
                           "test " cpuINT "," _(IFF1) NL \
                           "setnz dl" NL \
                           "and " ahF ",not " ___H_PN_ NL \
                           "shl dl,2" NL \
                           "or " ahF ",dl" NL

///////////////////////////////////////////////////////////////
// ..............

#define CALLdxADDR "push " rdxADDR NL \
                   "mov dx," bxPC NL \
                   PUSHdxNN SP_PUSH \
                   "pop " rdxADDR NL \
                   FTRACE_set_flags(3, 0x01) \
                   JPdxADDR
#ifdef FULLTRACE_ENABLED
# define FTRACE_append_val16(val) \
								FULLTRACE_debug_broken \
								"test " cpuSTATE "," _(STATE_OPCODE_TRACE) NL \
							/*	"jz " LOCAL "_not_ftrace"*/ ".byte 0x74,0x08" NL \
								"mov " PTR16 "[" r10FTRACE "]," #val /* 66 41 89 xx */ NL \
								"add " r10FTRACE ",2" /* 49 83 C2 02 */ NL \
						/*	LOCAL "_not_ftrace:" NL*/ \
								FULLTRACE_debug_backup
# define FTRACE_append_val8(val) \
								FULLTRACE_debug_broken \
								"test " cpuSTATE "," _(STATE_OPCODE_TRACE) NL \
							/*	"jz " LOCAL "_not_ftrace"*/ ".byte 0x74,0x06" NL \
								"mov " PTR8 "[" r10FTRACE "]," #val /* 41 88 xx */ NL \
								"inc " r10FTRACE /* 49 FF C2 */ NL \
						/*	LOCAL "_not_ftrace:" NL*/ \
								FULLTRACE_debug_backup
#else
# define FTRACE_append_val16(val) 
# define FTRACE_append_val8(val) 
#endif
#define       MRET POPdxNN \
                   FTRACE_set_flags(1, 0x01) \
                   FTRACE_append_val16("dx") \
                   JPdxADDR

///////////////////////////////////////////////////////////////
// ..............

#define ADD_alA(src8) "add " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P0_

#define ADC_alA(src8) "ror " ahF ",1" NL \
                      "adc " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P0_

#define SUB_alA(src8) "sub " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P1_ 

#define SBC_alA(src8) "ror " ahF ",1" NL \
                      "sbb " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P1_

#define AND_alA(src8) "and " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      "and " ahF ",not " ______N_ NL \
                      "or " ahF "," ___H____ NL

#define  OR_alA(src8) "or " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      "and " ahF ",not " ___H__N_ NL

#define XOR_alA(src8) "xor " alA "," src8 NL \
                      "mov " cpu__X_X___ "," alA NL \
                      mov_ahF_SZHPC NL \
                      "and " ahF ",not " ___H__N_ NL

#define  CP_alA(src8) "mov " dhU "," src8 NL \
                      "cmp " alA "," dhU NL \
                      "mov " cpu__X_X___ "," dhU NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P1_

#define INC_dlV       mov_SZHPC_ahF NL \
                      "inc " dlV NL \
                      "mov " cpu__X_X___ "," dlV NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P0_

#define DEC_dlV       mov_SZHPC_ahF NL \
                      "dec " dlV NL \
                      "mov " cpu__X_X___ "," dlV NL \
                      mov_ahF_SZHPC NL \
                      mov_ahF_____P1_ 

///////////////////////////////////////////////////////////////
// ..................

#define MADD16(dst16h, dst16l, reg16h, reg16l) "mov cl," ahF NL \
                                               "add " dst16l "," reg16l NL \
                                               "adc " dst16h "," reg16h NL \
                                               mov_ahF_SZHPC NL \
                                               "and " clF ",not " ___H__NC NL \
                                               "and " ahF "," ___H___C NL \
                                               "or " ahF "," clF NL

// UV<-HL
#define MADCHL(reg16h, reg16l) "mov " cST "," cpuHL NL \
                               "ror " ahF ",1" NL  \
                               "adc " clT "," reg16l NL \
                               "adc " chS "," reg16h NL \
                               "mov " cpu__X_X___ "," chS NL \
                               mov_ahF_SZHPC NL \
                               mov_ahF_____P0_ \
                               "and " ahF ",not " _Z______ NL \
                               "test " cST ",0xffff" NL \
                               "mov " cpuHL "," cST NL \
                               "setz " clT NL \
                               "shl " clT ",6" NL \
                               "or " ahF "," clT NL

#define MSBCHL(reg16h, reg16l) "mov " cST "," cpuHL NL \
                               "ror " ahF ",1" NL \
                               "sbb " clT "," reg16l NL \
                               "sbb " chS "," reg16h NL \
                               "mov " cpu__X_X___ "," chS NL \
                               mov_ahF_SZHPC NL \
                               mov_ahF_____P1_ \
                               "and " ahF ",not " _Z______ NL \
                               "test " cST ",0xffff" NL \
                               "mov " cpuHL "," cST NL \
                               "setz " clT NL \
                               "shl " clT ",6" NL \
                               "or " ahF "," clT NL

///////////////////////////////////////////////////////////////
// ..................

#define MRLCV "rol " dlV ",1" NL \
              "dec " dlV NL \
              "inc " dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL
       
#define MRRCV "ror " dlV ",1" NL \
              "dec " dlV NL \
              "inc " dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL
       
#define MRLV "ror " ahF ",1" NL \
             "adc " dlV "," dlV NL \
             "mov " cpu__X_X___ "," dlV NL \
             mov_ahF_SZHPC NL \
             "and " ahF ",not " ___H__N_ NL 

#define MRRV mov_SZHPC_ahF NL \
             "rcr " dlV ",1" NL \
             "dec " dlV NL \
             "inc " dlV NL \
             "mov " cpu__X_X___ "," dlV NL \
             mov_ahF_SZHPC NL \
             "and " ahF ",not " ___H__N_ NL

#define MSLAV "sal " dlV ",1" NL \
              "dec " dlV NL \
              "inc " dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL 
       
#define MSRAV "sar " dlV ",1" NL \
              "dec " dlV NL \
              "inc " dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL
       
#define MSLLV "stc" NL \
              "adc " dlV "," dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL
       
#define MSRLV "shr " dlV ",1" NL \
              "dec " dlV NL \
              "inc " dlV NL \
              "mov " cpu__X_X___ "," dlV NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL

///////////////////////////////////////////////////////////////
// RLD/RRD

#define MRLD "mov dx," cpuHL16 NL \
             READ8dlN_dxADDR \
             "mov " dhU "," dlV NL \
             "shl " dlV ",4" NL \
             "xor " dlV "," alA NL \
             "and " dlV ",0xf0" NL \
             "xor " dlV "," alA NL \
             "shr " dhU ",4" NL \
             "and " alA ",0xf0" NL \
             "or " alA "," dhU NL \
             "shl " edxUV ",16" NL \
             "mov dx," cpuHL16 NL \
             WRITE8d2N_dxADDR \
             mov_SZHPC_ahF NL \
             "inc " alA NL \
             "dec " alA NL \
             "mov " cpu__X_X___ "," alA NL \
             mov_ahF_SZHPC NL \
             "and " ahF ",not " ___H__N_ NL
 
#define MRRD "mov dx," cpuHL16 NL \
             READ8dlN_dxADDR \
             "mov " dhU "," alA NL \
             "xor " alA "," dlV NL \
             "and " alA ",0xf0" NL \
             "xor " alA "," dlV NL \
             "shl " dhU ",4" NL \
             "shr " dlV ",4" NL \
             "or " dlV "," dhU NL \
             "shl " edxUV ",16" NL \
             "mov dx," cpuHL16 NL \
             WRITE8d2N_dxADDR \
             mov_SZHPC_ahF NL \
             "inc " alA NL \
             "dec " alA NL \
             "mov " cpu__X_X___ "," alA NL \
             mov_ahF_SZHPC NL \
             "and " ahF ",not " ___H__N_ NL
 
///////////////////////////////////////////////////////////////
// ........

#define MENDIF(label) LC #label "_unmatch:" NL
#define MELSE(label) "ret" LF \
                     MENDIF(label)
#define MELSE0 MENDIF

#define  MIFNZ(label) "test " ahF "," _Z______ NL "jz  " LC #label "_unmatch" NL
#define   MIFZ(label) "test " ahF "," _Z______ NL "jnz " LC #label "_unmatch" NL
#define  MIFNC(label) "test " ahF "," _______C NL "jz  " LC #label "_unmatch" NL
#define   MIFC(label) "test " ahF "," _______C NL "jnz " LC #label "_unmatch" NL
#define  MIFPO(label) "test " ahF "," _____P__ NL "jz  " LC #label "_unmatch" NL
#define  MIFPE(label) "test " ahF "," _____P__ NL "jnz " LC #label "_unmatch" NL
#define   MIFP(label) "test " ahF "," S_______ NL "jz  " LC #label "_unmatch" NL
#define   MIFM(label) "test " ahF "," S_______ NL "jnz " LC #label "_unmatch" NL
#define MIFSUB(label) "test " ahF "," ______N_ NL "jnz " LC #label "_unmatch" NL

///////////////////////////////////////////////////////////////
//  ..................................

#define MRLCA mov_SZHPC_ahF NL \
              "rol " alA ",1" NL \
              "mov " cpu__X_X___ "," alA NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL

#define MRRCA mov_SZHPC_ahF NL \
              "ror " alA ",1" NL \
              "mov " cpu__X_X___ "," alA NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL 

#define MRLA  mov_SZHPC_ahF NL \
              "rcl " alA ",1" NL \
              "mov " cpu__X_X___ "," alA NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL

#define MRRA  mov_SZHPC_ahF NL \
              "rcr " alA ",1" NL \
              "mov " cpu__X_X___ "," alA NL \
              mov_ahF_SZHPC NL \
              "and " ahF ",not " ___H__N_ NL 

///////////////////////////////////////////////////////////////
// ................ ........

// PENDING: cannot use 'daa' and 'das' on 64bit-mode
#define MCPL "not " alA NL \
             "mov " cpu__X_X___ "," alA NL \
             "or " ahF "," ___H__N_ NL 

#define MSCF "mov " cpu__X_X___ "," alA NL \
             "or " ahF "," _______C NL \
             "and " ahF ",not " ___H__N_ NL

#define MCCF "mov " cpu__X_X___ "," alA NL \
             "mov dl," ahF NL \
             "and " ahF ",not " ___H__N_ NL \
             "and " dlF ",1" NL \
             "xor " ahF "," _______C NL \
             "shl " dlF ",4" NL \
             "or " ahF "," dlF NL

///////////////////////////////////////////////////////////////
// ................

#define MLDI cpuHLto_dADDR \
             READ8dlN_dxADDR \
             "shl " edxN ",16" NL \
             "mov dx," cpuDE16 NL \
             WRITE8d2N_dxADDR \
             "inc " cpuHL NL \
             movzx_from_cpu("cx", cpuBC) \
             "inc " cpuDE NL \
             "dec " cxBC NL \
             "test " cxBC ",0xffff" NL \
             movzx_to_cpu(cpuBC, cxBC) NL \
             "setnz " dlV NL \
             "and " ahF ",not " ___H_PN_ NL \
             "shl " dlV ",2" NL \
             "or " ahF "," dlV NL 

#define MLDD cpuHLto_dADDR \
             READ8dlN_dxADDR \
             "shl " edxN ",16" NL \
             "mov dx," cpuDE16 NL \
             WRITE8d2N_dxADDR \
             "dec " cpuHL NL \
             movzx_from_cpu("cx", cpuBC) \
             "dec " cpuDE NL \
             "dec " cxBC NL \
             "test " cxBC ",0xffff" NL \
             movzx_to_cpu(cpuBC, cxBC) NL \
             "setnz " dlV NL \
             "and " ahF ",not " ___H_PN_ NL \
             "shl " dlV ",2" NL \
             "or " ahF "," dlV NL 

///////////////////////////////////////////////////////////////
// ..................

#define MCPI "mov dx," cpuHL16 NL \
             "lea rcx, [" rdxHL "+1]" NL \
             "mov " cpuHL16 "," cxST NL \
             READ8dlN_dxADDR \
             "mov dh," ahF NL \
             mov_SZHPC_ahF NL \
             "cmp " alA "," dlV NL \
             mov_ahF_SZHPC NL \
             "and " dhF ",1" NL \
             "and " ahF ",not " _____P_C NL \
             "or " dhF "," ______N_ NL \
             "mov cx," cpuBC16 NL \
             "dec " cxBC NL \
             "test " cxBC ",0xffff" NL \
             "mov " cpuBC16 "," cxBC NL \
             "setnz " dlV NL \
             "or " ahF "," dhF NL \
             "shl " dlV ",2" NL /* _____P__ */ \
             "or " ahF "," dlV NL

#define MCPD "mov dx," cpuHL16 NL \
             "lea rcx, [" rdxHL "-1]" NL \
             "mov " cpuHL16 "," cxST NL \
             READ8dlN_dxADDR \
             "mov dh," ahF NL \
             mov_SZHPC_ahF NL \
             "cmp " alA "," dlV NL \
             mov_ahF_SZHPC NL \
             "and " dhF ",1" NL \
             "and " ahF ",not " _____P_C NL \
             "or " dhF "," ______N_ NL \
             "mov cx," cpuBC16 NL \
             "dec " cxBC NL \
             "test " cxBC ",0xffff" NL \
             "mov " cpuBC16 "," cxBC NL \
             "setnz " dlV NL \
             "or " ahF "," dhF NL \
             "shl " dlV ",2" NL /* _____P__ */ \
             "or " ahF "," dlV NL

///////////////////////////////////////////////////////////////
// TODO: ..Z80....................(......)

OPFUNC(DEV)
	"mov " cpuERRNO "," _(ECPU) NL
	"dec " r8eaPC NL
	"dec " bxPC NL

#if 0 // ......
	"push rcx" NL
	"push rdx" NL
	"mov dx," bxPC NL
	"call " cachePRE_LEA_R_dxADDR NL
	"pop rdx" NL
	"mov " PTR8 cpuERRAUX "[3],cl" NL
	"pop rcx" NL
#endif
	"jmp " LC "abort_rsiCPU2" NL // NOTE: cannot skip because modified bpCLK must not be accepted
OPEND(DEV)

OPFUNC(DEV2)
	"mov " cpuERRNO "," _(ECPU) NL
	"dec " r8eaPC NL
	"dec " bxPC NL
	"dec " r8eaPC NL
	"dec " bxPC NL

#if 0 // ......
	"push rcx" NL
	"push rdx" NL
	"mov dx," bxPC NL
	"call " cachePRE_LEA_R_dxADDR NL
	"pop rdx" NL
	"mov " PTR8 cpuERRAUX "[3],cl" NL
	"pop rcx" NL
#endif
	"jmp " LC "abort_rsiCPU2" NL // NOTE: cannot skip because modified bpCLK must not be accepted
OPEND(DEV2)

///////////////////////////////////////////////////////////////
// ..........................

//OPFUNC(DAA) MDAA(DAA) CLK1(4, 1) OPEND(DAA)
OPFUNC(CPL) MCPL CLK1(4, 1) OPEND(CPL)
OPFUNC(CCF) MCCF CLK1(4, 1) OPEND(CCF)
OPFUNC(SCF) MSCF CLK1(4, 1) OPEND(SCF)

OPFUNC(RLCA) MRLCA CLK1(4, 1) OPEND(RLCA)
OPFUNC(RRCA) MRRCA CLK1(4, 1) OPEND(RRCA)
OPFUNC(RLA)  MRLA  CLK1(4, 1) OPEND(RLA) 
OPFUNC(RRA)  MRRA  CLK1(4, 1) OPEND(RRA) 

OPFUNC(RLD) MRLD CLK2(18, 4) OPEND(RLD) // (4+1,4+1,3,4,3)
OPFUNC(RRD) MRRD CLK2(18, 4) OPEND(RRD)

OPFUNC(NEG) "mov " dlV "," alA NL "xor " alA "," alA NL CLK2(8, 2) SUB_alA(dlV) OPEND(NEG) // (4+1,4+1)

///////////////////////////////////////////////////////////////
// IM

OPFUNC(IM0) SETIM(0) CLK2(8, 2) OPEND(IM0) // (4+1,4+1)
OPFUNC(IM1) SETIM(1) CLK2(8, 2) OPEND(IM1)
OPFUNC(IM2) SETIM(2) CLK2(8, 2) OPEND(IM2)

///////////////////////////////////////////////////////////////
// ..........

OPFUNC(IN_A_N)
#define clN "cl"
	FETCH8rdxV
	CLK1(7, 2)
	IN_clN_rdxN
	"mov " alA "," clN NL
	CLK0(+4, 1)
	"ret" LF

#if 0
FETCH8PGBRK(IN_A_N)

LC "IN_A_N" "_wait_pair_cpu:" NL          // I/O..............................CPU................ (*)INP()..........
	CLK1BACK(7, 2)                     // (*4)(*8)
#if 1 // m88
	"pop "                  bPQ NL
	"sub "  TOLIMIT_NEG "," bPQ NL
	"add " IOCYCLE_EDGE "," bPQ NL
#endif
	rbxRIPsub2ret
#endif
#undef clN
OPEND0(IN_A_N)

OPFUNC(OUT_N_A)
	FETCH8rdxV
	FTRACE_set_flags(2, 0x02)
	FTRACE_append_val8("al")
	CLK1(11, 3)                        // (4+1,3,4) (*4)(*8)
	"mov " clT "," alA NL
#if 0
	SYNC
	"jc " LC "OUT_N_A" "_wait_pair_cpu" NL // I/O..............................CPU................
#endif

	OUT_rdxPORT_clN
	"ret" LF

#if 0
FETCH8PGBRK(OUT_N_A)

LC "OUT_N_A" "_wait_pair_cpu:" NL
	CLK1BACK(11, 3)                    // (*4)(*8)
#if 1 // m88
	"pop "                  PQ NL
	"sub "  TOLIMIT_NEG "," PQ NL
	"add " IOCYCLE_EDGE "," PQ NL
#endif
	rbxRIPsub2ret
#endif
OPEND0(OUT_N_A)

#define MINPC(reg8) \
		"movzx rdx," cpuC NL \
		CLK2(8, 2) /* (4+1,4+1,) (*4)(*8) */ \
		IN_clN_rdxN \
		CLK0(+4, 1) /* (,,4) (*4)(*8) */ \
		mov_SZHPC_ahF NL \
		"mov " reg8 "," clT NL \
		"inc " clT NL \
		"dec " clT NL \
      "mov " cpu__X_X___ "," clT NL \
      mov_ahF_SZHPC NL \
		"and " ahF ",not " ___H__N_ NL

OPFUNC(IN_B_C) MINPC(cpuB) OPEND(IN_B_C)
OPFUNC(IN_C_C) MINPC(cpuC) OPEND(IN_C_C)
OPFUNC(IN_D_C) MINPC(cpuD) OPEND(IN_D_C)
OPFUNC(IN_E_C) MINPC(cpuE) OPEND(IN_E_C)
OPFUNC(IN_H_C) MINPC(cpuH) OPEND(IN_H_C)
OPFUNC(IN_L_C) MINPC(cpuL) OPEND(IN_L_C)
//OPFUNC(IN_F_C) MINPC(T) OPEND(IN_F_C)
OPFUNC(IN_A_C) MINPC( alA) OPEND(IN_A_C)

#define MOUTPC(val8) \
		"movzx rdx," cpuC NL \
		CLK2(12, 3) /* (4+1,4+1,4) (*4) */ \
		"mov " clT "," val8 NL \
		OUT_rdxPORT_clN

OPFUNC(OUT_C_B) MOUTPC(cpuB) OPEND(OUT_C_B)
OPFUNC(OUT_C_C) MOUTPC(cpuC) OPEND(OUT_C_C)
OPFUNC(OUT_C_D) MOUTPC(cpuD) OPEND(OUT_C_D)
OPFUNC(OUT_C_E) MOUTPC(cpuE) OPEND(OUT_C_E)
OPFUNC(OUT_C_H) MOUTPC(cpuH) OPEND(OUT_C_H)
OPFUNC(OUT_C_L) MOUTPC(cpuL) OPEND(OUT_C_L)
OPFUNC(OUT_C_Z) MOUTPC( "0") OPEND(OUT_C_Z)
OPFUNC(OUT_C_A) MOUTPC( alA) OPEND(OUT_C_A)

///////////////////////////////////////////////////////////////
// ........

OPFUNC(JP)     FETCH16dxUV CLK1(10, 3) JPdxADDR "\t" OPEND0(JP) // (4+1,3,3)
OPFUNC(JR)     FETCH8rdxVs CLK1(12, 2) JRrdxVs  "\t" OPEND0(JR) // (4+1,3,5)

OPFUNC(JP_NZ) CLK1(10, 3) FETCH16dxUV  MIFZ(JP_NZ) JPdxADDR MELSE0(JP_NZ) OPEND(JP_NZ) // (4+1,3,3)
OPFUNC(JP_Z)  CLK1(10, 3) FETCH16dxUV MIFNZ(JP_Z)  JPdxADDR MELSE0(JP_Z)  OPEND(JP_Z) 
OPFUNC(JP_NC) CLK1(10, 3) FETCH16dxUV  MIFC(JP_NC) JPdxADDR MELSE0(JP_NC) OPEND(JP_NC)
OPFUNC(JP_C)  CLK1(10, 3) FETCH16dxUV MIFNC(JP_C)  JPdxADDR MELSE0(JP_C)  OPEND(JP_C) 
OPFUNC(JP_PO) CLK1(10, 3) FETCH16dxUV MIFPE(JP_PO) JPdxADDR MELSE0(JP_PO) OPEND(JP_PO)
OPFUNC(JP_PE) CLK1(10, 3) FETCH16dxUV MIFPO(JP_PE) JPdxADDR MELSE0(JP_PE) OPEND(JP_PE)
OPFUNC(JP_P)  CLK1(10, 3) FETCH16dxUV  MIFM(JP_P)  JPdxADDR MELSE0(JP_P)  OPEND(JP_P) 
OPFUNC(JP_M)  CLK1(10, 3) FETCH16dxUV  MIFP(JP_M)  JPdxADDR MELSE0(JP_M)  OPEND(JP_M) 

// NOTE: M88........FETCH........MIFx......................................................................
OPFUNC(JR_NZ)  FETCH8rdxVs  MIFZ(JR_NZ) CLK1(12, 2) JRrdxVs MELSE0(JR_NZ) CLK1(7, 2) OPEND(JR_NZ)
OPFUNC(JR_Z)   FETCH8rdxVs MIFNZ(JR_Z)  CLK1(12, 2) JRrdxVs MELSE0(JR_Z)  CLK1(7, 2) OPEND(JR_Z)
OPFUNC(JR_NC)  FETCH8rdxVs  MIFC(JR_NC) CLK1(12, 2) JRrdxVs MELSE0(JR_NC) CLK1(7, 2) OPEND(JR_NC)
OPFUNC(JR_C)   FETCH8rdxVs MIFNC(JR_C)  CLK1(12, 2) JRrdxVs MELSE0(JR_C)  CLK1(7, 2) OPEND(JR_C)

OPFUNC(JP_HL) "mov dx," cpuHL NL CLK1(4, 1) JPdxADDR "\t" OPEND0(JP_HL)
OPFUNC(JP_IX) "mov dx," cpuIX NL CLK1(4, 1) JPdxADDR "\t" OPEND0(JP_IX)
OPFUNC(JP_IY) "mov dx," cpuIY NL CLK1(4, 1) JPdxADDR "\t" OPEND0(JP_IY)
OPFUNC(CALL)   FETCH16dxUV CLK1(17, 5) CALLdxADDR "\t" OPEND0(CALL) // (4+1,3,4,3,3)
OPFUNC(CALL_NZ) FETCH16dxUV  MIFZ(CALL_NZ) CLK1(17, 5) CALLdxADDR MELSE0(CALL_NZ) CLK1(10, 3) OPEND(CALL_NZ) // (4+1,3,4,3,3) (4+1,3,3)
OPFUNC(CALL_Z)  FETCH16dxUV MIFNZ(CALL_Z)  CLK1(17, 5) CALLdxADDR MELSE0(CALL_Z)  CLK1(10, 3) OPEND(CALL_Z) 
OPFUNC(CALL_NC) FETCH16dxUV  MIFC(CALL_NC) CLK1(17, 5) CALLdxADDR MELSE0(CALL_NC) CLK1(10, 3) OPEND(CALL_NC)
OPFUNC(CALL_C)  FETCH16dxUV MIFNC(CALL_C)  CLK1(17, 5) CALLdxADDR MELSE0(CALL_C)  CLK1(10, 3) OPEND(CALL_C) 
OPFUNC(CALL_PO) FETCH16dxUV MIFPE(CALL_PO) CLK1(17, 5) CALLdxADDR MELSE0(CALL_PO) CLK1(10, 3) OPEND(CALL_PO)
OPFUNC(CALL_PE) FETCH16dxUV MIFPO(CALL_PE) CLK1(17, 5) CALLdxADDR MELSE0(CALL_PE) CLK1(10, 3) OPEND(CALL_PE)
OPFUNC(CALL_P)  FETCH16dxUV  MIFM(CALL_P)  CLK1(17, 5) CALLdxADDR MELSE0(CALL_P)  CLK1(10, 3) OPEND(CALL_P) 
OPFUNC(CALL_M)  FETCH16dxUV  MIFP(CALL_M)  CLK1(17, 5) CALLdxADDR MELSE0(CALL_M)  CLK1(10, 3) OPEND(CALL_M) 


OPFUNC(RET)    CLK1(10, 3) MRET OPEND0(RET) // (4+1,3,3)
OPFUNC(RET_NZ)  MIFZ(RET_NZ) CLK1(11, 3) MRET  MELSE0(RET_NZ) CLK1(5, 1) OPEND(RET_NZ) // (5+1,3,3)
OPFUNC(RET_Z)  MIFNZ(RET_Z)  CLK1(11, 3) MRET  MELSE0(RET_Z)  CLK1(5, 1) OPEND(RET_Z) 
OPFUNC(RET_NC)  MIFC(RET_NC) CLK1(11, 3) MRET  MELSE0(RET_NC) CLK1(5, 1) OPEND(RET_NC)
OPFUNC(RET_C)  MIFNC(RET_C)  CLK1(11, 3) MRET  MELSE0(RET_C)  CLK1(5, 1) OPEND(RET_C) 
OPFUNC(RET_PO) MIFPE(RET_PO) CLK1(11, 3) MRET  MELSE0(RET_PO) CLK1(5, 1) OPEND(RET_PO)
OPFUNC(RET_PE) MIFPO(RET_PE) CLK1(11, 3) MRET  MELSE0(RET_PE) CLK1(5, 1) OPEND(RET_PE)
OPFUNC(RET_P)   MIFM(RET_P)  CLK1(11, 3) MRET  MELSE0(RET_P)  CLK1(5, 1) OPEND(RET_P) 
OPFUNC(RET_M)   MIFP(RET_M)  CLK1(11, 3) MRET  MELSE0(RET_M)  CLK1(5, 1) OPEND(RET_M) 

OPFUNC(DJNZ)
#define LOCAL LC "DJNZ"
#define  dhB     "dh"
	FETCH8dlVs
	"dec " cpuB NL
	"jz " LOCAL "_reached_zero" NL
	"movsx rdx," dlVs NL
	CLK1(13, 2) // (5+1,3,5)
	JRrdxVs
LOCAL "_reached_zero:" NL
	CLK1(8, 2) // (5+1,3)
#undef LOCAL
OPEND(DJNZ)

OPFUNC(RST00) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x00" NL JPdxADDR "\t" OPEND(RST00) // (5+1,3,3)
OPFUNC(RST08) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x08" NL JPdxADDR "\t" OPEND(RST08)
OPFUNC(RST10) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x10" NL JPdxADDR "\t" OPEND(RST10)
OPFUNC(RST18) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x18" NL JPdxADDR "\t" OPEND(RST18)
OPFUNC(RST20) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x20" NL JPdxADDR "\t" OPEND(RST20)
OPFUNC(RST28) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x28" NL JPdxADDR "\t" OPEND(RST28)
OPFUNC(RST30) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x30" NL JPdxADDR "\t" OPEND(RST30)
OPFUNC(RST38) CLK1(11, 3) "mov dx," bxPC NL PUSHdxNN SP_PUSH "mov dx,0x38" NL JPdxADDR "\t" OPEND(RST38)

///////////////////////////////////////////////////////////////
// ..................

OPFUNC(ADDHL_BC) "mov " dUV "," cpuHL NL MADD16(dhU,dlV, cpuB  ,cpuC  ) "mov " cpuL "," dlV NL CLK1(11, 3) "mov " cpuH "," dhU NL OPEND(ADDHL_BC) // (4+1,4,3)
OPFUNC(ADDHL_DE) "mov " dUV "," cpuHL NL MADD16(dhU,dlV, cpuD  ,cpuE  ) "mov " cpuL "," dlV NL CLK1(11, 3) "mov " cpuH "," dhU NL OPEND(ADDHL_DE)
OPFUNC(ADDHL_HL) "mov " dUV "," cpuHL NL MADD16(dhU,dlV,  dhU  , dlV  ) "mov " cpuL "," dlV NL CLK1(11, 3) "mov " cpuH "," dhU NL OPEND(ADDHL_HL)
OPFUNC(ADDHL_SP) "mov " dUV "," cpuHL NL MADD16(dhU,dlV, cpuSPH,cpuSPL) "mov " cpuL "," dlV NL CLK1(11, 3) "mov " cpuH "," dhU NL OPEND(ADDHL_SP)

OPFUNC(ADDIX_BC) "mov " dUV "," cpuIX NL MADD16(dhU,dlV, cpuB  ,cpuC  ) "mov " cpuXL "," dlV NL CLK1(11, 3) "mov " cpuXH "," dhU NL OPEND(ADDIX_BC) // (,4+1,4,3)
OPFUNC(ADDIX_DE) "mov " dUV "," cpuIX NL MADD16(dhU,dlV, cpuD  ,cpuE  ) "mov " cpuXL "," dlV NL CLK1(11, 3) "mov " cpuXH "," dhU NL OPEND(ADDIX_DE)
OPFUNC(ADDIX_IX) "mov " dUV "," cpuIX NL MADD16(dhU,dlV,  dhU  , dlV  ) "mov " cpuXL "," dlV NL CLK1(11, 3) "mov " cpuXH "," dhU NL OPEND(ADDIX_IX)
OPFUNC(ADDIX_SP) "mov " dUV "," cpuIX NL MADD16(dhU,dlV, cpuSPH,cpuSPL) "mov " cpuXL "," dlV NL CLK1(11, 3) "mov " cpuXH "," dhU NL OPEND(ADDIX_SP)

OPFUNC(ADDIY_BC) "mov " dUV "," cpuIY NL MADD16(dhU,dlV, cpuB  ,cpuC  ) "mov " cpuYL "," dlV NL CLK1(11, 3) "mov " cpuYH "," dhU NL OPEND(ADDIY_BC) // (,4+1,4,3)
OPFUNC(ADDIY_DE) "mov " dUV "," cpuIY NL MADD16(dhU,dlV, cpuD  ,cpuE  ) "mov " cpuYL "," dlV NL CLK1(11, 3) "mov " cpuYH "," dhU NL OPEND(ADDIY_DE)
OPFUNC(ADDIY_IY) "mov " dUV "," cpuIY NL MADD16(dhU,dlV,  dhU  , dlV  ) "mov " cpuYL "," dlV NL CLK1(11, 3) "mov " cpuYH "," dhU NL OPEND(ADDIY_IY)
OPFUNC(ADDIY_SP) "mov " dUV "," cpuIY NL MADD16(dhU,dlV, cpuSPH,cpuSPL) "mov " cpuYL "," dlV NL CLK1(11, 3) "mov " cpuYH "," dhU NL OPEND(ADDIY_SP)

// ADC/SBC HL,HL : ...... ST <- HL ................
OPFUNC(ADCHL_BC) CLK2(15, 4) MADCHL(cpuB  ,cpuC  ) OPEND(ADCHL_BC) // (4+1,4+1,4,3)
OPFUNC(ADCHL_DE) CLK2(15, 4) MADCHL(cpuD  ,cpuE  ) OPEND(ADCHL_DE)
OPFUNC(ADCHL_HL) CLK2(15, 4) MADCHL( chS  , clT  ) OPEND(ADCHL_HL)
OPFUNC(ADCHL_SP) CLK2(15, 4) MADCHL(cpuSPH,cpuSPL) OPEND(ADCHL_SP)

OPFUNC(SBCHL_BC) CLK2(15, 4) MSBCHL(cpuB  ,cpuC  ) OPEND(SBCHL_BC) // (4+1,4+1,4,3)
OPFUNC(SBCHL_DE) CLK2(15, 4) MSBCHL(cpuD  ,cpuE  ) OPEND(SBCHL_DE)
OPFUNC(SBCHL_HL) CLK2(15, 4) MSBCHL( chS  , clT  ) OPEND(SBCHL_HL)
OPFUNC(SBCHL_SP) CLK2(15, 4) MSBCHL(cpuSPH,cpuSPL) OPEND(SBCHL_SP)

#define INC16X(cpu16)  "mov " dUV "," cpu16 NL "inc " dxUV NL CLK1(6, 1) "mov " cpu16 "," dUV NL // (6+1)
OPFUNC(INC_BC)         INC16X(cpuBC) OPEND(INC_BC) // INC ss ................
OPFUNC(INC_DE)         INC16X(cpuDE) OPEND(INC_DE)
OPFUNC(INC_HL)         INC16X(cpuHL) OPEND(INC_HL)
OPFUNC(INC_SP) SP_LOAD INC16X(cpuSP) OPEND(INC_SP)
OPFUNC(INC_IX)         INC16X(cpuIX) OPEND(INC_IX)
OPFUNC(INC_IY)         INC16X(cpuIY) OPEND(INC_IY)
#undef INC16X

#define DEC16X(cpu16)  "mov " dUV "," cpu16 NL "dec " dxUV NL CLK1(6, 1) "mov " cpu16 "," dUV NL // (6+1)
OPFUNC(DEC_BC)         DEC16X(cpuBC) OPEND(DEC_BC) // DEC ss ................
OPFUNC(DEC_DE)         DEC16X(cpuDE) OPEND(DEC_DE)
OPFUNC(DEC_HL)         DEC16X(cpuHL) OPEND(DEC_HL)
OPFUNC(DEC_SP) SP_LOAD DEC16X(cpuSP) OPEND(DEC_SP)
OPFUNC(DEC_IX)         DEC16X(cpuIX) OPEND(DEC_IX)
OPFUNC(DEC_IY)         DEC16X(cpuIY) OPEND(DEC_IY)
#undef DEC16X

///////////////////////////////////////////////////////////////
// ........

OPFUNC(EX_AF_AF)
#define dxAF    "dx"
#define cxREVAF "cx"
	mov_dlA_dhSZ_H_PNC_cpu__X_X___to_dxAF
	"mov cx," cpuREVAF NL
	"mov " cpuREVAF "," dxAF NL
	"mov " dxAF "," cxREVAF NL
	mov_dxAFto_dlA_dhSZ_H_PNC_cpu__X_X___
	CLK1(4, 1)
#undef cxREVAF
#undef dxAF
OPEND(EX_AF_AF)
#define cxNN "cx"
OPFUNC(EX_SP_HL) POPdxNN "mov cx," dxNN NL "mov dx," cpuHL NL CLK1(19, 5) "mov " cpuHL "," cxNN NL PUSHdxNN SP_LOAD OPEND(EX_SP_HL) // (4+1,3,4,3,5)
//OPFUNC(EX_SP_IX) POPdxNN MOV16(PQ, UV) MOV16(UV, IX) CLK1(19, 5) MOV16(IX, PQ) MPUSHUV SP_LOAD OPEND(EX_SP_IX)
//OPFUNC(EX_SP_IY) POPdxNN MOV16(PQ, UV) MOV16(UV, IY) CLK1(19, 5) MOV16(IY, PQ) MPUSHUV SP_LOAD OPEND(EX_SP_IY)
#undef cxNN

OPFUNC(EX_DE_HL)        "mov " dUV "," cpuDE NL "mov " cST "," cpuHL NL CLK1(4, 1) "mov " cpuHL "," dUV NL "mov " cpuDE "," cST NL OPEND(EX_DE_HL)

OPFUNC(EXX)
	"mov " dUV "," cpuREVBC NL "mov " cST "," cpuBC NL "mov " cpuBC "," dUV NL "mov " cpuREVBC "," cST NL
	"mov " dUV "," cpuREVDE NL "mov " cST "," cpuDE NL "mov " cpuDE "," dUV NL "mov " cpuREVDE "," cST NL
	"mov " dUV "," cpuREVHL NL "mov " cST "," cpuHL NL "mov " cpuHL "," dUV NL "mov " cpuREVHL "," cST NL
	CLK1(4, 1) 
OPEND(EXX)

///////////////////////////////////////////////////////////////
/// CPU control
// @note (*a)Exec0....ExecDual/ExecSingle..............................(......-........................)..
//           ExecDual............................(......0..............)..................
//           TODO: ............................................
//                 (......................................................0..3(4)............)

OPFUNC(NOP) CLK1(4, 1) OPEND(NOP)

OPFUNC(HALT)
#define LOCAL LC "HALT"
#define dlINT      "dl"
#define dlINT_NOT  "dl"
	"mov dl," cpuINT NL
	"or " cpuSTATE "," _(STATE_HALT) NL
	"not " dlINT NL
	"test " dlINT_NOT ",(" _(INT_PENDING) "+" _(IFF1) ")" NL
	"jz " LOCAL "_exit_halt" NL
	"mov " bpCLK "," cpuGOAL_CLK NL // (*a)
	"jmp " LOCAL "_pc_back" LF
LOCAL "_exit_halt:" NL
	CLK1LF(48, 1) // ???
LOCAL "_pc_back:" NL
	decPCret
#undef LOCAL
#undef dlINT
#undef dlINT_NOT
OPEND0(HALT)

///////////////////////////////////////////////////////////////
// 8 bit ........

#define	OPALU(label, macro) \
	OPFUNC(label##_B)  macro(cpuB)  CLK1(4, 1) OPEND(label##_B) \
	OPFUNC(label##_C)  macro(cpuC)  CLK1(4, 1) OPEND(label##_C) \
	OPFUNC(label##_D)  macro(cpuD)  CLK1(4, 1) OPEND(label##_D) \
	OPFUNC(label##_E)  macro(cpuE)  CLK1(4, 1) OPEND(label##_E) \
	OPFUNC(label##_H)  macro(cpuH)  CLK1(4, 1) OPEND(label##_H) \
	OPFUNC(label##_L)  macro(cpuL)  CLK1(4, 1) OPEND(label##_L) \
	OPFUNC(label##_A)  macro( alA)  CLK1(4, 1) OPEND(label##_A) \
/*	OPFUNC(label##_XH) macro(cpuXH) CLK1(4, 1) OPEND(label##_XH) \
	OPFUNC(label##_XL) macro(cpuXL) CLK1(4, 1) OPEND(label##_XL) \
	OPFUNC(label##_YH) macro(cpuYH) CLK1(4, 1) OPEND(label##_YH) \
	OPFUNC(label##_YL) macro(cpuYL) CLK1(4, 1) OPEND(label##_YL) */ \
	OPFUNC(label##_M)  cpuHLto_dADDR READ8dlN_dxADDR macro(dlV) CLK1(7, 2) OPEND(label##_M) /* (4+1,3) */ \
	OPFUNC(label##_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR macro(dlV) CLK1(15, 4) OPEND(label##_MX) /* (,4+1,3,5,3)(*1) */ \
	OPFUNC(label##_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR macro(dlV) CLK1(15, 4) OPEND(label##_MY) \
	OPFUNC(label##_N)  FETCH8dlV/*(label##_N)*/ macro(dlV) CLK1(7, 2) "ret" LF/* FETCH8PGBRK(label##_N)*/ OPEND0(label##_N) // (4+1,3)

OPALU(ADD_A, ADD_alA)   OPALU(ADC_A, ADC_alA)   OPALU(SUB, SUB_alA)   OPALU(SBC_A, SBC_alA)
OPALU(AND  , AND_alA)   OPALU(XOR  , XOR_alA)   OPALU( OR,  OR_alA)   OPALU( CP  ,  CP_alA)

OPFUNC(INC_B)  "mov " dlV "," cpuB NL INC_dlV "mov " cpuB "," dlV NL CLK1(4, 1) OPEND(INC_B)
OPFUNC(INC_C)  "mov " dlV "," cpuC NL INC_dlV "mov " cpuC "," dlV NL CLK1(4, 1) OPEND(INC_C)
OPFUNC(INC_D)  "mov " dlV "," cpuD NL INC_dlV "mov " cpuD "," dlV NL CLK1(4, 1) OPEND(INC_D)
OPFUNC(INC_E)  "mov " dlV "," cpuE NL INC_dlV "mov " cpuE "," dlV NL CLK1(4, 1) OPEND(INC_E)
OPFUNC(INC_H)  "mov " dlV "," cpuH NL INC_dlV "mov " cpuH "," dlV NL CLK1(4, 1) OPEND(INC_H)
OPFUNC(INC_L)  "mov " dlV "," cpuL NL INC_dlV "mov " cpuL "," dlV NL CLK1(4, 1) OPEND(INC_L)
OPFUNC(INC_A)  "mov " dlV ","  alA NL INC_dlV "mov "  alA "," dlV NL CLK1(4, 1) OPEND(INC_A)
OPFUNC(INC_XH) "mov " dlV "," cpuXH NL INC_dlV "mov " cpuXH "," dlV NL CLK1(4, 1) OPEND(INC_XH)
OPFUNC(INC_XL) "mov " dlV "," cpuXL NL INC_dlV "mov " cpuXL "," dlV NL CLK1(4, 1) OPEND(INC_XL)
OPFUNC(INC_YH) "mov " dlV "," cpuYH NL INC_dlV "mov " cpuYH "," dlV NL CLK1(4, 1) OPEND(INC_YH)
OPFUNC(INC_YL) "mov " dlV "," cpuYL NL INC_dlV "mov " cpuYL "," dlV NL CLK1(4, 1) OPEND(INC_YL)
OPFUNC(INC_M)  cpuHLto_dADDR "mov cx," dxADDR NL
                              "shl edx,16" NL
                              "mov dx," cxADDR NL
                              READ8dlN_dxADDR
                              INC_dlV
                              "rol edx,16" NL
                              WRITE8d2N_dxADDR CLK1(11, 3) OPEND(INC_M) // (4+1,4,3)
OPFUNC(INC_MX) cpuIXrbxRIPto_dxADDR "mov cx," dxADDR NL
                                    "shl edx,16" NL
                                    "mov dx," cxADDR NL
                                    READ8dlN_dxADDR
                                    INC_dlV
                                    "rol edx,16" NL
                                    WRITE8d2N_dxADDR CLK1(19, 5) OPEND(INC_MX) // (,4+1,3,5,4,3)
OPFUNC(INC_MY) cpuIYrbxRIPto_dxADDR "mov cx," dxADDR NL
                                    "shl edx,16" NL
                                    "mov dx," cxADDR NL
                                    READ8dlN_dxADDR
                                    INC_dlV
                                    "rol edx,16" NL
                                    WRITE8d2N_dxADDR CLK1(19, 5) OPEND(INC_MY)

OPFUNC(DEC_B)  "mov " dlV "," cpuB NL DEC_dlV "mov " cpuB "," dlV NL CLK1(4, 1) OPEND(DEC_B)
OPFUNC(DEC_C)  "mov " dlV "," cpuC NL DEC_dlV "mov " cpuC "," dlV NL CLK1(4, 1) OPEND(DEC_C)
OPFUNC(DEC_D)  "mov " dlV "," cpuD NL DEC_dlV "mov " cpuD "," dlV NL CLK1(4, 1) OPEND(DEC_D)
OPFUNC(DEC_E)  "mov " dlV "," cpuE NL DEC_dlV "mov " cpuE "," dlV NL CLK1(4, 1) OPEND(DEC_E)
OPFUNC(DEC_H)  "mov " dlV "," cpuH NL DEC_dlV "mov " cpuH "," dlV NL CLK1(4, 1) OPEND(DEC_H)
OPFUNC(DEC_L)  "mov " dlV "," cpuL NL DEC_dlV "mov " cpuL "," dlV NL CLK1(4, 1) OPEND(DEC_L)
OPFUNC(DEC_A)  "mov " dlV ","  alA NL DEC_dlV "mov "  alA "," dlV NL CLK1(4, 1) OPEND(DEC_A)
OPFUNC(DEC_XH) "mov " dlV "," cpuXH NL DEC_dlV "mov " cpuXH "," dlV NL CLK1(4, 1) OPEND(DEC_XH)
OPFUNC(DEC_XL) "mov " dlV "," cpuXL NL DEC_dlV "mov " cpuXL "," dlV NL CLK1(4, 1) OPEND(DEC_XL)
OPFUNC(DEC_YH) "mov " dlV "," cpuYH NL DEC_dlV "mov " cpuYH "," dlV NL CLK1(4, 1) OPEND(DEC_YH)
OPFUNC(DEC_YL) "mov " dlV "," cpuYL NL DEC_dlV "mov " cpuYL "," dlV NL CLK1(4, 1) OPEND(DEC_YL)
OPFUNC(DEC_M)  cpuHLto_dADDR "mov cx," dxADDR NL
                              "shl edx,16" NL
                              "mov dx," cxADDR NL
                              READ8dlN_dxADDR
                              DEC_dlV
                              "rol edx,16" NL
                              WRITE8d2N_dxADDR CLK1(11, 3) OPEND(DEC_M) // (4+1,4,3)
OPFUNC(DEC_MX) cpuIXrbxRIPto_dxADDR "mov cx," dxADDR NL
                                    "shl edx,16" NL
                                    "mov dx," cxADDR NL
                                    READ8dlN_dxADDR
                                    DEC_dlV
                                    "rol edx,16" NL
                                    WRITE8d2N_dxADDR CLK1(19, 5) OPEND(DEC_MX) // (,4+1,3,5,4,3)
OPFUNC(DEC_MY) cpuIYrbxRIPto_dxADDR "mov cx," dxADDR NL
                                    "shl edx,16" NL
                                    "mov dx," cxADDR NL
                                    READ8dlN_dxADDR
                                    DEC_dlV
                                    "rol edx,16" NL
                                    WRITE8d2N_dxADDR CLK1(19, 5) OPEND(DEC_MY)

///////////////////////////////////////////////////////////////
// ............

OPFUNC(PUSH_BC) "mov " dUV "," cpuBC NL PUSHdxNN SP_PUSH CLK1(11, 3) OPEND(PUSH_BC) // (5+1,3,3)
OPFUNC(PUSH_DE) "mov " dUV "," cpuDE NL PUSHdxNN SP_PUSH CLK1(11, 3) OPEND(PUSH_DE)
OPFUNC(PUSH_HL) "mov " dUV "," cpuHL NL PUSHdxNN SP_PUSH CLK1(11, 3) OPEND(PUSH_HL)
OPFUNC(PUSH_IX) "mov " dUV "," cpuIX NL PUSHdxNN SP_PUSH CLK1(11, 3) OPEND(PUSH_IX)
OPFUNC(PUSH_IY) "mov " dUV "," cpuIY NL PUSHdxNN SP_PUSH CLK1(11, 3) OPEND(PUSH_IY)
OPFUNC(PUSH_AF) mov_dlA_dhSZ_H_PNC_cpu__X_X___to_dxAF CLK1(11, 3) PUSHdxNN SP_PUSH OPEND(PUSH_AF)

OPFUNC(POP_BC) POPdxNN "mov " cpuBC "," dUV NL CLK1(10, 3) OPEND(POP_BC) // (4+1,3,3)
OPFUNC(POP_DE) POPdxNN "mov " cpuDE "," dUV NL CLK1(10, 3) OPEND(POP_DE)
OPFUNC(POP_HL) POPdxNN "mov " cpuHL "," dUV NL CLK1(10, 3) OPEND(POP_HL)
OPFUNC(POP_IX) POPdxNN "mov " cpuIX "," dUV NL CLK1(10, 3) OPEND(POP_IX)
OPFUNC(POP_IY) POPdxNN "mov " cpuIY "," dUV NL CLK1(10, 3) OPEND(POP_IY)
OPFUNC(POP_AF) POPdxNN CLK1(10, 3) mov_dxAFto_dlA_dhSZ_H_PNC_cpu__X_X___ OPEND(POP_AF)

///////////////////////////////////////////////////////////////
// 16 ................

OPFUNC(LD_BC_NN)          FETCH16dUV "mov " cpuBC "," dUV NL CLK1(10, 3) OPEND(LD_BC_NN) // (4+1,3,3)
OPFUNC(LD_DE_NN)          FETCH16dUV "mov " cpuDE "," dUV NL CLK1(10, 3) OPEND(LD_DE_NN)
OPFUNC(LD_HL_NN)          FETCH16dUV "mov " cpuHL "," dUV NL CLK1(10, 3) OPEND(LD_HL_NN)
OPFUNC(LD_IX_NN)          FETCH16dUV "mov " cpuIX "," dUV NL CLK1(10, 3) OPEND(LD_IX_NN)
OPFUNC(LD_IY_NN)          FETCH16dUV "mov " cpuIY "," dUV NL CLK1(10, 3) OPEND(LD_IY_NN)
OPFUNC(LD_SP_NN)  SP_LOAD FETCH16dUV "mov " cpuSP "," dUV NL CLK1(10, 3) OPEND(LD_SP_NN)

OPFUNC(LD_MM_HL)  FETCH16dxUV mov_dw(cpuHL16) CLK1(16, 5) WRITE16dwNN_dxADDR OPEND(LD_MM_HL)  // (4+1,3,3,3,3)
OPFUNC(LD_MM_BC)  FETCH16dxUV mov_dw(cpuBC16) CLK2(20, 6) WRITE16dwNN_dxADDR OPEND(LD_MM_BC)  // (4+1,4+1,3,3,3,3)
OPFUNC(LD_MM_DE)  FETCH16dxUV mov_dw(cpuDE16) CLK2(20, 6) WRITE16dwNN_dxADDR OPEND(LD_MM_DE) 
OPFUNC(LD_MM_HL2) FETCH16dxUV mov_dw(cpuHL16) CLK2(20, 6) WRITE16dwNN_dxADDR OPEND(LD_MM_HL2)
OPFUNC(LD_MM_SP)  FETCH16dxUV mov_dw(cpuSP16) CLK2(20, 6) WRITE16dwNN_dxADDR OPEND(LD_MM_SP) 
OPFUNC(LD_MM_IX)  FETCH16dxUV mov_dw(cpuIX16) CLK1(16, 5) WRITE16dwNN_dxADDR OPEND(LD_MM_IX) 
OPFUNC(LD_MM_IY)  FETCH16dxUV mov_dw(cpuIY16) CLK1(16, 5) WRITE16dwNN_dxADDR OPEND(LD_MM_IY) 

OPFUNC(LD_HL_MM)          FETCH16dxUV CLK1(16, 5)
	"cmp dx,0xF0AE" NL
	"jnz " LC _(LD_HL_MM) "_nz" NL
	"nop" LF
LC _(LD_HL_MM) "_nz:" NL
                                                  READ16dxNN_dxADDR movzx_to_cpu(cpuHL, "dx") NL OPEND(LD_HL_MM) // (4+1,3,3,3,3)
OPFUNC(LD_BC_MM)          FETCH16dxUV CLK2(20, 6) READ16dxNN_dxADDR movzx_to_cpu(cpuBC, "dx") NL OPEND(LD_BC_MM) // (4+1,4+1,3,3,3,3)
OPFUNC(LD_DE_MM)          FETCH16dxUV CLK2(20, 6) READ16dxNN_dxADDR movzx_to_cpu(cpuDE, "dx") NL OPEND(LD_DE_MM) 
OPFUNC(LD_HL_MM2)         FETCH16dxUV CLK2(20, 6) READ16dxNN_dxADDR movzx_to_cpu(cpuHL, "dx") NL OPEND(LD_HL_MM2)
OPFUNC(LD_SP_MM)  SP_LOAD FETCH16dxUV CLK2(20, 6) READ16dxNN_dxADDR movzx_to_cpu(cpuSP, "dx") NL OPEND(LD_SP_MM) 
OPFUNC(LD_IX_MM)          FETCH16dxUV CLK1(16, 5) READ16dxNN_dxADDR movzx_to_cpu(cpuIX, "dx") NL OPEND(LD_IX_MM) // (,4+1,3,3,3,3)
OPFUNC(LD_IY_MM)          FETCH16dxUV CLK1(16, 5) READ16dxNN_dxADDR movzx_to_cpu(cpuIY, "dx") NL OPEND(LD_IY_MM) 

OPFUNC(LD_SP_HL)  SP_LOAD "mov " dUV "," cpuHL NL CLK1(6, 1) "mov " cpuSP "," dUV NL OPEND(LD_SP_HL) // (6+1)

///////////////////////////////////////////////////////////////
// 8 ................

OPFUNC(LD_B_C)  MOVdl(cpuB, cpuC) CLK1(4, 1) OPEND(LD_B_C)
OPFUNC(LD_B_D)  MOVdl(cpuB, cpuD) CLK1(4, 1) OPEND(LD_B_D)
OPFUNC(LD_B_E)  MOVdl(cpuB, cpuE) CLK1(4, 1) OPEND(LD_B_E)
OPFUNC(LD_B_H)  MOVdl(cpuB, cpuH) CLK1(4, 1) OPEND(LD_B_H)
OPFUNC(LD_B_L)  MOVdl(cpuB, cpuL) CLK1(4, 1) OPEND(LD_B_L)
OPFUNC(LD_B_A) "mov " cpuB "," alA NL CLK1(4, 1) OPEND(LD_B_A)
OPFUNC(LD_B_XH) MOVdl(cpuB, cpuXH) CLK1(4, 1) OPEND(LD_B_XH)
OPFUNC(LD_B_YH) MOVdl(cpuB, cpuYH) CLK1(4, 1) OPEND(LD_B_YH)
OPFUNC(LD_B_XL) MOVdl(cpuB, cpuXL) CLK1(4, 1) OPEND(LD_B_XL)
OPFUNC(LD_B_YL) MOVdl(cpuB, cpuYL) CLK1(4, 1) OPEND(LD_B_YL)
OPFUNC(LD_B_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuB "," dlV NL CLK1(7, 2) OPEND(LD_B_M) // (4+1,3)
OPFUNC(LD_B_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuB "," dlV NL CLK1(15, 4) OPEND(LD_B_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_B_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuB "," dlV NL CLK1(15, 4) OPEND(LD_B_MY)
OPFUNC(LD_B_N) FETCH8dlV "mov " cpuB "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_B_N)*/ OPEND0(LD_B_N) // (4+1,3)

OPFUNC(LD_C_B)  MOVdl(cpuC, cpuB) CLK1(4, 1) OPEND(LD_C_B)
OPFUNC(LD_C_D)  MOVdl(cpuC, cpuD) CLK1(4, 1) OPEND(LD_C_D)
OPFUNC(LD_C_E)  MOVdl(cpuC, cpuE) CLK1(4, 1) OPEND(LD_C_E)
OPFUNC(LD_C_H)  MOVdl(cpuC, cpuH) CLK1(4, 1) OPEND(LD_C_H)
OPFUNC(LD_C_L)  MOVdl(cpuC, cpuL) CLK1(4, 1) OPEND(LD_C_L)
OPFUNC(LD_C_A) "mov " cpuC "," alA NL CLK1(4, 1) OPEND(LD_C_A)
OPFUNC(LD_C_XH) MOVdl(cpuC, cpuXH) CLK1(4, 1) OPEND(LD_C_XH)
OPFUNC(LD_C_YH) MOVdl(cpuC, cpuYH) CLK1(4, 1) OPEND(LD_C_YH)
OPFUNC(LD_C_XL) MOVdl(cpuC, cpuXL) CLK1(4, 1) OPEND(LD_C_XL)
OPFUNC(LD_C_YL) MOVdl(cpuC, cpuYL) CLK1(4, 1) OPEND(LD_C_YL)
OPFUNC(LD_C_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuC "," dlV NL CLK1(7, 2) OPEND(LD_C_M) // (4+1,3)
OPFUNC(LD_C_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuC "," dlV NL CLK1(15, 4) OPEND(LD_C_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_C_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuC "," dlV NL CLK1(15, 4) OPEND(LD_C_MY)
OPFUNC(LD_C_N) FETCH8dlV "mov " cpuC "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_C_N)*/ OPEND0(LD_C_N) // (4+1,3)

OPFUNC(LD_D_B)  MOVdl(cpuD, cpuB) CLK1(4, 1) OPEND(LD_D_B)
OPFUNC(LD_D_C)  MOVdl(cpuD, cpuC) CLK1(4, 1) OPEND(LD_D_C)
OPFUNC(LD_D_E)  MOVdl(cpuD, cpuE) CLK1(4, 1) OPEND(LD_D_E)
OPFUNC(LD_D_H)  MOVdl(cpuD, cpuH) CLK1(4, 1) OPEND(LD_D_H)
OPFUNC(LD_D_L)  MOVdl(cpuD, cpuL) CLK1(4, 1) OPEND(LD_D_L)
OPFUNC(LD_D_A) "mov " cpuD "," alA NL CLK1(4, 1) OPEND(LD_D_A)
OPFUNC(LD_D_XH) MOVdl(cpuD, cpuXH) CLK1(4, 1) OPEND(LD_D_XH)
OPFUNC(LD_D_YH) MOVdl(cpuD, cpuYH) CLK1(4, 1) OPEND(LD_D_YH)
OPFUNC(LD_D_XL) MOVdl(cpuD, cpuXL) CLK1(4, 1) OPEND(LD_D_XL)
OPFUNC(LD_D_YL) MOVdl(cpuD, cpuYL) CLK1(4, 1) OPEND(LD_D_YL)
OPFUNC(LD_D_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuD "," dlV NL CLK1(7, 2) OPEND(LD_D_M) // (4+1,3)
OPFUNC(LD_D_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuD "," dlV NL CLK1(15, 4) OPEND(LD_D_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_D_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuD "," dlV NL CLK1(15, 4) OPEND(LD_D_MY)
OPFUNC(LD_D_N) FETCH8dlV "mov " cpuD "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_D_N)*/ OPEND0(LD_D_N) // (4+1,3)

OPFUNC(LD_E_B)  MOVdl(cpuE, cpuB) CLK1(4, 1) OPEND(LD_E_B)
OPFUNC(LD_E_C)  MOVdl(cpuE, cpuC) CLK1(4, 1) OPEND(LD_E_C)
OPFUNC(LD_E_D)  MOVdl(cpuE, cpuD) CLK1(4, 1) OPEND(LD_E_D)
OPFUNC(LD_E_H)  MOVdl(cpuE, cpuH) CLK1(4, 1) OPEND(LD_E_H)
OPFUNC(LD_E_L)  MOVdl(cpuE, cpuL) CLK1(4, 1) OPEND(LD_E_L)
OPFUNC(LD_E_A) "mov " cpuE "," alA NL CLK1(4, 1) OPEND(LD_E_A)
OPFUNC(LD_E_XH) MOVdl(cpuE, cpuXH) CLK1(4, 1) OPEND(LD_E_XH)
OPFUNC(LD_E_YH) MOVdl(cpuE, cpuYH) CLK1(4, 1) OPEND(LD_E_YH)
OPFUNC(LD_E_XL) MOVdl(cpuE, cpuXL) CLK1(4, 1) OPEND(LD_E_XL)
OPFUNC(LD_E_YL) MOVdl(cpuE, cpuYL) CLK1(4, 1) OPEND(LD_E_YL)
OPFUNC(LD_E_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuE "," dlV NL CLK1(7, 2) OPEND(LD_E_M) // (4+1,3)
OPFUNC(LD_E_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR FTRACE_set_flags(3, 0x02) FTRACE_append_val8("dl") "mov " cpuE "," dlV NL CLK1(15, 4) OPEND(LD_E_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_E_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuE "," dlV NL CLK1(15, 4) OPEND(LD_E_MY)
OPFUNC(LD_E_N) FETCH8dlV "mov " cpuE "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_E_N)*/ OPEND0(LD_E_N) // (4+1,3)

OPFUNC(LD_H_B)  MOVdl(cpuH, cpuB) CLK1(4, 1) OPEND(LD_H_B)
OPFUNC(LD_H_C)  MOVdl(cpuH, cpuC) CLK1(4, 1) OPEND(LD_H_C)
OPFUNC(LD_H_D)  MOVdl(cpuH, cpuD) CLK1(4, 1) OPEND(LD_H_D)
OPFUNC(LD_H_E)  MOVdl(cpuH, cpuE) CLK1(4, 1) OPEND(LD_H_E)
OPFUNC(LD_H_L)  MOVdl(cpuH, cpuL) CLK1(4, 1) OPEND(LD_H_L)
OPFUNC(LD_H_A) "mov " cpuH "," alA NL CLK1(4, 1) OPEND(LD_H_A)
OPFUNC(LD_H_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuH "," dlV NL CLK1(7, 2) OPEND(LD_H_M) // (4+1,3)
OPFUNC(LD_H_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuH "," dlV NL CLK1(15, 4) OPEND(LD_H_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_H_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuH "," dlV NL CLK1(15, 4) OPEND(LD_H_MY)
OPFUNC(LD_H_N) FETCH8dlV "mov " cpuH "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_H_N)*/ OPEND0(LD_H_N) // (4+1,3)

OPFUNC(LD_L_B)  MOVdl(cpuL, cpuB) CLK1(4, 1) OPEND(LD_L_B)
OPFUNC(LD_L_C)  MOVdl(cpuL, cpuC) CLK1(4, 1) OPEND(LD_L_C)
OPFUNC(LD_L_D)  MOVdl(cpuL, cpuD) CLK1(4, 1) OPEND(LD_L_D)
OPFUNC(LD_L_E)  MOVdl(cpuL, cpuE) CLK1(4, 1) OPEND(LD_L_E)
OPFUNC(LD_L_H)  MOVdl(cpuL, cpuH) CLK1(4, 1) OPEND(LD_L_H)
OPFUNC(LD_L_A) "mov " cpuL "," alA NL CLK1(4, 1) OPEND(LD_L_A)
OPFUNC(LD_L_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " cpuL "," dlV NL CLK1(7, 2) OPEND(LD_L_M) // (4+1,3)
OPFUNC(LD_L_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuL "," dlV NL CLK1(15, 4) OPEND(LD_L_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_L_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " cpuL "," dlV NL CLK1(15, 4) OPEND(LD_L_MY)
OPFUNC(LD_L_N) FETCH8dlV "mov " cpuL "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_L_N)*/ OPEND0(LD_L_N) // (4+1,3)

OPFUNC(LD_M_B) "mov dl," cpuB NL "shl edx,16" NL cpuHLto_dxADDR WRITE8d2N_dxADDR CLK1(7, 2) OPEND(LD_M_B) // (4+1,3)
OPFUNC(LD_M_C) "mov dl," cpuC NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_C)
OPFUNC(LD_M_D) "mov dl," cpuD NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_D)
OPFUNC(LD_M_E) "mov dl," cpuE NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_E)
OPFUNC(LD_M_H) "mov dl," cpuH NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_H)
OPFUNC(LD_M_L) "mov dl," cpuL NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_L)
OPFUNC(LD_M_A) "mov dl,"  alA NL "shl edx,16" NL cpuHLto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_M_A)
OPFUNC(LD_M_N) FETCH8dlV         "shl edx,16" NL cpuHLto_dxADDR CLK1(10, 3) WRITE8d2N_dxADDR "ret" LF /*FETCH8PGBRK(LD_M_N)*/ OPEND0(LD_M_N) // (4+1,3,3)

OPFUNC(LD_MX_B) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuB NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_B) // (,4+1,3,5,3)
OPFUNC(LD_MX_C) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuC NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_C)
OPFUNC(LD_MX_D) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuD NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_D)
OPFUNC(LD_MX_E) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuE NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_E)
OPFUNC(LD_MX_H) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuH NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_H)
OPFUNC(LD_MX_L) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuL NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_L)
OPFUNC(LD_MX_A) cpuIXrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV ","  alA NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MX_A)
OPFUNC(LD_MX_N) FETCH16dxUV "shl edx,8" NL "movsx dx," dhU NL "add dx," cpuIX16 NL CLK1(15, 4) WRITE8d2N_dxADDR OPEND(LD_MX_N) // (,4+1,3,5,3)

OPFUNC(LD_MY_B) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuB NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_B) // (,4+1,3,5,3)
OPFUNC(LD_MY_C) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuC NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_C)
OPFUNC(LD_MY_D) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuD NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_D)
OPFUNC(LD_MY_E) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuE NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_E)
OPFUNC(LD_MY_H) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuH NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_H)
OPFUNC(LD_MY_L) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV "," cpuL NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_L)
OPFUNC(LD_MY_A) cpuIYrbxRIPto_dxADDR "ror edx,16" NL "mov " dlV ","  alA NL "rol edx,16" NL WRITE8d2N_dxADDR CLK1(15, 4) OPEND(LD_MY_A)
OPFUNC(LD_MY_N) FETCH16dxUV "shl edx,8" NL "movsx dx," dhU NL "add dx," cpuIY16 NL CLK1(15, 4) WRITE8d2N_dxADDR OPEND(LD_MY_N) // (,4+1,3,5,3)

OPFUNC(LD_A_B) "mov " alA "," cpuB NL CLK1(4, 1) OPEND(LD_A_B)
OPFUNC(LD_A_C) "mov " alA "," cpuC NL CLK1(4, 1) OPEND(LD_A_C)
OPFUNC(LD_A_D) "mov " alA "," cpuD NL CLK1(4, 1) OPEND(LD_A_D)
OPFUNC(LD_A_E) "mov " alA "," cpuE NL CLK1(4, 1) OPEND(LD_A_E)
OPFUNC(LD_A_H) "mov " alA "," cpuH NL CLK1(4, 1) OPEND(LD_A_H)
OPFUNC(LD_A_L) "mov " alA "," cpuL NL CLK1(4, 1) OPEND(LD_A_L)
OPFUNC(LD_A_XH) "mov " alA "," cpuXH NL CLK1(4, 1) OPEND(LD_A_XH)
OPFUNC(LD_A_YH) "mov " alA "," cpuYH NL CLK1(4, 1) OPEND(LD_A_YH)
OPFUNC(LD_A_XL) "mov " alA "," cpuXL NL CLK1(4, 1) OPEND(LD_A_XL)
OPFUNC(LD_A_YL) "mov " alA "," cpuYL NL CLK1(4, 1) OPEND(LD_A_YL)
OPFUNC(LD_A_M)         cpuHLto_dADDR READ8dlN_dxADDR "mov " alA "," dlV NL CLK1(7, 2) OPEND(LD_A_M) // (4+1,3)
OPFUNC(LD_A_MX) cpuIXrbxRIPto_dxADDR READ8dlN_dxADDR FTRACE_set_flags(3, 0x02) FTRACE_append_val8("dl") "mov " alA "," dlV NL CLK1(15, 4) OPEND(LD_A_MX) // (,4+1,3,5,3) (*1)
OPFUNC(LD_A_MY) cpuIYrbxRIPto_dxADDR READ8dlN_dxADDR "mov " alA "," dlV NL CLK1(15, 4) OPEND(LD_A_MY)
OPFUNC(LD_A_N) FETCH8dlV "mov " alA "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_A_N)*/ OPEND0(LD_A_N) // (4+1,3)

OPFUNC(LD_XH_B)  MOVdl(cpuXH, cpuB) CLK1(4, 1) OPEND(LD_XH_B)
OPFUNC(LD_XH_C)  MOVdl(cpuXH, cpuC) CLK1(4, 1) OPEND(LD_XH_C)
OPFUNC(LD_XH_D)  MOVdl(cpuXH, cpuD) CLK1(4, 1) OPEND(LD_XH_D)
OPFUNC(LD_XH_E)  MOVdl(cpuXH, cpuE) CLK1(4, 1) OPEND(LD_XH_E)
OPFUNC(LD_XH_A) "mov " cpuXH "," alA NL CLK1(4, 1) OPEND(LD_XH_A)
//OPFUNC(LD_XH_XL) MOV(XH, XL) CLK1(4, 1) OPEND(LD_XH_XL)
OPFUNC(LD_XH_N) FETCH8dlV "mov " cpuXH "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_XH_N)*/ OPEND0(LD_XH_N) // (4+1,3)

OPFUNC(LD_YH_B)  MOVdl(cpuYH, cpuB) CLK1(4, 1) OPEND(LD_YH_B)
OPFUNC(LD_YH_C)  MOVdl(cpuYH, cpuC) CLK1(4, 1) OPEND(LD_YH_C)
OPFUNC(LD_YH_D)  MOVdl(cpuYH, cpuD) CLK1(4, 1) OPEND(LD_YH_D)
OPFUNC(LD_YH_E)  MOVdl(cpuYH, cpuE) CLK1(4, 1) OPEND(LD_YH_E)
OPFUNC(LD_YH_A) "mov " cpuYH "," alA NL CLK1(4, 1) OPEND(LD_YH_A)
//OPFUNC(LD_YH_YL) MOV(YH, YL) CLK1(4, 1) OPEND(LD_YH_YL)
OPFUNC(LD_YH_N) FETCH8dlV "mov " cpuYH "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_YH_N)*/ OPEND0(LD_YH_N)

OPFUNC(LD_XL_B)  MOVdl(cpuXL, cpuB) CLK1(4, 1) OPEND(LD_XL_B)
OPFUNC(LD_XL_C)  MOVdl(cpuXL, cpuC) CLK1(4, 1) OPEND(LD_XL_C)
OPFUNC(LD_XL_D)  MOVdl(cpuXL, cpuD) CLK1(4, 1) OPEND(LD_XL_D)
OPFUNC(LD_XL_E)  MOVdl(cpuXL, cpuE) CLK1(4, 1) OPEND(LD_XL_E)
OPFUNC(LD_XL_A) "mov " cpuXL "," alA NL CLK1(4, 1) OPEND(LD_XL_A)
//OPFUNC(LD_XL_XH) MOV(XL, XH) CLK1(4, 1) OPEND(LD_XL_XH)
OPFUNC(LD_XL_N) FETCH8dlV "mov " cpuXL "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_XL_N)*/ OPEND0(LD_XL_N) // (4+1,3)

OPFUNC(LD_YL_B)  MOVdl(cpuYL, cpuB) CLK1(4, 1) OPEND(LD_YL_B)
OPFUNC(LD_YL_C)  MOVdl(cpuYL, cpuC) CLK1(4, 1) OPEND(LD_YL_C)
OPFUNC(LD_YL_D)  MOVdl(cpuYL, cpuD) CLK1(4, 1) OPEND(LD_YL_D)
OPFUNC(LD_YL_E)  MOVdl(cpuYL, cpuE) CLK1(4, 1) OPEND(LD_YL_E)
OPFUNC(LD_YL_A) "mov " cpuYL "," alA NL CLK1(4, 1) OPEND(LD_YL_A)
//OPFUNC(LD_YL_YH) MOV(YL, YH) CLK1(4, 1) OPEND(LD_YL_YH)
OPFUNC(LD_YL_N) FETCH8dlV "mov " cpuYL "," dlV NL CLK1(7, 2) "ret" LF /*FETCH8PGBRK(LD_YL_N)*/ OPEND0(LD_YL_N)

OPFUNC(LD_BC_A) "mov dl," alA NL "shl edx,16" NL cpuBCto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_BC_A) // (4+1,3)
OPFUNC(LD_DE_A) "mov dl," alA NL "shl edx,16" NL cpuDEto_dxADDR CLK1(7, 2)  WRITE8d2N_dxADDR OPEND(LD_DE_A)
OPFUNC(LD_MM_A) FETCH16dxUV "ror edx,16" NL "mov dl," alA NL "rol edx,16" NL CLK1(13, 4) WRITE8d2N_dxADDR OPEND(LD_MM_A) // (4+1,3,3,3)
OPFUNC(LD_A_BC) "mov " dUV "," cpuBC NL READ8dlN_dxADDR "mov " alA "," dlV NL CLK1(7, 2)  OPEND(LD_A_BC) // (4+1,3)
OPFUNC(LD_A_DE) "mov " dUV "," cpuDE NL READ8dlN_dxADDR "mov " alA "," dlV NL CLK1(7, 2)  OPEND(LD_A_DE)
OPFUNC(LD_A_MM)             FETCH16dxUV READ8dlN_dxADDR "mov " alA "," dlV NL CLK1(13, 4) OPEND(LD_A_MM) // (4+1,3,3,3)

OPFUNC(LD_A_I) "mov " alA "," cpuI NL                mov_ah___0_P0_IFF1 CLK2(9, 2) OPEND(LD_A_I) // (4+1,5+1)
OPFUNC(LD_A_R) "mov " alA "," cpuR8 NL mov_alA7_amR7 mov_ah___0_P0_IFF1 CLK2(9, 2) OPEND(LD_A_R)
OPFUNC(LD_I_A) "mov " cpuI "," alA NL                                   CLK2(9, 2) OPEND(LD_I_A)
OPFUNC(LD_R_A)  mov_amR7_alA7 "mov " cpuR8 "," alA NL                   CLK2(9, 2) OPEND(LD_R_A)

///////////////////////////////////////////////////////////////
// ..................

///////////////////////////////////////////////////////////////
// ................

OPFUNC(LDI) MLDI CLK2(16, 4) OPEND(LDI) // (4+1,4+1,3,5)
OPFUNC(LDD) MLDD CLK2(16, 4) OPEND(LDD)

OPFUNC(LDIR) MLDI MIFPO(LDIR) CLK2(21, 5) rbxRIPsub2ret MELSE0(LDIR) CLK2(16, 4) OPEND(LDIR) // (4+1,4+1,3,5,5)
OPFUNC(LDDR) MLDD MIFPO(LDDR) CLK2(21, 5) rbxRIPsub2ret MELSE0(LDDR) CLK2(16, 4) OPEND(LDDR)
#define rdxHL "rdx"
//#define  cxBC  "cx"
#define  dhF   "dh"
OPFUNC(CPI) MCPI CLK2(16, 4) OPEND(CPI) // (4+1,4+1,3,5)
OPFUNC(CPD) MCPD CLK2(16, 4) OPEND(CPD)

OPFUNC(CPIR) MCPI MIFZ(CPIR) MIFPO(CPIR) CLK2(21, 5) rbxRIPsub2ret MELSE0(CPIR) CLK2(16, 4) OPEND(CPIR) // (4+1,4+1,3,5,5)
OPFUNC(CPDR) MCPD MIFZ(CPDR) MIFPO(CPDR) CLK2(21, 5) rbxRIPsub2ret MELSE0(CPDR) CLK2(16, 4) OPEND(CPDR)
#undef rdxHL
//#undef  cxBC
#undef  dhF

///////////////////////////////////////////////////////////////
// CB ....

#define MBITV(n) "pop rcx" NL /* ............ */ \
                 "and " dlV "," #n NL \
                 "setz " clT NL \
                 "and " ahF ",not " SZ____N_ NL \
                 "shl  " clT ",6" NL /* _Z______ */ \
                 "or " ahF "," ___H____ NL \
                 "mov  " cpu__X_X___ "," dlV NL \
                 "and  " dlV "," S_______ NL /* (n=7) ? 1 : 0 */ \
                 "or   " ahF "," clT NL \
                 "shl  " clT ",1" NL /* (Z) ? 1 : 0 */ \
                 "not  " clT NL /* (Z) ? 0 : 1 */ \
                 "or   " ahF "," dlV NL \
                 "and  " ahF "," clT NL /* S = NZ && n = 7 */

#define MRESV(n) "and " dlV ", not " #n NL

#define MSETV(n) "or " dlV "," #n NL

OPFUNC(RLCV) MRLCV OPEND(RLCV) OPFUNC(RLV)  MRLV  OPEND(RLV)
OPFUNC(RRCV) MRRCV OPEND(RRCV) OPFUNC(RRV)  MRRV  OPEND(RRV)
OPFUNC(SLAV) MSLAV OPEND(SLAV) OPFUNC(SRAV) MSRAV OPEND(SRAV)
OPFUNC(SLLV) MSLLV OPEND(SLLV) OPFUNC(SRLV) MSRLV OPEND(SRLV)

OPFUNC(BITV_0) MBITV(0x01) OPEND(BITV_0) OPFUNC(BITV_1) MBITV(0x02) OPEND(BITV_1)
OPFUNC(BITV_2) MBITV(0x04) OPEND(BITV_2) OPFUNC(BITV_3) MBITV(0x08) OPEND(BITV_3)
OPFUNC(BITV_4) MBITV(0x10) OPEND(BITV_4) OPFUNC(BITV_5) MBITV(0x20) OPEND(BITV_5)
OPFUNC(BITV_6) MBITV(0x40) OPEND(BITV_6) OPFUNC(BITV_7) MBITV(0x80) OPEND(BITV_7)
 
OPFUNC(RESV_0) MRESV(0x01) OPEND(RESV_0) OPFUNC(RESV_1) MRESV(0x02) OPEND(RESV_1)
OPFUNC(RESV_2) MRESV(0x04) OPEND(RESV_2) OPFUNC(RESV_3) MRESV(0x08) OPEND(RESV_3)
OPFUNC(RESV_4) MRESV(0x10) OPEND(RESV_4) OPFUNC(RESV_5) MRESV(0x20) OPEND(RESV_5)
OPFUNC(RESV_6) MRESV(0x40) OPEND(RESV_6) OPFUNC(RESV_7) MRESV(0x80) OPEND(RESV_7)
 
OPFUNC(SETV_0) MSETV(0x01) OPEND(SETV_0) OPFUNC(SETV_1) MSETV(0x02) OPEND(SETV_1)
OPFUNC(SETV_2) MSETV(0x04) OPEND(SETV_2) OPFUNC(SETV_3) MSETV(0x08) OPEND(SETV_3)
OPFUNC(SETV_4) MSETV(0x10) OPEND(SETV_4) OPFUNC(SETV_5) MSETV(0x20) OPEND(SETV_5)
OPFUNC(SETV_6) MSETV(0x40) OPEND(SETV_6) OPFUNC(SETV_7) MSETV(0x80) OPEND(SETV_7)

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableCB2" ",@object" LF
LC "OpTableCB2:" NL
//	RELOC OP "RLCV,"   OP "RRCV,"   OP "RLV,"    OP "RRV,"    OP "SLAV,"   OP "SRAV,"   OP "SLLV,"   OP "SRLV"   NL
	RELOC OP "RLCV,"   OP "RRCV,"   OP "RLV,"    OP "RRV,"    OP "SLAV,"   OP "SRAV,"   OP "DEV2,"   OP "SRLV"   NL
	RELOC OP "BITV_0," OP "BITV_1," OP "BITV_2," OP "BITV_3," OP "BITV_4," OP "BITV_5," OP "BITV_6," OP "BITV_7" NL
	RELOC OP "RESV_0," OP "RESV_1," OP "RESV_2," OP "RESV_3," OP "RESV_4," OP "RESV_5," OP "RESV_6," OP "RESV_7" NL
	RELOC OP "SETV_0," OP "SETV_1," OP "SETV_2," OP "SETV_3," OP "SETV_4," OP "SETV_5," OP "SETV_6," OP "SETV_7" NL

	".size " LC "OpTableCB2" ",.-" LC "OpTableCB2" NL
);

#define BRUNCHrcxV \
	"lea r9," PTR64 LC "OpTableCB2" "[rip]" NL \
	"call " PTR64 "[r9+rcx*" sizeofPTR "]" NL
OPFUNC(CB_B) CLK2(8, 2) "mov " dlV "," cpuB NL BRUNCHrcxV "mov " cpuB "," dlV NL OPEND(CB_B) // (4+1,4+1)
OPFUNC(CB_C) CLK2(8, 2) "mov " dlV "," cpuC NL BRUNCHrcxV "mov " cpuC "," dlV NL OPEND(CB_C)
OPFUNC(CB_D) CLK2(8, 2) "mov " dlV "," cpuD NL BRUNCHrcxV "mov " cpuD "," dlV NL OPEND(CB_D)
OPFUNC(CB_E) CLK2(8, 2) "mov " dlV "," cpuE NL BRUNCHrcxV "mov " cpuE "," dlV NL OPEND(CB_E)
OPFUNC(CB_H) CLK2(8, 2) "mov " dlV "," cpuH NL BRUNCHrcxV "mov " cpuH "," dlV NL OPEND(CB_H)
OPFUNC(CB_L) CLK2(8, 2) "mov " dlV "," cpuL NL BRUNCHrcxV "mov " cpuL "," dlV NL OPEND(CB_L)
OPFUNC(CB_A) CLK2(8, 2) "mov " dlV ","  alA NL BRUNCHrcxV "mov "  alA "," dlV NL OPEND(CB_A)

//      rcx
// -----AAO
// AA = addr
//  O = ---00YYY RLC/RRC/...
//      ---XXnnn BIT/RES/SET N
OPFUNC(CB_M)
	"mov r9b," clT NL
	CLK2(12, 3) // (4+1,4+1,4) Fetch(CB xx) + Rd
	"shr e" cxADDR ",8" NL
	"mov dx," cxADDR NL
	"shl e" dxADDR ",16" NL
	"mov dx," cxADDR NL // -- -- AA AA
	READ8dlN_dxADDR
	"movzx rcx,r9b" NL
	BRUNCHrcxV
	"rol edx,16" NL
	CLK0(3, 1) // (,,,3) Wr
	WRITE8d2N_dxADDR
OPEND(CB_M)
#undef BRUNCHrcxV

///////////////////////////////////////////////////////////////
// .... DD/FD prefix ........................

///////////////////////////////////////////////////////////////

__asm__ (
	".section .rodata" NL
	".type " LC "OpTable" ",@object" LF
LC "OpTable:" NL
	RELOC OP "NOP,"      OP "LD_BC_NN," OP "LD_BC_A,"  OP "INC_BC," OP "INC_B," OP "DEC_B," OP "LD_B_N," OP "RLCA" NL
	RELOC OP "EX_AF_AF," OP "ADDHL_BC," OP "LD_A_BC,"  OP "DEC_BC," OP "INC_C," OP "DEC_C," OP "LD_C_N," OP "RRCA" NL
	RELOC OP "DJNZ,"     OP "LD_DE_NN," OP "LD_DE_A,"  OP "INC_DE," OP "INC_D," OP "DEC_D," OP "LD_D_N," OP "RLA"  NL
	RELOC OP "JR,"       OP "ADDHL_DE," OP "LD_A_DE,"  OP "DEC_DE," OP "INC_E," OP "DEC_E," OP "LD_E_N," OP "RRA"  NL
//	RELOC OP "JR_NZ,"    OP "LD_HL_NN," OP "LD_MM_HL," OP "INC_HL," OP "INC_H," OP "DEC_H," OP "LD_H_N," OP "DAA"  NL
	RELOC OP "JR_NZ,"    OP "LD_HL_NN," OP "LD_MM_HL," OP "INC_HL," OP "INC_H," OP "DEC_H," OP "LD_H_N," OP "DEV"  NL
	RELOC OP "JR_Z,"     OP "ADDHL_HL," OP "LD_HL_MM," OP "DEC_HL," OP "INC_L," OP "DEC_L," OP "LD_L_N," OP "CPL"  NL
	RELOC OP "JR_NC,"    OP "LD_SP_NN," OP "LD_MM_A,"  OP "INC_SP," OP "INC_M," OP "DEC_M," OP "LD_M_N," OP "SCF"  NL
	RELOC OP "JR_C,"     OP "ADDHL_SP," OP "LD_A_MM,"  OP "DEC_SP," OP "INC_A," OP "DEC_A," OP "LD_A_N," OP "CCF"  NL
//
	RELOC OP "NOP,"    OP "LD_B_C," OP "LD_B_D," OP "LD_B_E," OP "LD_B_H," OP "LD_B_L," OP "LD_B_M," OP "LD_B_A" NL
	RELOC OP "LD_C_B," OP "NOP,"    OP "LD_C_D," OP "LD_C_E," OP "LD_C_H," OP "LD_C_L," OP "LD_C_M," OP "LD_C_A" NL
	RELOC OP "LD_D_B," OP "LD_D_C," OP "NOP,"    OP "LD_D_E," OP "LD_D_H," OP "LD_D_L," OP "LD_D_M," OP "LD_D_A" NL
	RELOC OP "LD_E_B," OP "LD_E_C," OP "LD_E_D," OP "NOP,"    OP "LD_E_H," OP "LD_E_L," OP "LD_E_M," OP "LD_E_A" NL
	RELOC OP "LD_H_B," OP "LD_H_C," OP "LD_H_D," OP "LD_H_E," OP "NOP,"    OP "LD_H_L," OP "LD_H_M," OP "LD_H_A" NL
	RELOC OP "LD_L_B," OP "LD_L_C," OP "LD_L_D," OP "LD_L_E," OP "LD_L_H," OP "NOP,"    OP "LD_L_M," OP "LD_L_A" NL
	RELOC OP "LD_M_B," OP "LD_M_C," OP "LD_M_D," OP "LD_M_E," OP "LD_M_H," OP "LD_M_L," OP "HALT,"   OP "LD_M_A" NL
	RELOC OP "LD_A_B," OP "LD_A_C," OP "LD_A_D," OP "LD_A_E," OP "LD_A_H," OP "LD_A_L," OP "LD_A_M," OP "NOP"    NL
//
	RELOC OP "ADD_A_B," OP "ADD_A_C," OP "ADD_A_D," OP "ADD_A_E," OP "ADD_A_H," OP "ADD_A_L," OP "ADD_A_M," OP "ADD_A_A" NL
	RELOC OP "ADC_A_B," OP "ADC_A_C," OP "ADC_A_D," OP "ADC_A_E," OP "ADC_A_H," OP "ADC_A_L," OP "ADC_A_M," OP "ADC_A_A" NL
	RELOC OP "SUB_B,"   OP "SUB_C,"   OP "SUB_D,"   OP "SUB_E,"   OP "SUB_H,"   OP "SUB_L,"   OP "SUB_M,"   OP "SUB_A" NL
	RELOC OP "SBC_A_B," OP "SBC_A_C," OP "SBC_A_D," OP "SBC_A_E," OP "SBC_A_H," OP "SBC_A_L," OP "SBC_A_M," OP "SBC_A_A" NL
	RELOC OP "AND_B,"   OP "AND_C,"   OP "AND_D,"   OP "AND_E,"   OP "AND_H,"   OP "AND_L,"   OP "AND_M,"   OP "AND_A" NL
	RELOC OP "XOR_B,"   OP "XOR_C,"   OP "XOR_D,"   OP "XOR_E,"   OP "XOR_H,"   OP "XOR_L,"   OP "XOR_M,"   OP "XOR_A" NL
	RELOC OP "OR_B,"    OP "OR_C,"    OP "OR_D,"    OP "OR_E,"    OP "OR_H,"    OP "OR_L,"    OP "OR_M,"    OP "OR_A" NL
	RELOC OP "CP_B,"    OP "CP_C,"    OP "CP_D,"    OP "CP_E,"    OP "CP_H,"    OP "CP_L,"    OP "CP_M,"    OP "CP_A" NL
//
	RELOC OP "RET_NZ," OP "POP_BC,"   OP "JP_NZ," OP "JP,"       OP "CALL_NZ," OP "PUSH_BC," OP "ADD_A_N," OP "RST00" NL
	RELOC OP "RET_Z,"  OP "RET,"      OP "JP_Z,"  OP "CODE_CB,"  OP "CALL_Z,"  OP "CALL,"    OP "ADC_A_N," OP "RST08" NL
	RELOC OP "RET_NC," OP "POP_DE,"   OP "JP_NC," OP "OUT_N_A,"  OP "CALL_NC," OP "PUSH_DE," OP "SUB_N,"   OP "RST10" NL
	RELOC OP "RET_C,"  OP "EXX,"      OP "JP_C,"  OP "IN_A_N,"   OP "CALL_C,"  OP "CODE_DD," OP "SBC_A_N," OP "RST18" NL
	RELOC OP "RET_PO," OP "POP_HL,"   OP "JP_PO," OP "EX_SP_HL," OP "CALL_PO," OP "PUSH_HL," OP "AND_N,"   OP "RST20" NL
	RELOC OP "RET_PE," OP "JP_HL,"    OP "JP_PE," OP "EX_DE_HL," OP "CALL_PE," OP "CODE_ED," OP "XOR_N,"   OP "RST28" NL
	RELOC OP "RET_P,"  OP "POP_AF,"   OP "JP_P,"  OP "DI,"       OP "CALL_P,"  OP "PUSH_AF," OP "OR_N,"    OP "RST30" NL
	RELOC OP "RET_M,"  OP "LD_SP_HL," OP "JP_M,"  OP "EI,"       OP "CALL_M,"  OP "CODE_FD," OP "CP_N,"    OP "RST38" NL

	".size " LC "OpTable" ",.-" LC "OpTable" NL
);

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableDD" ",@object" LF
LC "OpTableDD:" NL
	RELOC OP "NOP,"      OP "LD_BC_NN," OP "LD_BC_A,"  OP "INC_BC," OP "INC_B,"  OP "DEC_B,"  OP "LD_B_N,"  OP "RLCA" NL
//	RELOC OP "EX_AF_AF," OP "ADDIX_BC," OP "LD_A_BC,"  OP "DEC_BC," OP "INC_C,"  OP "DEC_C,"  OP "LD_C_N,"  OP "RRCA" NL
	RELOC OP "EX_AF_AF," OP "ADDIX_BC," OP "LD_A_BC,"  OP "DEC_BC," OP "INC_C,"  OP "DEC_C,"  OP "LD_C_N,"  OP "RRCA" NL
	RELOC OP "DJNZ,"     OP "LD_DE_NN," OP "LD_DE_A,"  OP "INC_DE," OP "INC_D,"  OP "DEC_D,"  OP "LD_D_N,"  OP "RLA"  NL
//	RELOC OP "JR,"       OP "ADDIX_DE," OP "LD_A_DE,"  OP "DEC_DE," OP "INC_E,"  OP "DEC_E,"  OP "LD_E_N,"  OP "RRA"  NL
	RELOC OP "JR,"       OP "ADDIX_DE," OP "LD_A_DE,"  OP "DEC_DE," OP "INC_E,"  OP "DEC_E,"  OP "LD_E_N,"  OP "RRA"  NL
//	RELOC OP "JR_NZ,"    OP "LD_IX_NN," OP "LD_MM_IX," OP "INC_IX," OP "INC_XH," OP "DEC_XH," OP "LD_XH_N," OP "DAA"  NL
	RELOC OP "JR_NZ,"    OP "LD_IX_NN," OP "LD_MM_IX," OP "INC_IX," OP "DEV2,"   OP "DEV2,"   OP "LD_XH_N," OP "DEV2" NL
//	RELOC OP "JR_Z,"     OP "ADDIX_IX," OP "LD_IX_MM," OP "DEC_IX," OP "INC_XL," OP "DEC_XL," OP "LD_XL_N," OP "CPL"  NL
	RELOC OP "JR_Z,"     OP "ADDIX_IX," OP "LD_IX_MM," OP "DEC_IX," OP "DEV2,"   OP "DEV2,"   OP "LD_XL_N," OP "CPL"  NL
//	RELOC OP "JR_NC,"    OP "LD_SP_NN," OP "LD_MM_A,"  OP "INC_SP," OP "INC_MX," OP "DEC_MX," OP "LD_MX_N," OP "SCF"  NL
	RELOC OP "JR_NC,"    OP "LD_SP_NN," OP "LD_MM_A,"  OP "INC_SP," OP "INC_MX," OP "DEC_MX," OP "LD_MX_N," OP "SCF"  NL
//	RELOC OP "JR_C,"     OP "ADDIX_SP," OP "LD_A_MM,"  OP "DEC_SP," OP "INC_A,"  OP "DEC_A,"  OP "LD_A_N,"  OP "CCF"  NL
	RELOC OP "JR_C,"     OP "ADDIX_SP," OP "LD_A_MM,"  OP "DEC_SP," OP "INC_A,"  OP "DEC_A,"  OP "LD_A_N,"  OP "CCF"  NL

//	RELOC OP "NOP,"     OP "LD_B_C,"  OP "LD_B_D,"  OP "LD_B_E,"  OP "LD_B_XH,"  OP "LD_B_XL,"  OP "LD_B_MX,"  OP "LD_B_A"  NL
	RELOC OP "NOP,"     OP "LD_B_C,"  OP "LD_B_D,"  OP "LD_B_E,"  OP "LD_B_XH,"  OP "LD_B_XL,"  OP "LD_B_MX,"  OP "LD_B_A"  NL
//	RELOC OP "LD_C_B,"  OP "NOP,"     OP "LD_C_D,"  OP "LD_C_E,"  OP "LD_C_XH,"  OP "LD_C_XL,"  OP "LD_C_MX,"  OP "LD_C_A"  NL
	RELOC OP "LD_C_B,"  OP "NOP,"     OP "LD_C_D,"  OP "LD_C_E,"  OP "LD_C_XH,"  OP "LD_C_XL,"  OP "LD_C_MX,"  OP "LD_C_A"  NL
//	RELOC OP "LD_D_B,"  OP "LD_D_C,"  OP "NOP,"     OP "LD_D_E,"  OP "LD_D_XH,"  OP "LD_D_XL,"  OP "LD_D_MX,"  OP "LD_D_A"  NL
	RELOC OP "LD_D_B,"  OP "LD_D_C,"  OP "NOP,"     OP "LD_D_E,"  OP "LD_D_XH,"  OP "LD_D_XL,"  OP "LD_D_MX,"  OP "LD_D_A"  NL
//	RELOC OP "LD_E_B,"  OP "LD_E_C,"  OP "LD_E_D,"  OP "NOP,"     OP "LD_E_XH,"  OP "LD_E_XL,"  OP "LD_E_MX,"  OP "LD_E_A"  NL
	RELOC OP "LD_E_B,"  OP "LD_E_C,"  OP "LD_E_D,"  OP "NOP,"     OP "LD_E_XH,"  OP "LD_E_XL,"  OP "LD_E_MX,"  OP "LD_E_A"  NL
//	RELOC OP "LD_XH_B," OP "LD_XH_C," OP "LD_XH_D," OP "LD_XH_E," OP "NOP,"      OP "LD_XH_XL," OP "LD_H_MX,"  OP "LD_XH_A" NL
	RELOC OP "LD_XH_B," OP "LD_XH_C," OP "LD_XH_D," OP "LD_XH_E," OP "NOP,"      OP "DEV2,"     OP "LD_H_MX,"  OP "LD_XH_A"  NL
//	RELOC OP "LD_XL_B," OP "LD_XL_C," OP "LD_XL_D," OP "LD_XL_E," OP "LD_XL_XH," OP "NOP,"      OP "LD_L_MX,"  OP "LD_XL_A" NL
	RELOC OP "LD_XL_B," OP "LD_XL_C," OP "LD_XL_D," OP "LD_XL_E," OP "DEV2,"     OP "NOP,"      OP "LD_L_MX,"  OP "LD_XL_A"  NL
	RELOC OP "LD_MX_B," OP "LD_MX_C," OP "LD_MX_D," OP "LD_MX_E," OP "LD_MX_H,"  OP "LD_MX_L,"  OP "HALT,"     OP "LD_MX_A" NL
//	RELOC OP "LD_A_B,"  OP "LD_A_C,"  OP "LD_A_D,"  OP "LD_A_E,"  OP "LD_A_XH,"  OP "LD_A_XL,"  OP "LD_A_MX,"  OP "NOP"     NL
	RELOC OP "LD_A_B,"  OP "LD_A_C,"  OP "LD_A_D,"  OP "LD_A_E,"  OP "LD_A_XH,"  OP "LD_A_XL,"  OP "LD_A_MX,"  OP "NOP"     NL

//	RELOC OP "ADD_A_B," OP "ADD_A_C," OP "ADD_A_D," OP "ADD_A_E," OP "ADD_A_XH," OP "ADD_A_XL," OP "ADD_A_MX," OP "ADD_A_A" NL
	RELOC OP "ADD_A_B," OP "ADD_A_C," OP "ADD_A_D," OP "ADD_A_E," OP "DEV2,"     OP "DEV2,"     OP "ADD_A_MX," OP "ADD_A_A" NL
//	RELOC OP "ADC_A_B," OP "ADC_A_C," OP "ADC_A_D," OP "ADC_A_E," OP "ADC_A_XH," OP "ADC_A_XL," OP "ADC_A_MX," OP "ADC_A_A" NL
	RELOC OP "ADC_A_B," OP "ADC_A_C," OP "ADC_A_D," OP "ADC_A_E," OP "DEV2,"     OP "DEV2,"     OP "ADC_A_MX," OP "ADC_A_A" NL
//	RELOC OP "SUB_B,"   OP "SUB_C,"   OP "SUB_D,"   OP "SUB_E,"   OP "SUB_XH,"   OP "SUB_XL,"   OP "SUB_MX,"   OP "SUB_A"   NL
	RELOC OP "SUB_B,"   OP "SUB_C,"   OP "SUB_D,"   OP "SUB_E,"   OP "DEV2,"     OP "DEV2,"     OP "SUB_MX,"   OP "SUB_A"   NL
//	RELOC OP "SBC_A_B," OP "SBC_A_C," OP "SBC_A_D," OP "SBC_A_E," OP "SBC_A_XH," OP "SBC_A_XL," OP "SBC_A_MX," OP "SBC_A_A" NL
	RELOC OP "SBC_A_B," OP "SBC_A_C," OP "SBC_A_D," OP "SBC_A_E," OP "DEV2,"     OP "DEV2,"     OP "SBC_A_MX," OP "SBC_A_A" NL
//	RELOC OP "AND_B,"   OP "AND_C,"   OP "AND_D,"   OP "AND_E,"   OP "AND_XH,"   OP "AND_XL,"   OP "AND_MX,"   OP "AND_A"   NL
	RELOC OP "AND_B,"   OP "AND_C,"   OP "AND_D,"   OP "AND_E,"   OP "DEV2,"     OP "DEV2,"     OP "AND_MX,"   OP "AND_A"   NL
//	RELOC OP "XOR_B,"   OP "XOR_C,"   OP "XOR_D,"   OP "XOR_E,"   OP "XOR_XH,"   OP "XOR_XL,"   OP "XOR_MX,"   OP "XOR_A"   NL
	RELOC OP "XOR_B,"   OP "XOR_C,"   OP "XOR_D,"   OP "XOR_E,"   OP "DEV2,"     OP "DEV2,"     OP "XOR_MX,"   OP "XOR_A"   NL
//	RELOC OP "OR_B,"    OP "OR_C,"    OP "OR_D,"    OP "OR_E,"    OP "OR_XH,"    OP "OR_XL,"    OP "OR_MX,"    OP "OR_A"    NL
	RELOC OP "OR_B,"    OP "OR_C,"    OP "OR_D,"    OP "OR_E,"    OP "DEV2,"     OP "DEV2,"     OP "OR_MX,"    OP "OR_A"    NL
//	RELOC OP "CP_B,"    OP "CP_C,"    OP "CP_D,"    OP "CP_E,"    OP "CP_XH,"    OP "CP_XL,"    OP "CP_MX,"    OP "CP_A"    NL
	RELOC OP "CP_B,"    OP "CP_C,"    OP "CP_D,"    OP "CP_E,"    OP "DEV2,"     OP "DEV2,"     OP "CP_MX,"    OP "CP_A"    NL

	RELOC OP "RET_NZ," OP "POP_BC,"   OP "JP_NZ," OP "JP,"        OP "CALL_NZ," OP "PUSH_BC," OP "ADD_A_N," OP "RST00" NL
//	RELOC OP "RET_Z,"  OP "RET,"      OP "JP_Z,"  OP "CODE_DDCB," OP "CALL_Z,"  OP "CALL,"    OP "ADC_A_N," OP "RST08" NL
	RELOC OP "RET_Z,"  OP "RET,"      OP "JP_Z,"  OP "CODE_DDCB," OP "CALL_Z,"  OP "CALL,"    OP "ADC_A_N," OP "RST08" NL
	RELOC OP "RET_NC," OP "POP_DE,"   OP "JP_NC," OP "OUT_N_A,"   OP "CALL_NC," OP "PUSH_DE," OP "SUB_N,"   OP "RST10" NL
//	RELOC OP "RET_C,"  OP "EXX,"      OP "JP_C,"  OP "IN_A_N,"    OP "CALL_C,"  OP "DEC_PC,"  OP "SBC_A_N," OP "RST18" NL
	RELOC OP "RET_C,"  OP "EXX,"      OP "JP_C,"  OP "IN_A_N,"    OP "CALL_C,"  OP "DEV2,"    OP "SBC_A_N," OP "RST18" NL
//	RELOC OP "RET_PO," OP "POP_IX,"   OP "JP_PO," OP "EX_SP_IX,"  OP "CALL_PO," OP "PUSH_IX," OP "AND_N,"   OP "RST20" NL
	RELOC OP "RET_PO," OP "POP_IX,"   OP "JP_PO," OP "DEV2,"      OP "CALL_PO," OP "PUSH_IX," OP "AND_N,"   OP "RST20" NL
//	RELOC OP "RET_PE," OP "JP_IX,"    OP "JP_PE," OP "EX_DE_HL,"  OP "CALL_PE," OP "DEC_PC,"  OP "XOR_N,"   OP "RST28" NL
	RELOC OP "RET_PE," OP "DEV2,"     OP "JP_PE," OP "EX_DE_HL,"  OP "CALL_PE," OP "DEV2,"    OP "XOR_N,"   OP "RST28" NL
	RELOC OP "RET_P,"  OP "POP_AF,"   OP "JP_P,"  OP "DI,"        OP "CALL_P,"  OP "PUSH_AF," OP "OR_N,"    OP "RST30" NL
//	RELOC OP "RET_M,"  OP "LD_SP_IX," OP "JP_M,"  OP "EI,"        OP "CALL_M,"  OP "DEC_PC,"  OP "CP_N,"    OP "RST38" NL
	RELOC OP "RET_M,"  OP "DEV2,"     OP "JP_M,"  OP "EI,"        OP "CALL_M,"  OP "DEV2,"    OP "CP_N,"    OP "RST38" NL

	".size " LC "OpTableDD" ",.-" LC "OpTableDD" NL
);

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableFD" ",@object" LF
LC "OpTableFD:" NL
	RELOC OP "NOP,"      OP "LD_BC_NN," OP "LD_BC_A,"  OP "INC_BC," OP "INC_B,"  OP "DEC_B,"  OP "LD_B_N,"  OP "RLCA" NL
//	RELOC OP "EX_AF_AF," OP "ADDIY_BC," OP "LD_A_BC,"  OP "DEC_BC," OP "INC_C,"  OP "DEC_C,"  OP "LD_C_N,"  OP "RRCA" NL
	RELOC OP "EX_AF_AF," OP "ADDIY_BC," OP "LD_A_BC,"  OP "DEC_BC," OP "INC_C,"  OP "DEC_C,"  OP "LD_C_N,"  OP "RRCA" NL
	RELOC OP "DJNZ,"     OP "LD_DE_NN," OP "LD_DE_A,"  OP "INC_DE," OP "INC_D,"  OP "DEC_D,"  OP "LD_D_N,"  OP "RLA"  NL
//	RELOC OP "JR,"       OP "ADDIY_DE," OP "LD_A_DE,"  OP "DEC_DE," OP "INC_E,"  OP "DEC_E,"  OP "LD_E_N,"  OP "RRA"  NL
	RELOC OP "JR,"       OP "ADDIY_DE," OP "LD_A_DE,"  OP "DEC_DE," OP "INC_E,"  OP "DEC_E,"  OP "LD_E_N,"  OP "RRA"  NL
//	RELOC OP "JR_NZ,"    OP "LD_IY_NN," OP "LD_MM_IY," OP "INC_IY," OP "INC_YH," OP "DEC_YH," OP "LD_YH_N," OP "DAA"  NL
	RELOC OP "JR_NZ,"    OP "LD_IY_NN," OP "LD_MM_IY," OP "INC_IY," OP "DEV2,"   OP "DEV2,"   OP "LD_YH_N," OP "DEV2" NL
//	RELOC OP "JR_Z,"     OP "ADDIY_IY," OP "LD_IY_MM," OP "DEC_IY," OP "INC_YL," OP "DEC_YL," OP "LD_YL_N," OP "CPL"  NL
	RELOC OP "JR_Z,"     OP "ADDIY_IY," OP "LD_IY_MM," OP "DEC_IY," OP "DEV2,"   OP "DEV2,"   OP "LD_YL_N," OP "CPL"  NL
//	RELOC OP "JR_NC,"    OP "LD_SP_NN," OP "LD_MM_A,"  OP "INC_SP," OP "INC_MY," OP "DEC_MY," OP "LD_MY_N," OP "SCF"  NL
	RELOC OP "JR_NC,"    OP "LD_SP_NN," OP "LD_MM_A,"  OP "INC_SP," OP "INC_MY," OP "DEC_MY," OP "LD_MY_N," OP "SCF"  NL
//	RELOC OP "JR_C,"     OP "ADDIY_SP," OP "LD_A_MM,"  OP "DEC_SP," OP "INC_A,"  OP "DEC_A,"  OP "LD_A_N,"  OP "CCF"  NL
	RELOC OP "JR_C,"     OP "ADDIY_SP," OP "LD_A_MM,"  OP "DEC_SP," OP "INC_A,"  OP "DEC_A,"  OP "LD_A_N,"  OP "CCF"  NL

//	RELOC OP "NOP,"     OP "LD_B_C,"  OP "LD_B_D,"  OP "LD_B_E,"  OP "LD_B_YH,"  OP "LD_B_YL,"  OP "LD_B_MY,"  OP "LD_B_A"  NL
	RELOC OP "NOP,"     OP "LD_B_C,"  OP "LD_B_D,"  OP "LD_B_E,"  OP "LD_B_YH,"  OP "LD_B_YL,"  OP "LD_B_MY,"  OP "LD_B_A"  NL
//	RELOC OP "LD_C_B,"  OP "NOP,"     OP "LD_C_D,"  OP "LD_C_E,"  OP "LD_C_YH,"  OP "LD_C_YL,"  OP "LD_C_MY,"  OP "LD_C_A"  NL
	RELOC OP "LD_C_B,"  OP "NOP,"     OP "LD_C_D,"  OP "LD_C_E,"  OP "LD_C_YH,"  OP "LD_C_YL,"  OP "LD_C_MY,"  OP "LD_C_A"  NL
//	RELOC OP "LD_D_B,"  OP "LD_D_C,"  OP "NOP,"     OP "LD_D_E,"  OP "LD_D_YH,"  OP "LD_D_YL,"  OP "LD_D_MY,"  OP "LD_D_A"  NL
	RELOC OP "LD_D_B,"  OP "LD_D_C,"  OP "NOP,"     OP "LD_D_E,"  OP "LD_D_YH,"  OP "LD_D_YL,"  OP "LD_D_MY,"  OP "LD_D_A"  NL
//	RELOC OP "LD_E_B,"  OP "LD_E_C,"  OP "LD_E_D,"  OP "NOP,"     OP "LD_E_YH,"  OP "LD_E_YL,"  OP "LD_E_MY,"  OP "LD_E_A"  NL
	RELOC OP "LD_E_B,"  OP "LD_E_C,"  OP "LD_E_D,"  OP "NOP,"     OP "LD_E_YH,"  OP "LD_E_YL,"  OP "LD_E_MY,"  OP "LD_E_A"  NL
//	RELOC OP "LD_YH_B," OP "LD_YH_C," OP "LD_YH_D," OP "LD_YH_E," OP "NOP,"      OP "LD_YH_YL," OP "LD_H_MY,"  OP "LD_YH_A" NL
	RELOC OP "LD_YH_B," OP "LD_YH_C," OP "LD_YH_D," OP "LD_YH_E," OP "NOP,"      OP "DEV2,"     OP "LD_H_MY,"  OP "LD_YH_A"  NL
//	RELOC OP "LD_YL_B," OP "LD_YL_C," OP "LD_YL_D," OP "LD_YL_E," OP "LD_YL_YH," OP "NOP,"      OP "LD_L_MY,"  OP "LD_YL_A" NL
	RELOC OP "LD_YL_B," OP "LD_YL_C," OP "LD_YL_D," OP "LD_YL_E," OP "DEV2,"     OP "NOP,"      OP "LD_L_MY,"  OP "LD_YL_A"  NL
	RELOC OP "LD_MY_B," OP "LD_MY_C," OP "LD_MY_D," OP "LD_MY_E," OP "LD_MY_H,"  OP "LD_MY_L,"  OP "HALT,"     OP "LD_MY_A" NL
//	RELOC OP "LD_A_B,"  OP "LD_A_C,"  OP "LD_A_D,"  OP "LD_A_E,"  OP "LD_A_YH,"  OP "LD_A_YL,"  OP "LD_A_MY,"  OP "NOP"     NL
	RELOC OP "LD_A_B,"  OP "LD_A_C,"  OP "LD_A_D,"  OP "LD_A_E,"  OP "LD_A_YH,"  OP "LD_A_YL,"  OP "LD_A_MY,"  OP "NOP"     NL

//	RELOC OP "ADD_A_B," OP "ADD_A_C," OP "ADD_A_D," OP "ADD_A_E," OP "ADD_A_YH," OP "ADD_A_YL," OP "ADD_A_MY," OP "ADD_A_A" NL
	RELOC OP "ADD_A_B," OP "ADD_A_C," OP "ADD_A_D," OP "ADD_A_E," OP "DEV2,"     OP "DEV2,"     OP "ADD_A_MY," OP "ADD_A_A" NL
//	RELOC OP "ADC_A_B," OP "ADC_A_C," OP "ADC_A_D," OP "ADC_A_E," OP "ADC_A_YH," OP "ADC_A_YL," OP "ADC_A_MY," OP "ADC_A_A" NL
	RELOC OP "ADC_A_B," OP "ADC_A_C," OP "ADC_A_D," OP "ADC_A_E," OP "DEV2,"     OP "DEV2,"     OP "ADC_A_MY," OP "ADC_A_A" NL
//	RELOC OP "SUB_B,"   OP "SUB_C,"   OP "SUB_D,"   OP "SUB_E,"   OP "SUB_YH,"   OP "SUB_YL,"   OP "SUB_MY,"   OP "SUB_A"   NL
	RELOC OP "SUB_B,"   OP "SUB_C,"   OP "SUB_D,"   OP "SUB_E,"   OP "DEV2,"     OP "DEV2,"     OP "SUB_MY,"   OP "SUB_A"   NL
//	RELOC OP "SBC_A_B," OP "SBC_A_C," OP "SBC_A_D," OP "SBC_A_E," OP "SBC_A_YH," OP "SBC_A_YL," OP "SBC_A_MY," OP "SBC_A_A" NL
	RELOC OP "SBC_A_B," OP "SBC_A_C," OP "SBC_A_D," OP "SBC_A_E," OP "DEV2,"     OP "DEV2,"     OP "SBC_A_MY," OP "SBC_A_A" NL
//	RELOC OP "AND_B,"   OP "AND_C,"   OP "AND_D,"   OP "AND_E,"   OP "AND_YH,"   OP "AND_YL,"   OP "AND_MY,"   OP "AND_A"   NL
	RELOC OP "AND_B,"   OP "AND_C,"   OP "AND_D,"   OP "AND_E,"   OP "DEV2,"     OP "DEV2,"     OP "AND_MY,"   OP "AND_A"   NL
//	RELOC OP "XOR_B,"   OP "XOR_C,"   OP "XOR_D,"   OP "XOR_E,"   OP "XOR_YH,"   OP "XOR_YL,"   OP "XOR_MY,"   OP "XOR_A"   NL
	RELOC OP "XOR_B,"   OP "XOR_C,"   OP "XOR_D,"   OP "XOR_E,"   OP "DEV2,"     OP "DEV2,"     OP "XOR_MY,"   OP "XOR_A"   NL
//	RELOC OP "OR_B,"    OP "OR_C,"    OP "OR_D,"    OP "OR_E,"    OP "OR_YH,"    OP "OR_YL,"    OP "OR_MY,"    OP "OR_A"    NL
	RELOC OP "OR_B,"    OP "OR_C,"    OP "OR_D,"    OP "OR_E,"    OP "DEV2,"     OP "DEV2,"     OP "OR_MY,"    OP "OR_A"    NL
//	RELOC OP "CP_B,"    OP "CP_C,"    OP "CP_D,"    OP "CP_E,"    OP "CP_YH,"    OP "CP_YL,"    OP "CP_MY,"    OP "CP_A"    NL
	RELOC OP "CP_B,"    OP "CP_C,"    OP "CP_D,"    OP "CP_E,"    OP "DEV2,"     OP "DEV2,"     OP "CP_MY,"    OP "CP_A"    NL

	RELOC OP "RET_NZ," OP "POP_BC,"   OP "JP_NZ," OP "JP,"        OP "CALL_NZ," OP "PUSH_BC," OP "ADD_A_N," OP "RST00" NL
//	RELOC OP "RET_Z,"  OP "RET,"      OP "JP_Z,"  OP "CODE_FDCB," OP "CALL_Z,"  OP "CALL,"    OP "ADC_A_N," OP "RST08" NL
	RELOC OP "RET_Z,"  OP "RET,"      OP "JP_Z,"  OP "CODE_FDCB," OP "CALL_Z,"  OP "CALL,"    OP "ADC_A_N," OP "RST08" NL
	RELOC OP "RET_NC," OP "POP_DE,"   OP "JP_NC," OP "OUT_N_A,"   OP "CALL_NC," OP "PUSH_DE," OP "SUB_N,"   OP "RST10" NL
//	RELOC OP "RET_C,"  OP "EXX,"      OP "JP_C,"  OP "IN_A_N,"    OP "CALL_C,"  OP "DEC_PC,"  OP "SBC_A_N," OP "RST18" NL
	RELOC OP "RET_C,"  OP "EXX,"      OP "JP_C,"  OP "IN_A_N,"    OP "CALL_C,"  OP "DEV2,"    OP "SBC_A_N," OP "RST18" NL
//	RELOC OP "RET_PO," OP "POP_IY,"   OP "JP_PO," OP "EX_SP_IY,"  OP "CALL_PO," OP "PUSH_IY," OP "AND_N,"   OP "RST20" NL
	RELOC OP "RET_PO," OP "POP_IY,"   OP "JP_PO," OP "DEV2,"      OP "CALL_PO," OP "PUSH_IY," OP "AND_N,"   OP "RST20" NL
//	RELOC OP "RET_PE," OP "JP_IY,"    OP "JP_PE," OP "EX_DE_HL,"  OP "CALL_PE," OP "DEC_PC,"  OP "XOR_N,"   OP "RST28" NL
	RELOC OP "RET_PE," OP "DEV2,"     OP "JP_PE," OP "EX_DE_HL,"  OP "CALL_PE," OP "DEV2,"    OP "XOR_N,"   OP "RST28" NL
	RELOC OP "RET_P,"  OP "POP_AF,"   OP "JP_P,"  OP "DI,"        OP "CALL_P,"  OP "PUSH_AF," OP "OR_N,"    OP "RST30" NL
//	RELOC OP "RET_M,"  OP "LD_SP_IY," OP "JP_M,"  OP "EI,"        OP "CALL_M,"  OP "DEC_PC,"  OP "CP_N,"    OP "RST38" NL
	RELOC OP "RET_M,"  OP "DEV2,"     OP "JP_M,"  OP "EI,"        OP "CALL_M,"  OP "DEV2,"    OP "CP_N,"    OP "RST38" NL

	".size " LC "OpTableFD" ",.-" LC "OpTableFD" NL
);

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableED" ",@object" LF
LC "OpTableED:" NL
//	RELOC OP "IN_B_C," OP "OUT_C_B," OP "SBCHL_BC," OP "LD_MM_BC,"  OP "NEG," OP "RETN," OP "IM0," OP "LD_I_A" NL
	RELOC OP "IN_B_C," OP "OUT_C_B," OP "SBCHL_BC," OP "LD_MM_BC,"  OP "NEG," OP "DEV2," OP "IM0," OP "LD_I_A" NL
//	RELOC OP "IN_C_C," OP "OUT_C_C," OP "ADCHL_BC," OP "LD_BC_MM,"  OP "NEG," OP "RETI," OP "IM0," OP "LD_R_A" NL
	RELOC OP "IN_C_C," OP "OUT_C_C," OP "ADCHL_BC," OP "LD_BC_MM,"  OP "NEG," OP "DEV2," OP "IM0," OP "LD_R_A" NL
//	RELOC OP "IN_D_C," OP "OUT_C_D," OP "SBCHL_DE," OP "LD_MM_DE,"  OP "NEG," OP "RETN," OP "IM1," OP "LD_A_I" NL
	RELOC OP "IN_D_C," OP "OUT_C_D," OP "SBCHL_DE," OP "LD_MM_DE,"  OP "NEG," OP "DEV2," OP "IM1," OP "LD_A_I" NL
//	RELOC OP "IN_E_C," OP "OUT_C_E," OP "ADCHL_DE," OP "LD_DE_MM,"  OP "NEG," OP "RETN," OP "IM2," OP "LD_A_R" NL
	RELOC OP "IN_E_C," OP "OUT_C_E," OP "ADCHL_DE," OP "LD_DE_MM,"  OP "NEG," OP "DEV2," OP "IM2," OP "LD_A_R" NL
//	RELOC OP "IN_H_C," OP "OUT_C_H," OP "SBCHL_HL," OP "LD_MM_HL2," OP "NEG," OP "RETN," OP "IM0," OP "RRD"    NL
	RELOC OP "IN_H_C," OP "OUT_C_H," OP "SBCHL_HL," OP "LD_MM_HL2," OP "NEG," OP "DEV2," OP "IM0," OP "RRD"    NL
//	RELOC OP "IN_L_C," OP "OUT_C_L," OP "ADCHL_HL," OP "LD_HL_MM2," OP "NEG," OP "RETN," OP "IM0," OP "RLD"    NL
	RELOC OP "IN_L_C," OP "OUT_C_L," OP "ADCHL_HL," OP "LD_HL_MM2," OP "NEG," OP "DEV2," OP "IM0," OP "RLD"    NL
//	RELOC OP "IN_F_C," OP "OUT_C_Z," OP "SBCHL_SP," OP "LD_MM_SP,"  OP "NEG," OP "RETN," OP "IM1," OP "NOP"    NL
	RELOC OP "DEV2,"   OP "OUT_C_Z," OP "SBCHL_SP," OP "LD_MM_SP,"  OP "NEG," OP "DEV2," OP "IM1," OP "DEV2"   NL
//	RELOC OP "IN_A_C," OP "OUT_C_A," OP "ADCHL_SP," OP "LD_SP_MM,"  OP "NEG," OP "RETN," OP "IM2," OP "NOP"    NL
	RELOC OP "IN_A_C," OP "OUT_C_A," OP "ADCHL_SP," OP "LD_SP_MM,"  OP "NEG," OP "DEV2," OP "IM2," OP "DEV2"   NL

//	RELOC OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "LDI,"  OP "CPI,"  OP "INI,"  OP "OUTI," OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "LDI,"  OP "CPI,"  OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "LDD,"  OP "CPD,"  OP "IND,"  OP "OUTD," OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "LDD,"  OP "CPD,"  OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "LDIR," OP "CPIR," OP "INIR," OP "OTIR," OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "LDIR," OP "CPIR," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL
//	RELOC OP "LDDR," OP "CPDR," OP "INDR," OP "OTDR," OP "NOP,"  OP "NOP,"  OP "NOP,"  OP "NOP" NL
	RELOC OP "LDDR," OP "CPDR," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2," OP "DEV2"NL

	".size " LC "OpTableED" ",.-" LC "OpTableED" NL
);

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableCB1" ",@object" LF
LC "OpTableCB1:" NL
//	RELOC OP "CB_B," OP "CB_C," OP "CB_D," OP "CB_E," OP "CB_H," OP "CB_L," OP "CB_M," OP "CB_A" NL
	RELOC OP "CB_B," OP "CB_C," OP "CB_D," OP "CB_E," OP "CB_H," OP "CB_L," OP "CB_M," OP "CB_A" NL

	".size " LC "OpTableCB1" ",.-" LC "OpTableCB1" NL
);

__asm__ (
	".section .rodata" NL
	".type " LC "OpTableCB1X" ",@object" LF
LC "OpTableCB1X:" NL
//	RELOC OP "CB_MB," OP "CB_MC," OP "CB_MD," OP "CB_ME," OP "CB_MH," OP "CB_ML," OP "CB_M," OP "CB_MA" NL
	RELOC OP "DEV2,"  OP "DEV2,"  OP "DEV2,"  OP "DEV2,"  OP "DEV2,"  OP "DEV2,"  OP "CB_M," OP "DEV2"  NL

	".size " LC "OpTableCB1X" ",.-" LC "OpTableCB1X" NL
);

///////////////////////////////////////////////////////////////
// prefix ..........

OPFUNC(CODE_DD)
	FETCH8rdxV // DD xx
	inc_amR
	CLK1(4, 1) // DD
#if 0 // mMSX // (*)DD DD/FD insert 1 WAIT on MSX1/2/2+
	"movzx  ecx," dlV NL
	"or   " clT ",0x20" NL
	"cmp  " clT ",0xFD" NL
	"setz " clT NL // (DD|FD == dlV) ? 1 : 0
	"add  " bpCLK ",cx" NL
#endif
	"lea rcx," PTR64 LC "OpTableDD" "[rip]" NL
	"jmp " PTR64 "[rcx+rdx*" sizeofPTR "]" NL
OPEND0(CODE_DD)

OPFUNC(CODE_FD)
	FETCH8rdxV // FD xx
	inc_amR
	CLK1(4, 1) // FD
#if 0 // mMSX // (*)FD DD/FD insert 1 WAIT on MSX1/2/2+
	"movzx  ecx," dlV NL
	"or   " clT ",0x20" NL
	"cmp  " clT ",0xFD" NL
	"setz " clT NL
	"add  " bpCLK ",cx" NL
#endif
	"lea rcx," PTR64 LC "OpTableFD" "[rip]" NL
	"jmp " PTR64 "[rcx+rdx*" sizeofPTR "]" NL
OPEND0(CODE_FD)

OPFUNC(CODE_ED)
#define LOCAL LC "CODE_ED"
	FETCH8rdxV
	"sub " rdxUV ",0x40" NL 
	inc_amR
	"cmp " rdxUV ",0x80" NL
	"jnc " LOCAL "_none" NL
	"lea rcx," PTR64 LC "OpTableED" "[rip]" NL
	"jmp " PTR64 "[rcx+rdx*" sizeofPTR "]" LF
LOCAL "_none:" NL
#if 1 // TODO: ..Z80....................(......)
	"jmp " OP "DEV2" NL
#endif
	CLK2LF(8, 2) // (4+1,4+1)
//	FETCH8PGBRK(CODE_ED)
#undef LOCAL
OPEND0(CODE_ED)

OPFUNC(CODE_CB)
#define LOCAL LC "CODE_CB"
	FETCH8rdxV
	inc_amR
	"movzx rcx," dlV NL
	"shr " clT ",3" NL
	"and " dlV ",0x07" NL 
	"and " clT ",0x1F" NL 
	"cmp " dlV ",6" NL // (HL) (-> CB_M)
	"jnz " LOCAL "_reg8" NL
	"ror ecx,8" NL
	"mov " cxST "," cpuHL16 NL
	"rol ecx,8" LF
LOCAL "_reg8:" NL
	"lea r9," PTR64 LC "OpTableCB1" "[rip]" NL
	"jmp " PTR64 "[r9+rdx*" sizeofPTR "]" LF
#undef LOCAL
OPEND0(CODE_CB)

OPFUNC(CODE_DDCB)
	cpuIXrbxRIPto_dxADDR
	"push r" dxADDR NL
	FETCH8rdxV // XX nnn rrr
	"pop rcx" NL
	CLKm1(3+5-4, 2 -1) // (4+1,)(4+1,4+1,4) -> (4+1,4+1,3,5+0,4)
#if 1 // 5+0 ..............4th........................................5......................
	"dec " bpCLK NL // PENDING: ............OP..............(BGM......)........CPU....................
#endif
	"shl e" cxADDR ",8" NL
//	inc_amR // PENDING: (*3)
	"mov cl," dlV NL
	"movzx rdx," dlV NL
	"shr " clT ",3" NL
	"and " dlV ",0x07" NL // --- -- rrr
	"and " clT ",0x1F" NL // --- XX nnn
	"lea r9," PTR64 LC "OpTableCB1X" "[rip]" NL
	"jmp " PTR64 "[r9+rdx*" sizeofPTR "]" LF
OPEND0(CODE_DDCB)

OPFUNC(CODE_FDCB)
	cpuIYrbxRIPto_dxADDR
	"push r" dxADDR NL
	FETCH8rdxV // XX nnn rrr
	"pop rcx" NL
	CLKm1(3+5-4, 2 -1) // (4+1,)(4+1,4+1,4) -> (4+1,4+1,3,5+0,4)
#if 0 // 5+0 ..............4th........................................5......................
	"inc/dec " bpCLK NL
#endif
	"shl e" cxADDR ",8" NL
//	inc_amR // PENDING: (*3)
	"mov cl," dlV NL
	"movzx rdx," dlV NL
	"shr " clT ",3" NL
	"and " dlV ",0x07" NL // --- -- rrr
	"and " clT ",0x1F" NL // --- XX nnn
	"lea r9," PTR64 LC "OpTableCB1X" "[rip]" NL
	"jmp " PTR64 "[r9+rdx*" sizeofPTR "]" LF
OPEND0(CODE_FDCB)

///////////////////////////////////////////////////////////////
// ........

LCFNSTART(O_INTR)
#define LOCAL LC _(O_INTR)
#define dlINT      "dl"
#define clINT_VEC  "cl"
#define dxINT_ADDR "dx"
	// exit from HALT
	"test " cpuSTATE "," _(STATE_HALT) NL
	"jz " LOCAL "_incR_and_DI" NL
	incPC NL
	"and " cpuSTATE ",not " _(STATE_HALT) LF

LOCAL "_incR_and_DI:" NL
	// ++R, shift to DI
	inc_amR
	"and " cpuINT ",not (" _(IFF1) "+" _(IFF2) "+" _(EI_PENDING) ")" NL

	// read DATA bus
	"mov rdx," _(exINT_ACK) NL
	"stc" NL
	"call Bus_In" NL // (-0x30) // clN <- (data bus) (*)vector(MODE2) or Z80 opcode(MODE0)

	// brunch by INT-mode
	"mov dl," cpuINT NL
	"and " dlINT "," _(INT_MODE) NL
	"cmp " dlINT ",1" NL
assertnz(1,
	"nop") // PENDING: MODE1
	"jnc " LOCAL "_mode2" NL
//LOCAL "_mode0:" NL
	CLK1(13, 2) // PENDING: ..................Z80................ (........+2CLK)
	"ret" LF

LOCAL "_mode2:" NL
	"mov dh," cpuI NL
	"mov dl," clINT_VEC NL
	READ16dxNN_dxADDR // calling address by interrupt

	"sub " r8eaPCw "," bxPC NL
	"jnc " LOCAL "_sub16_nc" NL
	"sub " r8eaPC ",0x10000" LF
LOCAL "_sub16_nc:" NL
	"add " r8eaPCw "," dxINT_ADDR NL
	"jnc " LOCAL "_add16_nc" NL
	"add " r8eaPC ",0x10000" LF
LOCAL "_add16_nc:" NL

	"xchg " dxINT_ADDR "," bxPC NL
	PUSHdxNN SP_PUSH  // return address
//	"call SetPCbx" NL // NOTE: not needed (*)must execute at Exec0 after being back from O_INTR
	CLK1(19, 4)       // (7,3,3,6) vector fetch (data bus) + PC push + PC load
#undef LOCAL
#undef dlINT
#undef clINT_VEC
#undef dxINT_ADDR
LCFNEND(O_INTR)

///////////////////////////////////////////////////////////////
// ................
// ................

///////////////////////////////////////////////////////////////
// ........

#ifndef Z80C_GOAL
/* ............
   rbx      (........) r8eaPC
   rsi      (........) rsiCPU
   (others) (........) OpTable..........
 */
FNSTART(Exec0)
#define LOCAL LC _(Exec0)
	"push rbp" NL               // (-0x10)    (........) bpCLK
	".cfi_def_cfa_offset 16" NL
	".cfi_offset rbp, -16" NL
	"mov rbp,rsp" NL
	".cfi_def_cfa_register rbp" NL
	"sub  rsp,16" NL            // (-0x20)
#define rspARG "[rsp]"
#define rspITEMS    PTR64 rspARG "[0]"
#if 1 // TODO: ..............................
# define   rdiITEMS_ARG1  "rdi"
# define    raxRIP0       "rax"
# define    raxRIP0_NEG   "rax"
#endif
#define      rsiCPU_ARG2  "rsi"
#define  dxGOAL_CLK        "dx"
// abort handler
	"mov " cpuEHRSP ",rbp" NL

// .goal_clk = min(.goal_clk, 65535 - MOST_HEAVY_CLK)
	"mov dx," cpuGOAL_CLK NL
	"cmp " dxGOAL_CLK ",65535 -" _(MOST_HEAVY_CLK) NL
	"jc " LOCAL "_specified_clk_not_fixed" NL
	"mov " dxGOAL_CLK ",65535 -" _(MOST_HEAVY_CLK) LF
LOCAL "_specified_clk_not_fixed:" NL
	"mov " cpuGOAL_CLK "," dxGOAL_CLK NL

	"mov " rspITEMS    ","   rdiITEMS_ARG1 NL
//	"mov " rsiCPU "," rsiCPU_ARG2  NL

	".cfi_def_cfa_register rsp" NL
	".cfi_def_cfa_offset 24" NL
#if 1 // TODO: ................
	"mov " cpuERRNO ",0" NL // ..............
#endif
#if 1
	"movzx rbx," cpuPC16 NL // BANKBEGIN=BANKEND=0 (*)SetPCbx .......... r8eaPC bv bw ......
#elif 0
	"mov bx,0x7FFF" NL // BANKBEGIN=0000 BANKEND=7FFF
	"shl ebx,16" NL
	"mov bx," cpuPC16 NL
	"mov dx," bxPC NL
	"call " LC "cpuLEA_R_dxADDR" NL
#else
	"mov dx," cpuPC16 NL
	"call " cachePRE_LEA_R_dxADDR NL
assertnc(1,
	"nop") /* 90 */
	"mov bx,cx" NL
	"and bx,0x00FC" NL
	"shl ebx,10 -2 +16" NL // BANKBEGIN // TODO: 10 -6 ......
	"mov bh,ch" NL
	"and bx,0xFC00" NL
//	"shl bx,10 -10" NL
	"dec bx" NL // BANKEND
	"shl rbx,16" NL
	"mov bx," dxPC NL
	"shr ecx,16 -8" NL
	"and rcx," _(ofsMASK) NL
	"add cx," dxPC NL
	"jnc " LOCAL "_nc" NL
	"add ecx,0x10000" NL
LOCAL "_nc:" NL
	"add rcx," memory_item(rdiITEMS) NL
#endif
	"mov " r8 "," rcxRIP NL

// CLOCK
	"xor " bpCLK "," bpCLK NL // PENDING: _loop .......... 65535 .............. b31-b16 ................
	                          //          ............ e[a-d]x .................. b15-b8 ................(....: PC88 ..1..........432................)
	                          //          ................ bpCLK ........(................ CLK ........ abort_XXX .... jmp ......................)
	movzx_to_cpu(cpuOK_CLK, bpCLK) LF

LOCAL "_loop:" NL
//--- TRACE#log-OPCODE.streaming .
#ifdef FULLTRACE_ENABLED
# if 1 // for gdb
	"sub " r10FTRACE "," ftrace_item(rdiITEMS) NL
	"cmp " r10FTRACE ",0xC6B" NL
	"jnz " LOCAL "_dbg" NL
	"nop" NL
LOCAL "_dbg:" NL
	"add " r10FTRACE "," ftrace_item(rdiITEMS) NL
# endif
	"sub rsp,8" NL
	"push " r10FTRACE NL          // (-0x30)
	"push " r8eaPC NL
	"push " rsiCPU NL             // (-0x40)
	"push " rdiITEMS NL
	"push r" bpCLK NL             // (-0x50)
	"push " rbx__MNMXPC NL
	"push " rax____R_FA NL        // (-0x60)
	"mov dx," bpCLK NL
	"mov rcx," r10FTRACE NL
	"call " _(ftrace_onOPCODE) NL // (-0x70)
	"test eax,1" NL
	"pop " rax____R_FA NL
	"pop " rbx__MNMXPC NL
	"pop r" bpCLK NL
	"pop " rdiITEMS NL
	"pop " rsiCPU NL
	"pop " r8eaPC NL
	"pop " r10FTRACE NL
	"jz " LOCAL "_not_new_block" NL
	FULLTRACE_debug_broken
	"mov " r10FTRACE "," ftrace_item(rdiITEMS) NL
	"mov " PTR16 "[" r10FTRACE "]," bxPC NL
	"add " r10FTRACE ",2" NL
	FULLTRACE_debug_backup
LOCAL "_not_new_block:" NL
	"add rsp,8" NL
#endif
	mov_amR7_cpuR7
#define dlINT      "dl"
#define dlINT_NOT  "dl"
// INT signal handling or pending (IM 0 / IM 2)
	"mov dl," cpuINT NL
	"not " dlINT NL
	"test " dlINT_NOT ",(" _(IFF1) "+" _(INT_PENDING) ")" NL
	"not " dlINT NL
	"jnz " LOCAL "_int_not_accepted" NL
	// if (IFF1 && INT_PENDING)
	"call " LC "O_INTR" NL // (-0x28)
	"mov dl," cpuINT NL
	movzx_to_cpu(cpuOK_CLK, bpCLK) LF
	// TODO: .... Req/Ack ............................................ (..........)
LOCAL "_int_not_accepted:" NL

// EI_PENDING -> IFF1/IFF2
	"test " dlINT "," _(EI_PENDING) NL
	"jz " LOCAL "_ei_not_pending" NL
	"and " dlINT ",not " _(EI_PENDING) NL
	"or " dlINT ",(" _(IFF1) "+" _(IFF2) ")" NL
	"mov " cpuINT "," dlINT LF
LOCAL "_ei_not_pending:" NL
#undef dlINT
#undef dlINT_NOT

// 1st fetch <- [PC]
	"call " LC "SetPCbx" NL // (-0x28)
#if 1 // TODO: GVRAM/TVRAM(CRTC DMA..)........................
//	"inc " bpCLK NL // ..............(*)FH/MH............................
#endif
	"movzx rdx," PTR8 "[" r8eaPC "]" NL
	"inc " r8eaPC NL
	"inc " bxPC NL

// A, F, R
//#define mov_axFAcpu__X_X____cpuFA
	"mov " aFA "," cpuFA NL
	"mov " cpu__X_X___ "," ahF NL
	"lea rcx," PTR64 LC "OpTable" "[rip]" NL
	"mov rdi," rspITEMS NL
	inc_amR

	"call " PTR64 "[rcx+rdx*" sizeofPTR "]" NL // (-0x28)

assertz(4,
	"cmp rdi," rspITEMS) /* 48 3B 3C 24 */
	movzx_to_cpu(cpuPC, bxPC) NL
	movzx_to_cpu(cpuOK_CLK, bpCLK) NL
	mov_cpuR7_amR7
//#define mov_cpuFA_axFAcpu__X_X____
	"xor " ahF "," cpu__X_X___ NL
	"and " ahF ",not " __X_X___ NL
	"xor " ahF "," cpu__X_X___ NL
	"mov " cpuFA "," aFA NL

	"cmp " bpCLK "," cpuGOAL_CLK NL
	"jc " LOCAL "_loop" LF
//	"mov " cpuERRNO ",0" NL // safety ..............

// rsiCPU ....................................
LC "abort_rsiCPU:" NL // TODO: ............PC................................
	"mov " PTR8 cpuERRAUX "[3],0" LF
LC "abort_rsiCPU2:" NL // TODO: ..Z80....................(......)
	movzx_from_cpu("ax", cpuOK_CLK)
	"mov rbp," cpuEHRSP NL
	".cfi_def_cfa_register rbp" NL
	".cfi_def_cfa_offset 16" NL
	"leave" NL
	".cfi_def_cfa_register rsp" NL
	".cfi_def_cfa_offset 8" NL
	".cfi_restore rbp" NL
	"ret" LF // (..)...... arg1, arg2, arg3 ................

LOCAL "_cannot_read_PC:" NL
	"mov " cpuERRNO "," _(EFAULT) NL
	"mov " PTR8 cpuERRAUX "[0],1+" _(EFAULT_RAM) NL
	"mov " PTR16 cpuERRAUX "[1]," bxPC NL
	"jmp " LC "abort_rsiCPU" NL

#undef LOCAL
#undef rspARG
#undef rspITEMS
#undef  dxGOAL_CLK
#undef      rsiCPU_ARG2
#if 1 // TODO: ..............................
# undef   rdiITEMS_ARG1
# undef    raxRIP0
# undef    raxRIP0_NEG
#endif
FNEND0(Exec0)
#else //def Z80C_GOAL
LCFNSTART(abort_rsiCPU)
	// TODO: FAKE
LCFNEND(abort_rsiCPU)
LCFNSTART(abort_rsiCPU2)
	// TODO: FAKE
LCFNEND(abort_rsiCPU2)
#endif

///////////////////////////////////////////////////////////////
// inturrupt control

OPFUNC(DI)
	CLK1(4, 1)
	"and " cpuINT ",not (" _(IFF1) "+" _(IFF2) "+" _(EI_PENDING) ")" NL
OPEND(DI)

OPFUNC(EI)
	CLK1(4, 1)
	"or " cpuINT "," _(EI_PENDING) NL // ................
OPEND(EI)

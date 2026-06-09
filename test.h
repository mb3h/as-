#ifndef TEST_H_INCLUDED__
#define TEST_H_INCLUDED__

#ifdef DECL_SHARED_ASM
__asm__ (
	".equ " _(MOST_HEAVY_CLK) ", 23+432" NL
);
#endif //def DECL_SHARED_ASM
#define      MOST_HEAVY_CLK    (23+432) // (72 * 6) N88-V1S/N80/N GVRAM !VSYNC OUT(40h).b4=0
//#define      MOST_HEAVY_CLK    (23+ 30) // ( 5 * 6) N88-V2/V1H
//#define      MOST_HEAVY_CLK    (23+  2) // ( 1 * 2) MSX
                                        // (4,4,3,4,3,5) EX (SP),IX
                                        // (4,4,3,5,4,3) INC/RLC/SET (IX+d)
#define      PC88CLK           4000000U

typedef struct {
	void *ptr;
	unsigned len;
#ifndef __i386__
	uint8_t pad12[4];
#endif
} Z80_ITEM;

#ifdef DECL_SHARED_ASM
__asm__ (
	".struct 0" LF
M "item_ptr:" NL
	".struct " M "item_ptr +" sizeofPTR LF
M "item_len:" NL
	".struct " M "item_len +4" LF
# ifndef __i386__
M "item_pad12:" NL
	".struct " M "item_pad12 +4" LF
# endif
_(sizeofITEM) ":" NL
);
#endif //def DECL_SHARED_ASM

#ifdef DECL_SHARED_ASM
__asm__ (
	".equ " _(iCPU ) ", 0" NL
	".equ " _(iMEM ) ", 1" NL
	".equ " _(iIO  ) ", 2" NL
	".equ " _(iSCHE) ", 3" NL
	".equ " _(iPC88) ", 4" NL
	".equ " _(iCRTC) ", 5" NL
	".equ " _(iFDC ) ", 6" NL
	".equ " _(iOPN ) ", 7" NL
	".equ " _(iKROM) ", 8" NL
	".equ " _(iUI  ) ", 9" NL
	".equ " _(iPAL ) ", 10" NL
);
#endif //def DECL_SHARED_ASM
#define      iCPU      0
#define      iMEM      1
#define      iIO       2
#define      iSCHE     3
#define      iPC88     4
#define      iCRTC     5
#define      iFDC      6
#define      iOPN      7
#define      iKROM     8
#define      iUI       9
#define      iPAL      10
#define      iMAX      10

///////////////////////////////////////////////////////////////
// iIO(#2)

#ifdef DECL_SHARED_ASM
__asm__ (
	".struct 0" LF
M "cb_lpfn:" NL
	".struct " M "cb_lpfn +" sizeofPTR LF
M "cb_this:" NL
	".struct " M "cb_this +" sizeofPTR LF
_(sizeofCBINFO) ":" NL
	".equ " _(sizeofCBINFOdiv2) ", " _(sizeofCBINFO) " / 2" NL
);
#endif
typedef uint8_t __attribute__((stdcall)) (*IOCTL_CB)(
	  void *rdiAUX_
	, void *rsiCPU_
	, uint16_t dxOK_CLK
	, uint32_t ecxTPN // uint24_t
	);
typedef struct {
	IOCTL_CB lpfn;
	void *this_;
} CBINFO;

#ifdef DECL_SHARED_ASM
__asm__ (
	".struct 0" LF
M "in_last:" LF
M "in_aux:" LF
M "in_fixed:" NL
	".struct " M "in_aux +1" LF
M "in_type:" NL
	".struct " M "in_type +1" LF
_(sizeofINTYPE) ":" NL
);
#endif
typedef struct {
	union {
		uint8_t last;
		uint8_t fixed;
		uint8_t iCB;
	};
	uint8_t type;
} INTYPE;

#ifdef DECL_SHARED_ASM
__asm__ (
	".struct 0" LF
M "out_aux:" LF
M "out_last:" NL
	".struct " M "out_aux +1" LF
M "out_type:" NL
	".struct " M "out_type +1" LF
_(sizeofOUTTYPE) ":" NL
);
#endif
typedef struct {
	union {
		uint8_t last;
		uint8_t iCB;
	};
	uint8_t type;
} OUTTYPE;

#ifdef DECL_SHARED_ASM
__asm__ (
	".equ " _(CB_MAX)         ", 64" NL
	".equ " _(PORT_MAX)       ", 256" NL
	".equ " _(EX_PORT_IN_MAX) ", 1" NL
);
__asm__ (
	".struct 0" LF
M "cb_infos:" NL
	".struct " M "cb_infos +" _(CB_MAX) " * " _(sizeofCBINFO) LF
M "in_types:" NL
	".struct " M "in_types +" _(PORT_MAX) " * " _(sizeofINTYPE) LF
M "out_types:" NL
	".struct " M "out_types +" _(PORT_MAX) " * " _(sizeofOUTTYPE) LF
M "ex_in_types:" NL
	".struct " M "ex_in_types +" _(EX_PORT_IN_MAX) " * " _(sizeofINTYPE) LF
M "pad_ioctl:" NL
	".struct " M "pad_ioctl +5" LF
M "out_5x:" NL
	".struct " M "out_5x +1" LF
_(IOCTL_LEN) ":" NL
);
#endif
#define      CB_MAX             64
#define      PORT_MAX           256
#define      EX_PORT_IN_MAX     1
typedef struct {
	 CBINFO    cb_infos[        CB_MAX];
	 INTYPE    in_types[      PORT_MAX];
	OUTTYPE   out_types[      PORT_MAX];
	 INTYPE ex_in_types[EX_PORT_IN_MAX];
	uint8_t pad1[2];
	uint8_t aux[3]; // for ALUR (works on CPU1 only)
	uint8_t out_5x_last :2;
	uint8_t pad2        :6;
} IOCTL;

#ifdef DECL_SHARED_ASM
__asm__ (
	".equ " _(exINT_ACK) ",0x00" NL
);
#endif
#define      exINT_ACK    0x00

///////////////////////////////////////////////////////////////
// iCPU(#0)

typedef struct {
	uint8_t no;
	uint8_t aux[3];
	uint8_t pad9[4];
	const void *ptr;
} Z80_ERR;

#ifdef DECL_SHARED_ASM
__asm__ (
	".equ " _(   INT_MODE) ",0x03" NL
	".equ " _(       IFF1) ",0x04" NL
	".equ " _(       IFF2) ",0x08" NL
	".equ " _(INT_PENDING) ",0x40" NL
	".equ " _( EI_PENDING) ",0x80" NL
);
#endif //def DECL_SHARED_ASM
#define         INT_MODE    0x03
#define             IFF1    0x04
#define             IFF2    0x08
#define      INT_PENDING    0x40
#define       EI_PENDING    0x80

typedef struct {
	uint8_t inner1[0x16];
	uint16_t pc;
	uint8_t inner2[0x1B -0x18];
	uint8_t int_control;
	uint8_t inner3[0x20 -0x1C];
	Z80_ERR err;
	uint8_t inner4[0x40 -0x30];
	IOCTL *ioctl;
	uint8_t inner5[0x78 -0x48];
#ifdef FULLTRACE_ENABLED
	uint32_t opcode_trace_chunked;
	uint8_t inner6[0x80 -0x7C];
#endif
} Z80_x86_outer;
void __attribute__((stdcall)) z80_abortC_thunk (uint8_t errno_, void *cpu_);

#define evidFDC_WAIT  0
#define evidFDD_HEAD  1
#define evidVRTC      2
#define evidCLOCK     3
#define evidOPN_TIMER 4
#define SCHEDULE_EVID_MAX 5
typedef struct {
	uint64_t total_clk; // 0=(empty) PENDING: rap around (*)48bits = 776.72days [4MHz]
	uint16_t aux;      // +08 u10:FDC.seek
	uint8_t evid   :3; // +0A
	uint8_t isCPU1 :1;
	uint8_t pad1   :4;
	uint8_t pad2[5];   // +0B
} SCHEDULE_ITEM;      // +10

#define SCHEDULE_MAX 4 // FDC, VRTC, CLOCK, OPN
typedef struct {
	uint32_t (__attribute__((stdcall)) *handlers[SCHEDULE_EVID_MAX])(Z80_ITEM *items, void *cpu, uint64_t now_clk, uint16_t aux);
	SCHEDULE_ITEM events[SCHEDULE_MAX];
} SCHEDULE;

void schedule_register (void *sche_
	, uint8_t evid
	, uint32_t (__attribute__((stdcall)) *handler)(Z80_ITEM *items, void *cpu, uint64_t now_clk, uint16_t aux)
);
uint64_t schedule_get_next_time (void *sche_, uint8_t evid);
void schedule_remove (void *sche_, uint8_t evid);
uint8_t schedule_insert (
	  Z80_ITEM *items
	, uint8_t evid
	, uint32_t offset_clk // u23:FDC u31:VRTC u13:CLOCK
	, uint16_t aux // u10:FDC.seek
);
#define NO_AUX 0

typedef struct tagPIO {
	struct tagPIO *peer;
	uint8_t recvmask[3];
	uint8_t     send[3];
	uint8_t      pad[2];
} PIO;

typedef struct {
	uint64_t total_clk_offset; // (*)uint48_t = 776.72 days (4MHz)
	uint16_t cpu1_clk, cpu2_clk; // +008,2   +00A,2
	uint8_t pio_idle_count;      // +00C,1
	struct {                     // +00D,1
		uint8_t out1_pending :1;
		uint8_t cpu2_suspend :1;
		uint8_t cpu1_onExec0 :1;
		uint8_t bINT1        :1;  // scheduler has VRTC event
		uint8_t bINT2        :1;  // scheduler has CLOCK event
		uint8_t pad0D_b5     :3;
	};
	uint8_t intc_pending;        // +00E,1
	struct {                     // +00F,1
		uint8_t bMUTE_PCM    :1;  // don't send audio pcm to device(SDL)
		uint8_t bFASTEST_Z80 :1;  // enable for running fastest
		                          // (with allowing frame-skip)
		uint8_t bTRACE_LOG   :1;  // log each behavior of Z80 code
		uint8_t pad0F_b3     :5;
	};
	PIO pio1, pio2;              // +010,16  +020,16
} PC88;

#endif //ndef TEST_H_INCLUDED__

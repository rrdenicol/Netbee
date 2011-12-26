/* -----------------------------------------------------------------------------
 * opcmap.c
 *
 * Copyright (c) 2006, Vivek Mohan <vivek@sig9.com>
 * All rights reserved. See LICENSE
 * -----------------------------------------------------------------------------
 */

#include "dis_types.h"
#include "mnemonics.h"
#include "opcmap.h"
#include "input.h"

/* operand types - check out the intel/amd manuals */
#define Ap	{ OP_A, SZ_P 	}
#define E	{ OP_E, 0 	}
#define Eb	{ OP_E, SZ_B 	}
#define Ew	{ OP_E, SZ_W 	}
#define Ev	{ OP_E, SZ_V 	}
#define Ed	{ OP_E, SZ_D 	}
#define Ez	{ OP_E, SZ_Z 	}
#define Ex	{ OP_E, SZ_MDQ 	}
#define Ep	{ OP_E, SZ_P 	}
#define G	{ OP_G, 0 	}
#define Gb	{ OP_G, SZ_B 	}
#define Gw	{ OP_G, SZ_W 	}
#define Gv	{ OP_G, SZ_V 	}
#define Gvw	{ OP_G, SZ_MDQ 	}
#define Gd	{ OP_G, SZ_D 	}
#define Gx	{ OP_G, SZ_MDQ 	}
#define Gz	{ OP_G, SZ_Z 	}
#define M	{ OP_M, 0 	}
#define Mb	{ OP_M, SZ_B 	}
#define Mw	{ OP_M, SZ_W 	}
#define Ms	{ OP_M, SZ_W 	}
#define Md	{ OP_M, SZ_D 	}
#define Mq	{ OP_M, SZ_Q 	}
#define	I1	{ OP_I1, 0 	}
#define	I3	{ OP_I3, 0 	}
#define Ib	{ OP_I, SZ_B 	}
#define Iw	{ OP_I, SZ_W	}
#define Iv	{ OP_I, SZ_V	}
#define Iz	{ OP_I, SZ_Z	}
#define Jv	{ OP_J, SZ_V	}
#define Jz	{ OP_J, SZ_Z	}
#define Jb	{ OP_J, SZ_B	}
#define R	{ OP_R, SZ_RDQ	}
#define C	{ OP_C, 0	} 
#define D	{ OP_D, 0	}
#define S	{ OP_S, 0	}
#define Ob	{ OP_O, SZ_B	}
#define Ow	{ OP_O, SZ_W	}
#define Ov	{ OP_O, SZ_V	}
#define V	{ OP_V, 0	}
#define W	{ OP_W, 0	}
#define P	{ OP_P, 0	}
#define Q	{ OP_Q, 0	}
#define VR	{ OP_VR, 0	}
#define PR	{ OP_PR, 0	}
#define AL	{ OP_AL, 0	}
#define CL	{ OP_CL, 0	}
#define DL	{ OP_DL, 0	}
#define BL	{ OP_BL, 0	}
#define AH	{ OP_AH, 0	}
#define CH	{ OP_CH, 0	}
#define DH	{ OP_DH, 0	}
#define BH	{ OP_BH, 0	}
#define AX	{ OP_AX, 0	}
#define CX	{ OP_CX, 0	}
#define DX	{ OP_DX, 0	}
#define BX	{ OP_BX, 0	}
#define SI	{ OP_SI, 0	}
#define DI	{ OP_DI, 0	}
#define SP	{ OP_SP, 0	}
#define BP	{ OP_BP, 0	}
#define eAX	{ OP_eAX, 0	}
#define eCX	{ OP_eCX, 0	}
#define eDX	{ OP_eDX, 0	}
#define eBX	{ OP_eBX, 0	}
#define eSI	{ OP_eSI, 0	}
#define eDI	{ OP_eDI, 0	}
#define eSP	{ OP_eSP, 0	}
#define eBP	{ OP_eBP, 0	}
#define rAX	{ OP_rAX, 0	}
#define rCX	{ OP_rCX, 0	}
#define rBX	{ OP_rDX, 0	}
#define rDX	{ OP_rDX, 0	}
#define rSI	{ OP_rSI, 0	}
#define rDI	{ OP_rDI, 0	}
#define rSP	{ OP_rSP, 0	}
#define rBP	{ OP_rBP, 0	}
#define ES	{ OP_ES, 0	}
#define CS	{ OP_CS, 0	}
#define DS	{ OP_DS, 0	}
#define SS	{ OP_SS, 0	}
#define GS	{ OP_GS, 0	}
#define FS	{ OP_FS, 0	}
#define ST0	{ OP_ST0, 0	}
#define ST1	{ OP_ST1, 0	}
#define ST2	{ OP_ST2, 0	}
#define ST3	{ OP_ST3, 0	}
#define ST4	{ OP_ST4, 0	}
#define ST5	{ OP_ST5, 0	}
#define ST6	{ OP_ST6, 0	}
#define ST7	{ OP_ST7, 0	}
#define NOARG	{ 0, 0 		}
#define ALr8b	{ OP_ALr8b,0 	}
#define CLr9b	{ OP_CLr9b,0 	}
#define DLr10b	{ OP_DLr10b,0 	}
#define BLr11b	{ OP_BLr11b,0 	}
#define AHr12b	{ OP_AHr12b,0 	}
#define CHr13b	{ OP_CHr13b,0 	}
#define DHr14b	{ OP_DHr14b,0 	}
#define BHr15b	{ OP_BHr15b,0 	}
#define rAXr8	{ OP_rAXr8,0 	}
#define rCXr9	{ OP_rCXr9,0 	}
#define rDXr10	{ OP_rDXr10,0 	}
#define rBXr11	{ OP_rBXr11,0 	}
#define rSPr12	{ OP_rSPr12,0 	}
#define rBPr13	{ OP_rBPr13,0 	}
#define rSIr14	{ OP_rSIr14,0 	}
#define rDIr15	{ OP_rDIr15,0 	}

/* 1 byte opcode */
struct map_entry itab_1byte[0x100] = 
{
	/* Instruction, op1, op2, op3, Valid Prefixes */

/* 00 */ { UD_Iadd,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B) },
/* 01 */ { UD_Iadd,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 02 */ { UD_Iadd,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B) },
/* 03 */ { UD_Iadd,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 04 */ { UD_Iadd,	AL,	Ib,	NOARG,	Pnone },
/* 05 */ { UD_Iadd,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 06 */ { UD_Ipush,	ES,	NOARG,	NOARG,	Pnone },
/* 07 */ { UD_Ipop,	ES,	NOARG,	NOARG,	Pnone },
/* 08 */ { UD_Ior,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B) },
/* 09 */ { UD_Ior,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 0A */ { UD_Ior,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)},
/* 0B */ { UD_Ior,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 0C */ { UD_Ior,	AL,	Ib,	NOARG,	Pnone },
/* 0D */ { UD_Ior,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 0E */ { UD_Ipush,	CS,	NOARG,	NOARG,	Pinv64 },
/* 0F */ { UD_Iesc,	NOARG,	NOARG,	NOARG,	Pnone },
/* 10 */ { UD_Iadc,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 11 */ { UD_Iadc,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 12 */ { UD_Iadc,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 13 */ { UD_Iadc,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 14 */ { UD_Iadc,	AL,	Ib,	NOARG,	Pnone },
/* 15 */ { UD_Iadc,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 16 */ { UD_Ipush,	SS,	NOARG,	NOARG,	Pnone | Pinv64 },
/* 17 */ { UD_Ipop,	SS,	NOARG,	NOARG,	Pnone | Pinv64 },
/* 18 */ { UD_Isbb,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 19 */ { UD_Isbb,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 1A */ { UD_Isbb,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 1B */ { UD_Isbb,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 1C */ { UD_Isbb,	AL,	Ib,	NOARG,	Pnone },
/* 1D */ { UD_Isbb,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 1E */ { UD_Ipush,	DS,	NOARG,	NOARG,	Pinv64 },
/* 1F */ { UD_Ipop,	DS,	NOARG,	NOARG,	Pinv64 },
/* 20 */ { UD_Iand,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 21 */ { UD_Iand,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 22 */ { UD_Iand,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 23 */ { UD_Iand,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 24 */ { UD_Iand,	AL,	Ib,	NOARG,	Pnone },
/* 25 */ { UD_Iand,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 26 */ { UD_Ies,	ES,	NOARG,	NOARG,	Pnone },
/* 27 */ { UD_Idaa,	NOARG,	NOARG,	NOARG,	Pnone | Pinv64 },
/* 28 */ { UD_Isub,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 29 */ { UD_Isub,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 2A */ { UD_Isub,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 2B */ { UD_Isub,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 2C */ { UD_Isub,	AL,	Ib,	NOARG,	Pnone },
/* 2D */ { UD_Isub,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 2E */ { UD_Ics,	CS,	NOARG,	NOARG,	Pnone },
/* 2F */ { UD_Idas,	NOARG,	NOARG,	NOARG,	Po32 | Pinv64 },
/* 30 */ { UD_Ixor,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 31 */ { UD_Ixor,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 32 */ { UD_Ixor,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 33 */ { UD_Ixor,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 34 */ { UD_Ixor,	AL,	Ib,	NOARG,	Pnone },
/* 35 */ { UD_Ixor,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 36 */ { UD_Iss,	ES,	NOARG,	NOARG,	Pinv64 },
/* 37 */ { UD_Iaaa,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 38 */ { UD_Icmp,	Eb,	Gb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 39 */ { UD_Icmp,	Ev,	Gv,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 3A */ { UD_Icmp,	Gb,	Eb,	NOARG,	Pa32 | REX(_X|_B)  },
/* 3B */ { UD_Icmp,	Gv,	Ev,	NOARG,	Pa32 | Po32 | REX(_W|_R|_X|_B) },
/* 3C */ { UD_Icmp,	AL,	Ib,	NOARG,	Pnone },
/* 3D */ { UD_Icmp,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* 3E */ { UD_Ids,	ES,	NOARG,	NOARG,	Pnone },
/* 3F */ { UD_Iaas,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 40 */ { UD_Iinc,	eAX,	NOARG,	NOARG,	Po32 },
/* 41 */ { UD_Iinc,	eCX,	NOARG,	NOARG,	Po32 },
/* 42 */ { UD_Iinc,	eDX,	NOARG,	NOARG,	Po32 },
/* 43 */ { UD_Iinc,	eBX,	NOARG,	NOARG,	Po32 },
/* 44 */ { UD_Iinc,	eSP,	NOARG,	NOARG,	Po32 },
/* 45 */ { UD_Iinc,	eBP,	NOARG,	NOARG,	Po32 },
/* 46 */ { UD_Iinc,	eSI,	NOARG,	NOARG,	Po32 },
/* 47 */ { UD_Iinc,	eDI,	NOARG,	NOARG,	Po32 },
/* 48 */ { UD_Idec,	eAX,	NOARG,	NOARG,	Po32 },
/* 49 */ { UD_Idec,	eCX,	NOARG,	NOARG,	Po32 },
/* 4A */ { UD_Idec,	eDX,	NOARG,	NOARG,	Po32 },
/* 4B */ { UD_Idec,	eBX,	NOARG,	NOARG,	Po32 },
/* 4C */ { UD_Idec,	eSP,	NOARG,	NOARG,	Po32 },
/* 4D */ { UD_Idec,	eBP,	NOARG,	NOARG,	Po32 },
/* 4E */ { UD_Idec,	eSI,	NOARG,	NOARG,	Po32 },
/* 4F */ { UD_Idec,	eDI,	NOARG,	NOARG,	Po32 },
/* 50 */ { UD_Ipush,	rAXr8,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 51 */ { UD_Ipush,	rCXr9,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 52 */ { UD_Ipush,	rDXr10,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 53 */ { UD_Ipush,	rBXr11,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 54 */ { UD_Ipush,	rSPr12,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 55 */ { UD_Ipush,	rBPr13,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 56 */ { UD_Ipush,	rSIr14,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 57 */ { UD_Ipush,	rDIr15,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 58 */ { UD_Ipop,	rAXr8,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 59 */ { UD_Ipop,	rCXr9,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5A */ { UD_Ipop,	rDXr10,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5B */ { UD_Ipop,	rBXr11,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5C */ { UD_Ipop,	rSPr12,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5D */ { UD_Ipop,	rBPr13,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5E */ { UD_Ipop,	rSIr14,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 5F */ { UD_Ipop,	rDIr15,	NOARG,	NOARG,	Po32 | Pdef64 | REX(_B) },
/* 60 */ { UD_Ipusha,	NOARG,	NOARG,	NOARG,	Po32 | Pinv64 | PdepM },
/* 61 */ { UD_Ipopa,	NOARG,	NOARG,	NOARG,	Po32 | Pinv64 | PdepM },
/* 62 */ { UD_Ibound,	Gv,	E,	NOARG,	Po32 | Pa32 | Pinv64 },
/* 63 */ { UD_Iarpl,	Ew,	Gw,	NOARG,	Pa32 | Pinv64 },
/* 64 */ { UD_Ifs,	ES,	NOARG,	NOARG,	Pnone },
/* 65 */ { UD_Igs,	GS,	NOARG,	NOARG,	Pnone },
/* 66 */ { UD_Ia32,	NOARG,	NOARG,	NOARG,	Pnone },
/* 67 */ { UD_Io32,	NOARG,	NOARG,	NOARG,	Pnone },
/* 68 */ { UD_Ipush,	Iz,	NOARG,	NOARG,	Pc1 | Po32 },
/* 69 */ { UD_Iimul,	Gv,	Ev,	Iz,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 6A */ { UD_Ipush,	Ib,	NOARG,	NOARG,	Pnone },
/* 6B */ { UD_Iimul,	Gv,	Ev,	Ib,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 6C */ { UD_Iinsb,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6D */ { UD_Iinsw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM },
/* 6E */ { UD_Ioutsb,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6F */ { UD_Ioutsw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM },
/* 70 */ { UD_Ijo,	Jb,	NOARG,	NOARG,	Pnone },
/* 71 */ { UD_Ijno,	Jb,	NOARG,	NOARG,	Pnone },
/* 72 */ { UD_Ijb,	Jb,	NOARG,	NOARG,	Pnone },
/* 73 */ { UD_Ijnb,	Jb,	NOARG,	NOARG,	Pnone },
/* 74 */ { UD_Ijz,	Jb,	NOARG,	NOARG,	Pnone },
/* 75 */ { UD_Ijnz,	Jb,	NOARG,	NOARG,	Pnone },
/* 76 */ { UD_Ijbe,	Jb,	NOARG,	NOARG,	Pnone },
/* 77 */ { UD_Ijnbe,	Jb,	NOARG,	NOARG,	Pnone },
/* 78 */ { UD_Ijs,	Jb,	NOARG,	NOARG,	Pnone },
/* 79 */ { UD_Ijns,	Jb,	NOARG,	NOARG,	Pnone },
/* 7A */ { UD_Ijp,	Jb,	NOARG,	NOARG,	Pnone },
/* 7B */ { UD_Ijnp,	Jb,	NOARG,	NOARG,	Pnone },
/* 7C */ { UD_Ijl,	Jb,	NOARG,	NOARG,	Pnone },
/* 7D */ { UD_Ijnl,	Jb,	NOARG,	NOARG,	Pnone },
/* 7E */ { UD_Ijle,	Jb,	NOARG,	NOARG,	Pnone },
/* 7F */ { UD_Ijnle,	Jb,	NOARG,	NOARG,	Pnone },
/* 80 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 81 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 82 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 83 */ { UD_Igrp,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
/* 84 */ { UD_Itest,	Eb,	Gb,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 85 */ { UD_Itest,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 86 */ { UD_Ixchg,	Eb,	Gb,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 87 */ { UD_Ixchg,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 88 */ { UD_Imov,	Eb,	Gb,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 89 */ { UD_Imov,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 8A */ { UD_Imov,	Gb,	Eb,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 8B */ { UD_Imov,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 8C */ { UD_Imov,	Ev,	S,	NOARG,	Po32 | Pa32 | REX(_R|_X|_B) },
/* 8D */ { UD_Ilea,	Gv,	E,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 8E */ { UD_Imov,	S,	Ev,	NOARG,	Po32 | Pa32 | REX(_R|_X|_B) },
/* 8F */ { UD_Ipop,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | Pdef64 | REX(_W|_R|_X|_B) },
/* 90 */ { UD_Ixchg,	rAXr8,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 91 */ { UD_Ixchg,	rCXr9,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 92 */ { UD_Ixchg,	rDXr10,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 93 */ { UD_Ixchg,	rBXr11,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 94 */ { UD_Ixchg,	rSPr12,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 95 */ { UD_Ixchg,	rBPr13,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 96 */ { UD_Ixchg,	rSIr14,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 97 */ { UD_Ixchg,	rDIr15,	rAX,	NOARG,	Po32 | REX(_W|_B) },
/* 98 */ { UD_Icbw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* 99 */ { UD_Icwd,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* 9A */ { UD_Icall,	Ap,	NOARG,	NOARG,	Pc1 | Po32 | Pinv64 },
/* 9B */ { UD_Iwait,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9C */ { UD_Ipushfw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* 9D */ { UD_Ipopfw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* 9E */ { UD_Isahf,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9F */ { UD_Ilahf,	NOARG,	NOARG,	NOARG,	Pnone },
/* A0 */ { UD_Imov,	AL,	Ob,	NOARG,	Pnone },
/* A1 */ { UD_Imov,	rAX,	Ov,	NOARG,	Po32 | Pa32 },
/* A2 */ { UD_Imov,	Ob,	AL,	NOARG,	Pnone },
/* A3 */ { UD_Imov,	Ov,	rAX, 	NOARG,	Po32 | Pa32 },
/* A4 */ { UD_Imovsb,	NOARG,	NOARG,	NOARG,	Pnone },
/* A5 */ { UD_Imovsw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* A6 */ { UD_Icmpsb,	NOARG,	NOARG,	NOARG,	Pnone },
/* A7 */ { UD_Icmpsw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* A8 */ { UD_Itest,	AL,	Ib,	NOARG,	Pnone },
/* A9 */ { UD_Itest,	rAX,	Iz,	NOARG,	Po32 | REX(_W) },
/* AA */ { UD_Istosb,	NOARG,	NOARG,	NOARG,	Pnone },
/* AB */ { UD_Istosw,	NOARG,	NOARG,	NOARG,	PdepM | REX(_W) },
/* AC */ { UD_Ilodsb,	NOARG,	NOARG,	NOARG,	Pnone },
/* AD */ { UD_Ilodsw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* AE */ { UD_Iscasb,	NOARG,	NOARG,	NOARG,	Pnone },
/* AF */ { UD_Iscasw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* B0 */ { UD_Imov,	ALr8b,	Ib,	NOARG,	REX(_B) },
/* B1 */ { UD_Imov,	CLr9b,	Ib,	NOARG,	REX(_B) },
/* B2 */ { UD_Imov,	DLr10b,	Ib,	NOARG,	REX(_B) },
/* B3 */ { UD_Imov,	BLr11b,	Ib,	NOARG,	REX(_B) },
/* B4 */ { UD_Imov,	AHr12b,	Ib,	NOARG,	REX(_B) },
/* B5 */ { UD_Imov,	CHr13b,	Ib,	NOARG,	REX(_B) },
/* B6 */ { UD_Imov,	DHr14b,	Ib,	NOARG,	REX(_B) },
/* B7 */ { UD_Imov,	BHr15b,	Ib,	NOARG,	REX(_B) },
/* B8 */ { UD_Imov,	rAXr8,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* B9 */ { UD_Imov,	rCXr9,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BA */ { UD_Imov,	rDXr10,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BB */ { UD_Imov,	rBXr11,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BC */ { UD_Imov,	rSPr12,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BD */ { UD_Imov,	rBPr13,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BE */ { UD_Imov,	rSIr14,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* BF */ { UD_Imov,	rDIr15,	Iv,	NOARG,	Po32 | REX(_W|_B) },
/* C0 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C1 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C2 */ { UD_Iret,	Iw,	NOARG,	NOARG,	Pnone },
/* C3 */ { UD_Iret,	NOARG,	NOARG,	NOARG,	Pnone },
/* C4 */ { UD_Iles,	Gv,	E,	NOARG,	Po32 | Pa32 | Pinv64 },
/* C5 */ { UD_Ilds,	Gv,	E,	NOARG,	Po32 | Pa32 | Pinv64 },
/* C6 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C8 */ { UD_Ienter,	Iw,	Ib,	NOARG,	Pnone | Pdef64 },
/* C9 */ { UD_Ileave,	NOARG,	NOARG,	NOARG,	Pnone },
/* CA */ { UD_Iretf,	Iw,	NOARG,	NOARG,	Pnone },
/* CB */ { UD_Iretf,	NOARG,	NOARG,	NOARG,	Pnone },
/* CC */ { UD_Iint3,	NOARG,	NOARG,	NOARG,	Pnone },
/* CD */ { UD_Iint,	Ib,	NOARG,	NOARG,	Pnone },
/* CE */ { UD_Iinto,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* CF */ { UD_Iiretw,	NOARG,	NOARG,	NOARG,	Po32 | PdepM | REX(_W) },
/* D0 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* D1 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* D2 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* D3 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* D4 */ { UD_Iaam,	Ib,	NOARG,	NOARG,	Pinv64 },
/* D5 */ { UD_Iaad,	Ib,	NOARG,	NOARG,	Pinv64 },
/* D6 */ { UD_Isalc,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* D7 */ { UD_Ixlat,	NOARG,	NOARG,	NOARG,	Pnone },
/* D8 */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* D9 */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DA */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DB */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DC */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DD */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DE */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* DF */ { UD_Ix87,	NOARG,	NOARG,	NOARG,	Pnone },
/* E0 */ { UD_Iloopn,	Jb,	NOARG,	NOARG,	Pnone },
/* E1 */ { UD_Iloope,	Jb,	NOARG,	NOARG,	Pnone },
/* E2 */ { UD_Iloop,	Jb,	NOARG,	NOARG,	Pnone | Pdef64 },
/* E3 */ { UD_Ijcxz,	Jb,	NOARG,	NOARG,	PdepM | Pa32 },
/* E4 */ { UD_Iin,	AL,	Ib,	NOARG,	Pnone },
/* E5 */ { UD_Iin,	eAX,	Ib,	NOARG,	Pnone | Po32 },
/* E6 */ { UD_Iout,	Ib,	AL,	NOARG,	Pnone },
/* E7 */ { UD_Iout,	Ib,	eAX,	NOARG,	Pnone },
/* E8 */ { UD_Icall,	Jz,	NOARG,	NOARG,	Po32 | Pdef64 },
/* E9 */ { UD_Ijmp,	Jz,	NOARG,	NOARG,	Po32 | Pdef64 },
/* EA */ { UD_Ijmp,	Ap,	NOARG,	NOARG,	Pinv64 },
/* EB */ { UD_Ijmp,	Jb,	NOARG,	NOARG,	Pnone },
/* EC */ { UD_Iin,	AL,	DX,	NOARG,	Pnone },
/* ED */ { UD_Iin,	eAX,	DX,	NOARG,	Pnone },
/* EE */ { UD_Iout,	DX,	AL,	NOARG,	Pnone },
/* ED */ { UD_Iout,	DX,	eAX,	NOARG,	Pnone },
/* F0 */ { UD_Ilock,	NOARG,	NOARG,	NOARG,	Pnone },
/* F1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F2 */ { UD_Irepne,	NOARG,	NOARG,	NOARG,	Pnone },
/* F3 */ { UD_Irep,	NOARG,	NOARG,	NOARG,	Pnone },
/* F4 */ { UD_Ihlt,	NOARG,	NOARG,	NOARG,	Pnone },
/* F5 */ { UD_Icmc,	NOARG,	NOARG,	NOARG,	Pnone },
/* F6 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* F7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* F8 */ { UD_Iclc,	NOARG,	NOARG,	NOARG,	Pnone },
/* F9 */ { UD_Istc,	NOARG,	NOARG,	NOARG,	Pnone },
/* FA */ { UD_Icli,	NOARG,	NOARG,	NOARG,	Pnone },
/* FB */ { UD_Isti,	NOARG,	NOARG,	NOARG,	Pnone },
/* FC */ { UD_Icld,	NOARG,	NOARG,	NOARG,	Pnone },
/* FD */ { UD_Istd,	NOARG,	NOARG,	NOARG,	Pnone },
/* FE */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* FF */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone }

};


/* This is the table for 2byte no-prefix opcodes */

struct map_entry itab_2byte[0x100] = 
{
	/* Instruction, op1, op2, op3, Valid Prefixes */

/* 00 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 01 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 02 */ { UD_Ilar,	Gv,	Ew,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 03 */ { UD_Ilsl,	Gv,	Ew,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 04 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 05 */ { UD_Isyscall,	NOARG,	NOARG,	NOARG,	Pnone },
/* 06 */ { UD_Iclts,	NOARG,	NOARG,	NOARG,	Pnone },
/* 07 */ { UD_Isysret,	NOARG,	NOARG,	NOARG,	Pnone },
/* 08 */ { UD_Iinvd,	NOARG,	NOARG,	NOARG,	Pnone },
/* 09 */ { UD_Iwbinvd,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0B */ { UD_Iud2,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0D */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0E */ { UD_Ifemms,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0F */ { UD_I3dnow,	NOARG,	NOARG,	NOARG,	Pnone },
/* 10 */ { UD_Imovups,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 11 */ { UD_Imovups,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 12 */ { UD_Imovlps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 13 */ { UD_Imovlps,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 14 */ { UD_Iunpcklps,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 15 */ { UD_Iunpckhps,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 16 */ { UD_Imovhps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 17 */ { UD_Imovhps,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 18 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone  },
/* 19 */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1A */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1B */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1C */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1D */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1E */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 1F */ { UD_Inop,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 20 */ { UD_Imov,	R,	C,	NOARG,	REX(_R) },
/* 21 */ { UD_Imov,	R,	D,	NOARG,	REX(_R) },
/* 22 */ { UD_Imov,	C,	R,	NOARG,	REX(_R) },
/* 23 */ { UD_Imov,	D,	R,	NOARG,	REX(_R) },
/* 24 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 25 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 26 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 27 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 28 */ { UD_Imovaps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 29 */ { UD_Imovaps,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2A */ { UD_Icvtpi2ps,V,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2B */ { UD_Imovntps,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2C */ { UD_Icvttps2pi,P,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2D */ { UD_Icvtps2pi,P,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2E */ { UD_Iucomiss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2F */ { UD_Icomiss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 30 */ { UD_Iwrmsr,	NOARG,	NOARG,	NOARG,	},
/* 31 */ { UD_Irdtsc,	NOARG,	NOARG,	NOARG,	},
/* 32 */ { UD_Irdmsr,	NOARG,	NOARG,	NOARG,	},
/* 33 */ { UD_Irdpmc,	NOARG,	NOARG,	NOARG,	},
/* 34 */ { UD_Isysenter, NOARG,	NOARG,	NOARG,	Pinv64 },
/* 35 */ { UD_Isysexit,	NOARG,	NOARG,	NOARG,	Pinv64 },
/* 36 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 37 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 38 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 39 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 40 */ { UD_Icmovo,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 41 */ { UD_Icmovno,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 42 */ { UD_Icmovb,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 43 */ { UD_Icmovnb,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 44 */ { UD_Icmovz,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 45 */ { UD_Icmovnz,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 46 */ { UD_Icmovbe,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 47 */ { UD_Icmovnbe,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 48 */ { UD_Icmovs,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 49 */ { UD_Icmovns,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4A */ { UD_Icmovp,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4B */ { UD_Icmovnp,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4C */ { UD_Icmovl,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4D */ { UD_Icmovnl,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4E */ { UD_Icmovle,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 4F */ { UD_Icmovnle,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* 50 */ { UD_Imovmskps,Gd,	VR,	NOARG,	Po32 | REX(_W|_R|_B) },
/* 51 */ { UD_Isqrtps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 52 */ { UD_Irsqrtps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 53 */ { UD_Ircpps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 54 */ { UD_Iandps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 55 */ { UD_Iandnps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 56 */ { UD_Iorps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 57 */ { UD_Ixorps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 58 */ { UD_Iaddps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 59 */ { UD_Imulps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5A */ { UD_Icvtps2pd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5B */ { UD_Icvtdq2ps,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5C */ { UD_Isubps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5D */ { UD_Iminps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5E */ { UD_Idivps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5F */ { UD_Imaxps,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 60 */ { UD_Ipunpcklbw,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 61 */ { UD_Ipunpcklwd,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 62 */ { UD_Ipunpckldq,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 63 */ { UD_Ipacksswb, P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 64 */ { UD_Ipcmpgtb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 65 */ { UD_Ipcmpgtw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 66 */ { UD_Ipcmpgtd,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 67 */ { UD_Ipackuswb,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 68 */ { UD_Ipunpckhbw,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 69 */ { UD_Ipunpckhwd,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6A */ { UD_Ipunpckhdq,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6B */ { UD_Ipackssdw,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6E */ { UD_Imovd,	P,	Ex,	NOARG,	Pc2 | Pa32 | REX(_R|_X|_B) },
/* 6F */ { UD_Imovq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 70 */ { UD_Ipshufw,	P,	Q,	Ib,	Pa32 | REX(_R|_X|_B) },
/* 71 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 72 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 73 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 74 */ { UD_Ipcmpeqb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 75 */ { UD_Ipcmpeqw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 76 */ { UD_Ipcmpeqd,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 77 */ { UD_Iemms,	NOARG,	NOARG,	NOARG,	Pnone },
/* 78 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 79 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7E */ { UD_Imovd,	Ex,	P,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
/* 7F */ { UD_Imovq,	Q,	P,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 80 */ { UD_Ijo,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64},
/* 81 */ { UD_Ijno,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 82 */ { UD_Ijb,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 83 */ { UD_Ijnb,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 84 */ { UD_Ijz,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 85 */ { UD_Ijnz,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 86 */ { UD_Ijbe,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 87 */ { UD_Ijnbe,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 88 */ { UD_Ijs,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 89 */ { UD_Ijns,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64},
/* 8A */ { UD_Ijp,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 8B */ { UD_Ijnp,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 8C */ { UD_Ijl,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 8D */ { UD_Ijnl,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 8E */ { UD_Ijle,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 8F */ { UD_Ijnle,	Jz,	NOARG,	NOARG,	Pc1 | Po32 | Pdef64 },
/* 90 */ { UD_Iseto,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 91 */ { UD_Isetno,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 92 */ { UD_Isetb,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 93 */ { UD_Isetnb,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 94 */ { UD_Isetz,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 95 */ { UD_Isetnz,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 96 */ { UD_Isetbe,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 97 */ { UD_Isetnbe,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 98 */ { UD_Isets,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 99 */ { UD_Isetns,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9A */ { UD_Isetp,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9B */ { UD_Isetnp,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9C */ { UD_Isetl,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9D */ { UD_Isetnl,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9E */ { UD_Isetle,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 9F */ { UD_Isetnle,	Eb,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* A0 */ { UD_Ipush,	FS,	NOARG,	NOARG,	Pnone },
/* A1 */ { UD_Ipop,	FS,	NOARG,	NOARG,	Pnone },
/* A2 */ { UD_Icpuid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A3 */ { UD_Ibt,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* A4 */ { UD_Ishld,	Ev,	Gv,	Ib,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* A5 */ { UD_Ishld,	Ev,	Gv,	CL,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* A6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A8 */ { UD_Ipush,	GS,	NOARG,	NOARG,	Pnone },
/* A9 */ { UD_Ipop,	GS,	NOARG,	NOARG,	Pnone },
/* AA */ { UD_Irsm,	NOARG,	NOARG,	NOARG,	Pnone },
/* AB */ { UD_Ibts,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* AC */ { UD_Ishrd,	Ev,	Gv,	Ib,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* AD */ { UD_Ishrd,	Ev,	Gv,	CL,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* AE */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* AF */ { UD_Iimul,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B0 */ { UD_Icmpxchg,	Eb,	Gb,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* B1 */ { UD_Icmpxchg,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B2 */ { UD_Ilss,	Gz,	E,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B3 */ { UD_Ibtr,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B4 */ { UD_Ilfs,	Gz,	E,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B5 */ { UD_Ilgs,	Gz,	E,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B6 */ { UD_Imovzx,	Gv,	Eb,	NOARG,	Pc2 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B7 */ { UD_Imovzx,	Gv,	Ew,	NOARG,	Pc2 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* B8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B9 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* BA */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* BB */ { UD_Ibtc,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* BC */ { UD_Ibsf,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* BD */ { UD_Ibsr,	Gv,	Ev,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* BE */ { UD_Imovsx,	Gv,	Eb,	NOARG,	Pc2 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* BF */ { UD_Imovsx,	Gv,	Ew,	NOARG,	Pc2 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C0 */ { UD_Ixadd,	Eb,	Gb,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C1 */ { UD_Ixadd,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C2 */ { UD_Icmpps,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C3 */ { UD_Imovnti,	M,	Gx,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* C4 */ { UD_Ipinsrw,	P,	Ew,	Ib,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C5 */ { UD_Ipextrw,	Gd,	PR,	Ib,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C6 */ { UD_Ishufps,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C8 */ { UD_Ibswap,	rAXr8,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* C9 */ { UD_Ibswap,	rCXr9,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CA */ { UD_Ibswap,	rDXr10,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CB */ { UD_Ibswap,	rBXr11,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CC */ { UD_Ibswap,	rSPr12,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CD */ { UD_Ibswap,	rBPr13,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CE */ { UD_Ibswap,	rSIr14,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* CF */ { UD_Ibswap,	rDIr15,	NOARG,	NOARG,	Po32 | REX(_W|_B) },
/* D0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D1 */ { UD_Ipsrlw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D2 */ { UD_Ipsrld,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D3 */ { UD_Ipsrlq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D4 */ { UD_Ipaddq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D5 */ { UD_Ipmullw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D7 */ { UD_Ipmovmskb,Gd,	PR,	NOARG,	Pnone },
/* D8 */ { UD_Ipsubusb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D9 */ { UD_Ipsubusw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DA */ { UD_Ipminub,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DB */ { UD_Ipand,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DC */ { UD_Ipaddusb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DD */ { UD_Ipaddusw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DE */ { UD_Ipmaxub,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DF */ { UD_Ipandn,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E0 */ { UD_Ipavgb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E1 */ { UD_Ipsraw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E2 */ { UD_Ipsrad,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E3 */ { UD_Ipavgw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E4 */ { UD_Ipmulhuw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E5 */ { UD_Ipmulhw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E7 */ { UD_Imovntq,	M,	P,	NOARG,	Pnone },
/* E8 */ { UD_Ipsubsb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E9 */ { UD_Ipsubsw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EA */ { UD_Ipminsw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EB */ { UD_Ipor,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EC */ { UD_Ipaddsb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* ED */ { UD_Ipaddsw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EE */ { UD_Ipmaxsw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EF */ { UD_Ipxor,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F1 */ { UD_Ipsllw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F2 */ { UD_Ipslld,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F3 */ { UD_Ipsllq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F4 */ { UD_Ipmuludq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F5 */ { UD_Ipmaddwd,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F6 */ { UD_Ipsadbw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F7 */ { UD_Imaskmovq,P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F8 */ { UD_Ipsubb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F9 */ { UD_Ipsubw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FA */ { UD_Ipsubd,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FB */ { UD_Ipsubq,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FC */ { UD_Ipaddb,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FD */ { UD_Ipaddw,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FE */ { UD_Ipaddd,	P,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

struct map_entry itab_2byte_prefixF3[0x100] = 
{
/* 00 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 01 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 02 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 03 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 04 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 05 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 06 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 07 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 08 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 09 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 10 */ { UD_Imovss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 11 */ { UD_Imovss,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 12 */ { UD_Imovsldup,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 13 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 14 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 15 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 16 */ { UD_Imovshdup,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 17 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 18 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 19 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 20 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 21 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 22 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 23 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 24 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 25 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 26 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 27 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 28 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 29 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2A */ { UD_Icvtsi2ss,V,	Ex,	NOARG,	Pc2 | Pa32 | REX(_R|_X|_B) },
/* 2B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2C */ { UD_Icvttsi2ss,Gvw,	W,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
/* 2D */ { UD_Icvtss2si,Gvw,	W,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
/* 2E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 30 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 31 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 32 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 33 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 34 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 35 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 36 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 37 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 38 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 39 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 40 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 41 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 42 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 43 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 44 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 45 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 46 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 47 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 48 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 49 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 50 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 51 */ { UD_Isqrtss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 52 */ { UD_Irsqrtss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 53 */ { UD_Ircpss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 54 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 55 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 56 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 57 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 58 */ { UD_Iaddss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 59 */ { UD_Imulss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5A */ { UD_Icvtss2sd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5B */ { UD_Icvttps2dq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5C */ { UD_Isubss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5D */ { UD_Iminss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5E */ { UD_Idivss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5F */ { UD_Imaxss,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 60 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 61 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 62 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 63 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 64 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 65 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 66 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 67 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 68 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 69 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6F */ { UD_Imovdqu,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 70 */ { UD_Ipshufhw,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* 71 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 72 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 73 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 74 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 75 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 76 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 77 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 78 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 79 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7E */ { UD_Imovq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 7F */ { UD_Imovdqu,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 80 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 81 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 82 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 83 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 84 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 85 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 86 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 87 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 88 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 89 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 90 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 91 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 92 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 93 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 94 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 95 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 96 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 97 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 98 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 99 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C0 */ { UD_Ixadd,	Eb,	Gb,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* C1 */ { UD_Ixadd,	Ev,	Gv,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* C2 */ { UD_Icmpss,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D6 */ { UD_Imovq2dq,	V,	PR,	NOARG,	Pa32  },
/* D7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E6 */ { UD_Icvtdq2pd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* ED */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

struct map_entry itab_2byte_prefix66[0x100] = 
{
/* 00 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 01 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 02 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 03 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 04 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 05 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 06 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 07 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 08 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 09 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 10 */ { UD_Imovupd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 11 */ { UD_Imovupd,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 12 */ { UD_Imovlpd,	V,	M,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 13 */ { UD_Imovlpd,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 14 */ { UD_Iunpcklpd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 15 */ { UD_Iunpckhpd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 16 */ { UD_Imovhpd,	V,	M,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 17 */ { UD_Imovhpd,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 18 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 19 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 20 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 21 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 22 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 23 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 24 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 25 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 26 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 27 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 28 */ { UD_Imovapd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 29 */ { UD_Imovapd,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2A */ { UD_Icvtpi2pd,V,	Q,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2B */ { UD_Imovntpd,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2C */ { UD_Icvttpd2pi,P,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2D */ { UD_Icvtpd2pi,P,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2E */ { UD_Iucomisd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 2F */ { UD_Icomisd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 30 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 31 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 32 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 33 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 34 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 35 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 36 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 37 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 38 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 39 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 40 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 41 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 42 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 43 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 44 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 45 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 46 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 47 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 48 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 49 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 50 */ { UD_Imovmskpd,Gd,	VR,	NOARG,	Po32 | REX(_W|_R|_B) },
/* 51 */ { UD_Isqrtpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 52 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 53 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 54 */ { UD_Iandpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 55 */ { UD_Iandnpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 56 */ { UD_Iorpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 57 */ { UD_Ixorpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 58 */ { UD_Iaddpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 59 */ { UD_Imulpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5A */ { UD_Icvtpd2ps,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5B */ { UD_Icvtps2dq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5C */ { UD_Isubpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5D */ { UD_Iminpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5E */ { UD_Idivpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5F */ { UD_Imaxpd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 60 */ { UD_Ipunpcklbw,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 61 */ { UD_Ipunpcklwd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 62 */ { UD_Ipunpckldq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 63 */ { UD_Ipacksswb,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 64 */ { UD_Ipcmpgtb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 65 */ { UD_Ipcmpgtw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 66 */ { UD_Ipcmpgtd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 67 */ { UD_Ipackuswb,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 68 */ { UD_Ipunpckhbw,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 69 */ { UD_Ipunpckhwd,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6A */ { UD_Ipunpckhdq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6B */ { UD_Ipackssdw,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6C */ { UD_Ipunpcklqdq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6D */ { UD_Ipunpckhqdq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 6E */ { UD_Imovd,	V,	Ex,	NOARG,	Pc2 | Pa32 | REX(_W|_R|_X|_B) },
/* 6F */ { UD_Imovqa,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 70 */ { UD_Ipshufd,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* 71 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 72 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 73 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* 74 */ { UD_Ipcmpeqb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 75 */ { UD_Ipcmpeqw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 76 */ { UD_Ipcmpeqd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 77 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 78 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 79 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7E */ { UD_Imovd,	Ex,	V,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
/* 7F */ { UD_Imovdqa,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 80 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 81 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 82 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 83 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 84 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 85 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 86 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 87 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 88 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 89 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 90 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 91 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 92 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 93 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 94 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 95 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 96 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 97 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 98 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 99 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C0 */ { UD_Ixadd,	Eb,	Gb,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* C1 */ { UD_Ixadd,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
/* C2 */ { UD_Icmppd,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C4 */ { UD_Ipinsrw,	V,	Ew,	Ib,	Pa32 | REX(_W|_R|_X|_B) },
/* C5 */ { UD_Ipextrw,	Gd,	VR,	Ib,	Pa32  },
/* C6 */ { UD_Ishufpd,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D1 */ { UD_Ipsrlw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D2 */ { UD_Ipsrld,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D3 */ { UD_Ipsrlq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D4 */ { UD_Ipaddq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D5 */ { UD_Ipmullw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D6 */ { UD_Iinvalid, W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D7 */ { UD_Ipmovmskb,Gd,	VR,	NOARG,	Pnone },
/* D8 */ { UD_Ipsubusb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* D9 */ { UD_Ipsubusw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DA */ { UD_Ipminub,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DB */ { UD_Ipand,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DC */ { UD_Ipaddusb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DD */ { UD_Ipaddusw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DE */ { UD_Ipmaxub,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* DF */ { UD_Ipandn,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E0 */ { UD_Ipavgb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E1 */ { UD_Ipsraw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E2 */ { UD_Ipsrad,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E3 */ { UD_Ipavgw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E4 */ { UD_Ipmulhuw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E5 */ { UD_Ipmulhw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E6 */ { UD_Icvttpd2dq,V,	W,	NOARG,	Pnone },
/* E7 */ { UD_Imovntdq,	M,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E8 */ { UD_Ipsubsb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E9 */ { UD_Ipsubsw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EA */ { UD_Ipminsw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EB */ { UD_Ipor,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EC */ { UD_Ipaddsb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* ED */ { UD_Ipaddsw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EE */ { UD_Ipmaxsw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* EF */ { UD_Ipxor,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F1 */ { UD_Ipsllw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F2 */ { UD_Ipslld,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F3 */ { UD_Ipsllq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F4 */ { UD_Ipmuludq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F5 */ { UD_Ipmaddwd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F6 */ { UD_Ipsadbw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F7 */ { UD_Imaskmovq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F8 */ { UD_Ipsubb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* F9 */ { UD_Ipsubw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FA */ { UD_Ipsubd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FB */ { UD_Ipsubq,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FC */ { UD_Ipaddb,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FD */ { UD_Ipaddw,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FE */ { UD_Ipaddd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* FF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

struct map_entry itab_2byte_prefixF2[0x100] = {
/* 00 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 01 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 02 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 03 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 04 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 05 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 06 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 07 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 08 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 09 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 0F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 10 */ { UD_Imovsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 11 */ { UD_Imovsd,	W,	V,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 12 */ { UD_Imovddup,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 13 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 14 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 15 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 16 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 17 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 18 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 19 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 1F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 20 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 21 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 22 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 23 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 24 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 25 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 26 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 27 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 28 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 29 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2A */ { UD_Icvtsi2sd,V,	Ex,	NOARG,	Pc2 | Pa32 | REX(_W|_R|_X|_B) },
/* 2B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2C */ { UD_Icvttsi2sd,Gvw,	W,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
/* 2D */ { UD_Icvtsd2si,Gvw,	W,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
/* 2E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 2F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 30 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 31 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 32 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 33 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 34 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 35 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 36 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 37 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 38 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 39 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 3F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 40 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 41 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 42 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 43 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 44 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 45 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 46 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 47 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 48 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 49 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 4F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 50 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 51 */ { UD_Isqrtsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 52 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 53 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 54 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 55 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 56 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 57 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 58 */ { UD_Iaddsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 59 */ { UD_Imulsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5A */ { UD_Icvtsd2ss,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 5C */ { UD_Isubsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5D */ { UD_Iminsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5E */ { UD_Idivsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 5F */ { UD_Imaxsd,	V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* 60 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 61 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 62 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 63 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 64 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 65 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 66 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 67 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 68 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 69 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 6F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 70 */ { UD_Ipshuflw,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* 71 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 72 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 73 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 74 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 75 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 76 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 77 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 78 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 79 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 7F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 80 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 81 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 82 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 83 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 84 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 85 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 86 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 87 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 88 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 89 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 8F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 90 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 91 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 92 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 93 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 94 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 95 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 96 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 97 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 98 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 99 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9A */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9B */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9C */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9D */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9E */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* 9F */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* A9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* AF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* B9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* BF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C0 */ { UD_Ixadd,	Eb,	Gb,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
/* C1 */ { UD_Ixadd,	Ev,	Gv,	NOARG,	Po32 | Pa32 | REX(_R|_X|_B) },
/* C2 */ { UD_Icmpsd,	V,	W,	Ib,	Pa32 | REX(_R|_X|_B) },
/* C3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C7 */ { UD_Igrp,	NOARG,	NOARG,	NOARG,	Pnone },
/* C8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* C9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* CF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D6 */ { UD_Imovdq2q,	P,	VR,	NOARG,	Pa32  },
/* D7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* D9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* DF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E6 */ { UD_Icvtpd2dq,V,	W,	NOARG,	Pa32 | REX(_R|_X|_B) },
/* E7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* E9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* ED */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* EF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F0 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F1 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F2 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F3 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F4 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F5 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F6 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F7 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F8 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* F9 */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FA */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FB */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FC */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FD */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FE */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
/* FF */ { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

struct map_entry itab_g1_op80[0x8] = 
{
  { UD_Iadd,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ior,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iadc,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isbb,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iand,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isub,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ixor,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Icmp,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g1_op81[0x8] = 
{
  { UD_Iadd,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ior,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iadc,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isbb,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iand,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isub,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ixor,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Icmp,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g1_op82[0x8] = {
  { UD_Iadd,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ior,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iadc,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isbb,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iand,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isub,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ixor,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Icmp,	Eb,	Ib,	NOARG,	Pinv64 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g1A_op8F[0x8] = {
  { UD_Ipop,		Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};


struct map_entry itab_g1_op83[0x8] = {
  { UD_Iadd,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Ior,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Iadc,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Isbb,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Iand,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Isub,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Ixor,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Icmp,	Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_R|_X|_B) }
};

struct map_entry itab_g2_opC0[0x8] = {
  { UD_Irol,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isal,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Eb,	Ib,	NOARG,	Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g2_opC1[0x8] = {
  { UD_Irol,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isal,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Ev,	Ib,	NOARG,	Po32 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g2_opD0[0x8] = {
  { UD_Irol,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isal,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Eb,	I1,	NOARG,	Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g2_opD1[0x8] = {
  { UD_Irol,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Ev,	I1,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g2_opD2[0x8] = {
  { UD_Irol,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isal,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Eb,	CL,	NOARG,	Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g2_opD3[0x8] = {
  { UD_Irol,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iror,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircl,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ircr,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishl,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ishr,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isal,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Isar,	Ev,	CL,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g3_opF6[0x8] = {
  { UD_Itest,	Eb,	Ib,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Itest,	Eb,	Ib,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Inot,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ineg,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Imul,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iimul,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Idiv,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iidiv,	Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g3_opF7[0x8] = {
  { UD_Itest,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Itest,	Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Inot,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ineg,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Imul,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iimul,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Idiv,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iidiv,	Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) }
};

struct map_entry itab_g4_opFE[0x8] = {
  { UD_Iinc,		Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Idec,		Eb,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

struct map_entry itab_g5_opFF[0x8] = {
  { UD_Iinc,		Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Idec,		Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Icall,		Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | Pdef64 | REX(_W|_R|_X|_B) },
  { UD_Icall,		Ep,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ijmp,		Ev,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ijmp,		Ep,	NOARG,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ipush,		Ev,	NOARG,	NOARG,	Pc1 | Pdef64 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG, 	NOARG,	NOARG,	Pnone }
};

/* group 6 */
struct map_entry itab_g6_op0F00[0x8] = {
  { UD_Isldt,		E,	NOARG,	NOARG,	Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Istr,		E,	NOARG,	NOARG,	Po32 | Pa32 | REX(_R|_X|_B) },
  { UD_Illdt,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iltr,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iverr,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iverw,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};


/* group 7  */
struct map_entry itab_g7_op0F01[0x8] = {
  { UD_Isgdt,		M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Isidt,		M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ilgdt,		M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ilidt,		M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ismsw,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ilmsw,		E,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvlpg,		M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) }
};

/* group 7 -- Reg7 */
struct map_entry itab_g7_op0F01_Reg7[0x8] = {
  { UD_Iswapgs, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Irdtscp,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* group 7 -- Reg3 */
struct map_entry itab_g7_op0F01_Reg3[0x8] = {
  { UD_Ivmrun,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ivmmcall,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ivmload, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ivmsave, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Istgi,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iclgi,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iskinit, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvlpga,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* group 8  */
struct map_entry itab_g8_op0FBA[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ibt,		Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ibts,		Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ibtr,		Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ibtc,		Ev,	Ib,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
};

/* group 9  */
struct map_entry itab_g9_op0FC7[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Icmpxchg8b,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group A  */
struct map_entry itab_gA_op0FB9[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group B  */
struct map_entry itab_gB_opC6[0x8] = {
  { UD_Imov,		Eb,	Ib,	NOARG,	Pc1 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group B  */
struct map_entry itab_gB_opC7[0x8] = {
  { UD_Imov,		Ev,	Iz,	NOARG,	Pc1 | Po32 | Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group C  */
struct map_entry itab_gC_op0F71[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrlw,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsraw,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsllw,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group C  */
struct map_entry itab_gC_op0F71_prefix66[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrlw,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsraw,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsllw,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group D  */
struct map_entry itab_gD_op0F72[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrld,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrad,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipslld,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group D, prefixed by 0x66  */
struct map_entry itab_gD_op0F72_prefix66[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrld,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrad,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipslld,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group E  */
struct map_entry itab_gE_op0F73[0x8] = {
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrlq,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsllq,		PR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
};

/* group E, prefixed by 0x66  */
struct map_entry itab_gE_op0F73_prefix66[0x8] = 
{
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsrlq,		VR,	Ib,	NOARG,	Pnone },
  { UD_Ipsrldq,		VR,	Ib,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ipsllq,		VR,	Ib,	NOARG,	Pnone },
  { UD_Ipslldq,		VR,	Ib,	NOARG,	Pnone },
};


/* group F  */
struct map_entry itab_gF_op0FAE[0x8] = {
  { UD_Ifxsave,		M,	NOARG,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ifxrstor,	M,	NOARG,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Ildmxcsr,	Md,	NOARG,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Istmxcsr,	Md,	NOARG,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iclflush,	M,	NOARG,	NOARG,	Pa32 | REX(_W|_R|_X|_B) },
};

/* group F -- Extensions */
struct map_entry itab_gF_op0FAE_Reg5 = { 
	UD_Ilfence,	NOARG,	NOARG,	NOARG,	Pnone
};
struct map_entry itab_gF_op0FAE_Reg6 = {
	UD_Imfence,	NOARG,	NOARG,	NOARG,	Pnone
};
struct map_entry itab_gF_op0FAE_Reg7 = {
	UD_Isfence,	NOARG,	NOARG,	NOARG,	Pnone
};

/* D8 Opcode Map */
struct map_entry itab_x87_opD8reg[0x8] = 
{
  { UD_Ifadd,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifmul,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifcom,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifcomp,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifsub,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifsubr,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifdiv,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifdivr,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* D9 Opcode Map */
struct map_entry itab_x87_opD9reg[0x8] = 
{
  { UD_Ifld,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifstp,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifldenv,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ifldcw,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifstenv,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ifstcw,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* DA Opcode Map */
struct map_entry itab_x87_opDAreg[0x8] = 
{
  { UD_Ifiadd,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifimul,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ificom,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ificomp,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifisub,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifisubr,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifidiv,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifidivr,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* DB Opcode Map */
struct map_entry itab_x87_opDBreg[0x8] = 
{
  { UD_Ifild,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifist,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifistp,	Md,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifld,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) }
};

/* DC Opcode Map */
struct map_entry itab_x87_opDCreg[0x8] = 
{
  { UD_Ifadd,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifmul,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifcom,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifcomp,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifsub,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifsubr,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifdiv,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifdivr,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* DD Opcode Map */
struct map_entry itab_x87_opDDreg[0x8] = 
{
  { UD_Ifld,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifstp,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifrstor,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsave,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ifstsw,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) }
};

/* DE Opcode Map */
struct map_entry itab_x87_opDEreg[0x8] = 
{
  { UD_Ifiadd,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifimul,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ificom,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ificomp,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifisub,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifisubr,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifidiv,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifidivr,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* DF Opcode Map */
struct map_entry itab_x87_opDFreg[0x8] = 
{
  { UD_Ifild,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifist,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifistp,	Mw,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifbld,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ifild,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) },
  { UD_Ifbstp,	M,	NOARG,	NOARG,	Pa32 | REX(_R|_X|_B) },
  { UD_Ifistp,	Mq,	NOARG,	NOARG,	Pc1 | Pa32 | REX(_R|_X|_B) }
};

/* D8 Opcode Map */
struct map_entry itab_x87_opD8[0x8*0x8] = 
{
  { UD_Ifadd,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifadd,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST7,	NOARG,	Pnone },
    { UD_Ifcom,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcom,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcomp,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST7,	NOARG,	Pnone }
};

/* D9 Opcode Map */
struct map_entry itab_x87_opD9[0x8*0x8] = 
{
  { UD_Ifld,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifld,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifxch,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifnop,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Inone,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifchs,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifabs,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iftst,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifxam,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifld1,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldl2t,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldl2e,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldlpi,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldlg2,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldln2,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifldz,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_If2xm1,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifyl2x,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifptan,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifpatan,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifpxtract,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifprem1,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifdecstp,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifncstp,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifprem,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifyl2xp1,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsqrt,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsincos,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifrndint,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifscale,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsin,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifcos,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* DA Opcode Map */
struct map_entry itab_x87_opDA[0x8*0x8] = {
  { UD_Ifcmovb,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovb,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmove,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovbe,ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovu,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifucompp,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* DB Opcode Map */
struct map_entry itab_x87_opDB[0x8*0x8] = 
{
  { UD_Ifcmovnb,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovnb,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovne,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovnbe,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcmovnu,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifclex,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifinit,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifucomi,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcomi,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid, 	NOARG,	NOARG,	NOARG,	Pnone }
};

/* DC Opcode Map */
struct map_entry itab_x87_opDC[0x8*0x8] = 
{
  { UD_Ifadd,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifadd,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifmul,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsubr,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifsubr,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifsub,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifdivr,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifdiv,		ST7,	ST0,	NOARG,	Pnone }
};	

/* DD Opcode Map */
struct map_entry itab_x87_opDD[0x8*0x8] = 
{
  { UD_Iffree,		ST0,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST1,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST2,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST3,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST4,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST5,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST6,	NOARG,	NOARG,	Pnone },
  { UD_Iffree,		ST7,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST0,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST1,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST2,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST3,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST4,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST5,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST6,	NOARG,	NOARG,	Pnone },
  { UD_Ifst,		ST7,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST0,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST1,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST2,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST3,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST4,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST5,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST6,	NOARG,	NOARG,	Pnone },
  { UD_Ifstp,		ST7,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST0,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST1,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST2,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST3,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST4,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST5,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST6,	NOARG,	NOARG,	Pnone },
  { UD_Ifucom,		ST7,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST0,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST1,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST2,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST3,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST4,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST5,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST6,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomp,		ST7,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* DE Opcode Map */
struct map_entry itab_x87_opDE[0x8*0x8] = 
{
  { UD_Ifaddp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifaddp,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifmulp,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifcompp,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifsubrp,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifsubp,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifdivrp,		ST7,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST0,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST1,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST2,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST3,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST4,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST5,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST6,	ST0,	NOARG,	Pnone },
  { UD_Ifdivp,		ST7,	ST0,	NOARG,	Pnone }
};

/* DF Opcode Map */
struct map_entry itab_x87_opDF[0x8*0x8] = 
{
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifstsw,		NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifucomip,	ST0,	ST7,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,		ST0,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST1,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST2,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST3,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST4,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST5,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST6,	NOARG,	Pnone },
  { UD_Ifcomip,		ST0,	ST7,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone },
  { UD_Iinvalid,	NOARG,	NOARG,	NOARG,	Pnone }
};

/* AMD 3DNow! Instructions */
struct map_entry itab_3DNow =
  { UD_I3dnow,	P,	Q,	NOARG  };

/* 3D Now instructions with suffix */
extern enum ud_mnemonic_code 
ud_map_get_3dnow(unsigned char suffix)
{
  switch(suffix) {
	case 0x0C: return UD_Ipi2fw;
	case 0x0D: return UD_Ipi2fd;
	case 0x1C: return UD_Ipf2iw;
	case 0x1D: return UD_Ipf2id;
	case 0x8A: return UD_Ipfnacc;
	case 0x90: return UD_Ipfcmpge;
	case 0x94: return UD_Ipfmin;
	case 0x96: return UD_Ipfrcp;
	case 0x97: return UD_Ipfrsqrt;
	case 0x9A: return UD_Ipfsub;
	case 0x9E: return UD_Ipfadd;
	case 0xA0: return UD_Ipfcmpgt;
	case 0xA4: return UD_Ipfmax;
	case 0xA6: return UD_Ipfrcpit1;
	case 0xA7: return UD_Ipfrsqit1;
	case 0xAA: return UD_Ipfsubr;
	case 0xAE: return UD_Ipfacc;
	case 0xB0: return UD_Ipfcmpeq;
	case 0xB4: return UD_Ipfmul;
	case 0xB6: return UD_Ipfrcpit2;
	case 0xB7: return UD_Ipmulhrw;
	case 0xBB: return UD_Ipswapd;
	case 0xBF: return UD_Ipavgusb;
  }
  return(0);
}

struct map_entry *itab_x87_reg[] = 
{
  itab_x87_opD8reg,
  itab_x87_opD9reg,
  itab_x87_opDAreg,
  itab_x87_opDBreg,
  itab_x87_opDCreg,
  itab_x87_opDDreg,
  itab_x87_opDEreg,
  itab_x87_opDFreg
};

struct map_entry *itab_x87[] = 
{
  itab_x87_opD8,
  itab_x87_opD9,
  itab_x87_opDA,
  itab_x87_opDB,
  itab_x87_opDC,
  itab_x87_opDD,
  itab_x87_opDE,
  itab_x87_opDF
};

struct map_entry nop = { UD_Inop, NOARG, NOARG,	NOARG, Pnone };
struct map_entry movsxd = { UD_Imovsxd, Gv, Ed, NOARG, Pc2 | Po32 | Pa32 | REX(_X|_W|_B|_R) };

/* -----------------------------------------------------------------------------
 * search_1byte_insn() - Searches for 1-byte instructions.
 * -----------------------------------------------------------------------------
 */
static void search_1byte_insn(register struct ud* u)
{
  u->opcmap_entry = &itab_1byte[inp_curr(u)];

  if (inp_curr(u) == 0x90 && u->dis_mode == 32)
	u->opcmap_entry = &nop;	

  else
  /* special case for 64bit mode */
  if (u->dis_mode == 64 && (u->opcmap_entry)->mnemonic == UD_Iarpl)
	u->opcmap_entry = &movsxd;

  /* if the opcode points to a group */
  else if ((u->opcmap_entry)->mnemonic == UD_Igrp) {
	switch(inp_curr(u)) {
		/* group 1 */
		case 0x80:
			u->opcmap_entry = &itab_g1_op80[MODRM_REG(inp_peek(u))];
			break;
		case 0x81:
			u->opcmap_entry = &itab_g1_op81[MODRM_REG(inp_peek(u))];
			break;
		case 0x82:
			u->opcmap_entry = &itab_g1_op82[MODRM_REG(inp_peek(u))];
			break;
		case 0x83:
			u->opcmap_entry = &itab_g1_op83[MODRM_REG(inp_peek(u))];
			break;
		case 0x8F:
			u->opcmap_entry = &itab_g1A_op8F[MODRM_REG(inp_peek(u))];
			break;
		/* group 2 */
		case 0xC0:
			u->opcmap_entry = &itab_g2_opC0[MODRM_REG(inp_peek(u))];
			break;
		case 0xC1:
			u->opcmap_entry = &itab_g2_opC1[MODRM_REG(inp_peek(u))];
			break;
		/* group 11 */
		case 0xC6:
			u->opcmap_entry = &itab_gB_opC6[MODRM_REG(inp_peek(u))];
			break;
		case 0xC7:
			u->opcmap_entry = &itab_gB_opC7[MODRM_REG(inp_peek(u))];
			break;
		case 0xD0:
			u->opcmap_entry = &itab_g2_opD0[MODRM_REG(inp_peek(u))];
			break;
		case 0xD1:
			u->opcmap_entry = &itab_g2_opD1[MODRM_REG(inp_peek(u))];
			break;
		case 0xD2:
			u->opcmap_entry = &itab_g2_opD2[MODRM_REG(inp_peek(u))];
			break;
		case 0xD3:
			u->opcmap_entry = &itab_g2_opD3[MODRM_REG(inp_peek(u))];
			break;
		/* group 3 */
		case 0xF6:
			u->opcmap_entry = &itab_g3_opF6[MODRM_REG(inp_peek(u))];
			break;
		case 0xF7:
			u->opcmap_entry = &itab_g3_opF7[MODRM_REG(inp_peek(u))];
			break;
		/* group 4 */
		case 0xFE:
			u->opcmap_entry = &itab_g4_opFE[MODRM_REG(inp_peek(u))];
			break;
		/* group 5 */
		case 0xFF:
			u->opcmap_entry = &itab_g5_opFF[MODRM_REG(inp_peek(u))];
			break;
	}
  }

  /* if the opcode points to an x87 instruction */
  else if ((u->opcmap_entry)->mnemonic == UD_Ix87) {
	/* When the ModRM byte value falls within the range of
	 * 0x00 - 0xBF, then the reg field selects the inst.
	 */
	if (inp_peek(u) <= 0xBF)
		u->opcmap_entry = &itab_x87_reg[(inp_curr(u))-0xD8][MODRM_REG(inp_peek(u))];
	else {
		u->opcmap_entry = &itab_x87[inp_curr(u)-0xD8][inp_peek(u)-0xC0];
		inp_next(u);
	}
  }  
}

/* -----------------------------------------------------------------------------
 * search_2byte_insn() - Searches for 2-byte instructions.
 * -----------------------------------------------------------------------------
 */
static void search_2byte_insn(register struct ud* u)
{
  inp_next(u);

  if (u->pfx_opr) { /* 0x66 */
	u->opcmap_entry = &itab_2byte_prefix66[inp_curr(u)];
	if ((u->opcmap_entry)->mnemonic != UD_Iinvalid)
		u->pfx_opr = 0;
	else	u->opcmap_entry = NULL;
  }

  if (u->opcmap_entry == NULL && u->pfx_rep) {  /* 0xF3 */
	u->opcmap_entry = &itab_2byte_prefixF3[inp_curr(u)];
	if ((u->opcmap_entry)->mnemonic != UD_Iinvalid)
		u->pfx_rep = 0;
	else	u->opcmap_entry = NULL;
  }

  if (u->opcmap_entry == NULL && u->pfx_repne) { /* 0xF2 */
	u->opcmap_entry = &itab_2byte_prefixF2[inp_curr(u)];
	if ((u->opcmap_entry)->mnemonic != UD_Iinvalid)
		u->pfx_repne = 0;
	else	u->opcmap_entry = NULL;
  }

  if (u->opcmap_entry == NULL) /* No Prefix */
	u->opcmap_entry = &itab_2byte[inp_curr(u)];

  /* check if the opcode points to the 3dnow group */
  if ((u->opcmap_entry)->mnemonic == UD_I3dnow) {
	u->opcmap_entry = &itab_3DNow;
	return;
  } 

  /* check if the opcode points to a group */
  if ((u->opcmap_entry)->mnemonic != UD_Igrp) {
	return;
  }

  switch (inp_curr(u)) {
	/* group 6 */
	case 0x00:
		u->opcmap_entry = &itab_g6_op0F00[MODRM_REG(inp_peek(u))];
		break;
	/* group 7 */
	case 0x01:
	{
		uint8_t reg = MODRM_REG(inp_peek(u));
		uint8_t mod = MODRM_MOD(inp_peek(u));
		uint8_t rm  = MODRM_RM(inp_peek(u));

		if (reg == 3 && mod == 3) {
			u->opcmap_entry = &itab_g7_op0F01_Reg3[rm];
			inp_next(u);
		} else if (reg == 7 && mod == 3) {
			u->opcmap_entry = &itab_g7_op0F01_Reg7[rm];
			inp_next(u);
		} else u->opcmap_entry = &itab_g7_op0F01[reg];
		break;
	}
	/* group 8 */
	case 0xBA:
		u->opcmap_entry = &itab_g8_op0FBA[MODRM_REG(inp_peek(u))];
		break;	
	/* group 9 */
	case 0xC7:
		u->opcmap_entry = &itab_g9_op0FC7[MODRM_REG(inp_peek(u))];			
		break;
	/* group A */
	case 0xB9:
		u->opcmap_entry = &itab_gA_op0FB9[MODRM_REG(inp_peek(u))];
		break;
	/* group C */
	case 0x71:
		if (u->pfx_opr) {
			u->opcmap_entry = &itab_gC_op0F71_prefix66[MODRM_REG(inp_peek(u))];
			u->pfx_opr = 0;
		} else	u->opcmap_entry = &itab_gC_op0F71[MODRM_REG(inp_peek(u))];
		break;
	/* group D */
	case 0x72:
		if (u->pfx_opr) {
			u->opcmap_entry = &itab_gD_op0F72_prefix66[MODRM_REG(inp_peek(u))];
			u->pfx_opr = 0;
		} else	u->opcmap_entry = &itab_gD_op0F72[MODRM_REG(inp_peek(u))];
		break;
	/* group E */
	case 0x73:
		if (u->pfx_opr) {
			u->opcmap_entry = &itab_gE_op0F73_prefix66[MODRM_REG(inp_peek(u))];
			u->pfx_opr = 0;
		} else	u->opcmap_entry = &itab_gE_op0F73[MODRM_REG(inp_peek(u))];
		break;
	/* group F */
	case 0xAE:
	{
		uint8_t reg = MODRM_REG(inp_peek(u));
		uint8_t mod = MODRM_MOD(inp_peek(u));

		if (reg == 5 && mod == 3) {
			u->opcmap_entry = &itab_gF_op0FAE_Reg5;
			inp_next(u);
		} else if (reg == 6 && mod == 3) {
			u->opcmap_entry = &itab_gF_op0FAE_Reg6;
			inp_next(u);
		} else if (reg == 7 && mod == 3) {
			u->opcmap_entry = &itab_gF_op0FAE_Reg7;
			inp_next(u);
		} else u->opcmap_entry = &itab_gF_op0FAE[reg];
		break;
	}
	break;

	/* Error */
	default:
		u->error = 1;
  }
}

/* =============================================================================
 * ud_search_map() - Searches the x86 opcode table for the instruction
 * corresponding to the opcode given by next byte in the byte stream.
 * =============================================================================
 */
void 
ud_search_map(register struct ud* u) 
{
  inp_next(u);

  u->opcmap_entry = NULL;

  /* check for two byte opcodes (0x0F) */
  if (0x0F == inp_curr(u))
	search_2byte_insn(u);
  else	search_1byte_insn(u);

  u->mnemonic = u->opcmap_entry->mnemonic;
}

/* =============================================================================
 * ud_lookup_mnemonic() - Looks-up the mnemonic code.
 * =============================================================================
 */
const char* 
ud_lookup_mnemonic(ud_mnemonic_code_t c) 
{
  if (c < UD_I3vil) 
	return ud_mnemonics[c];
  return NULL;
}

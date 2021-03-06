/* -----------------------------------------------------------------------------
 * syn-intel.c
 *
 * Copyright (c) 2002, 2003, 2004 Vivek Mohan <vivek@sig9.com>
 * All rights reserved. See (LICENSE)
 * -----------------------------------------------------------------------------
 */

#include "dis_types.h"
#include "extern.h"
#include "opcmap.h"
#include "syn.h"

/* -----------------------------------------------------------------------------
 * opr_cast() - Prints an operand cast.
 * -----------------------------------------------------------------------------
 */
static INLINE void 
opr_cast(struct ud* u, struct ud_operand* op)
{
  switch(op->size) {
	case  8: mkasm(u, "byte " ); break;
	case 16: mkasm(u, "word " ); break;
	case 32: mkasm(u, "dword "); break;
	case 64: mkasm(u, "qword "); break;
	default: break;
  }
  if (u->br_far)
	mkasm(u, "far "); 
  else if (u->br_near)
	mkasm(u, "near ");
}

/* -----------------------------------------------------------------------------
 * gen_operand() - Generates assembly output for each operand.
 * -----------------------------------------------------------------------------
 */
static void gen_operand(struct ud* u, struct ud_operand* op, int syn_cast)
{
  switch(op->type) {
	case UD_OP_REG:
		mkasm(u, ud_reg_tab[op->base - UD_R_AL]);
		break;

	case UD_OP_MEM:
		if (syn_cast) opr_cast(u, op);
		mkasm(u, "[");
		if (u->pfx_seg)
			mkasm(u, "%s:", ud_reg_tab[u->pfx_seg - UD_R_AL]);
		if (op->base)
			mkasm(u, "%s", ud_reg_tab[op->base - UD_R_AL]);
		if (op->index) {
			if (op->base)
				mkasm(u, "+");
			mkasm(u, "%s", ud_reg_tab[op->index - UD_R_AL]);
		}
		if (op->scale)
			mkasm(u, "*%d", op->scale);
		if (op->offset && (op->base || op->index)) {
			mkasm(u, "+");
		}

		if (op->offset == 8) {
			if (op->lval.sbyte < 0)
				mkasm(u, "(-0x%x)", (-op->lval.sbyte) & 0xFF);
			else	mkasm(u, "0x%x", op->lval.sbyte);
		} 
		else if (op->offset == 16) 
			mkasm(u, "0x%x", op->lval.uword);
		else if (op->offset == 32) 
			mkasm(u, "0x%lx", op->lval.udword);
		else if (op->offset == 64) 
			mkasm(u, "0x%llx", op->lval.udword);
		mkasm(u, "]");
		break;

			
	case UD_OP_IMM:
		if (syn_cast) opr_cast(u, op);
		switch (op->size) {
			case  8: mkasm(u, "0x%x", op->lval.ubyte);    break;
			case 16: mkasm(u, "0x%x", op->lval.uword);    break;
			case 32: mkasm(u, "0x%lx", op->lval.udword);  break;
			case 64: mkasm(u, "0x%llx", op->lval.uqword); break;
			default: break;
		}
		break;

	case UD_OP_JIMM:
		if (syn_cast) opr_cast(u, op);
		switch (op->size) {
			case  8:
				mkasm(u, "0x%x", (u->pc + op->lval.sbyte) & 0xFF); 
				break;
			case 16:
				mkasm(u, "0x%lx", (u->pc + op->lval.sword) & 0xFFFF ); 
				break;
			case 32:
				mkasm(u, "0x%lx", (u->pc + op->lval.sdword) & 0xFFFFFFFF );
				break;
			default:break;
		}
		break;

	case UD_OP_PTR:
		switch (op->size) {
			case 32:
				mkasm(u, "word 0x%x:0x%x", op->lval.ptr.seg, 
					op->lval.ptr.off & 0xFFFF);
				break;
			case 48:
				mkasm(u, "dword 0x%x:0x%lx", op->lval.ptr.seg, 
					op->lval.ptr.off);
				break;
		}
		break;

	case UD_OP_CONST:
		if (syn_cast) opr_cast(u, op);
		mkasm(u, "%d", op->lval.udword);
		break;

	default: return;
  }
}

/* =============================================================================
 * translates to intel syntax 
 * =============================================================================
 */
 
#define INSN_MAX_LEN 12

extern void ud_translate_intel(struct ud* u)
{
  unsigned int c = 0;
  /* prefixes */
  if (u->pfx_lock)
	mkasm(u, "lock ");
  if (u->pfx_rep)
	mkasm(u, "rep ");
  if (u->pfx_repne)
	mkasm(u, "repne ");

  /* print the instruction mnemonic */
  mkasm(u, "%s", ud_lookup_mnemonic(u->mnemonic));
  
  for (c = u->insn_fill; c < INSN_MAX_LEN; c++)
	mkasm(u, " ");
	
  /* operand 1 */
  if (u->operand[0].type != UD_NONE) {
	gen_operand(u, &u->operand[0], u->c1);
  }

  /* operand 2 */
  if (u->operand[1].type != UD_NONE) {
	mkasm(u, ", ");
	gen_operand(u, &u->operand[1], u->c2);
  }

  /* operand 3 */
  if (u->operand[2].type != UD_NONE) {
	mkasm(u, ", ");
	gen_operand(u, &u->operand[2], u->c3);
  }
}

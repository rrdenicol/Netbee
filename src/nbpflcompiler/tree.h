/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "defs.h"
#include "symbols.h"
#include "pflmirnode.h"
#include <stdio.h>



struct OpDescr
{
	char*		Sym;
	char*		Name;
};


enum IROperators
{
#define IR_OPERATOR(code, sym, name) code,
#include "iroperators.h"
#undef IR_OPERATOR
};


enum OpDetails
{
	IR_TERM	= 0,
	IR_UNOP	= 1, 
	IR_BINOP	= 2,
	IR_TEROP = 3,
	IR_STMT	= 4,
	IR_LAST = 0xF
};

enum IRType
{
	IR_TYPE_INT		= 1,
	IR_TYPE_STR		= 2,
	IR_TYPE_BOOL	= 3
};

enum IRTypeAliases
{
	I = IR_TYPE_INT,
	S = IR_TYPE_STR,
	B = IR_TYPE_BOOL
};



enum IROpcodes
{
#define IR_OPCODE(code, sym, name, nKids, nSyms, op, flags, optypes, rtypes,  description) code = (op << 8) + (flags<<4) + (rtypes<<2) + optypes,
	#include "irops.h"
#undef IR_OPCODE

#define nvmOPCODE(id, name, pars, code, consts, desc) id,
	#include "../nbnetvm/opcodes.txt"
#undef nvmOPCODE
};


/*
Opcode Details:
	Operands type:
		[IR_TYPE_INT	= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1
		IR_TYPE_STR		= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0
		IR_TYPE_BOOL	= 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1]
	Return Type:		 |       |       |       |       |
		[IR_TYPE_INT	= 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0
		IR_TYPE_STR		= 0 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0
		IR_TYPE_BOOL	= 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0]
	Flags:				 |       |       |       |       |
		[HIR_TERM      = 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
		HIR_UNOP       = 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0
		HIR_BINOP		= 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0
		HIR_TEROP		= 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 0
		HIR_STMT		= 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0]

		[OP				= X X X X X X X X 0 0 0 0 0 0 0 0]
*/

#define IR_OP_MASK		0xFF00
#define IR_FLAGS_MASK	0x00F0
#define IR_RTYPE_MASK	0x000C
#define IR_TYPE_MASK	0x0003

#define GET_OP(op)         ((op & IR_OP_MASK)    >> 8)
#define GET_OP_FLAGS(op)   ((op & IR_FLAGS_MASK) >> 4)
#define GET_OP_TYPE(op)    ( op & IR_TYPE_MASK)
#define GET_OP_RTYPE(op)   ((op & IR_RTYPE_MASK) >> 2)


extern OpDescr OpDescriptions[];
extern OpDescr NvmOps[];
extern char *IRTypeNames[];

struct Node
{
	uint16		Op;
	Symbol		*Sym;
	Symbol		*SymEx;
	Node		*Kids[3];
	uint32		Value;
	uint32		NRefs;


	Node(uint16 op)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}


	//!\Constructor for leaf nodes
	Node(uint16 op, Symbol *symbol, uint32 value = 0, Symbol *symEx = 0/*,  IRType resType, IRType opType*/)
		:Op(op), Sym(symbol), SymEx(symEx), Value(value), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	Node(uint16 op, uint32 value)
		:Op(op), Sym(0), SymEx(0), Value(value), NRefs(0)/*, ResType(IR_TYPE_INT), OpType(IR_TYPE_INT)*/
	{
		Kids[0] = NULL;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	//!\Constructor for unary operators
	Node(uint16 op, Node	*kidNode, Symbol *sym = 0, uint32 value = 0/*, IRType resType = IR_TYPE_INT, IRType opType = IR_TYPE_INT*/)
		:Op(op), Sym(sym), SymEx(0), Value(value), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		if (kidNode)
		{
			nbASSERT(kidNode->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kidNode->NRefs++;
		}
		Kids[0] = kidNode;
		Kids[1] = NULL;
		Kids[2] = NULL;
	}

	//!\Constructor for binary operators
	Node(uint16 op, Node	*kid1, Node *kid2/*,  IRType resType, IRType opType*/)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = NULL;
	}

	//!\Constructor for references
	Node(uint16 op, Symbol *label, Node *kid1, Node *kid2/*,  IRType resType, IRType opType*/)
		:Op(op), Sym(label), SymEx(0), Value(0), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = NULL;
	}

	//!\Constructor for ternary operators
	Node(uint16 op, Node	*kid1, Node *kid2, Node *kid3/*,  IRType resType, IRType opType*/)
		:Op(op), Sym(0), SymEx(0), Value(0), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		if (kid3)
		{
			nbASSERT(kid3->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid3->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = kid3;
	}

	//!\Constructor for references
	Node(uint16 op, Symbol *label, Node *kid1, Node *kid2, Node *kid3/*,  IRType resType, IRType opType*/)
		:Op(op), Sym(label), SymEx(0), Value(0), NRefs(0)/*, ResType(resType), OpType(opType)*/
	{
		if (kid1)
		{
			nbASSERT(kid1->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid1->NRefs++;
		}
		if (kid2)
		{
			nbASSERT(kid2->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid2->NRefs++;
		}
		if (kid3)
		{
			nbASSERT(kid3->NRefs == 0, "DAGs NOT ALLOWED INSIDE THE PFL IR");
			kid3->NRefs++;
		}
		Kids[0] = kid1;
		Kids[1] = kid2;
		Kids[2] = kid3;
	}


	~Node()
	{
		if (Kids[0])
		{
			delete(Kids[0]);
			Kids[0] = NULL;
		}
		if (Kids[1])
		{
			delete(Kids[1]);
			Kids[1] = NULL;
		}
		if (Kids[2])
		{
			delete(Kids[2]);
			Kids[2] = NULL;
		}
	}

	void SwapChilds(void)
	{
		Node *tmp = Kids[0];
		Kids[0] = Kids[1];
		Kids[1] = tmp;
	}


	Node *GetLeftChild(void)
	{
		return Kids[0];
	}

	Node *GetRightChild(void)
	{
		return Kids[1];
	}

	Node *GetThirdChild(void)
	{
		return Kids[2];
	}

	void SetLeftChild(Node *child)
	{
		Kids[0] = child;
	}

	void SetRightChild(Node *child)
	{
		Kids[1] = child;
	}

	void SetThirdChild(Node *child)
	{
		Kids[2] = child;
	}

	


	bool IsBoolean(void);
	bool IsString(void);
	bool IsInteger(void);
	bool IsTerOp(void);
	bool IsBinOp(void);
	bool IsUnOp(void);
	bool IsTerm(void);
	//bool IsStmt(void);
	bool IsConst(void);

	uint32 GetConstVal(void);
	
	Node *Clone();

	//***********************************************
	PFLMIRNode * translateToPFLMIRNode();

};


bool ReverseCondition(Node **expr);


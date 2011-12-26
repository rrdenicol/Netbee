/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/
#include "lircodegen.h"
#include "tree.h"

using namespace std;



SymbolLabel *MIRCodeGen::NewLabel(LabelKind kind, string name)
{
	uint32 label = m_GlobalSymbols.GetNewLabel(1);
	string lblName;
	if (name.size() == 0)
			lblName = string("l") + int2str(label, 10);
		else
			lblName = name + string("_l") + int2str(label, 10);
	SymbolLabel *lblSym = new SymbolLabel(kind, label, lblName);
	if (lblSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreLabelSym(label, lblSym);
	return lblSym;
}


SymbolTemp *MIRCodeGen::NewTemp(std::string name, u_int32_t &tempCount)
{
	uint32 temp = m_GlobalSymbols.GetNewTemp(1);
	tempCount++;
	string tmpName;
	if (name.size() == 0)
			tmpName = string("t") + int2str(temp, 10);
		else
			tmpName = name + string("_t") + int2str(temp, 10);
	SymbolTemp *tempSym = new SymbolTemp(temp, tmpName);
	if (tempSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreTempSym(temp, tempSym);
	return tempSym;
}



SymbolVarInt *MIRCodeGen::NewIntVar(string name, VarValidity validity)
{
	if (name[0] != '$')
		name = string("$") + name;

	SymbolVarInt *varSym = new SymbolVarInt(name, validity);
	if (varSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreVarSym(name, varSym);
	return varSym;
}


Symbol *MIRCodeGen::ConstIntSymbol(const uint32 value)
{
	SymbolIntConst *constant = m_GlobalSymbols.LookUpConst(value);
	if (constant == NULL)
	{
		constant = new SymbolIntConst(value);
		if (constant == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		m_GlobalSymbols.StoreConstSym(value, constant);
	}

	return constant;
}


Symbol *MIRCodeGen::ConstStrSymbol(const string value)
{
	SymbolStrConst *constant = m_GlobalSymbols.LookUpConst(value);
	if (constant == NULL)
	{
		uint32 len = value.length();
		constant = new SymbolStrConst(value, len, m_GlobalSymbols.IncConstOffs(len));
		if (constant == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		m_GlobalSymbols.StoreConstSym(value, constant);
	}

	return constant;
}


PFLMIRNode *MIRCodeGen::TerOp(uint16 opKind, PFLMIRNode *op1, PFLMIRNode *op2, PFLMIRNode *op3, Symbol *sym)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	PFLMIRNode *node = new PFLMIRNode(opKind, sym, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

PFLMIRNode *MIRCodeGen::BinOp(uint16 opKind, PFLMIRNode *op1, PFLMIRNode *op2, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	PFLMIRNode *node = new PFLMIRNode(opKind, lbl, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


PFLMIRNode *MIRCodeGen::UnOp(uint16 opKind, PFLMIRNode *operand, Symbol *sym)
{
	nbASSERT(operand != NULL, "child node cannot be NULL");
	PFLMIRNode *node = new PFLMIRNode(opKind, operand, sym);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}


PFLMIRNode *MIRCodeGen::TermNode(uint16 nodeKind, Symbol *sym, PFLMIRNode *op1, PFLMIRNode *op2)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	PFLMIRNode *node = new PFLMIRNode(nodeKind, sym, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


PFLMIRNode *MIRCodeGen::TermNode(u_int16_t nodeKind, u_int32_t value)
{
	PFLMIRNode *node = new PFLMIRNode(nodeKind, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


PFLMIRNode *MIRCodeGen::TermNode(u_int16_t nodeKind)
{
	PFLMIRNode *node = new PFLMIRNode(nodeKind);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

void MIRCodeGen::AppendTree(PFLMIRNode *node)
{
	nbASSERT(m_CodeList != NULL, "Cannot perform Append operation when Code List is NULL!!!");
	nbASSERT(m_CodeList->size() != 0, "Code list should contain at least one statement. Call IRCodeGen::Statement before IRCodeGen::AppendTree");
	
	PFLMIRNode *last = *(--m_CodeList->end());
	nbASSERT(last != NULL && last->getKid(0) == NULL, "the current statement already has a linked forest");

	last->setKid(node, 0);
	return;
}


PFLMIRNode *MIRCodeGen::Statement(PFLMIRNode *stmt)
{
	nbASSERT(m_CodeList != NULL, "Cannot generate a statement when Code List is NULL!!!");
	m_CodeList->push_back(stmt);
	stmts++;
	return stmt;
}

/*
StmtBlock *IRCodeGen::BlockStatement(string comment)
{
	StmtBlock *stmtBlck = new StmtBlock(comment);
	CHECK_MEM_ALLOC(stmtBlck);
	Statement(stmtBlck);
	return stmtBlck;

}
*/


LabelPFLMIRNode *MIRCodeGen::LabelStatement(SymbolLabel *lblSym)
{
//	nbASSERT(lblSym->LblKind == LBL_CODE, "Label must be of kind LBL_CODE");
	
	LabelPFLMIRNode *lblStmt =  new LabelPFLMIRNode();
	CHECK_MEM_ALLOC(lblStmt);
	Statement(lblStmt);
	AppendTree(TermNode(IR_LABEL, lblSym));
//	if (lblSym->LblKind == LBL_CODE)
//		lblSym->Code = lblStmt;
	return lblStmt;
}

PFLMIRNode *MIRCodeGen::GenStatement(PFLMIRNode *node)
{
	StmtPFLMIRNode *stmt = new StmtPFLMIRNode(STMT_GEN, node);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}

JumpPFLMIRNode *MIRCodeGen::JumpStatement(SymbolLabel *lblSym)
{
	nbASSERT(lblSym != NULL, "label symbol cannot be null for an unconditional jump");
	
	JumpPFLMIRNode *stmt = new JumpPFLMIRNode(lblSym);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

JumpPFLMIRNode *MIRCodeGen::JumpStatement(uint16 opcode, SymbolLabel *lblSym)
{
	JumpPFLMIRNode *stmt = new JumpPFLMIRNode(lblSym);
	stmt->setOpcode(opcode);
	return stmt;
}


SwitchPFLMIRNode *MIRCodeGen::SwitchStatement(PFLMIRNode *expr)
{
	SwitchPFLMIRNode *stmt = new SwitchPFLMIRNode(expr);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}

JumpPFLMIRNode *MIRCodeGen::JCondStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *op1, PFLMIRNode *op2)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	JumpPFLMIRNode *stmt = new JumpPFLMIRNode(trueLbl, falseLbl, BinOp(cond, op1, op2));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

JumpPFLMIRNode *MIRCodeGen::JFieldStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *op1, PFLMIRNode *op2, PFLMIRNode *size)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	JumpPFLMIRNode *stmt = new JumpPFLMIRNode(trueLbl, falseLbl, TerOp(cond, op1, op2, size));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}




JumpPFLMIRNode *MIRCodeGen::JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *expr)
{
	JumpPFLMIRNode *stmt = new JumpPFLMIRNode(trueLbl, falseLbl, expr);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

CommentPFLMIRNode *MIRCodeGen::CommentStatement(string comment)
{
	CommentPFLMIRNode *stmtComment = new CommentPFLMIRNode(comment);
	CHECK_MEM_ALLOC(stmtComment);
	Statement(stmtComment);
	return stmtComment;
}


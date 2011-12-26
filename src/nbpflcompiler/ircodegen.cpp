/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "defs.h"
#include "ircodegen.h"
#include "errors.h"
#include "tree.h"
#include "statements.h"
#include <string>

using namespace std;

uint32 stmts = 0;

Node *IRCodeGen::ProtoBytesRef(SymbolProto *protoSym, uint32 offs, uint32 len)
{
	nbASSERT(m_GlobalSymbols.GetProtoOffsVar(protoSym) != NULL, "The protocol symbol has not an associated $proto_offs variable");
	nbASSERT(m_GlobalSymbols.PacketBuffSym != NULL, "The $packet variable has not been set");

	Node *protoOffs = TermNode(IR_IVAR, m_GlobalSymbols.GetProtoOffsVar(protoSym));
	SymbolVarBufRef *packetVar = m_GlobalSymbols.PacketBuffSym;
	// startAt = $proto_offs + offs
	Node *startAt = BinOp(IR_ADDI, protoOffs, TermNode(IR_ICONST, ConstIntSymbol(offs)));
	Node *size = TermNode(IR_ICONST, ConstIntSymbol(len));
	// return the resulting ProtoBytesRef node: $packet[$proto_offs + offs:len]
	return TermNode(IR_IVAR, packetVar, startAt, size);
	
}

Node *IRCodeGen::ProtoHIndex(SymbolProto *protoSym, uint32 index)
{
	nbASSERT(m_GlobalSymbols.GetProtoOffsVar(protoSym) != NULL, "The protocol symbol has not an associated $proto_offs variable");
	nbASSERT(m_GlobalSymbols.PacketBuffSym != NULL, "The $packet variable has not been set");

	TermNode(IR_IVAR, m_GlobalSymbols.GetProtoOffsVar(protoSym));
	//SymbolVarBufRef *packetVar = m_GlobalSymbols.PacketBuffSym;


	return TermNode(IR_IVAR);//, packetVar);//, startAt, size);
}
	

SymbolLabel *IRCodeGen::NewLabel(LabelKind kind, string name)
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


SymbolTemp *IRCodeGen::NewTemp(string name, uint32 &tempCount)
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


SymbolVarInt *IRCodeGen::NewIntVar(string name, VarValidity validity)
{
	if (name[0] != '$')
		name = string("$") + name;

	SymbolVarInt *varSym = new SymbolVarInt(name, validity);
	if (varSym == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_GlobalSymbols.StoreVarSym(name, varSym);
	return varSym;
}


Symbol *IRCodeGen::ConstIntSymbol(const uint32 value)
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


Symbol *IRCodeGen::ConstStrSymbol(const string value)
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



Node *IRCodeGen::TerOp(uint16 opKind, Node *op1, Node *op2, Node *op3, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	nbASSERT(op3 != NULL, "child node cannot be NULL");

	Node *node = new Node(opKind, lbl, op1, op2, op3);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

Node *IRCodeGen::BinOp(uint16 opKind, Node *op1, Node *op2, Symbol *lbl)
{
	nbASSERT(op1 != NULL, "child node cannot be NULL");
	nbASSERT(op2 != NULL, "child node cannot be NULL");
	Node *node = new Node(opKind, lbl, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


Node *IRCodeGen::UnOp(uint16 opKind, Node *operand, Symbol *sym, uint32 value)
{
	nbASSERT(operand != NULL, "child node cannot be NULL");
	Node *node = new Node(opKind, operand, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}


Node *IRCodeGen::UnOp(uint16 opKind, Symbol *sym, uint32 value, Symbol *symEx)
{
	nbASSERT(sym != NULL, "symbol cannot be NULL");
	Node *node = new Node(opKind, sym, value, symEx);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	
	return node;
}


Node *IRCodeGen::TermNode(uint16 nodeKind, Symbol *sym, Node *op1, Node *op2)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	Node *node = new Node(nodeKind, sym, op1, op2);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

Node *IRCodeGen::TermNode(uint16 nodeKind, Symbol *sym, uint32 value)
{
	nbASSERT(sym != NULL, "Symbol cannot be NULL");
	Node *node = new Node(nodeKind, sym, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

Node *IRCodeGen::TermNode(uint16 nodeKind, uint32 value)
{
	Node *node = new Node(nodeKind, value);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}


Node *IRCodeGen::TermNode(uint16 nodeKind)
{
	Node *node = new Node(nodeKind);
	if (node == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return node;
}

void IRCodeGen::AppendTree(Node *node)
{
	nbASSERT(m_CodeList != NULL, "Cannot perform Append operation when Code List is NULL!!!");
	nbASSERT(!m_CodeList->Empty(), "Code list should contain at least one statement. Call IRCodeGen::Statement before IRCodeGen::AppendTree");
	
	StmtBase *lastStmt = m_CodeList->Back();
	nbASSERT(lastStmt->Forest == NULL, "the current statement already has a linked forest");

	lastStmt->Forest = node;
	return;
}


StmtBase *IRCodeGen::Statement(StmtBase *stmt)
{
	nbASSERT(m_CodeList != NULL, "Cannot generate a statement when Code List is NULL!!!");
	m_CodeList->PushBack(stmt);
	stmts++;
	return stmt;
}


StmtComment *IRCodeGen::CommentStatement(string comment)
{
	StmtComment *stmtComment = new StmtComment(comment);
	CHECK_MEM_ALLOC(stmtComment);
	Statement(stmtComment);
	return stmtComment;
}

StmtBlock *IRCodeGen::BlockStatement(string comment)
{
	StmtBlock *stmtBlck = new StmtBlock(comment);
	CHECK_MEM_ALLOC(stmtBlck);
	Statement(stmtBlck);
	return stmtBlck;

}

StmtCase *IRCodeGen::CaseStatement(StmtSwitch *switchStmt, Node *constVal)
{
	nbASSERT(switchStmt != NULL, "switchStmt cannot be NULL");
	nbASSERT(constVal != NULL, "constVal cannot be NULL");
	StmtCase *stmtCase = new StmtCase(constVal);
	CHECK_MEM_ALLOC(stmtCase);
	switchStmt->Cases->PushBack(stmtCase);
	return stmtCase;
}

StmtCase *IRCodeGen::DefaultStatement(StmtSwitch *switchStmt)
{
	nbASSERT(switchStmt != NULL, "switchStmt cannot be NULL");
	StmtCase *stmtCase = new StmtCase();
	CHECK_MEM_ALLOC(stmtCase);
	switchStmt->Default = stmtCase;
	return stmtCase;
}

StmtIf *IRCodeGen::IfStatement(Node *condition)
{
	nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtIf *stmtIf = new StmtIf(condition);
	CHECK_MEM_ALLOC(stmtIf);
	Statement(stmtIf);
	return stmtIf;

}

StmtWhile *IRCodeGen::WhileDoStatement(Node *condition)
{
	nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtWhile *stmtWhile = new StmtWhile(STMT_WHILE, condition);
	CHECK_MEM_ALLOC(stmtWhile);
	Statement(stmtWhile);
	return stmtWhile;
}

StmtWhile *IRCodeGen::DoWhileStatement(Node *condition)
{
	//[OM]: The condition is undefined when generating a Do-While statement, so this assert is wrong!
	//nbASSERT(condition != NULL, "condition cannot be NULL");
	StmtWhile *stmtWhile = new StmtWhile(STMT_DO_WHILE, condition);
	CHECK_MEM_ALLOC(stmtWhile);
	Statement(stmtWhile);
	return stmtWhile;
}

StmtLoop *IRCodeGen::LoopStatement(Node *indexVar, Node *initVal, Node *termCond, StmtGen *incStmt)
{
	nbASSERT(indexVar != NULL, "indexVar cannot be NULL");
	nbASSERT(initVal != NULL, "initVal cannot be NULL");
	nbASSERT(termCond != NULL, "termCond cannot be NULL");
	nbASSERT(incStmt != NULL, "incStmt");
	StmtLoop *stmtLoop = new StmtLoop(indexVar, initVal, termCond, incStmt);
	CHECK_MEM_ALLOC(stmtLoop);
	Statement(stmtLoop);
	return stmtLoop;
}

StmtCtrl *IRCodeGen::BreakStatement(void)
{
	StmtCtrl *stmtCtrl = new StmtCtrl(STMT_BREAK);
	CHECK_MEM_ALLOC(stmtCtrl);
	Statement(stmtCtrl);
	return stmtCtrl;
}


StmtCtrl *IRCodeGen::ContinueStatement(void)
{
	StmtCtrl *stmtCtrl = new StmtCtrl(STMT_CONTINUE);
	CHECK_MEM_ALLOC(stmtCtrl);
	Statement(stmtCtrl);
	return stmtCtrl;
}



StmtLabel *IRCodeGen::LabelStatement(SymbolLabel *lblSym)
{
//	nbASSERT(lblSym->LblKind == LBL_CODE, "Label must be of kind LBL_CODE");
	
	StmtLabel *lblStmt =  new StmtLabel();
	CHECK_MEM_ALLOC(lblStmt);
	Statement(lblStmt);
	AppendTree(TermNode(IR_LABEL, lblSym));
	if (lblSym->LblKind == LBL_CODE)
		lblSym->Code = lblStmt;
	return lblStmt;
}

StmtBase *IRCodeGen::GenStatement(Node *node)
{
	StmtBase *stmt = new StmtGen(node);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}


StmtSwitch *IRCodeGen::SwitchStatement(Node *expr)
{
	StmtSwitch *stmt = new StmtSwitch(expr);
	CHECK_MEM_ALLOC(stmt);
	Statement(stmt);
	return stmt;
}


StmtJump *IRCodeGen::JumpStatement(SymbolLabel *lblSym)
{
	nbASSERT(lblSym != NULL, "label symbol cannot be null for an unconditional jump");
	
	StmtJump *stmt = new StmtJump(lblSym);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

StmtJump *IRCodeGen::JumpStatement(uint16 opcode, SymbolLabel *lblSym)
{
	StmtJump *stmt = JumpStatement(lblSym);
	stmt->Opcode = opcode;
	return stmt;
}

StmtJump *IRCodeGen::JCondStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *op1, Node *op2)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	StmtJump *stmt = new StmtJump(trueLbl, falseLbl, BinOp(cond, op1, op2));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}

StmtJump *IRCodeGen::JFieldStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *op1, Node *op2, Node *size)
{
	nbASSERT(trueLbl != NULL, "trueLbl cannot be null for a conditional jump");
	nbASSERT(falseLbl != NULL, "falseLbl cannot be null for a conditional jump");

	StmtJumpField *stmt = new StmtJumpField(trueLbl, falseLbl, TerOp(cond, op1, op2, size));
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}



StmtJump *IRCodeGen::JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *expr)
{
	StmtJump *stmt = new StmtJump(trueLbl, falseLbl, expr);
	if (stmt == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	Statement(stmt);
	return stmt;
}







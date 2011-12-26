/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "symbols.h"
#include "globalsymbols.h"
#include <utility>
#include "statements.h"
#include "tree.h"
using namespace std;


/*!
	\brief 
*/


class IRCodeGen
{	

private:

	GlobalSymbols		&m_GlobalSymbols;
	CodeList			*m_CodeList;

public:
	
	

	//constructor

	IRCodeGen(GlobalSymbols &globalSymbols, CodeList *codeList = 0)
		:m_GlobalSymbols(globalSymbols), m_CodeList(codeList){}

	

	~IRCodeGen(void)
	{
	}

	SymbolTemp *CurrOffsTemp(void)
	{
		nbASSERT(m_GlobalSymbols.CurrentOffsSym != NULL, "CurrentOffsSym cannot be NULL");
		return m_GlobalSymbols.CurrentOffsSym->Temp;
	}

	SymbolVarInt *CurrOffsVar(void)
	{
		return m_GlobalSymbols.CurrentOffsSym;
	}

	Symbol *ConstIntSymbol(uint32 value);
	Symbol *ConstStrSymbol(const string value);
	StmtBase *Statement(StmtBase *stmt);

	void AppendTree(Node *node);
	Node *TerOp(uint16 opKind, Node *op1, Node *op2, Node *op3, Symbol *sym = 0);
	Node *BinOp(uint16 opKind, Node *op1, Node *op2, Symbol *sym = 0);
	Node *UnOp(uint16 opKind, Node *op, Symbol *sym = 0, uint32 value = 0);
	Node *UnOp(uint16 opKind, Symbol *sym, uint32 value = 0, Symbol *symEx = 0);
	Node *TermNode(uint16 nodeKind, Symbol *sym, Node *op1 = 0, Node *op2 = 0);
	Node *TermNode(uint16 nodeKind, Symbol *sym, uint32 value);
	Node *TermNode(uint16 nodeKind, uint32 val);	
	Node *TermNode(uint16 nodeKind);
	Node *ProtoBytesRef(SymbolProto *protoSym, uint32 offs, uint32 len);
	Node *ProtoHIndex(SymbolProto *protoSym, uint32 index);

	StmtLabel *LabelStatement(SymbolLabel *lblSym);
	StmtComment *CommentStatement(string comment);
	StmtJump *JumpStatement(SymbolLabel *lblSym);
	StmtJump *JumpStatement(uint16 opcode, SymbolLabel *lblSym);
	StmtJump *JCondStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *op1, Node *op2);
	StmtJump *JFieldStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *op1, Node *op2, Node *size);
	StmtJump *JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, Node *expr);
	StmtCase *CaseStatement(StmtSwitch *switchStmt, Node *constVal);
	StmtCase *DefaultStatement(StmtSwitch *switchStmt);
	StmtSwitch *SwitchStatement(Node *expr=0);
	StmtIf *IfStatement(Node *condition);
	StmtWhile *WhileDoStatement(Node *condition);
	StmtWhile *DoWhileStatement(Node *condition);
	StmtLoop *LoopStatement(Node *indexVar, Node *initVal, Node *termCond, StmtGen *incStmt);
	StmtCtrl *BreakStatement(void);
	StmtCtrl *ContinueStatement(void);
	StmtBlock *BlockStatement(string comment = "");
	StmtBase *GenStatement(Node *node);
	SymbolLabel *NewLabel(LabelKind kind, string name = "");
	SymbolTemp *NewTemp(string name, uint32 &tempCount);
	SymbolVarInt *NewIntVar(string name, VarValidity validity = VAR_VALID_THISPKT);
	GlobalSymbols &GetGlobalSymbols(void)
	{
		return m_GlobalSymbols;
	}
	
};

extern uint32 stmts;



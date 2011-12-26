#ifndef _LIRCODEGEN_H
#define _LIRCODEGEN_H

#include "pflmirnode.h"
#include "defs.h"
#include "symbols.h"
#include "globalsymbols.h"
#include <utility>


class MIRCodeGen
{	
	typedef PFLMIRNode IR;
private:

	GlobalSymbols		&m_GlobalSymbols;
	std::list<IR*>			*m_CodeList;

	u_int32_t stmts;
public:
	
	

	//constructor

	MIRCodeGen(GlobalSymbols &globalSymbols, std::list<IR*> *codeList = 0)
		:m_GlobalSymbols(globalSymbols), m_CodeList(codeList), stmts(0){}

	

	~MIRCodeGen(void)
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
	PFLMIRNode *Statement(PFLMIRNode *stmt);

	void AppendTree(PFLMIRNode *node);
	//Node *TerOp(uint16 opKind, MIRNode *op1, MIRNode *op2, MIRNode *op3, Symbol *sym = 0);
	PFLMIRNode *TerOp(uint16 opKind, PFLMIRNode *op1, PFLMIRNode *op2, PFLMIRNode *op3, Symbol *sym = 0);
	PFLMIRNode *BinOp(uint16 opKind, PFLMIRNode *op1, PFLMIRNode *op2, Symbol *sym = 0);
	PFLMIRNode *UnOp(uint16 opKind, PFLMIRNode *op, Symbol *sym = 0);
	PFLMIRNode *TermNode(uint16 nodeKind, Symbol *sym, PFLMIRNode *op1 = 0, PFLMIRNode *op2 = 0);
	PFLMIRNode *TermNode(u_int16_t nodeKind, u_int32_t val);	
	PFLMIRNode *TermNode(u_int16_t nodeKind);
	PFLMIRNode *ProtoBytesRef(SymbolProto *protoSym, uint32 offs, uint32 len);

	LabelPFLMIRNode *LabelStatement(SymbolLabel *lblSym);
	CommentPFLMIRNode *CommentStatement(std::string comment);
	JumpPFLMIRNode *JumpStatement(SymbolLabel *lblSym);
	JumpPFLMIRNode *JumpStatement(uint16 opcode, SymbolLabel *lblSym);
	SwitchPFLMIRNode *SwitchStatement(PFLMIRNode *expr=0);
	JumpPFLMIRNode *JCondStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *op1, PFLMIRNode *op2);
	JumpPFLMIRNode *JFieldStatement(uint16 cond, SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *op1, PFLMIRNode *op2, PFLMIRNode *size);
	JumpPFLMIRNode *JCondStatement(SymbolLabel *trueLbl, SymbolLabel *falseLbl, PFLMIRNode *expr);
	PFLMIRNode *GenStatement(PFLMIRNode *node);
	SymbolLabel *NewLabel(LabelKind kind, string name = "");
	SymbolTemp *NewTemp(std::string name, u_int32_t &tempCount);
	SymbolVarInt *NewIntVar(std::string name, VarValidity validity = VAR_VALID_THISPKT);
	GlobalSymbols &GetGlobalSymbols(void)
	{
		return m_GlobalSymbols;
	}
	
};


#endif

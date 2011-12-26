/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "compunit.h"
#include "ircodegen.h"


#define	LOOKUP_EX_OP_INIT			(uint32)0
#define	LOOKUP_EX_OP_INIT_TABLE	1
#define	LOOKUP_EX_OP_ADD_KEY		2
#define	LOOKUP_EX_OP_ADD_VALUE	3
#define	LOOKUP_EX_OP_SELECT		4
#define	LOOKUP_EX_OP_GET_VALUE	5
#define	LOOKUP_EX_OP_UPD_VALUE	6
#define	LOOKUP_EX_OP_DELETE		7
#define	LOOKUP_EX_OP_RESET		8

#define	LOOKUP_EX_OUT_TABLE_ID		(uint32)0
#define	LOOKUP_EX_OUT_ENTRIES		1
#define	LOOKUP_EX_OUT_KEYS_SIZE		2
#define	LOOKUP_EX_OUT_VALUES_SIZE	3
#define	LOOKUP_EX_OUT_GENERIC		1
#define	LOOKUP_EX_OUT_VALUE_OFFSET	1
#define	LOOKUP_EX_OUT_VALUE			6
#define LOOKUP_EX_OUT_FIELD_SIZE    5
#define LOOKUP_EX_OUT_FIELD_VALUE   6

#define LOOKUP_EX_IN_VALID      7
#define	LOOKUP_EX_IN_VALUE		6

#define	REGEX_MATCH					1
#define	REGEX_MATCH_WITH_OFFSET	2
#define	REGEX_GET_RESULT			3

#define	REGEX_OUT_PATTERN_ID		0
#define	REGEX_OUT_BUF_OFFSET		1
#define	REGEX_OUT_BUF_LENGTH		2

#define	REGEX_IN_MATCHES_FOUND	(uint32)0
#define	REGEX_IN_OFFSET_FOUND		2
#define	REGEX_IN_LENGTH_FOUND		3


class IRLowering
{

	struct JCondInfo
	{
		SymbolLabel *TrueLbl;
		SymbolLabel *FalseLbl;

		JCondInfo(SymbolLabel *trueLbl, SymbolLabel *falseLbl)
			:TrueLbl(trueLbl), FalseLbl(falseLbl){}
	};


	struct LoopInfo
	{
		SymbolLabel		*LoopStart;
		SymbolLabel		*LoopExit;

		LoopInfo(SymbolLabel *start, SymbolLabel *exit)
			:LoopStart(start), LoopExit(exit){}
	};


	CompilationUnit	&m_CompUnit;
	IRCodeGen		&m_CodeGen;
	SymbolProto		*m_Protocol;
	GlobalSymbols	&m_GlobalSymbols;
	list<LoopInfo>	m_LoopStack;
	uint32			m_TreeDepth;
	SymbolLabel		*m_FilterFalse;

	SymbolTemp *AssociateVarTemp(SymbolVarInt *var);

	void TranslateLabel(StmtLabel *stmt);

	void TranslateGen(StmtGen *stmt);

	SymbolLabel *ManageLinkedLabel(SymbolLabel *label, string name);

	void TranslateJump(StmtJump *stmt);

	void TranslateSwitch(StmtSwitch *stmt);

	void TranslateCase(StmtSwitch *newSwitchSt, StmtCase *caseSt, SymbolLabel *swExit, bool IsDefault);

	void TranslateCases(StmtSwitch *newSwitchSt, CodeList *cases, SymbolLabel *swExit);

	void TranslateSwitch2(StmtSwitch *stmt);

	void TranslateIf(StmtIf *stmt);

	void TranslateLoop(StmtLoop *stmt);

	void TranslateWhileDo(StmtWhile *stmt);

	void TranslateDoWhile(StmtWhile *stmt);

	void TranslateBreak(StmtCtrl *stmt);

	void TranslateContinue(StmtCtrl *stmt);

	Node *TranslateTree(Node *node);

	Node *TranslateIntVarToInt(Node *node);

	Node *TranslateCInt(Node *node);

	Node *TranslateStrVarToInt(Node *node, Node* offset, uint32 size);

	Node *TranslateFieldToInt(Node *node, Node* offset, uint32 size);

	Node *GenMemLoad(Node *offsNode, uint32 size);

	Node *TranslateConstInt(Node *node);

	Node *TranslateConstStr(Node *node);

	Node *TranslateConstBool(Node *node);

	Node *TranslateTemp(Node *node);

	void TranslateBoolExpr(Node *expr, JCondInfo &jcInfo);

	void TranslateRelOpInt(uint16 op, Node *relopExpr, JCondInfo &jcInfo);

	void TranslateRelOpStr(uint16 op, Node *relopExpr, JCondInfo &jcInfo);

	void TranslateRelOpStrOperand(Node *operand, Node **loadOperand, Node **loadSize, uint32 *size);

	void TranslateRelOpLookup(uint16 opCode, Node *expr, JCondInfo &jcInfo);

	void TranslateRelOpRegEx(Node *expr, JCondInfo &jcInfo);

	bool TranslateLookupSetValue(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *values, bool isKeysList);

	bool TranslateLookupSelect(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *keys);

	void TranslateLookupInitTable(Node *node);

	void TranslateLookupInit(Node *node);

	void TranslateLookupUpdate(Node *node);

	void TranslateLookupDelete(Node *node);

	void TranslateLookupAdd(Node *node);

	void TranslateFieldDef(Node *node);

	void TranslateVarDeclInt(Node *node);

	void TranslateVarDeclStr(Node *node);

	void TranslateAssignInt(Node *node);

	void TranslateAssignStr(Node *node);

	void TranslateAssignRef(SymbolVarBufRef *refVar, Node *right);

	Node *TranslateArithBinOp(uint16 op, Node *node);

	Node *TranslateArithUnOp(uint16 op, Node *node);

	Node *TranslateDiv(Node *node);

	void TranslateStatement(StmtBase *stmt);

	void LowerHIRCode(CodeList *code);

	void EnterLoop(SymbolLabel *start, SymbolLabel *exit);

	void ExitLoop(void);

	void GenTempForVar(Node *node);

	void GenerateInfo(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);

	void GenerateWarning(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);

	void GenerateError(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);

public:

	IRLowering(CompilationUnit &compUnit, IRCodeGen &codeGen, SymbolLabel *filterFalse)
		:m_CompUnit(compUnit), m_CodeGen(codeGen), m_Protocol(0), m_GlobalSymbols(codeGen.GetGlobalSymbols()),
		m_TreeDepth(0), m_FilterFalse(filterFalse)
	{
		m_CompUnit.MaxStack = 0;
		m_CompUnit.NumLocals = 0;
	}



	void LowerHIRCode(CodeList *code, SymbolProto *proto, string comment = "");

	void LowerHIRCode(CodeList *code, string comment);


};


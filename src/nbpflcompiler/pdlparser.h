/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "symbols.h"
#include "ircodegen.h"
#include <nbprotodb.h>
#include "globalsymbols.h"
#include "treeoptimize.h"
#include "compilerconsts.h"
#include "filtersubgraph.h"
#include "errors.h"

#include <utility>

using namespace std;


// use this define to visit or not next-proto candidate instructions
#define VISIT_CANDIDATE_PROTO
// use this define to consider not supported protocols as next proto
//#define VISIT_NEXT_PROTO_DEFINED_ONLY
// use this define to execute the 'before' code as soon as you find a protocol
#define EXEC_BEFORE_AS_SOON_AS_FOUND


enum
{
	PARSER_VISIT_ENCAP = true,
	PARSER_NO_VISIT_ENCAP = false
};


/*!
\brief
*/


class PDLParser
{
private:



	struct FilterInfo
	{
		SymbolLabel			*FilterFalse;
		FilterSubGraph		&SubGraph;

		FilterInfo(SymbolLabel *filterFalse, FilterSubGraph &subGraph)
			:FilterFalse(filterFalse), SubGraph(subGraph){}
	};


	typedef list<IRCodeGen*> CodeGenStack_t;

	struct SwitchCaseLess : public std::binary_function<_nbNetPDLElementCase *, _nbNetPDLElementCase *, bool>
	{
		bool operator()(_nbNetPDLElementCase * left, _nbNetPDLElementCase * right) const
		{
			if(left->ValueNumber < right->ValueNumber)
				return true;
			else
				return false;

		};
	};

	_nbNetPDLDatabase		*m_ProtoDB;			//!<Pointer to the NetPDL protocol database
	GlobalSymbols			&m_GlobalSymbols;
	ErrorRecorder			&m_ErrorRecorder;
	//FldScopeStack_t		*m_FieldScopes;
	IRCodeGen				*m_CodeGen;
	SymbolProto				*m_CurrProtoSym;
	uint32					m_LoopCount;
	uint32					m_IfCount;
	uint32					m_SwCount;
	//	list<LoopInfo>			m_LoopStack;
	bool						m_IsEncapsulation;
	SymbolFieldContainer	*m_ParentUnionField;	// [ds] parsing a field requires to save the current field and try parsing inner fields
	uint32					m_BitFldCount;
	uint32					m_ScopeLevel;
	FilterInfo				*m_FilterInfo;
	CompilerConsts			*m_CompilerConsts;
	EncapGraph			*m_ProtoGraph;
	GlobalInfo				*m_GlobalInfo;

	//ParsingSection			m_CurrentParsingSection;

	SymbolVarInt			*m_nextProtoCandidate;	//!< ID of the next candidate resolved
	SymbolLabel				*m_ResolveCandidateLabel;
	list<int>				m_CandidateProtoToResolve;

	bool						m_ConvertingToInt;

	CodeGenStack_t			m_CodeGenStack;

	bool						m_InnerElementsNotDefined;

	void ParseFieldDef(_nbNetPDLElementFieldBase *fieldElement);
	void ParseAssign(_nbNetPDLElementAssignVariable *assignElement);
	void ParseLoop(_nbNetPDLElementLoop *loopElement);
	Node *ParseExpressionInt(_nbNetPDLExprBase *expr);
	Node *ParseExpressionStr(_nbNetPDLExprBase *expr);
	Node *ParseOperandStr(_nbNetPDLExprBase *expr);
	Node *ParseOperandInt(_nbNetPDLExprBase *expr);
	void ParseRTVarDecl(_nbNetPDLElementVariable *variable);
	void ParseElement(_nbNetPDLElementBase *element);
	void ParseElements(_nbNetPDLElementBase *firstElement);
	void ParseEncapsulation(_nbNetPDLElementBase *firstElement);
	void ParseExecSection(_nbNetPDLElementExecuteX *execSection);
	//	void EnterLoop(SymbolLabel *start, SymbolLabel *exit);
	//	void ExitLoop(void);
	void EnterSwitch(SymbolLabel *next);
	void ExitSwitch(void);
	//	LoopInfo &GetLoopInfo(void);
	void ParseLoopCtrl(_nbNetPDLElementLoopCtrl *loopCtrlElement);
	void ParseIf(_nbNetPDLElementIf *ifElement);
	void ParseVerify(_nbNetPDLElementExecuteX *verifySection);
	void ParseSwitch(_nbNetPDLElementSwitch *switchElement);
	void ParseBoolExpression(_nbNetPDLExprBase *expr);
	void ParseNextProto(_nbNetPDLElementNextProto *element);
	void ParseNextProtoCandidate(_nbNetPDLElementNextProto *element);

	void ParseLookupTable(_nbNetPDLElementLookupTable *table);
	void ParseLookupTableUpdate(_nbNetPDLElementUpdateLookupTable *element);
	Node *ParseLookupTableSelect(uint16 op, string tableName, SymbolLookupTableKeysList *keys);
	void ParseLookupTableAssign(_nbNetPDLElementAssignLookupTable *element);
	Node *ParseLookupTableItem(string tableName, string itemName, Node *offsetStart, Node *offsetSize);

	void CheckVerifyResult(uint32 nextProtoID);

	bool CheckAllowedElement(_nbNetPDLElementBase *element);
	//void StoreFieldInScope(const string name, SymbolField *fieldSymbol);
	//FieldsList_t *LookUpFieldInScope(const string name);
	SymbolField *StoreFieldSym(SymbolField *fieldSymbol);
	SymbolField *StoreFieldSym(SymbolField *fieldSymbol, bool force);
	SymbolField *LookUpFieldSym(const string name);
	SymbolField *LookUpFieldSym(const SymbolField *field);
	//void EnterFldScope(void);
	//void ExitFldScope(void);
	char *GetProtoName(struct _nbNetPDLExprBase *expr);

	void ChangeCodeGen(IRCodeGen &codeGen);
	void RestoreCodeGen(void);

	void GenProtoEntryCode(SymbolProto &protoSymbol);


	void VisitNextProto(_nbNetPDLElementNextProto *element);
	void VisitElement(_nbNetPDLElementBase *element);
	void VisitElements(_nbNetPDLElementBase *firstElement);
	void VisitEncapsulation(_nbNetPDLElementBase *firstElement);
	void VisitSwitch(_nbNetPDLElementSwitch *switchElem);
	void VisitIf(_nbNetPDLElementIf *ifElement);

	//void ParseStartProto(IRCodeGen &IRCodeGen);

	// [ds] added functions to generate debug information, warnings and errors
	void GenerateInfo(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);
	void GenerateWarning(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);
	void GenerateError(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation);


public:

	//constructor

	PDLParser(_nbNetPDLDatabase &protoDB, GlobalSymbols &globalSymbols, ErrorRecorder &errRecorder);



	~PDLParser(void)
	{
	}

	void ParseStartProto(IRCodeGen &IRCodeGen, bool visitEncap);
	void ParseProtocol(SymbolProto &protoSymbol, IRCodeGen &IRCodeGen, bool visitEncap);
	void ParseEncapsulation(SymbolProto &protoSymbol, FilterSubGraph &subGraph, SymbolLabel *filterFalse, IRCodeGen &IRCodeGen);
	void ParseExecBefore(SymbolProto &protoSymbol, FilterSubGraph &subGraph, SymbolLabel *filterFalse, IRCodeGen &IRCodeGen);


};

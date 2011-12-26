/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "defs.h"
#include "symbols.h"
#include "errors.h"
#include <nbprotodb.h>
#include "ircodegen.h"
#include "globalsymbols.h"
#include "digraph.h"
#include "compunit.h"
#include "pflexpression.h"
#include "pdlparser.h"
#include "irlowering.h"
#include "pflcfg.h"

#include <map>

using namespace std;



/*!
\brief This class represents the NPFL NetPFLFrontEnd
*/

typedef map<EncapGraph::GraphNode*, EncapGraph::GraphNode*> NodeMap_t;


class NetPFLFrontEnd//: public EncapGraph::IGraphVisitor
{
private:

	struct FilterCodeGenInfo
	{
		FilterSubGraph			&SubGraph;
		SymbolLabel				*FilterFalseLbl;
		SymbolLabel				*FilterTrueLbl;
		//SymbolProto				*Protocol;

		FilterCodeGenInfo(FilterSubGraph &subGraph, SymbolLabel *filterTrue, SymbolLabel *filterFalse)
			:SubGraph(subGraph), FilterFalseLbl(filterFalse),
			FilterTrueLbl(filterTrue)/*, Protocol(proto)*/ {}

	};





	_nbNetPDLDatabase   *m_ProtoDB;			//!< Reference to the NetPDL protocol database
	ErrorRecorder       m_ErrorRecorder;	//!< The Error Recorder (for tracking compilation errors/warnings)
	GlobalInfo          m_GlobalInfo;
	GlobalSymbols       m_GlobalSymbols;
	CompilationUnit	    *m_CompUnit;
	IRCodeGen           *m_LIRCodeGen;
	IRLowering          *m_NetVMIRGen;
	bool                m_StartProtoGenerated;
	//FilterCodeGenInfo *m_ProtoFilterInfo;
	//PDLParser         *m_PDLParser;
	PDLParser           NetPDLParser;
	ostream             *HIR_FilterCode;
	ostream	            *NetVMIR_FilterCode;
	ostream	            *PFL_Tree;
	string              NetIL_FilterCode;
	FieldsList_t        m_FieldsList;   //!< Fields that will be extracted

#ifdef OPTIMIZE_SIZED_LOOPS
	FieldsList_t        m_ReferredFieldsInFilter;
	FieldsList_t        m_ReferredFieldsInCode;
	uint32              m_DefinedReferredFieldsInFilter;
	bool                m_ParsingFilter;
	bool                m_ContinueParsingCode;
	SymbolProto         *m_Protocol;
#endif


	void GenProtoCode(SymbolProto *proto, FilterCodeGenInfo &protoFilterInfo, bool encap = true, bool fieldExtract = false);

	void GenCode(PFLStatement *filterExpr);

	void GenProtoHIR(EncapGraph::GraphNode &node, FilterCodeGenInfo &protoFilterInfo, bool encap, bool fieldExtract);


	void CloneHIR(CodeList &code, IRCodeGen &codegen);

	void GenInitCode(void);

	void GenRegexEntries(void);

	void DumpPFLTreeDotFormat(PFLExpression *expr, ostream &stream);

	void VisitFilterExpression(PFLExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel);

	void VisitFilterBinExpr(PFLBinaryExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel);

	void VisitFilterUnExpr(PFLUnaryExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel);

	void GenFieldExtract(NodeList_t &protocols, SymbolLabel *trueLabel, SymbolLabel *falseLabel);

	void VisitFilterTermExpr(PFLTermExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel);

	void ParseExtractField(FieldsList_t *fldList);

	void VisitAction(PFLAction *action);

	void SetupCompilerConsts(CompilerConsts &compilerConsts, uint16 linkLayer);

	PFLStatement *ParseFilter(string filter);

#ifdef OPTIMIZE_SIZED_LOOPS
	void GetReferredFields(CodeList *code)
	{
		m_ParsingFilter= false;
		m_DefinedReferredFieldsInFilter=0;

		VisitCode(code);
	}

	void GetReferredFieldsInBoolExpr(Node *expr)
	{
		m_ReferredFieldsInFilter.clear();
		m_ParsingFilter= true;
		VisitBoolExpression(expr);
		m_ParsingFilter= false;
	}

	void GetReferredFieldsExtractField(SymbolProto *proto)
	{
		CodeList *code = &m_GlobalSymbols.GetFieldExtractCode(proto);
		m_ContinueParsingCode= true;
	    VisitCode(code);
		m_ReferredFieldsInFilter.unique();
	}

	void VisitCode(CodeList *code)
	{
		StmtBase *next = code->Front();

		while(next && m_ContinueParsingCode)
		{
			VisitStatement(next);
			next = next->Next;
		}
	}

	void VisitStatement(StmtBase *stmt)
	{
		switch(stmt->Kind)
		{
		case STMT_GEN:
			VisitGen((StmtGen*)stmt);
			break;
		case STMT_JUMP:
			VisitJump((StmtJump*)stmt);
			break;
		case STMT_SWITCH:
			VisitSwitch((StmtSwitch*)stmt);
			break;
		case STMT_IF:
			VisitIf((StmtIf*)stmt);
			break;
		case STMT_LOOP:
			VisitLoop((StmtLoop*)stmt);
			break;
		case STMT_WHILE:
			VisitWhileDo((StmtWhile*)stmt);
			break;
		case STMT_DO_WHILE:
			VisitDoWhile((StmtWhile*)stmt);
			break;
		case STMT_FINFOST:
			VisitFldInfo((StmtFieldInfo*)stmt);
			break;
		default:
			break;
		}
	}

	void VisitLoop(StmtLoop *stmt)
	{
		VisitTree(stmt->Forest);
		VisitTree(stmt->InitVal);

		//loop body
		VisitCode(stmt->Code->Code);

		//<inc_statement>
		VisitStatement(stmt->IncStmt);

		VisitBoolExpression(stmt->TermCond);
	}

	void VisitWhileDo(StmtWhile *stmt)
	{
		VisitCode(stmt->Code->Code);

		VisitBoolExpression(stmt->Forest);
	}

	void VisitDoWhile(StmtWhile *stmt)
	{
		VisitCode(stmt->Code->Code);

		VisitBoolExpression(stmt->Forest);
	}

	void VisitIf(StmtIf *stmt)
	{
		Node *expr = stmt->Forest;

		VisitBoolExpression(expr);

		if (stmt->TrueBlock->Code->Empty()==false)
		{
			VisitCode(stmt->TrueBlock->Code);

			if(stmt->FalseBlock->Code->Empty()==false)
			{
				//code for the false branch
				VisitCode(stmt->FalseBlock->Code);
			}
		}
	}

	void VisitJump(StmtJump *stmt)
	{
		VisitBoolExpression(stmt->Forest);
	}

	void VisitSwitch(StmtSwitch *stmt)
	{
		VisitTree(stmt->Forest);

		VisitCases(stmt->Cases);
		if (stmt->Default != NULL)
		{
			VisitCase(stmt->Default, true);
		}
	}

	void VisitCases(CodeList *cases)
	{
		StmtBase *caseSt = cases->Front();
		while(caseSt)
		{
			VisitCase((StmtCase*)caseSt, false);
			caseSt = caseSt->Next;
		}
	}

	void VisitCase(StmtCase *caseSt, bool IsDefault)
	{
		//if the code for the current case is empty we drop it
		//! \todo manage the number of cases!
		if (caseSt->Code->Code->Empty())
			return;

		if (!IsDefault)
		{
			VisitTree(caseSt->Forest);
		}

		VisitCode(caseSt->Code->Code);
	}


	void VisitGen(StmtGen *stmt)
	{
		Node *tree = stmt->Forest;

		switch (tree->Op)
		{
		case IR_DEFFLD:
			VisitFieldDef(tree);
			break;

		case IR_ASGNI:
			VisitAssignInt(tree);
			break;

		case IR_ASGNS:
			VisitAssignStr(tree);
			break;

			//case IR_LKADD:
			//	TranslateLookupAdd(tree);
			//	break;

			//case IR_LKDEL:
			//	TranslateLookupDelete(tree);
			//	break;

			//case IR_LKHIT:
			//case IR_LKSEL:
			//	{
			//		SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)tree->Sym;
			//		nbASSERT(tree->GetLeftChild() != NULL, "Lookup select instruction should specify a keys list");
			//		SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)tree->GetLeftChild()->Sym;
			//		TranslateLookupSelect(entry, keys);
			//	}break;

			//case IR_LKUPDS:
			//case IR_LKUPDI:
			//	TranslateLookupUpdate(tree);
			//	break;

			//case IR_LKINIT:
			//	TranslateLookupInit(tree);
			//	break;

			//case IR_LKINITTAB:
			//	TranslateLookupInitTable(tree);
			//	break;
		}
	}

	void VisitAssignInt(Node *node)
	{
		node->GetLeftChild();
		// Node *leftChild = node->GetLeftChild();
		Node *rightChild = node->GetRightChild();

		VisitTree(rightChild);
	}

	void VisitAssignStr(Node *node)
	{
		Node *leftChild = node->GetLeftChild();
		node->GetRightChild();
		// Node *rightChild = node->GetRightChild();
		SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;

		switch (varSym->VarType)
		{
		case PDL_RT_VAR_REFERENCE:
			{
				// SymbolVarBufRef *ref=(SymbolVarBufRef *)varSym;
				SymbolField *referee=(SymbolField *)node->Sym;

				switch (referee->FieldType)
				{
				case PDL_FIELD_VARLEN:
					{	
					// SymbolFieldVarLen *varLenField=(SymbolFieldVarLen *)referee;
					break;
					}
				case PDL_FIELD_TOKEND:
					{
					// SymbolFieldTokEnd *tokEndField=(SymbolFieldTokEnd *)referee;
					break;
					}
				case PDL_FIELD_TOKWRAP:
					{
					// SymbolFieldTokWrap *tokWrapField=(SymbolFieldTokWrap *)referee;
					break;
					}
				case PDL_FIELD_LINE:
					{
					// SymbolFieldLine *lineField=(SymbolFieldLine *)referee;
					break;
					}
				case PDL_FIELD_PATTERN:
					{
					// SymbolFieldPattern *patternField=(SymbolFieldPattern *)referee;
					break;
					}
				case PDL_FIELD_EATALL:
					{
					// SymbolFieldEatAll *eatAllField=(SymbolFieldEatAll *)referee;
					break;
					}
				default:
					break;
				}
			}
		default:
			break;
		}
	}


	void VisitFieldDef(Node *node)
	{
		Node *leftChild = node->GetLeftChild();
		SymbolField *fieldSym = (SymbolField*)leftChild->Sym;

		SymbolField *sym=this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, fieldSym);

		switch(fieldSym->FieldType)
		{
		case PDL_FIELD_FIXED:
			break;
		case PDL_FIELD_VARLEN:
			{
				SymbolFieldVarLen *varlenFieldSym = (SymbolFieldVarLen*)sym;

				VisitTree(varlenFieldSym->LenExpr);
			}break;

		case PDL_FIELD_TOKEND:
			{
				SymbolFieldTokEnd *tokendFieldSym = (SymbolFieldTokEnd*)sym;
				if(tokendFieldSym->EndOff!=NULL)
					VisitTree(tokendFieldSym->EndOff);
				if(tokendFieldSym->EndDiscard!=NULL)
					VisitTree(tokendFieldSym->EndDiscard);
			}break;


		case PDL_FIELD_TOKWRAP:
			{
				SymbolFieldTokWrap *tokwrapFieldSym = (SymbolFieldTokWrap*)sym;

				if(tokwrapFieldSym->BeginOff!=NULL)
					VisitTree(tokwrapFieldSym->BeginOff);
				if(tokwrapFieldSym->EndOff!=NULL)
					VisitTree(tokwrapFieldSym->EndOff);
				if(tokwrapFieldSym->EndDiscard!=NULL)
					VisitTree(tokwrapFieldSym->EndDiscard);

			}break;

		case PDL_FIELD_LINE:
		case PDL_FIELD_PATTERN:
		case PDL_FIELD_EATALL:
			break;
		default:
			break;
		}

		bool usedField=false;
		for (FieldsList_t::iterator i=m_ReferredFieldsInFilter.begin(); i!=m_ReferredFieldsInFilter.end() && usedField==false; i++)
		{
			if ( (*i)==sym)
			{
				usedField=true;
			}
		}

		if (usedField)
		{
			m_DefinedReferredFieldsInFilter++;

			if (m_DefinedReferredFieldsInFilter==m_ReferredFieldsInFilter.size())
				m_ContinueParsingCode=false;
		}

	}

	void VisitFldInfo(StmtFieldInfo* stmt)
	{
		if(stmt->Field->FieldType==PDL_FIELD_ALLFIELDS)
		{
			SymbolProto * proto=stmt->Field->Protocol;
			SymbolField **list=m_GlobalSymbols.GetFieldList(proto);
			for(int i =0;i < (int)m_GlobalSymbols.GetNumField(proto);i++)
			{
			   m_ReferredFieldsInFilter.push_front(list[i]);
			}

		}
		else
			m_ReferredFieldsInFilter.push_front(stmt->Field);
	}

	void VisitBoolExpression(Node *expr)
	{
		switch(expr->Op)
		{
		case IR_NOTB:
			VisitBoolExpression(expr->GetLeftChild());
			break;
		case IR_ANDB:
		case IR_ORB:
			VisitBoolExpression(expr->GetLeftChild());
			VisitBoolExpression(expr->GetRightChild());
			break;
		case IR_EQI:
		case IR_GEI:
		case IR_GTI:
		case IR_LEI:
		case IR_LTI:
		case IR_NEI:
		case IR_EQS:
		case IR_NES:
		case IR_GTS:
		case IR_LTS:
			VisitTree(expr->GetLeftChild());
			VisitTree(expr->GetRightChild());
			break;
			//case IR_IVAR:
			//	{
			//		SymbolVariable *sym = (SymbolVariable *)expr->Sym;
			//	};break;
		case IR_CINT:
			VisitCInt(expr->GetLeftChild());
			break;
		}
	}

	void VisitTree(Node *node)
	{
		switch(node->Op)
		{
			//case IR_IVAR:
			//	TranslateIntVarToInt(node);
			//	break;
		case IR_CINT:
			VisitCInt(node);
			break;
		case IR_FIELD:
			SymbolField *fieldSym = (SymbolField*)node->Sym;
			if (m_ParsingFilter)
			{
				m_ReferredFieldsInFilter.push_front(fieldSym);
			}
			else
			{
				m_ReferredFieldsInCode.push_front(fieldSym);
			}
			break;
			//case IR_SVAR:
			//	{
			//		SymbolVarBufRef *ref = (SymbolVarBufRef*)node->Sym;
			//	}break;
		}
	}

	void VisitCInt(Node *node)
	{
		node->GetLeftChild();
		// Node *child = node->GetLeftChild();

		switch (node->Op)
		{
			//case IR_IVAR:
			//	return TranslateIntVarToInt(child);
			//	break;
			//case IR_SVAR:
			//	return TranslateStrVarToInt(child, offsNode, size);
			//	break;
		case IR_FIELD:
			SymbolField *fieldSym = (SymbolField*)node->Sym;
			if (m_ParsingFilter)
			{
				m_ReferredFieldsInFilter.push_front(fieldSym);
			}
			else
			{
				m_ReferredFieldsInCode.push_front(fieldSym);
			}
			break;
		}
	}
#endif



public:
	/*!
	\brief	Object constructor

	\param	protoDB		reference to a NetPDL protocol database

	\param	errRecorder reference to an ErrorRecorder for tracking errors and warnings

	\return nothing
	*/

	NetPFLFrontEnd(_nbNetPDLDatabase &protoDB, nbNetPDLLinkLayer_t LinkLayer,
		const unsigned int DebugLevel=0, const char *dumpHIRCodeFilename= NULL, const char *dumpLIRNoOptGraphFilename=NULL, const char *dumpProtoGraphFilename=NULL);

	/*!
	\brief	Object destructor
	*/
	~NetPFLFrontEnd();

    bool CheckFilter(string filter);

	bool CompileFilter(string filter, bool optimizationCycles = true);

	ErrorRecorder &GetErrRecorder(void)
	{
		return m_ErrorRecorder;
	}

	string &GetNetILFilter(void)
	{
		return NetIL_FilterCode;
	}



	void DumpCFG(ostream &stream, bool graphOnly, bool netIL);

	void DumpFilter(ostream &stream, bool netIL);

	FieldsList_t GetExField(void);

	SymbolField* GetFieldbyId(const string protoName, int index);


};


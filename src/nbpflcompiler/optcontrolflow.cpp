/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "optcontrolflow.h"
#include "cfg.h"
#include "ircodegen.h"
#include "dump.h"


OptControlFlow::OptControlFlow(CFG &cfg, GlobalSymbols &globalSyms)
		:m_CFG(cfg), m_Changed(0), m_GlobalSyms(globalSyms){}



void OptControlFlow::VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom)
{
	
}


void OptControlFlow::VisitBBEdge(BasicBlock *from, BasicBlock *to)
{
}


void OptControlFlow::RemConstBranch(BasicBlock *bb)
{
	if (bb->Code.Empty())
		return;

	SymbolLabel *branch2Keep(0);
	CFGBuilder cfgBuild(m_CFG);
	CodeWriter cw(cout);
	IRCodeGen codeGen(m_GlobalSyms, &bb->Code);

	StmtBase *lastStmt = bb->Code.Back();
	/*
	printf("  Trying to remove statement: ");
	cw.DumpStatement(lastStmt);
	cout <<endl;
	*/
	switch (lastStmt->Kind)
	{
		case STMT_SWITCH:
		{
			nbASSERT(lastStmt->Forest != NULL, "forest should not be null for a switch");
			if (!lastStmt->Forest->IsConst())
				return;
			uint32 val = lastStmt->Forest->GetConstVal();
			StmtSwitch *swStmt = (StmtSwitch*)lastStmt;
			StmtCase *caseStmt = (StmtCase*)swStmt->Cases.Front();
			while (caseStmt)
			{
				if (caseStmt->Forest->GetConstVal() == val)
				{
					branch2Keep = caseStmt->Target;
				}
				else
				{
					bool res = cfgBuild.RemEdge(bb->StartLabel, caseStmt->Target);
					nbASSERT(res, "Trying to remove a non-existent edge");
				}
				caseStmt = (StmtCase*)caseStmt->Next;
			}
			if (swStmt->Default)
			{
				if (branch2Keep != NULL)
				{
					bool res = cfgBuild.RemEdge(bb->StartLabel, swStmt->Default->Target);
					nbASSERT(res, "Trying to remove a non-existent edge");
				}
				else
				{
					branch2Keep = swStmt->Default->Target;
				}
			}
			bb->Code.Remove(swStmt);
		}break;
		case STMT_JUMP:
		{
			if (lastStmt->Forest == NULL)
				return;

			StmtJump *jump = (StmtJump*)lastStmt;
			nbASSERT(jump->Forest->GetLeftChild() != NULL && jump->Forest->GetRightChild() != NULL, "Operator should be binary");
			Node *left = jump->Forest->GetLeftChild();
			if (!left->IsConst())
				return;
			Node *right = jump->Forest->GetRightChild();
			if (!right->IsConst())
				return;
			uint32 lval = left->GetConstVal();
			uint32 rval = right->GetConstVal();
			bool val = false;
			switch (jump->Forest->Op)
			{
				case JCMPEQ:
					val = (lval == rval);
					break;
				case JCMPNEQ:
					val = (lval != rval);
					break;
				case JCMPG:
					val = (lval > rval);
					break;
				case JCMPGE:
					val = (lval >= rval);
					break;
				case JCMPL:
					val = (lval < rval);
					break;
				case JCMPLE:
					val = (lval <= rval);
					break;
				default:
					nbASSERT(false, "CANNOT BE HERE");
					break;
			}
			if (val)
			{
				branch2Keep = jump->TrueBranch;
				bool res = cfgBuild.RemEdge(bb->StartLabel, jump->FalseBranch);
					nbASSERT(res, "Trying to remove a non-existent edge");
			}
			else
			{
				branch2Keep = jump->FalseBranch;
				bool res = cfgBuild.RemEdge(bb->StartLabel, jump->TrueBranch);
					nbASSERT(res, "Trying to remove a non-existent edge");
			}
			bb->Code.Remove(jump);
		}break;
		default:
			return;
			break;
	}
	if (branch2Keep)
	{
		m_Changed = true;
		StmtJump *stmt = codeGen.JumpStatement(JUMPW, branch2Keep);
		/*
		printf("  substituted with: ");
		cw.DumpStatement(stmt);
		cout <<endl;
		cout.flush();
		*/

	}
}

void OptControlFlow::RemConstBranches(void)
{
	list<BasicBlock*> bbList;
	m_CFG.GetBBList(bbList);

	m_Changed = false;

	list<BasicBlock*>::iterator i = bbList.begin();
	for (i; i != bbList.end(); i++)
		RemConstBranch(*i);

	m_CFG.RemoveDeadNodes();
}

void OptControlFlow::RemBranch2Branch(void)
{
}
/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "optdeadcode.h"

OptDeadCodeElim::OptDeadCodeElim(CFG &cfg, uint32 nvars)
		:m_CFG(cfg), m_NVars(nvars), m_Changed(0)
{
	uint32 numBB = cfg.GetBBCount();
	m_BBLiveInfo = new BBInfo[numBB];
	if (m_BBLiveInfo == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	for (uint32 i=0; i < numBB; i++)
		m_BBLiveInfo[i].Init(nvars);
}


void OptDeadCodeElim::Apply(void)
{
	CFGVisitor visitor(*this);
	m_Changed = 0;	
	visitor.VisitPostOrder(m_CFG);
}


void OptDeadCodeElim::VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom)
{
	//cout << "Visiting bb " << bb->StartLabel->Name << endl;
	list<BasicBlock*> succ;
	m_CFG.GetSuccessors(bb, succ);
	for (list<BasicBlock*>::iterator i = succ.begin(); i != succ.end(); i++)
	{
		(*m_BBLiveInfo[bb->ID].LiveOut) |= (*m_BBLiveInfo[(*i)->ID].LiveIn);
	}

	nbBitVector liveOut = *m_BBLiveInfo[bb->ID].LiveOut;
	string liveoutset = liveOut.ToString();
	/*
		if (liveoutset.length() > 0)
			cout << "live-out: " << liveoutset << endl << endl;
	*/
	StmtBase *stmt = bb->Code.Back();
	while (stmt)
	{
		uint32 defvar = 0;
		if (!(stmt->Flags & STMT_FL_DEAD))
		{
			if (GetStmtDef(stmt, defvar))
			{
				if (!liveOut.Test(defvar))
				{
					stmt->Flags |= STMT_FL_DEAD;
					m_Changed = true;
				}
				liveOut.Reset(defvar); //kill the defined var
			}
			GetStmtRefs(stmt, liveOut);
			string liveoutset = liveOut.ToString();
			/*
			if (liveoutset.length() > 0)
				cout << "live-out: " << liveoutset << endl;
			*/
		}
		stmt = stmt->Prev;
	}
	*m_BBLiveInfo[bb->ID].LiveIn = liveOut;
	string liveinset = m_BBLiveInfo[bb->ID].LiveIn->ToString();
	/*
	if (liveinset.length() > 0)
		cout << "live-in: " << liveinset << endl;
	*/

	

}

bool OptDeadCodeElim::GetStmtDef(StmtBase *stmt, uint32 &var)
{
	if (stmt->Kind != STMT_GEN)
		return false;

	if (stmt->Forest->Op == LOCST)
	{
		var = ((SymbolTemp*)stmt->Forest->Sym)->Temp;
		return true;
	}

	return false;
}


void OptDeadCodeElim::GetStmtRefs(StmtBase *stmt, nbBitVector &liveOut)
{
	if (stmt->Forest)
		GetTreeRefs(stmt->Forest, liveOut);
	
}

void OptDeadCodeElim::GetTreeRefs(Node *node, nbBitVector &liveOut)
{
	Node *left = node->GetLeftChild();
	if (left)
		GetTreeRefs(left, liveOut);
	Node *right = node->GetRightChild();
	if (right)
		GetTreeRefs(right, liveOut);

	if (node->Op == LOCLD)
		liveOut.Set(((SymbolTemp*)node->Sym)->Temp);
}


void OptDeadCodeElim::VisitBBEdge(BasicBlock *from, BasicBlock *to)
{
}

/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "optconstprop.h"
#include "dump.h"


bool operator==(const DefPoint &left, const DefPoint &right)
{
	if (left.Def != right.Def)
		return false;
	if (left.BB != right.BB)
		return false;
	if (left.Value != right.Value)
		return false;
	return true;
}

bool operator<(const DefPoint &left, const DefPoint &right)
{
	if (left.Def >= right.Def)
		return false;
	if (left.BB >= right.BB)
		return false;
	if (left.Value >= right.Value)
		return false;
	return true;
}


OptConstantPropagation::OptConstantPropagation(CFG &cfg, IRCodeGen &codegen, uint32 nvars)
		:m_CFG(cfg), m_NVars(nvars), m_CodeGen(codegen), m_Changed(0), m_TreeOpt(codegen)
{
	uint32 numBB = cfg.GetBBCount();
	m_BBDefsInfo = new BBInfo[numBB];
	if (m_BBDefsInfo == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	for (uint32 i=0; i < numBB; i++)
		m_BBDefsInfo[i].Init(nvars);
}


void OptConstantPropagation::Apply(void)
{
	CFGVisitor visitor(*this);
	m_Changed = 0;	
	visitor.VisitRevPostOrder(m_CFG);
}

void OptConstantPropagation::MergeBBInfoIn(BBInfo &dst, BBInfo &src)
{
	(*dst.Reach) |= (*src.Reach);
	for(uint32 i = 0; i < m_NVars; i++)
	{
		dst.Defs[i].merge(src.Defs[i]);
	}
}

void OptConstantPropagation::VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom)
{
	//cout << "Visiting bb " << bb->StartLabel->Name << endl;
	list<BasicBlock*> pred;
	m_CFG.GetPredecessors(bb, pred);
	m_BBDefsInfo[bb->ID].Reset();

	list<BasicBlock*>::iterator i = pred.begin();

	for (i; i != pred.end(); i++)
	{
		MergeBBInfoIn(m_BBDefsInfo[bb->ID], m_BBDefsInfo[(*i)->ID]);
	}

	//cout << "Vars that are constants: " << m_BBDefsInfo[bb->ID].Reach->ToString() << endl;

	StmtBase *stmt = bb->Code.Front();
	CodeWriter cw(cout);
	while (stmt)
	{
		uint32 defvar = 0;
		if (!(stmt->Flags & STMT_FL_DEAD))
		{
			/*
			printf("Stmt before: ");
			cw.DumpStatement(stmt);
			cout<<endl;
			cout.flush();
			*/
			GetStmtDef(stmt, m_BBDefsInfo[bb->ID]);
			/*
			printf("Stmt after: ");
			cw.DumpStatement(stmt);
			cout << endl<<endl;
			cout.flush();
			*/
			
		}
		stmt = stmt->Next;
	}

}

bool OptConstantPropagation::GetStmtDef(StmtBase *stmt, BBInfo &bbinfo)
{
	CodeWriter cw(cout);
	uint32 var = 0;
	if (stmt->Forest == NULL)
		return false;

	m_TreeOpt.OptimizeTree(stmt->Forest);
	/*
	printf("  Stmt after tree optimizations: ");
	cw.DumpStatement(stmt);
	cout << endl;
	cout.flush();
	*/
	GetStmtRefs(stmt, bbinfo);
	/*
	printf("  Stmt after const prop: ");
	cw.DumpStatement(stmt);
	cout << endl;
	cout.flush();
	*/
	if (stmt->Forest != NULL)
		m_TreeOpt.OptimizeTree(stmt->Forest);
	/*
	printf("  Stmt after tree optimizations: ");
	cw.DumpStatement(stmt);
	cout << endl;
	cout.flush();
	*/
	if (stmt->Forest->Op == LOCST)
	{
		Node *node = stmt->Forest->GetLeftChild();
		nbASSERT(node != NULL, "node should not be null");
		var = ((SymbolTemp*)stmt->Forest->Sym)->Temp;
		if (node->IsConst())
		{
			bbinfo.GenDef(var, node->Value, stmt);
			return true;
		}

		bbinfo.KillDef(var);

		return false;
	}
	return false;
}


void OptConstantPropagation::GetStmtRefs(StmtBase *stmt, BBInfo &bbinfo)
{
	if (stmt->Forest)
		stmt->Forest = GetTreeRefs(stmt->Forest, bbinfo);
	
}

Node  *OptConstantPropagation::GetTreeRefs(Node *node, BBInfo &bbinfo)
{
	Node *leftnode = node->GetLeftChild();
	if (leftnode)
		node->SetLeftChild(GetTreeRefs(leftnode, bbinfo));
	Node *rightnode = node->GetRightChild();
	if (rightnode)
		node->SetRightChild(GetTreeRefs(rightnode, bbinfo));

	if (node->Op == LOCLD)
	{
		uint32 var = ((SymbolTemp*)node->Sym)->Temp;
		if (bbinfo.Reach->Test(var) && bbinfo.Defs[var].size() == 1)
		{
			//the variable is a constant!!!
			DefPoint &defpt = bbinfo.Defs[var].front();
			Node *newNode = m_CodeGen.TermNode(PUSH, defpt.Value);
			if (newNode == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");		
			/*
			TODO [OM] (FIXME) the following is not always correct
				We should eliminate the original statement only if the variable that is constant is not
				live-out of the current statement!!!
			*/
			//defpt.Def->Flags = STMT_FL_DEAD;
			m_Changed = true;
			delete node;
			return newNode;
		}
	}
	return node;
}


void OptConstantPropagation::VisitBBEdge(BasicBlock *from, BasicBlock *to)
{
}

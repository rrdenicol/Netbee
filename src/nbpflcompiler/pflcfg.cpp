/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "pflcfg.h"
#include "errors.h"
#include "tree.h"
#include "cfgwriter.h"
#include "node_translation.h"

#include <iostream>
using namespace std;


PFLBasicBlock::~PFLBasicBlock()
{
	typedef list<PFLMIRNode*>::iterator it_t;
	for(it_t i = PFLMIRNodeCode->begin(); i != PFLMIRNodeCode->end(); i++)
	{
		StmtPFLMIRNode *stmt = dynamic_cast<StmtPFLMIRNode*>(*i);
		delete stmt;
	}

	delete PFLMIRNodeCode;
	//if(StartLabel)
	//	delete StartLabel;
}

void PFLBasicBlock::addHeadInstruction(PFLMIRNode *node)
{
	assert(node != NULL && "Adding a NULL node to a BasicBlock");
	//std::cout << "Aggiungo un nodo al BB: " << getId() << std::endl;
	PFLMIRNodeCode->push_front(node);
}

void PFLBasicBlock::addTailInstruction(PFLMIRNode *node)
{
	assert(node != NULL && "Adding a NULL node to a BasicBlock");
	PFLMIRNodeCode->push_back(node);
}

PFLCFG::PFLCFG()
	: jit::CFG<PFLMIRNode, PFLBasicBlock>(std::string("pflCFG"), true), EntryLbl(LBL_CODE, 0, "ENTRY"), ExitLbl(LBL_CODE, 0, "EXIT"), m_BBCount(0), m_CurrMacroBlock(0)
{
	entryNode = NewBasicBlock(BB_CFG_ENTRY, &EntryLbl);

	exitNode = NewBasicBlock(BB_CFG_EXIT, &ExitLbl);
	
}


PFLCFG::~PFLCFG()
{
	//debug
	//cout << "PFLCFG distrutto" << endl;
#if 0
	uint32_t size = this->m_NodeList.size();
	for (uint32_t i = 0; i < size; i++)
	{
		if (this->m_NodeList[i] != NULL)
			delete this->m_NodeList[i]->NodeInfo;
	}
#endif
}


bool PFLCFG::StoreLabelMapping(string label, BBType *bb)
{
	return m_BBLabelMap.LookUp(label, bb, ENTRY_ADD);
}

void PFLCFG::EnterMacroBlock(void)
{
	MacroBlock *mb = new MacroBlock;
	if (mb == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	m_CurrMacroBlock = mb;
	MacroBlocks.push_back(mb);

}

void PFLCFG::ExitMacroBlock(void)
{
	m_CurrMacroBlock = NULL;
}

PFLCFG::BBType *PFLCFG::LookUpLabel(string label)
{
	BBType *bb = NULL;
	if (!m_BBLabelMap.LookUp(label, bb))
		return NULL;

	return bb;
}


PFLCFG::BBType *PFLCFG::NewBasicBlock(BBKind kind, SymbolLabel *startLbl)
{
	if (startLbl == NULL)
	{
		startLbl = new SymbolLabel(LBL_CODE, m_BBCount, string("BB_")+int2str(m_BBCount,10));
		if (startLbl == NULL)
			throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	}
	BBType *bb = new BBType(kind, startLbl, m_BBCount++);
	if (bb == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	StoreLabelMapping(startLbl->Name, bb);

	node_t *graphNode = &AddNode(bb);
	bb->setNode(graphNode);
	allBB[bb->getId()] = bb;

	//std::cout << ">>>>> New Basic Block: " << startLbl->Name << " BBID: " << bb->getId() << std::endl;
	
	if (m_CurrMacroBlock)
		m_CurrMacroBlock->BasicBlocks.push_back(bb);

	return bb;
}


PFLCFG::BBType *PFLCFG::NewBasicBlock(SymbolLabel *startLbl)
{
	return NewBasicBlock(BB_CODE, startLbl);
}

void PFLCFG::RemoveDeadNodes(void)
{
	bool changed = true;
	//uint32 round = 0;
	while (changed)
	{
		//cout << "Round " << round++ << endl;
		NodeIterator i = FirstNode();
		changed = false;
		for (; i != LastNode(); i++)
		{
			BBType *bb = i.current()->NodeInfo;
			if (bb->Kind != BB_CFG_ENTRY && bb->Valid)
			{
				if (i.current()->GetPredecessors().empty())
				{
					changed = true;
					bb->Valid = false;
				}
			}
		}
		if (changed)
		{
			NodeIterator i = FirstNode();
			for(; i != LastNode(); i++)
			{
				BBType *bb = i.current()->NodeInfo;
				if (!bb->Valid)
				{ 
					list<node_t*> &succ = bb->getNode()->GetSuccessors();
					list<node_t*>::iterator k = succ.begin();
					for (; k != succ.end(); k++)
						DeleteEdge(*(*k), *bb->getNode());
					//delete bb;
					//Graph.DeleteNode(*i.current());
				}
			}
		}
	}
	//cout << endl;
}

void PFLCFG::ResetValid(void)
{
	NodeIterator i = FirstNode();
	for (; i != LastNode(); i++)
	{
		BBType *bb = i.current()->NodeInfo;
		bb->Valid = false;
	}
}

void CFGBuilder::AddEdge(BBType *from, BBType *to)
{
	nbASSERT(from != NULL, "from cannot be NULL");
	nbASSERT(to != NULL, "to cannot be NULL");
	m_CFG.AddEdge(*from->getNode(), *to->getNode());
}


void CFGBuilder::RemEdge(BBType *from, BBType *to)
{
	nbASSERT(from != NULL, "from cannot be NULL");
	nbASSERT(to != NULL, "to cannot be NULL");
	m_CFG.DeleteEdge(*from->getNode(), *to->getNode());
	
}


bool CFGBuilder::AddEdge(SymbolLabel *from, SymbolLabel *to)
{
	BBType *bbFrom = m_CFG.LookUpLabel(from->Name);
	if (bbFrom == NULL)
		return false;
	BBType *bbTo = m_CFG.LookUpLabel(to->Name);
	if (bbTo == NULL)
		return false;
	AddEdge(bbFrom, bbTo);
	return true;
}

bool CFGBuilder::RemEdge(SymbolLabel *from, SymbolLabel *to)
{
	BBType *bbFrom = m_CFG.LookUpLabel(from->Name);
	if (bbFrom == NULL)
		return false;
	BBType *bbTo = m_CFG.LookUpLabel(to->Name);
	if (bbTo == NULL)
		return false;
	RemEdge(bbFrom, bbTo);
	return true;
}



void CFGBuilder::FoundLabel(StmtLabel *stmtLabel)
{
	BBType *bb = m_CFG.LookUpLabel(LABEL_SYMBOL(stmtLabel)->Name);
	if (bb)
	{
		//The label is the target of a forward jump
		if (m_CurrBB != NULL)
		{
			m_CurrBB->Code.SetTail(stmtLabel->Prev);
			AddEdge(m_CurrBB, bb);
		}
		bb->Code.SetHead(stmtLabel);
		m_CurrBB = bb;
		return;
	}	

	if (m_CurrBB == NULL)
	{
		//First label found, the current basic block is not set, so we create it
		m_CurrBB = m_CFG.NewBasicBlock(LABEL_SYMBOL(stmtLabel));
		m_CurrBB->Code.SetHead(stmtLabel);
		m_CurrBB->Code.SetTail(stmtLabel);
		if (IsFirstBB)
		{
			AddEdge(m_CFG.getEntryBB(), m_CurrBB);
			IsFirstBB = false;
		}
		return;
	}

	//The label is in the middle of a basic block
	m_CFG.StoreLabelMapping(LABEL_SYMBOL(stmtLabel)->Name, m_CurrBB);

}



void CFGBuilder::ManageJumpTarget(SymbolLabel *target)
{
	BBType *targetBB = m_CFG.LookUpLabel(target->Name);
	if (targetBB)
	{
		//Target label has already a mapping

		if (!targetBB->Code.Empty())
		{	
			//Target is a previously visited BB (otherwise this could be a forward branch to a label that was
			//already target of a forward branch
			if (targetBB->StartLabel->Name.compare(target->Name) != 0)
			{
				//target label is in the middle of a basic block, so we split it
				BBType *newBB = m_CFG.NewBasicBlock(target);
				StmtBase *oldTail = targetBB->Code.Back();
				targetBB->Code.SetTail(target->Code->Prev);
				newBB->Code.SetHead(target->Code);
				newBB->Code.SetTail(oldTail);
				AddEdge(m_CurrBB, newBB);
				AddEdge(targetBB, newBB);
				if (targetBB == m_CurrBB)
					m_CurrBB = newBB;
				return;
			}
		}
		AddEdge(m_CurrBB, targetBB);
		return;
	}
	// Target label does not have a mapping
	targetBB = m_CFG.NewBasicBlock(target);
	AddEdge(m_CurrBB, targetBB);
}

void CFGBuilder::FoundJump(StmtJump *stmtJump)
{

	nbASSERT(stmtJump->TrueBranch, "a jump stmt should have at least a true branch");
	m_CurrBB->Code.SetTail(stmtJump);
	SymbolLabel *trueBr = stmtJump->TrueBranch;
	if (trueBr)
		ManageJumpTarget(trueBr);
	SymbolLabel *falseBr = stmtJump->FalseBranch;
	if (falseBr)
		ManageJumpTarget(falseBr);
	m_CurrBB = NULL;
}



void CFGBuilder::FoundSwitch(StmtSwitch *stmtSwitch)
{
	StmtBase *caseSt = stmtSwitch->Cases->Front();
	while (caseSt)
	{
		StmtCase *caseStmt = (StmtCase*)caseSt;
		nbASSERT(caseStmt->Target != NULL, "target should be != NULL");
		ManageJumpTarget(caseStmt->Target);
		caseSt = caseSt->Next;
	}
	if (stmtSwitch->Default)
	{
		nbASSERT(stmtSwitch->Default->Target != NULL, "default target should be != NULL");
		ManageJumpTarget(stmtSwitch->Default->Target);
	}
	m_CurrBB->Code.SetTail(stmtSwitch);
	m_CurrBB = NULL;
}


void CFGBuilder::ManageNoLabelBB(StmtBase *stmt)
{
	m_CurrBB = m_CFG.NewBasicBlock(NULL);
	m_CurrBB->Code.SetHead(stmt);
	m_CurrBB->Code.SetTail(stmt);
}


void CFGBuilder::BuildCFG(CodeList &codeList)
{
	StmtBase *first = codeList.Front();
	StmtBase *last = codeList.Back();
	StmtBase *stmt = first;
	while(stmt)
	{
		switch(stmt->Kind)
		{
			case STMT_LABEL:
				FoundLabel((StmtLabel*)stmt);			
				break;
			
			case STMT_GEN:
				if (m_CurrBB == NULL)
					ManageNoLabelBB(stmt);
				m_CurrBB->Code.SetTail(stmt);
				// [ds] this should be fixed...
				if (stmt->Forest->Op == RET /*|| stmt->Forest->Op == SNDPKT*/)
				{
					AddEdge(m_CurrBB, m_CFG.getExitBB());
					m_CurrBB = NULL;
				}
				break;

			case STMT_JUMP:
			case STMT_JUMP_FIELD:
				if (m_CurrBB == NULL)
					ManageNoLabelBB(stmt);
				FoundJump((StmtJump*)stmt);
				break;

			case STMT_SWITCH:
				if (m_CurrBB == NULL)
					ManageNoLabelBB(stmt);
				FoundSwitch((StmtSwitch*)stmt);
				break;

			case STMT_BLOCK:
				m_CFG.EnterMacroBlock();
				m_CFG.m_CurrMacroBlock->Caption =((StmtBlock*)stmt)->Comment;
				BuildCFG(*((StmtBlock*)stmt)->Code);
				m_CFG.ExitMacroBlock();
				break;
			case STMT_COMMENT:
				break;

			default:
				nbASSERT(false, "CANNOT BE HERE");
				break;
		}
		if (stmt == last && m_CurrBB)
			AddEdge(m_CurrBB, m_CFG.getExitBB());
		stmt = stmt->Next;
	}
}

void CFGBuilder::Build(CodeList &codeList)
{
	typedef PFLCFG::SortedIterator s_it_t;
	BuildCFG(codeList);

	//patch the code-list of every basic block
	PFLCFG::NodeIterator i = m_CFG.FirstNode();
	for (; i != m_CFG.LastNode(); i++)
	{
		BBType *bb = i.current()->NodeInfo;
		if (!bb->Code.Empty())
		{
			bb->Code.Front()->Prev = NULL;
			bb->Code.Back()->Next = NULL;
		}
		//Translate code generated into the PFLMIRNode form
		//std::cout << "Traduco il BB: " << bb->getId() << std::endl;
		NodeTranslator nt(&bb->Code, bb->getId());
		(nt.translate(bb->getMIRNodeCode()));
		bb->setProperty("reached", false);
	}

	//ofstream irCFG("cfg_ir_beforeBBE.dot");
	//CFGWriter cfgWriter(irCFG);
	//cfgWriter.DumpCFG(m_CFG, false, false);
	//irCFG.close();


	m_CFG.SortPreorder(*m_CFG.getEntryNode());
	for(s_it_t j = m_CFG.FirstNodeSorted(); j != m_CFG.LastNodeSorted(); j++)
	{
		BBType *bb = (*j)->NodeInfo;
		//std::cout << "Incontro il BB: " << bb->getId() << std::endl;
		bb->setProperty<bool>("reached", true);
	}

	PFLCFG::NodeIterator k = m_CFG.FirstNode();
	for (; k != m_CFG.LastNode(); )
	{
		BBType *bb = (*k)->NodeInfo;
		if(!bb->getProperty<bool>("reached"))
		{
			//std::cout << "Il BB: " << bb->getId() << " Non è raggiungibile" << std::endl;
			//iterate over predecessors
			m_CFG.removeBasicBlock(bb->getId());
			k++;

		}
		else
			k++;

	}
	
}

void PFLBasicBlock::add_copy_head(RegType src, RegType dst)
{
		// Ask the IR to generate a copy node
		IRNode::CopyType *n(IRNode::make_copy(getId(), src, dst));
		
		// Scroll the list of instruction until we find the last phi
		std::list<IRNode *>::iterator i, j;
		for(i = getCode().begin(); i != getCode().end() && (*i)->isPhi(); ++i)
			;
			
		getCode().insert(i, n);
	
		return;
}

void PFLBasicBlock::add_copy_tail(PFLBasicBlock::IRNode::RegType src, PFLBasicBlock::IRNode::RegType dst)
{
			// Ask the IR to generate a copy node
			IRNode::CopyType *n(IRNode::make_copy(getId(), src, dst));

			// Scroll the list of instruction until we find one that is not a jump - should take one try only
			std::list<IRNode *>::iterator i, j;

			getCode().reverse(); // Consider the list from the last element to the first
			for(i = getCode().begin(); i != getCode().end() && (*i)->isJump(); ++i)
				;

			getCode().insert(i, n);

			getCode().reverse(); // Go back to the original order

			return;
}


uint32_t PFLBasicBlock::getCodeSize()
{
	uint32_t counter = 0;
	typedef std::list<PFLMIRNode*>::iterator l_it;
	for(IRStmtNodeIterator i = codeBegin(); i != codeEnd(); i++)
	{
		if( (*i)->isStateChanger() )	
			counter++;
	}
	return counter;
}

PFLMIRNode *PFLBasicBlock::getLastStatement()
{
	if(PFLMIRNodeCode->size() > 0)
		return *(--codeEnd());
	else
		return NULL;
}

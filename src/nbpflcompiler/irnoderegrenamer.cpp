#include "irnoderegrenamer.h"
#include "registers.h"
#include "basicblock.h"
#include "pflmirnode.h"
#include "tree.h"


IRNodeRegRenamer::IRNodeRegRenamer(PFLCFG &cfg): _cfg(cfg)
{
	def_reg = PFLMIRNode::RegType::get_new(2);
}

void IRNodeRegRenamer::visitNode(PFLMIRNode *node)
{
	typedef PFLMIRNode::RegType RegType;
	if(node)
	{
		//std::cout << "Analizzo il nodo: " << std::endl;
		//node->printNode(std::cout, true);
		//std::cout << std::endl;
	}

	if(node && !node->isJump() && node->getOpcode() != PUSH)
	{
		if( node->getDefReg() == RegType::invalid)
			node->setDefReg(def_reg);
	}

	for(int i = 0; i < 3; i++)
	{
		if(node->getKid(i))
			visitNode(node->getKid(i));
	}
}

void IRNodeRegRenamer::VisitBasicBlock(PFLBasicBlock *bb, PFLBasicBlock *comingFrom)
{
	typedef PFLMIRNode::RegType RegType;
	typedef PFLBasicBlock BBType;
	typedef PFLMIRNode IRType;
	typedef IRType::IRNodeIterator it_t;

	BBType::IRStmtNodeIterator i;
	for(i = bb->codeBegin(); i != bb->codeEnd(); i++)
	{
		//std::cout << "************Reg namer*************" << std::endl;
		//std::cout << "Analizzo il nodo" << std::endl;
		//(*i)->printNode(std::cout, true);
		//std::cout << std::endl;


		IRType *root = (*i)->getTree();
		if(root && root->getKid(0))
		{
			visitNode(root->getKid(0));
		}
		//std::cout << "Dopo renaming" << std::endl;
		//(*i)->printNode(std::cout, true);
		//std::cout << std::endl;
	}
	return;
}

void IRNodeRegRenamer::VisitBBEdge(PFLBasicBlock *from, PFLBasicBlock *to)
{
	//NOP
	return;
}

bool IRNodeRegRenamer::run()
{
	CFGVisitor visitor(*this);

	visitor.VisitPreOrder(_cfg);
	return true;
}

/** @file
 * \brief This file contains definitions of class NodeTranslator
 */
#include "node_translation.h"
#include "pflmirnode.h"
#include "statements.h"

std::list<PFLMIRNode*> *NodeTranslator::translate()
{
	assert(_target_list != NULL && "Target list cannot be void");
	if(_code_list->Empty())
		return _target_list;
	StmtBase *iter;
	iter = _code_list->Front();
	while(iter)
	{
		PFLMIRNode *node = iter->translateToPFLMIRNode();
		assert(node != NULL);
		//std::cout << "Translator BB ID: " << _bb_id << std::endl;
		//node->printNode(std::cout) << std::endl;
		//std::cout << std::endl;
		_target_list->push_back(node);
	
		PFLMIRNode::IRNodeIterator it = node->nodeBegin();
		for(; it != node->nodeEnd(); it++)
			(*it)->setBBId(_bb_id);
		iter = iter->Next;
	}
//	_code_list->SetHead(NULL);
	return _target_list;
}


void NodeTranslator::translate(std::list<PFLMIRNode*> * code_list)
{
	_target_list = code_list;
	translate();
}

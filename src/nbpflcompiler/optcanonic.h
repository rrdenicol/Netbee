/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "tree.h"
#include "ircodegen.h"


class OptCanonic
{
private:
	IRCodeGen	&m_CodeGen;
	bool		m_TreeChanged;
	
	Node *EvalIdentity(Node *node);
	Node *EvalUnExpr(Node *node);
	Node *EvalBinExpr(Node *node);
	Node *SimplifyUnExpr(Node *node);
	Node *SimplifyBinExpr(Node *node);
	Node *SimplifyExpr(Node *node);
	void CommuteExpr(Node *node);

public:

	OptCanonic(IRCodeGen &codeGen)
		:m_CodeGen(codeGen), m_TreeChanged(false){}

	bool SimplifyTree(Node *&node);

};


/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once

#include "tree.h"
#include "ircodegen.h"
#include "errors.h"


class OptConstFolding
{
private:
	IRCodeGen		&m_CodeGen;
	bool			m_TreeChanged;

	Node *EvalUnExpr(Node *node);
	Node *EvalBinExpr(Node *node);
	Node *SimplifyUnExpr(Node *node);
	Node *SimplifyBinExpr(Node *node);
	Node *SimplifyExpr(Node *node);

public:

	OptConstFolding(IRCodeGen &codeGen)
		:m_CodeGen(codeGen), m_TreeChanged(false){}


	bool SimplifyTree(Node *&node);

};


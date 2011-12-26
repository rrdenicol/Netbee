/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#pragma once


#include "ircodegen.h"
#include "optfold.h"
#include "optcanonic.h"

class TreeOptimizer
{
private:

	IRCodeGen			&m_CodeGen;	
	OptConstFolding		m_ConstFolding;
	OptCanonic			m_OptCanonic;

public:

	TreeOptimizer(IRCodeGen &codeGen)
		:m_CodeGen(codeGen), m_ConstFolding(codeGen), m_OptCanonic(codeGen){}


	void OptimizeTree(Node *node);
};


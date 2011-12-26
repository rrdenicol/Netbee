/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "treeoptimize.h"
#include "errors.h"

void TreeOptimizer::OptimizeTree(Node *node)
{
	bool result = true;
	while (result != false)
	{
		bool res1 = m_OptCanonic.SimplifyTree(node);
		bool res2 = m_ConstFolding.SimplifyTree(node);
		result = res1 || res2;
	}

}


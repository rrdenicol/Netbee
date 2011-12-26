/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "tree.h"
#include "ircodegen.h"
#include "cfg.h"
#include "errors.h"
#include "bitvectorset.h"
#include "cfgvisitor.h"

class OptControlFlow: public ICFGVisitorHandler
{
private:

	CFG		&m_CFG;
	GlobalSymbols &m_GlobalSyms;

	bool m_Changed;
	
	virtual void VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom);
	virtual void VisitBBEdge(BasicBlock *from, BasicBlock *to);
	void RemConstBranch(BasicBlock *bb);
public:

	OptControlFlow(CFG &cfg, GlobalSymbols &globalSyms);

	void RemConstBranches(void);

	void RemBranch2Branch(void);

	bool HasChanged(void)
	{
		return m_Changed;
	}


};


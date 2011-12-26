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

class OptDeadCodeElim: public ICFGVisitorHandler
{
private:

	struct BBInfo
	{
		nbBitVector *LiveIn;
		nbBitVector *LiveOut;
		
		BBInfo()
			:LiveIn(0), LiveOut(0){}

		~BBInfo()
		{
			delete LiveIn;
			delete LiveOut;
		}

		void Init(uint32 nVars)
		{
			LiveIn = new nbBitVector(nVars);
			if (LiveIn == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			LiveOut = new nbBitVector(nVars);
			if (LiveOut == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
		}


	};

	CFG		&m_CFG;
	BBInfo	*m_BBLiveInfo;
	uint32	m_NVars;

	bool m_Changed;
	
	virtual void VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom);
	virtual void VisitBBEdge(BasicBlock *from, BasicBlock *to);
	
	bool GetStmtDef(StmtBase *stmt, uint32 &var);
	void GetStmtRefs(StmtBase *stmt, nbBitVector &liveOut);
	void GetTreeRefs(Node *node, nbBitVector &liveOut);

public:

	OptDeadCodeElim(CFG &cfg, uint32 nvars);

	void Apply(void);

	bool HasChanged(void)
	{
		return m_Changed;
	}


};

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
#include "treeoptimize.h"


struct DefPoint
{
	uint32		Value;
	StmtBase	*Def;
	BasicBlock	*BB;

	DefPoint(uint32 val, StmtBase *stmt)
		:Value(val), Def(stmt){}
};


bool operator==(const DefPoint &left, const DefPoint &right);

bool operator<(const DefPoint &left, const DefPoint &right);

class OptConstantPropagation: public ICFGVisitorHandler
{
private:	

	struct BBInfo
	{
		nbBitVector		*Reach;
		list<DefPoint>	*Defs;
		uint32			NVars;

		BBInfo()	
			:Reach(0), Defs(0), NVars(0){}

		~BBInfo()
		{
			delete []Defs;
			delete Reach;
		}

		void Init(uint32 nVars)
		{
			Reach = new nbBitVector(nVars);
			if (Reach == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			Defs = new list<DefPoint>[nVars];
			if (Defs == NULL)
				throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
			NVars = nVars;
		}

		void KillDef(uint32 var)
		{
			Defs[var].clear();
			Reach->Reset(var);
		}

		void GenDef(uint32 var, uint32 value, StmtBase *stmt)
		{
			DefPoint def(value, stmt);
			Defs[var].push_back(def);
			Defs[var].unique();
			Reach->Set(var);
		}
		
		void Reset(void)
		{
			Reach->Reset();
			for (uint32 i = 0; i < NVars; i++)
				Defs[i].clear();
		}

	};

	CFG			&m_CFG;
	BBInfo		*m_BBDefsInfo;
	uint32		m_NVars;
	IRCodeGen	&m_CodeGen;
	bool		m_Changed;
	TreeOptimizer m_TreeOpt;

	virtual void VisitBasicBlock(BasicBlock *bb, BasicBlock *comingFrom);
	virtual void VisitBBEdge(BasicBlock *from, BasicBlock *to);
	
	void MergeBBInfoIn(BBInfo &dst, BBInfo &src);
	bool GetStmtDef(StmtBase *stmt, BBInfo &bbinfo);
	void GetStmtRefs(StmtBase *stmt, BBInfo &bbinfo);
	Node *GetTreeRefs(Node *node, BBInfo &bbinfo);

public:

	OptConstantPropagation(CFG &cfg, IRCodeGen &codegen, uint32 nvars);

	void Apply(void);
	
	bool HasChanged(void)
	{
		return m_Changed;
	}

};


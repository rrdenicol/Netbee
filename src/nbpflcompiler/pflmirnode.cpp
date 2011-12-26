#include "pflmirnode.h"
#include "pflcfg.h"
#include "dump.h"
#include "tree.h"



PFLCFG* PFLMIRNode::_cfg(NULL);

PFLMIRNode::~PFLMIRNode()
{
	if(kids[0])
	{
		delete kids[0];
		kids[0] = NULL;
	}
	if(kids[1])
	{
		delete kids[1];
		kids[1] = NULL;
	}
	if(kids[2])
	{
		delete kids[2];
		kids[2] = NULL;
	}
	//if(Sym)
	//{
	//	//delete Sym;
	//	Sym = NULL;
	//}
}

PFLMIRNode *PFLMIRNode::getKid(uint8_t pos) const
{
	if(pos <=2)
		return kids[pos];
	return NULL;
};

void PFLMIRNode::setKid(PFLMIRNode *node, uint8_t pos)
{
	if(pos <=2)
		kids[pos] = node;
}

void PFLMIRNode::deleteKid(uint8_t pos)
{
	assert(pos <= 2);
	delete kids[pos];
}

PFLMIRNode *PFLMIRNode::unlinkKid(uint8_t pos)
{
	assert(pos <= 2);
	PFLMIRNode *kid = kids[pos];
	kid->removeRef();
	kids[pos] = NULL;
	return kid;
}

void PFLMIRNode::swapKids()
{
	PFLMIRNode *tmp;
	tmp = kids[0];
	kids[0] = kids[1];
	kids[1] = tmp;
};

bool PFLMIRNode::IsBoolean(void)
{
	if (OpCode > HIR_LAST_OP)
		return false;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_BOOL);
}

bool PFLMIRNode::IsInteger(void)
{
	if (OpCode > HIR_LAST_OP)
		return true;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_INT);
}

bool PFLMIRNode::IsString(void)
{
	if (OpCode > HIR_LAST_OP)
		return false;
	return (GET_OP_RTYPE(OpCode) == IR_TYPE_STR);
}


bool PFLMIRNode::IsTerOp(void)
{
	if (OpCode > HIR_LAST_OP)
		return (kids[0] != NULL && kids[1] != NULL && kids[2] != NULL);
	return (GET_OP_FLAGS(OpCode) == IR_TEROP);
}


bool PFLMIRNode::IsBinOp(void)
{
	if (OpCode > HIR_LAST_OP)
		return (kids[0] != NULL && kids[1] != NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_BINOP);
}


bool PFLMIRNode::IsUnOp(void)
{
	if (OpCode > HIR_LAST_OP)
		return (kids[0] != NULL && kids[1] == NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_UNOP);
}


bool PFLMIRNode::IsTerm(void)
{
	if (OpCode > HIR_LAST_OP)
		return (kids[0] == NULL && kids[1] == NULL && kids[2] == NULL);
	return (GET_OP_FLAGS(OpCode) == IR_TERM);
}

bool PFLMIRNode::isCopy() const
{
	//assert(1 == 0 && "Not yet implemented");
	return getOpcode() == LOCST && getKid(0) && getKid(0)->getOpcode() == LOCLD;
}

bool PFLMIRNode::isJump() const
{
	switch(getOpcode())
	{
		case JCMPEQ:
		case JCMPNEQ:
		case JCMPG:
		case JCMPGE:
		case JCMPL:
		case JCMPLE:
		case JCMPG_S:
		case JCMPGE_S:
		case JCMPL_S:
		case JCMPLE_S:
		case JFLDEQ:
		case JFLDNEQ:
		case JFLDLT:
		case JFLDGT:
		case CMP_S:
		case JEQ:
		case JNE:
		case JUMP:
		case JUMPW:
		case SWITCH:
		case CALL:
		case CALLW:
		case RET:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

void PFLMIRNode::rewrite_destination(uint16 old_bb_id, uint16 new_bb)
{
	assert(1 == 0 && "Rewrite destination called on a non jump node");
}

void PFLMIRNode::rewrite_use(RegType old, RegType new_reg)
{
	//NOP
	//assert(1 == 0 && "Not yet implemented");
	if(getOpcode() == LOCLD)
	{
		assert(old == ((SymbolTemp*)Sym)->LocalReg && "Tried to rewrite the wrong register in a LDREG node");
		((SymbolTemp*)Sym)->LocalReg = new_reg;
	}
	return;
}

void PFLMIRNode::setSymbol(Symbol *sym)
{
	if(sym)
		Sym = sym->copy();
	else
		Sym = NULL;
}

PFLMIRNode::RegType PFLMIRNode::getDefReg()
{
	if(Sym && Sym->SymKind == SYM_TEMP)
		return ((SymbolTemp*)Sym)->LocalReg; 
	else
		return RegType::invalid;
}

void PFLMIRNode::setDefReg(RegType reg)
{ 
	if(Sym && Sym->SymKind != SYM_TEMP)
		return;

	if(Sym == NULL)
	{
		std::string name("");
		Sym = new SymbolTemp(0, name);
	}
	((SymbolTemp*)Sym)->LocalReg = reg; 
};

PFLMIRNode::RegType * PFLMIRNode::getOwnReg()
{ 
	if(!Sym)
	{
		return NULL;
		// "Nodo senza simbolo!" << std::endl;
		printNode(std::cout, true);
		assert(1 == 0 && "Symbol annot be NULL");
	}

	if(Sym && Sym->SymKind == SYM_TEMP)
	{
		if( ((SymbolTemp*)Sym)->LocalReg != RegType::invalid )
			return &((SymbolTemp*)Sym)->LocalReg;
		return NULL;
	}
	
	return NULL;
}


std::set<PFLMIRNode::RegType> PFLMIRNode::getDefs()
{
	std::set<RegType> res;
	//if(OpCode == LOCST)
	if(OpCode == LOCLD)
		return res;
	if(Sym && Sym->SymKind == SYM_TEMP)
		if( ((SymbolTemp*)Sym)->LocalReg != RegType::invalid)
			res.insert( ((SymbolTemp*)Sym)->LocalReg);
	return res;
}

std::set<PFLMIRNode::RegType> PFLMIRNode::getAllDefs()
{
	std::set<RegType> res(getDefs());

	for(int i(0); i < 3; i++)
	{
		if(getKid(i)){
			std::set<RegType> t(kids[i]->getAllDefs()), p(res);
			res.clear();
			set_union(p.begin(), p.end(), t.begin(), t.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
		}
	}
	return res;
}

std::set<PFLMIRNode::RegType> PFLMIRNode::getUses()
{
	std::set<RegType> res;
	if(OpCode == LOCLD)
		res.insert( ((SymbolTemp*)Sym)->LocalReg);
	else
	{
		for(int i = 0; i < 3; i++)
		{
			if(getKid(i) && getKid(i)->getOwnReg())
				res.insert(*getKid(i)->getOwnReg());
		}
	}
	return res;
}

std::set<PFLMIRNode::RegType> PFLMIRNode::getAllUses()
{
	std::set<RegType> res(getUses());

	for(int i=2; i >= 0; --i)
	{
		if(kids[i])
		{
			std::set<RegType> use = kids[i]->getAllUses();
			set_union(res.begin(), res.end(), use.begin(), use.end(),
					std::insert_iterator<std::set<RegType> >(res, res.begin()));
		}
	}
	return res;
}

std::list<std::pair<PFLMIRNode::RegType,PFLMIRNode::RegType> > PFLMIRNode::getCopiedPair()
{
	//std::cout << "CopiedPair: considero lo statement "; 
	//printNode(std::cout, false);
	//std::cout << std::endl;
	std::list<std::pair<RegType,RegType> > res;

	
	PFLMIRNode *tree = getTree();

	if(!tree)
		return res;
	if(tree->isStore() && tree->getKid(0) && tree->getKid(0)->isLoad())
	{
		RegType dst = tree->getDefReg();
		RegType src = tree->getKid(0)->getDefReg();
		res.push_back(std::pair<RegType,RegType>(dst, src));
	}
	return res;
}

/*

bool BaseNode::IsStmt(void)
{
	return (GET_OP_FLAGS(Op) == IR_STMT);
}
*/
PFLMIRNode::ConstType * PFLMIRNode::make_const(uint16_t bbId, PFLMIRNode::ValueType const_value, PFLMIRNode::RegType def_reg)
{
	PFLMIRNode *const_node = new PFLMIRNode(PUSH, const_value);
	const_node->setBBId(bbId);
	return const_node;
}

PFLMIRNode::StoreType * PFLMIRNode::make_store(uint16_t bbID, PFLMIRNode *kid, PFLMIRNode::RegType defReg)
{
	PFLMIRNode *new_store = new PFLMIRNode(LOCST, kid, new SymbolTemp(defReg));
	new_store->setBBId(bbID);
	StmtPFLMIRNode *new_stmt = new StmtPFLMIRNode(STMT_GEN, new_store);
	new_stmt->setBBId(bbID);
	return new_stmt;
}

PFLMIRNode::LoadType * PFLMIRNode::make_load(uint16_t bbID, PFLMIRNode::RegType defReg)
{
	PFLMIRNode *new_load = new PFLMIRNode(LOCLD, new SymbolTemp(defReg));
	return new_load;
}

std::set<PFLMIRNode::RegType> StmtPFLMIRNode::getDefs()
{
	std::set<RegType> res;
	//if(getKid(0))
	//	return getKid(0)->getDefs();
	return res;
}

std::set<PFLMIRNode::RegType> StmtPFLMIRNode::getAllUses()
{
	std::set<RegType> res;

	if(getKid(0))
	{
		std::set<RegType> use = getKid(0)->getAllUses();
		set_union(res.begin(), res.end(), use.begin(), use.end(),
				std::insert_iterator<std::set<RegType> >(res, res.begin()));
	}
	return res;
}

PFLMIRNode::RegType * StmtPFLMIRNode::getOwnReg()
{ 
	//assert(1 == 0 && "Not implemented"); 
	if(getKid(0))
		return getKid(0)->getOwnReg();
	
	return NULL;
}

bool PFLMIRNode::isConst(void)
{
	if (OpCode > HIR_LAST_OP)
		return (OpCode == PUSH);
	return (GET_OP(OpCode) == OP_CONST);
}

bool PFLMIRNode::isStore()
{
	return getOpcode() == LOCST;
}

bool PFLMIRNode::isLoad()
{
	return getOpcode() == LOCLD;
}

PFLMIRNode::RegType PFLMIRNode::getUsed()
{
	if(isLoad())
		return *getOwnReg();
	else
		return RegType::invalid;
}

void PFLMIRNode::setUsed(PFLMIRNode::RegType reg)
{
	if(isLoad())
		setDefReg(reg);
	return;
}

/*Interface for reassociation */

bool PFLMIRNode::isOpMemPacket()
{
	switch(getOpcode())
	{
		case PBLDS:
		case PBLDU:
		case PSLDS:
		case PSLDU:
		case PILD:
		case PBSTR:
		case PSSTR:
		case PISTR:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool PFLMIRNode::isOpMemData()
{
	switch(getOpcode())
	{
		case DBLDS:
		case DBLDU:
		case DSLDS:
		case DSLDU:
		case DILD:
		case DBSTR:
		case DSSTR:
		case DISTR:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool PFLMIRNode::isMemLoad()
{
	switch(getOpcode())
	{
		case PBLDS:
		case PBLDU:
		case PSLDS:
		case PSLDU:
		case PILD:
		case DBLDS:
		case DBLDU:
		case DSLDS:
		case DSLDU:
		case DILD:
		case SBLDS:
		case SBLDU:
		case SSLDS:
		case SSLDU:
		case SILD:
		case ISBLD: 
		case ISSLD: 
		case ISSBLD:
		case ISSSLD:
		case ISSILD:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

bool PFLMIRNode::isMemStore()
{
	switch(getOpcode())
	{
		case DBSTR:
		case DSSTR:
		case DISTR:
		case PBSTR:
		case PSSTR:
		case PISTR:
		case SBSTR:
		case SSSTR:
		case SISTR:
		case IBSTR:
		case ISSTR:
		case IISTR:
			return true;
			break;
		default:
			return false;
			break;
	}
	return false;
}

uint8_t PFLMIRNode::num_kids = 3;
/*END Interface for reassociation */

PFLMIRNode::ValueType PFLMIRNode::getValue()
{
	return Value;
}

uint32 PFLMIRNode::GetConstVal(void)
{
	nbASSERT(isConst(), "node must be constant");
	if (OpCode < HIR_LAST_OP)
		return ((SymbolIntConst *)Sym)->Value;
	
	return Value;
}


PFLMIRNode *PFLMIRNode::copy()
{
	PFLMIRNode *newnode = new PFLMIRNode(getOpcode(), Sym);
	for(int i = 0; i < 3; i++)
		if(kids[i])
			newnode->setKid(kids[i]->copy(), i);
	//newnode->setDefReg(defReg);
	newnode->setValue(Value);
	return newnode;
}

PFLMIRNode::PhiType *PFLMIRNode::make_phi(uint16_t bb, RegType reg)
{
	//assert(1 == 0 && "Not yet implemented");
	PhiPFLMIRNode *newnode = new PhiPFLMIRNode(bb, reg);
	return newnode;
}

PFLMIRNode::CopyType *PFLMIRNode::make_copy(uint16_t bbid, RegType src, RegType dst)
{
	//assert(1 == 0 && "Not yet implemented");
	PFLMIRNode *srcnode = new PFLMIRNode(LOCLD, new SymbolTemp(src));
	srcnode->setBBId(bbid);
	PFLMIRNode *dstnode = new PFLMIRNode(LOCST, srcnode, new SymbolTemp(dst));

	GenPFLMIRNode *stmtnode = new GenPFLMIRNode(dstnode);

	return stmtnode;
}

PFLMIRNode::ContType PFLMIRNode::get_reverse_instruction_list()
{
	ordered_list.clear();
	sortPostOrder();
	ContType t(ordered_list);
	t.reverse();
	return t;
}

PFLMIRNode *StmtPFLMIRNode::copy() 
{
	return new StmtPFLMIRNode(Kind, getKid(0)->copy() );
}

PFLMIRNode *LabelPFLMIRNode::copy() 
{
	return new LabelPFLMIRNode(); 
}

PFLMIRNode *CommentPFLMIRNode::copy() 
{ 
	return new CommentPFLMIRNode(Comment);
}


PFLMIRNode *GenPFLMIRNode::copy() { 
	GenPFLMIRNode *newnode = new GenPFLMIRNode();
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

JumpPFLMIRNode::JumpPFLMIRNode(uint32_t opcode, uint16_t bb, 
			uint32_t jt, 
			uint32_t jf,
			RegType defReg,
			PFLMIRNode * param1,
			PFLMIRNode* param2)
				:StmtPFLMIRNode(STMT_JUMP, NULL)
{
	setBBId(bb);
	if(jf == 0)
	{
	TrueBranch = getCFG()->getBBById(jt)->getStartLabel();
	setOpcode(opcode);
	FalseBranch = NULL;
	}
	else
	{
		TrueBranch = getCFG()->getBBById(jt)->getStartLabel();
		FalseBranch = getCFG()->getBBById(jf)->getStartLabel();
		PFLMIRNode *kid = new PFLMIRNode(opcode, param1->copy(), param2->copy());
		this->setKid(kid, 0);
	}
}

PFLMIRNode *JumpPFLMIRNode::copy() {
	JumpPFLMIRNode *newnode = new JumpPFLMIRNode(belongsTo(), TrueBranch, FalseBranch);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode; 
}

void JumpPFLMIRNode::rewrite_destination(uint16 old_bb_id, uint16 new_bb_id)
{
	//assert(1 == 0 && "Not yet implemented!!!!!!");
	assert(_cfg != NULL && "Cfg not setted");
	SymbolLabel *oldl = _cfg->getBBById(old_bb_id)->getStartLabel();	
	SymbolLabel *newl = _cfg->getBBById(new_bb_id)->getStartLabel();
	assert( (TrueBranch == oldl) || ((FalseBranch == oldl) && "No matching label"));

	if(TrueBranch == oldl)
		TrueBranch = newl;
	if(FalseBranch == oldl)
		FalseBranch = newl;
		
}

uint16_t JumpPFLMIRNode::getTrueTarget()
{
	return getCFG()->LookUpLabel(TrueBranch->Name)->getId();
}

uint16_t JumpPFLMIRNode::getFalseTarget()
{
	if(FalseBranch)
		return getCFG()->LookUpLabel(FalseBranch->Name)->getId();
	else
		return 0;
}

uint16_t JumpPFLMIRNode::getOpcode() const
{
	if(getKid(0))
		return getKid(0)->getOpcode();
	else
		return this->OpCode;
}

SwitchPFLMIRNode::targets_iterator SwitchPFLMIRNode::TargetsBegin()
{
	typedef std::list<PFLMIRNode*>::iterator l_it;
	_target_list.clear();
	for(l_it i = Cases.begin(); i != Cases.end(); i++)
	{
		uint32_t case_value = (*i)->getKid(0)->getValue();
		CasePFLMIRNode *case_node = dynamic_cast<CasePFLMIRNode*>(*i);
		uint32_t target = getCFG()->LookUpLabel(case_node->getTarget()->Name)->getId();
		_target_list.push_back(std::make_pair<uint32_t, uint32_t>(case_value, target));
	}
	return _target_list.begin();
}

SwitchPFLMIRNode::targets_iterator SwitchPFLMIRNode::TargetsEnd()
{
	return _target_list.end();
}


void SwitchPFLMIRNode::removeCase(uint32_t case_n)
{
	typedef std::list<PFLMIRNode*>::iterator l_it;

	for(l_it i = Cases.begin(); i != Cases.end(); )
	{
		uint32_t case_value = (*i)->getKid(0)->getValue();
		if(case_value == case_n)
		{
			//this case has to be removed
			i = Cases.erase(i);
			NumCases--;
		}
		else
			i++;
	}
}

uint32_t SwitchPFLMIRNode::getDefaultTarget()
{
	assert(Default != NULL);
	SymbolLabel *target = Default->getTarget();
	assert(target != NULL);
	PFLCFG *_cfg = getCFG();
	assert(_cfg != NULL);
	PFLBasicBlock *bb = _cfg->LookUpLabel(target->Name);
	assert(bb != NULL);
	
	return bb->getId();
}

SwitchPFLMIRNode::~SwitchPFLMIRNode()
{
	delete SwExit;
	delete Default;
	typedef std::list<PFLMIRNode*>::iterator l_it;

	for(l_it i = Cases.begin(); i != Cases.end(); i++)
	{
		PFLMIRNode * case_node = *i;
		delete case_node;
	}
}

void JumpPFLMIRNode::swapTargets()
{
	SymbolLabel *tmp = TrueBranch;
	TrueBranch = FalseBranch;
	FalseBranch = tmp;
}

PFLMIRNode *JumpFieldPFLMIRNode::copy()
{
	JumpPFLMIRNode *newnode = new JumpPFLMIRNode(belongsTo(), TrueBranch, FalseBranch);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

PFLMIRNode *CasePFLMIRNode::copy()
{
	CasePFLMIRNode *newnode = new CasePFLMIRNode(0, this->Target);
	if(kids[0])
		newnode->setKid(kids[0]->copy(), 0);
	return newnode;
}

PFLMIRNode *SwitchPFLMIRNode::copy() {
	std::cerr << "Copy Switch not implemetned"; 
	assert(1==0); 
	return NULL;
}

void SwitchPFLMIRNode::rewrite_destination(uint16 oldbb, uint16 newbb)
{
	//assert(1 == 0 && "Function not yet implemented. Sorry");
	assert(_cfg != NULL && "CFG not setted");
	typedef std::list<PFLMIRNode*>::iterator it_t;
	
	//std::cout << "Rewriting Switch targets: (BBID: " << this->belongsTo() << std::endl;
	
	PFLBasicBlock * old_bb = _cfg->getBBById(oldbb);
	assert(old_bb != NULL && "BB cannot be NULL");
	SymbolLabel *oldl = old_bb->getStartLabel();	
	PFLBasicBlock * new_bb = _cfg->getBBById(newbb);
	assert(new_bb != NULL && "BB cannot be NULL");
	SymbolLabel *newl = new_bb->getStartLabel();
	bool found = false;
	if(Default->getTarget() == oldl)
	{
		found = true;
		Default->setTarget(newl);
		//std::cout << "===> Changed default " << oldl->Name << " - " << oldbb << " into " << newl->Name << " - " << newbb <<  std::endl;
	}
	for(it_t i = Cases.begin(); i != Cases.end(); i++)
	{
		if( ((CasePFLMIRNode*)*i)->getTarget() == oldl)
		{
			found = true;
			((CasePFLMIRNode*)*i)->setTarget(newl); 
			//std::cout << "===> Changed case " << oldl->Name << " - " << oldbb << " into " << newl->Name << " - " << newbb <<  std::endl;
		}
	}
	/*
	if (!found)
		std::cout << "===> No change" << std::endl;
	*/
	//assert(found == true && "No match");
}

//---------------------------------------------------
//PRINTER METODS
//--------------------------------------------------

std::ostream& PFLMIRNode::printNode(std::ostream &os, bool SSAForm)
{
	CodeWriter::DumpOpCode_s(this->getOpcode(), os);
	if(Sym && Sym->SymKind == SYM_TEMP && getOwnReg() != NULL)
		os << "[" << *getOwnReg() << "]"; 
		//os << "[" << ((SymbolTemp*)Sym)->LocalReg << "]";
	//else
	//	os << "[" << defReg << "]";
		
	os << "(";
	if (this->isConst())
	{
		os << this->Value;
	}
	else
	{	
		for(unsigned int i = 0; i < 3; i++)
		{
			if(kids[i])
			{
				if(i)
					os << ", ";
				kids[i]->printNode(os, SSAForm);
			}
		}
	}
	os << " ) ";
	return os;
};


std::ostream& StmtPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "StmtPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& BlockPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "BlockPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& LabelPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "LabelPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& CommentPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "CommentPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& GenPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "GenPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& JumpPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	os << "JumpPFLMIRNode";
	if(getKid(0))
	{
		os << "(";
		getKid(0)->printNode(os, SSAForm) << ")";
	}
	os << "TRUE: " << TrueBranch->Name;
	if(FalseBranch)
		os << ", FALSE: " << FalseBranch->Name;
	os << std::endl;
	
	return os;
}

std::ostream& JumpFieldPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "JumpFieldPFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	return os;
}

std::ostream& CasePFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "CasePFLMIRNode";
	if(getKid(0))
	{
		std::cout << "(";
		getKid(0)->printNode(std::cout, SSAForm) << ")";
	}
	SymbolLabel *target = this->Target;
	assert(target != NULL);
	PFLBasicBlock *bb = _cfg->LookUpLabel(target->Name);
	assert(bb != NULL);
	std::cout << " -> " << target->Name << " BBID: " << bb->getId();	
	return os;
}

std::ostream& SwitchPFLMIRNode::printNode(std::ostream& os, bool SSAForm)
{
	std::cout << "SwitchPFLMIRNode [" << NumCases << "](";
	getKid(0)->printNode(std::cout, true);
	std::cout << std::endl;
	for(std::list<PFLMIRNode*>::iterator i = Cases.begin(); i != Cases.end(); i++)
	{
		std::cout << "	";
		(*i)->printNode(std::cout, SSAForm);
		std::cout << std::endl;
	}
	std::cout << "Default: ";
	Default->printNode(std::cout, SSAForm);
	std::cout << std::endl;
	return os;
}

std::ostream& PhiPFLMIRNode::printNode(std::ostream& os, bool ssa_form)
{
	os << "Phi node" << "[" << getDefReg() << "] (";
	PhiPFLMIRNode::params_iterator i = paramsBegin();
	while(i != paramsEnd())
	{
		os << i->second << ", ";
		i++;
	}
	os << ")";
	return os;
}

PhiPFLMIRNode::PhiPFLMIRNode(uint32_t BBId, RegType defReg)
	: StmtPFLMIRNode(STMT_PHI) {
		BBid = BBId;
		setDefReg(defReg);
	};

PhiPFLMIRNode::PhiPFLMIRNode(const PhiPFLMIRNode& n)
	: StmtPFLMIRNode(STMT_PHI) {
		BBid = n.BBid;		
	};


std::set<PFLMIRNode::RegType> PhiPFLMIRNode::getUses()
{
	std::set<RegType> reg_set;
	//assert(1 == 0 && "Not yet implemented");
	for(params_iterator i = params.begin(); i != params.end(); i++)
		reg_set.insert((*i).second);
	return reg_set;
}

std::set<PFLMIRNode::RegType> PhiPFLMIRNode::getDefs()
{
	std::set<RegType> reg_set;
	reg_set.insert(getDefReg());
	return reg_set;
}

std::set<PFLMIRNode::RegType> PhiPFLMIRNode::getAllUses()
{
	std::set<RegType> reg_set(getUses());
	return reg_set;
}

bool GenPFLMIRNode::isStore()
{
	return getKid(0)->isStore();
}

bool GenPFLMIRNode::isCopy() const
{
	return getKid(0)->isCopy();
}

PFLMIRNode::ValueType GenPFLMIRNode::getValue()
{
	return getKid(0)->getValue();
}

std::ostream& operator<<(std::ostream& os, PFLMIRNode& node)
{
	node.printNode(os, true);
	return os;
}

std::set<PFLMIRNode::RegType> SwitchPFLMIRNode::getDefs()
{
	std::set<RegType> res;
	return res;
}

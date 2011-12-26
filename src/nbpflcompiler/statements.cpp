/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "statements.h"
#include "pflmirnode.h"
#include "tree.h"
#include "errors.h"


StmtBase::~StmtBase()
{
	if (Forest)
		delete Forest;
	Forest = NULL;
}


StmtBase *StmtBase::Clone()
{
	StmtBase *newStmt = new StmtBase(this->Kind, this->Forest->Clone());
	newStmt->Flags = this->Flags;
	newStmt->Comment = this->Comment;
	return newStmt;
}

StmtLoop::~StmtLoop()
{
	if (InitVal)
		delete InitVal;
	if (TermCond)
		delete TermCond;
	if (IncStmt)
		delete IncStmt;
	delete Code;
}

CodeList::~CodeList()
{
	//DEBUG
	//cout << "Codelist distrutto" << endl;

	if (!OwnsStatements)
		return;
	if (Empty())
		return;

	StmtBase *element = Head;

	while (element)
	{
		StmtBase *next = element->Next;
		// element-Kind must be greater than 17 to prevent double free
		if (element != NULL && element->Kind > 17)
			delete element;
		element = next;
	}
}


void CodeList::PushBack(StmtBase *stmt)
{
	nbASSERT(stmt != NULL, "Trying to insert a null statement!");

	if (Head == NULL && Tail == NULL)
	{
		Head = stmt;
		Tail = stmt;
		return;
	}

	Tail->Next = stmt;
	stmt->Prev = Tail;
	Tail = stmt;
	NumStatements++;
}

void CodeList::PushFront(StmtBase *stmt)
{
	nbASSERT(stmt != NULL, "Trying to insert a null statement!");

	if (Head == NULL && Tail == NULL)
	{
		PushBack(stmt);
		return;
	}

	stmt->Next = Head;
	Head->Prev = stmt;
	Head = stmt;
	NumStatements++;

}



void CodeList::Remove(StmtBase *stmt)
{
	nbASSERT(stmt != NULL, "Trying to remove a null statement!");
	nbASSERT(Head != NULL && Tail != NULL, "Empty List");

	StmtBase *prevItem(0), *nextItem(0);


	prevItem = stmt->Prev;
	nextItem = stmt->Next;


	if (prevItem == NULL)
		Head = nextItem;
	else
		prevItem->Next = nextItem;


	if (nextItem == NULL)
		Tail = prevItem;
	else
		nextItem->Prev = prevItem;

	delete stmt;
	NumStatements--;

}


void CodeList::InsertAfter(StmtBase *toInsert, StmtBase *stmt)
{
	nbASSERT(stmt != NULL, "Trying to insert a null statement!");
	nbASSERT(Head != NULL && Tail == NULL, "Empty List");

	StmtBase *nextItem = stmt->Next;

	if (nextItem == NULL)
	{
		PushBack(stmt);
		return;
	}

	nextItem->Prev = toInsert;
	toInsert->Next = nextItem;
	stmt->Next =  toInsert;
	toInsert->Prev = stmt;
	NumStatements++;
}


void CodeList::InsertBefore(StmtBase *toInsert, StmtBase *stmt)
{
	nbASSERT(stmt != NULL, "Trying to insert a null statement!");
	nbASSERT(Head != NULL && Tail == NULL, "Empty List");

	StmtBase *prevItem = stmt->Prev;

	if (prevItem == NULL)
	{
		PushFront(stmt);
		return;
	}

	toInsert->Next = stmt;
	stmt->Prev = toInsert;
	prevItem->Next = toInsert;
	toInsert->Prev = prevItem;
	NumStatements++;
}


StmtLabel *StmtLabel::Clone()
{
	StmtLabel *stmt = new StmtLabel();
	stmt->Forest = this->Forest->Clone();
	return stmt;
}


StmtGen *StmtGen::Clone()
{
	return new StmtGen(this->Forest->Clone());
}


StmtJump *StmtJump::Clone()
{
	StmtJump *stmt = new StmtJump(this->TrueBranch, this->FalseBranch);
	if (this->Forest)
		stmt->Forest = this->Forest->Clone();
	return stmt;
}


StmtJumpField *StmtJumpField::Clone()
{
	StmtJumpField *stmt = new StmtJumpField(this->TrueBranch, this->FalseBranch);
	if (this->Forest)
		stmt->Forest = this->Forest->Clone();
	return stmt;
}



StmtCase::StmtCase(StmtBlock *code, Node *val, SymbolLabel *target)
		:StmtBase(STMT_CASE, val), Code(code), Target(target)
{
}

StmtCase::StmtCase(Node *val, SymbolLabel *target)
		:StmtBase(STMT_CASE, val), Target(target)
{
	Code = new StmtBlock();
}


StmtCase::~StmtCase()
{
	delete Code;
}


StmtCase *StmtCase::Clone()
{
	return new StmtCase(this->Code, this->Forest->Clone(), this->Target);
}


StmtSwitch *StmtSwitch::Clone()
{
	return new StmtSwitch(Cases->Clone(), Forest->Clone(), Default->Clone(), NumCases, SwExit, ForceDefault);
}

StmtIf *StmtIf::Clone()
{
	return new StmtIf(Forest->Clone(), TrueBlock->Clone(), FalseBlock->Clone());
}

StmtLoop *StmtLoop::Clone()
{
	return new StmtLoop(Forest->Clone(), InitVal->Clone(), TermCond->Clone(), IncStmt->Clone(), Code->Clone());
}

StmtWhile *StmtWhile::Clone()
{
	return new StmtWhile(this->Kind, Forest->Clone(), Code->Clone());
}

PFLMIRNode *StmtBase::translateToPFLMIRNode() {
	return new StmtPFLMIRNode(Kind, Forest->translateToPFLMIRNode());
}

PFLMIRNode *StmtComment::translateToPFLMIRNode() {
	return new CommentPFLMIRNode(Comment);
}


PFLMIRNode *StmtLabel::translateToPFLMIRNode() {
	LabelPFLMIRNode *new_node = new LabelPFLMIRNode();
	if(Forest)
		new_node->setKid(Forest->translateToPFLMIRNode(), 0);
	return new_node;
}


PFLMIRNode *StmtGen::translateToPFLMIRNode() {
	GenPFLMIRNode *new_node = new GenPFLMIRNode(Forest->translateToPFLMIRNode());
	if(Forest != NULL)
		new_node->setKid(Forest->translateToPFLMIRNode(), 0);
	else
		new_node->setKid(NULL, 0);

	return new_node;
}

PFLMIRNode *StmtJump::translateToPFLMIRNode() {
	JumpPFLMIRNode *new_node = new JumpPFLMIRNode(0, TrueBranch, FalseBranch);
	new_node->setOpcode(this->Opcode);
	if(Forest)
		new_node->setKid(Forest->translateToPFLMIRNode(), 0);
	return new_node;
}

PFLMIRNode *StmtJumpField::translateToPFLMIRNode() {
	JumpFieldPFLMIRNode *new_node = new JumpFieldPFLMIRNode(0, TrueBranch, FalseBranch);
	new_node->setOpcode(this->Opcode);
	if(Forest)
		new_node->setKid(Forest->translateToPFLMIRNode(), 0);
	return new_node;
}

PFLMIRNode *StmtCase::translateToPFLMIRNode() {
	CasePFLMIRNode *new_node = new CasePFLMIRNode(NULL, Target);
	if(Forest)
		new_node->setKid(Forest->translateToPFLMIRNode(), 0);
	return new_node;
}

PFLMIRNode *StmtSwitch::translateToPFLMIRNode() {
	SwitchPFLMIRNode *new_node = new SwitchPFLMIRNode(Forest->translateToPFLMIRNode());
	new_node->setOpcode(this->Opcode);
	new_node->SwExit = this->SwExit;
	new_node->NumCases = this->NumCases;
	if (this->Default!=NULL)
		new_node->Default = dynamic_cast<CasePFLMIRNode*>(this->Default->translateToPFLMIRNode());
	StmtCase *iter = dynamic_cast<StmtCase*>(this->Cases->Front());
	while(iter)
	{
		new_node->Cases.push_back(dynamic_cast<CasePFLMIRNode*>(iter->translateToPFLMIRNode()) );
		iter = dynamic_cast<StmtCase*>(iter->Next);
	}
	return new_node;
}

PFLMIRNode *StmtBlock::translateToPFLMIRNode() {
	std::cout << "StmtBlock translator not implemeted" << std::endl;
	assert(1 == 0);
	return NULL;
}

PFLMIRNode *StmtIf::translateToPFLMIRNode() {
	cerr << "StmtBlock translator not implemeted" << endl;
	assert(1 == 0);
	return NULL;
}

PFLMIRNode *StmtLoop::translateToPFLMIRNode() {
	cerr << "StmtBlock translator not implemeted" << endl;
	assert(1 == 0);
	return NULL;
}

PFLMIRNode *StmtWhile::translateToPFLMIRNode() {
	cerr << "StmtBlock translator not implemeted" << endl;
	assert(1 == 0);
	return NULL;
}

PFLMIRNode *StmtCtrl::translateToPFLMIRNode() {
	cerr << "StmtBlock translator not implemeted" << endl;
	assert(1 == 0);
	return NULL;
}

PFLMIRNode *StmtFieldInfo::translateToPFLMIRNode() {
	cerr << "StmtBlock translator not implemeted" << endl;
	assert(1 == 0);
	return NULL;
}



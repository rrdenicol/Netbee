/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "errors.h"
#include <string>
#include <cassert>
#include <iostream>
#include <stdlib.h>

//using namespace std;

#ifdef L7_SUPPORT
	#define USE_REGEX
	#define USE_LOOKUPTABLE
#endif

#define USE_STRING_COMPARISON


#define LABEL_SYMBOL(s) ((SymbolLabel*)s->Forest->Sym)


//Fw decl
struct Node;
struct Symbol;
struct SymbolLabel;
struct SymbolProto;
struct SymbolField;
class PFLMIRNode;
class BlockPFLMIRNode;
class LabelPFLMIRNode;
class CommentPFLMIRNode;
class GenPFLMIRNode;
class JumpPFLMIRNode;
class JumpFieldPFLMIRNode;
class CasePFLMIRNode;
class SwitchPFLMIRNode;

enum StmtKind
{
	STMT_LABEL	= 1,
	STMT_GEN,
	STMT_JUMP,
	STMT_JUMP_FIELD,
	STMT_SWITCH,
	STMT_CASE,
	STMT_BLOCK,
	STMT_COMMENT,
	STMT_IF,
	STMT_LOOP,
	STMT_WHILE,
	STMT_DO_WHILE,
	STMT_BREAK,
	STMT_CONTINUE,
	STMT_PHI,
	STMT_FINFOST,
};

enum StmtFlags
{
	STMT_FL_DEAD = 0x01
};


struct StmtBase
{
	uint16		Opcode;
	StmtKind	Kind;
	Node		*Forest;
	StmtBase	*Next;
	StmtBase	*Prev;
	string		Comment;
	uint16		Flags;


	StmtBase(StmtKind kind, Node *forest = 0)
		: Opcode(0), Kind(kind), Forest(forest), Next(0), Prev(0),  Flags(0) 
	{
		Comment = "";
	}

	virtual ~StmtBase();
	
	virtual StmtBase *Clone();

	virtual PFLMIRNode * translateToPFLMIRNode(); 
};





enum StmtOwner
{
	OWNS_STATEMENTS = true,
	NOT_OWNS_STATEMENTS = false
};


class CodeList
{
private:
	StmtBase	*Head;
	StmtBase	*Tail;
	bool		OwnsStatements;		
	uint32		NumStatements;

public:
	CodeList(bool ownsStmts = OWNS_STATEMENTS)
		:Head(NULL), Tail(NULL), OwnsStatements(ownsStmts), NumStatements(0){}

	~CodeList();

	bool Empty(void)
	{
		return (Head == NULL) || (Tail == NULL) ;
		nbASSERT(!((Head == NULL && Tail != NULL) || (Head != NULL && Tail == NULL)), "The code list is not coherent");
	}


	void PushBack(StmtBase *stmt);
	void PushFront(StmtBase *stmt);
	void Remove(StmtBase *stmt);
	void InsertAfter(StmtBase *toInsert, StmtBase *stmt);
	void InsertBefore(StmtBase *toInsert, StmtBase *stmt);

	uint32 GetNumStatements(void)
	{
		return NumStatements;
	}

	void SetHead(StmtBase *head)
	{
		Head = head;
		/*
		if (head != NULL)
			head->Prev = NULL;
		*/
	}

	void SetTail(StmtBase *tail)
	{
		Tail = tail;
		/*
		if (tail != NULL)
			tail->Next = NULL;
		*/
	}

	StmtBase *Back(void)
	{
		return Tail;
	}

	StmtBase *Front(void)
	{
		return Head;
	}
	
	
	CodeList *Clone()
	{
		CodeList *newCodeList = new CodeList();
		StmtBase *next = this->Head;
		while (next)
		{
			newCodeList->PushBack(next->Clone());
			next = next->Next;
		}
		return newCodeList;
	}
};


struct StmtBlock: public StmtBase
{
	friend class BlockPFLMIRNode;
	CodeList	*Code;

	//this should be private!
	StmtBlock(CodeList *codeList, string &comment)
			:StmtBase(STMT_BLOCK), Code(codeList)
	{
		Comment = comment;
	}

	
	StmtBlock(string comment = "")
		:StmtBase(STMT_BLOCK)
	{
		Code = new CodeList();
		Comment = comment;
	}
	
	~StmtBlock()
	{
		delete Code;
	}

	virtual StmtBlock *Clone()
	{
		StmtBlock *stmt = new StmtBlock(this->Code->Clone(), this->Comment);
		return stmt;
	}
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtLabel: public StmtBase
{

	friend class LabelPFLMIRNode;

	StmtLabel(void)
		:StmtBase(STMT_LABEL){}
	
	virtual StmtLabel *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode(); 
};

struct StmtComment: public StmtBase
{
	friend class CommentPFLMIRNode;
	StmtComment(string comment)
		:StmtBase(STMT_COMMENT)
	{
		Comment = comment;
	}
	
	virtual StmtComment *Clone()
	{
		return new StmtComment(this->Comment);
	}
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtGen: public StmtBase
{
	friend class GenPFLMIRNode;

	StmtGen(Node *forest = 0)
		:StmtBase(STMT_GEN, forest){}

	
	virtual StmtGen *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};



struct StmtJump: public StmtBase
{
	friend class JumpPFLMIRNode;

	SymbolLabel *TrueBranch;
	SymbolLabel *FalseBranch;

	StmtJump(SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, Node *forest = 0)
		:StmtBase(STMT_JUMP, forest), TrueBranch(trueBranch), FalseBranch(falseBranch)
	{
		if (falseBranch == NULL)
		{
			nbASSERT(forest == NULL, "falseBranch = NULL with forest != NULL");
		}
	}
	
	virtual StmtJump *Clone();

	virtual PFLMIRNode *translateToPFLMIRNode();
};


struct StmtJumpField: public StmtJump
{
	friend class JumpFieldPFLMIRNode;

	StmtJumpField(SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, Node *forest = 0)
		:StmtJump(trueBranch, falseBranch, forest)
	{
		if (falseBranch == NULL)
			nbASSERT(forest == NULL, "falseBranch = NULL with forest != NULL");

		this->Kind=STMT_JUMP_FIELD;
	}

	virtual StmtJumpField *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};


struct StmtCase: public StmtBase
{
	friend class CasePFLMIRNode;

	StmtBlock *Code;
	SymbolLabel *Target;

	StmtCase(StmtBlock *code, Node *val, SymbolLabel *target);
	
	StmtCase(Node *val = 0, SymbolLabel *target = 0);

	~StmtCase();
	
	virtual StmtCase *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};


struct StmtSwitch: public StmtBase
{
	friend class SwitchPFLMIRNode;

	CodeList	*Cases;
	StmtCase	*Default;
	uint32	NumCases;
	SymbolLabel *SwExit;
	bool ForceDefault;

	StmtSwitch(CodeList *cases, Node *expr, StmtCase *def, uint32 numCases, SymbolLabel *swExit, bool forceDefault)
		:StmtBase(STMT_SWITCH, expr), Cases(cases), Default(def),
		NumCases(numCases),	SwExit(swExit), ForceDefault(forceDefault){}
	
	StmtSwitch(Node *expr = 0)
		:StmtBase(STMT_SWITCH, expr), Default(0), NumCases(0), SwExit(0), ForceDefault(true)
	{
		Cases = new CodeList();
	}

	~StmtSwitch()
	{
		if (Default)
			delete Default;
		delete Cases;
	}

	virtual StmtSwitch *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtIf: public StmtBase
{
	StmtBlock *TrueBlock;
	StmtBlock *FalseBlock;

	StmtIf(Node *expr, StmtBlock *trueBlock, StmtBlock *falseBlock)
		:StmtBase(STMT_IF, expr), TrueBlock(trueBlock), FalseBlock(falseBlock){}
	
	StmtIf(Node *expr)
		:StmtBase(STMT_IF, expr)
	{
		TrueBlock = new StmtBlock();
		FalseBlock = new StmtBlock();
	}
	
	~StmtIf()
	{
		delete TrueBlock;
		delete FalseBlock;
	}
	
	virtual StmtIf *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtLoop: public StmtBase
{
	Node *InitVal;
	Node *TermCond;
	StmtGen *IncStmt;
	StmtBlock *Code;
//	SymbolLabel *LoopExit;

	StmtLoop(Node *indexVar, Node *initVal, Node *termCond, StmtGen *incStmt, StmtBlock *code = 0)
		:StmtBase(STMT_LOOP, indexVar), InitVal(initVal), TermCond(termCond), IncStmt(incStmt)
	{
		if (code)
			Code = code;
		else
			Code = new StmtBlock();
	}
	
	~StmtLoop();
	
	virtual StmtLoop *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtWhile: public StmtBase
{
	StmtBlock *Code;
	
	StmtWhile(StmtKind kind, Node *cond, StmtBlock *code = 0)
		:StmtBase(kind, cond)/*, WhileExit(whileExit)*/
	{
		nbASSERT(kind == STMT_WHILE || kind == STMT_DO_WHILE, "wrong statement kind");
		if (code)
			Code = code;
		else
			Code = new StmtBlock();
	}
	
	
	~StmtWhile()
	{
		delete Code;
	}
	
	virtual StmtWhile *Clone();
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtCtrl: public StmtBase
{
	StmtCtrl(StmtKind kind)
		:StmtBase(kind)
	{
		nbASSERT(kind == STMT_BREAK || kind == STMT_CONTINUE, "wrong statement kind");
	}
	

	
	virtual PFLMIRNode *translateToPFLMIRNode();
};

struct StmtFieldInfo: public StmtBase
{
	SymbolField *Field;
	uint32		Position;

	StmtFieldInfo(SymbolField *field, uint32 position)
		:StmtBase(STMT_FINFOST), Field(field), Position(position)
	{ }

	
	virtual StmtFieldInfo *Clone()
	{
		return new StmtFieldInfo(Field, Position);
	}
	
	virtual PFLMIRNode *translateToPFLMIRNode();
};


#ifndef _PFLMIRNODE_H
#define _PFLMIRNODE_H

#include "symbols.h"
#include "registers.h"
#include "irnode.h"
#include "statements.h"

struct Symbol;
class CodeWriter;
class PFLCFG;


class PhiPFLMIRNode; //fwd declaration for SSA interface
class StmtPFLMIRNode;
class JumpPFLMIRNode;
class SwitchPFLMIRNode;

class PFLMIRNode: public jit::TableIRNode<jit::RegisterInstance, uint16_t>
{
	friend class CodeWriter;
	private:
	protected:
		static PFLCFG *_cfg;
		PFLMIRNode* kids[3]; //!<The kids of this node
		/*!
		 * This register id the virtual register written by this node. It is returned 
		 * by getDefs only if this node is a store register.
		 */
		//RegType defReg; //!<The register defined by this node

		std::list<PFLMIRNode*> ordered_list; //!<the list of this node kids in post order ???

		uint32_t Value;
		std::string Comment;
		uint32_t numRefs;
		//RegType defReg;
		
	public:
		Symbol *Sym;
		Symbol *SymEx;

		static void setCFG(PFLCFG *cfg) { PFLMIRNode::_cfg = cfg; } //FIXME !! This is very bad
		static PFLCFG * getCFG() { return PFLMIRNode::_cfg; }

		virtual PFLMIRNode *copy();

		void setSymbol(Symbol *sym);
		void setValue(uint32_t value) {Value = value; };
		void setComment(std::string str) { Comment = str; };
		void setDefReg(RegType reg); 
		RegType getDefReg();

		RegType *getOwnReg();

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

		// ------ THIS INTERFACE IS REQUIRED FOR THE SSA ALGORITHMS -------------------------
		typedef PhiPFLMIRNode PhiType; //!< Required interface for the SSA algorithm.
		static PhiType *make_phi(uint16_t, RegType);

		typedef PFLMIRNode CopyType;	//!< Type representing a copy operation
		static CopyType *make_copy(uint16_t, RegType src, RegType dst); //!< ctor for a node that implements a register-to-register copy

		virtual void rewrite_destination(uint16_t old_bb_id, uint16_t new_bb_id);
		virtual void rewrite_use(RegType old, RegType new_reg);

		//! Needed for exit from SSA - true if this is a control flow operation, of any type
		virtual bool isJump() const; 

		virtual bool isPhi() const { return false; };
		// ------ END OF INTERFACE REQUIRED BY THE SSA ALGORITHMS ---------------------------
		
		// ------ THIS INTERFACE IS REQUIRED FOR THE COPY FOLDING ALGORITHM -----------------
		typedef std::list<PFLMIRNode *> ContType;
		ContType get_reverse_instruction_list();
		virtual bool isCopy() const;
		// ------ END OF INTERFACE REQUIRED BY THE COPY FOLDING ALGORITHM -------------------
		// ------ INTERFACE REQUIRED BY CONSTANT PROPAGATION ALGORITHM --------------------------
		typedef PFLMIRNode ConstType;
		typedef PFLMIRNode LoadType;
		typedef int32_t ValueType;

		static ConstType * make_const(uint16_t bbID, ValueType const_value, RegType defReg = RegType::invalid);

		virtual bool isConst();
		virtual bool isStore();
		virtual bool isLoad();

		RegType getUsed();
		void setUsed(RegType);

/*#######################################################################
 * Reassociation algorithm interface
 * #####################################################################*/
		virtual bool isOpMemPacket();
		virtual bool isOpMemData();
		virtual bool isMemLoad();
		virtual bool isMemStore();

		static uint8_t num_kids;
/*#######################################################################
 * END reassociation algorithm interface
 * #####################################################################*/

/*********************************************************
 * Control Flow simplification algorithm required interface
 * *******************************************************/
		typedef JumpPFLMIRNode 		JumpType;
		typedef SwitchPFLMIRNode 	SwitchType;
		virtual bool isSwitch() const {return false;}
/*********************************************************
 * END Control Flow simplification algorithm required interface
 * *******************************************************/

		virtual ValueType getValue();

		virtual PFLMIRNode * getTree() { return this; };

		virtual void addRef(void)
		{
			numRefs++;
		}

		virtual void removeRef(void)
		{
			numRefs--;
		}

		virtual bool isStateChanger() { return true; }

		typedef StmtPFLMIRNode StoreType;
		
		static StoreType * make_store(uint16_t bbID, PFLMIRNode *kid, RegType defReg);
		static LoadType * make_load(uint16_t bbID, RegType defReg);
		// ------ END OF INTERFACE REQUIRED BY THE CONSTANT PROPAGATION ALGORITHM -------------------

		//!get the registers defined by this node
		virtual std::set<RegType> getDefs();

		//!get the registers defined by this tree
		virtual std::set<RegType> getAllDefs();

		//!get the registers used by this node
		virtual std::set<RegType> getUses();

		//!get the registers used by this tree
		virtual std::set<RegType> getAllUses();

		/*!
		 * \brief get copy done by this node
		 * \return A list of pair<dest, src>. Register copied by this node
		 */
		virtual std::list<std::pair<RegType,RegType> > getCopiedPair();

		virtual PFLMIRNode *getKid(uint8_t pos) const;
		virtual	void deleteKid(uint8_t pos);

		virtual	PFLMIRNode *unlinkKid(uint8_t pos);

		virtual void swapKids();

		void setKid(PFLMIRNode *node, uint8_t pos);

		bool IsBoolean(void);

		bool IsInteger(void);

		bool IsString(void);


		bool IsTerOp(void);


		bool IsBinOp(void);


		bool IsUnOp(void);


		bool IsTerm(void);


		/*

		   bool BaseNode::IsStmt(void)
		   {
		   return (GET_OP_FLAGS(Op) == IR_STMT);
		   }
		   */

		//bool IsConst(void);

		uint32 GetConstVal(void);


		//Constructors
		PFLMIRNode(uint16_t op)
			: jit::TableIRNode<RegType, OpCodeType>(op),  Value(0),  Sym(0)
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}


		//!\Constructor for leaf nodes
		PFLMIRNode(uint16_t op, Symbol *symbol)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(symbol)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			if(!symbol)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		PFLMIRNode(uint16_t op, uint32 value)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(value), Sym(0)/*, ResType(IR_TYPE_INT), OpType(IR_TYPE_INT)*/
		{
			kids[0] = NULL;
			kids[1] = NULL;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for unary operators
		PFLMIRNode(uint16_t op, PFLMIRNode	*kidNode, Symbol *sym = 0)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0),  Sym(sym)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kidNode;
			kids[1] = NULL;
			kids[2] = NULL;
			if(!sym)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		//!\Constructor for binary operators
		PFLMIRNode(uint16_t op, PFLMIRNode	*kid1, PFLMIRNode *kid2)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(0)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = NULL;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for references
		PFLMIRNode(uint16_t op, Symbol *label, PFLMIRNode *kid1, PFLMIRNode *kid2)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(label)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = NULL;
			if(!label)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		//!\Constructor for ternary operators
		PFLMIRNode(uint16_t op, PFLMIRNode	*kid1, PFLMIRNode *kid2, PFLMIRNode *kid3)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(0)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = kid3;
			//std::string name("");
			//Sym = new SymbolTemp(0, name);
		}

		//!\Constructor for references
		PFLMIRNode(uint16_t op, Symbol *label, PFLMIRNode *kid1, PFLMIRNode *kid2, PFLMIRNode *kid3)
			:jit::TableIRNode<RegType, OpCodeType>(op), Value(0), Sym(label)/*, ResType(resType), OpType(opType)*/
		{
			kids[0] = kid1;
			kids[1] = kid2;
			kids[2] = kid3;
			if(!label)
			{
				//std::string name("");
				//Sym = new SymbolTemp(0, name);
			}
		}

		~PFLMIRNode();

		//*************************************
		//NODE ITERATOR
		//************************************
		struct Orderer
		{
			std::list<PFLMIRNode*> &list;
			Orderer(std::list<PFLMIRNode*> &l): list(l) {}

			void operator()(PFLMIRNode *n)
			{
				list.push_back(n);
			}
		};


		//!do an action on every node in this tree in post order
		template<class Function>
			void visitPostOrder(Function action)
			{
				if(kids[0])
					kids[0]->visitPostOrder(action);
				if(kids[1])
					kids[1]->visitPostOrder(action);
				if(kids[2])
					kids[2]->visitPostOrder(action);

				action(this);
			}

		//!do an action on every node in this tree in order
		template<class Function>
			void visitPreOrder(Function action)
			{
				action(this);

				if(kids[0])
					kids[0]->visitPostOrder(action);
				if(kids[1])
					kids[1]->visitPostOrder(action);
				if(kids[2])
					kids[2]->visitPostOrder(action);

			}

		//!sort this tree in post order
		void sortPostOrder()
		{
			visitPostOrder(Orderer(ordered_list));
		}

		//!sort this tree in post order
		void sortPreOrder()
		{
			visitPreOrder(Orderer(ordered_list));
		}

		//!printing operation
		//friend std::ostream& operator<<(std::ostream& os, PFLMIRNode& node);

		/*!
		 * \brief class that implements an iterator on the nodes of this tree
		 */
		class PFLMIRNodeIterator
		{
			private:
				std::list<PFLMIRNode*> &nodes; //!<ordered list containing the nodes of this tree
				std::list<PFLMIRNode*>::iterator _it; //!<wrapped iterator on the list
			public:

				typedef PFLMIRNode* value_type;
				typedef ptrdiff_t difference_type;
				typedef PFLMIRNode** pointer;
				typedef PFLMIRNode*& reference;
				typedef std::forward_iterator_tag iterator_category;

				PFLMIRNodeIterator(std::list<PFLMIRNode*> &l): nodes(l) { _it = nodes.begin();}
				PFLMIRNodeIterator(std::list<PFLMIRNode*> &l, bool dummy):nodes(l), _it(nodes.end()) { }

				PFLMIRNode*& operator*()
				{
					return *_it;
				}

				PFLMIRNode*& operator->()
				{
					return *_it;
				}

				PFLMIRNodeIterator& operator++(int)
				{
					_it++;
					return *this;
				}


				bool operator==(const PFLMIRNodeIterator& bb) const
				{
					return bb._it == _it;
				}

				bool operator!=(const PFLMIRNodeIterator& bb) const
				{
					return bb._it != _it;
				}
		};

		typedef PFLMIRNodeIterator IRNodeIterator; //!< Required for the SSA algorithm interface.
		friend class MIRNodeIterator;

		//!get an iterator on the nodes of this tree in post order
		virtual PFLMIRNodeIterator nodeBegin()
		{
			ordered_list.clear();
			sortPostOrder();
			return PFLMIRNodeIterator(ordered_list);
		}

		virtual //!get an iterarot to the end of the list of nodes
			PFLMIRNodeIterator nodeEnd()
			{
				return PFLMIRNodeIterator(ordered_list, true);
			}

};



/*!
 * \brief This class is a genericstatement in PFLNode form
 */
class StmtPFLMIRNode: public PFLMIRNode
{
	friend class CodeWriter;
	friend class CFGWriter;
	friend class NetILTraceBuilder;
	friend class PFLTraceBuilder;
	protected:
		StmtKind Kind;
		std::string Comment;
		StmtPFLMIRNode *GenBy;
		uint16_t Flags;
	public:
		StmtPFLMIRNode(StmtKind kind, PFLMIRNode *kid = 0): PFLMIRNode(0, kid), Kind(kind), Comment(""), GenBy(0), Flags(0) {};

		virtual ~StmtPFLMIRNode() {
		}
		PFLMIRNode *copy();

		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		virtual PFLMIRNode *getTree() { return getKid(0);}

		virtual std::set<PFLMIRNode::RegType> getDefs();

		virtual RegType* getOwnReg();

		virtual bool isPFLStmt() { return true;};
		 
		std::set<RegType> getAllUses();

};

class BlockPFLMIRNode: public StmtPFLMIRNode
{
	friend class CodeWriter;
	protected:
		std::list<PFLMIRNode*> *Code;

	public:
	BlockPFLMIRNode(std::string str)
		:StmtPFLMIRNode(STMT_BLOCK) { Comment = str; }

	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
};

/*!
 * \brief This class is a label In PFLMIRNode form
 */
class LabelPFLMIRNode: public StmtPFLMIRNode
{
	public:
		LabelPFLMIRNode(): StmtPFLMIRNode(STMT_LABEL) {};

		PFLMIRNode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);

		virtual bool isStateChanger() { return false; }
};


/*!
 * \brief Comment Statement
 */
class CommentPFLMIRNode: public StmtPFLMIRNode
{
	public:
		CommentPFLMIRNode(std::string comment)
			: StmtPFLMIRNode(STMT_COMMENT)
		{
			Comment = comment;
		}

		PFLMIRNode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		virtual bool isStateChanger() { return false; }
};

/*!
 * \brief Gen Statement
 */
class GenPFLMIRNode: public StmtPFLMIRNode
{
	public:
		GenPFLMIRNode(PFLMIRNode *kid = 0)
			: StmtPFLMIRNode(STMT_GEN, kid) {};
		PFLMIRNode *copy();
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
		//virtual bool isConst();
		virtual bool isStore();

		virtual bool isCopy() const;
		//virtual bool isLoad();
		virtual ValueType getValue();
};

/*!
 * \brief Jump node
 */
class JumpPFLMIRNode: public StmtPFLMIRNode
{
	friend class CodeWriter;
	friend class CFGWriter;
	friend class NetILTraceBuilder;
	protected:
	SymbolLabel *TrueBranch;
	SymbolLabel *FalseBranch;

	public:
	JumpPFLMIRNode(uint16_t bb, SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, PFLMIRNode *kid0 = 0)
		:StmtPFLMIRNode(STMT_JUMP, kid0), TrueBranch(trueBranch), FalseBranch(falseBranch)
	{
		setBBId(bb);
		if (falseBranch == NULL)
		{
			assert(getKid(0) == NULL && "falseBranch = NULL with kid != NULL");
		}
	}
	JumpPFLMIRNode(uint32_t opcode, uint16_t bb, 
			uint32_t jt, 
			uint32_t jf,
			RegType defReg,
			PFLMIRNode * param1,
			PFLMIRNode* param2);
	PFLMIRNode *copy();
	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
	virtual void rewrite_destination(uint16 oldbb, uint16 newbb);
	
	//! Needed for exit from SSA - true if this is a control flow operation, of any type
	virtual bool isJump() const { return true;} ;
	uint16_t getTrueTarget();
	uint16_t getFalseTarget();
//	uint16_t getTrueTarget() { return getTrueBranch(); }
//	uint16_t getFalseTarget() { return getFalseBranch(); }
	void swapTargets();
	uint16_t getOpcode() const;
};

/*!
 * \brief jump Field node
 */
class JumpFieldPFLMIRNode: public JumpPFLMIRNode
{
	public:
		JumpFieldPFLMIRNode(uint16_t bb, SymbolLabel *trueBranch, SymbolLabel *falseBranch = 0, PFLMIRNode *kid0 = 0)
			:JumpPFLMIRNode(bb, trueBranch, falseBranch, kid0) 
		{ Kind = STMT_JUMP_FIELD; }

		PFLMIRNode *copy();
		
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
};


class CasePFLMIRNode: public StmtPFLMIRNode
{
	friend class CodeWriter;
	private:
		StmtBlock	Code;
		SymbolLabel *Target;

	public:
		CasePFLMIRNode(PFLMIRNode *val = 0, SymbolLabel *target = 0)
			:StmtPFLMIRNode(STMT_CASE, val), Target(target) {}

		PFLMIRNode *copy();
		
		virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);

		void setTarget(SymbolLabel * target) { Target = target; }
		SymbolLabel * getTarget() {return Target;}
};

/*!
 * \brief  Switch node
 */
class SwitchPFLMIRNode: public StmtPFLMIRNode
{
	friend struct StmtSwitch;
	friend class CodeWriter;
	protected:
		SymbolLabel *SwExit; 
		std::list<PFLMIRNode*> Cases;
		uint32_t NumCases;
		CasePFLMIRNode *Default;
		std::vector<std::pair<uint32_t, uint32_t> > _target_list;

	public:
		typedef std::vector< std::pair<uint32_t, uint32_t> >::iterator targets_iterator;

		SwitchPFLMIRNode(PFLMIRNode *expr = 0)
			: StmtPFLMIRNode(STMT_SWITCH, expr), SwExit(0), NumCases(0), Default(0) {};

		virtual ~SwitchPFLMIRNode();

	PFLMIRNode *copy();
	virtual std::ostream& printNode(std::ostream& os, bool SSAForm = false);
	virtual void rewrite_destination(uint16 oldbb, uint16 newbb);
	virtual bool has_side_effects() { return true; }

	uint32_t get_targets_num() const { return NumCases; } 

	targets_iterator TargetsBegin();
	targets_iterator TargetsEnd();
	std::vector<std::pair<uint32_t, uint32_t> > &getTargetsList() { return _target_list; }
	uint32_t getDefaultTarget();
	
	//! Needed for exit from SSA - true if this is a control flow operation, of any type
	virtual bool isJump() const { return true;} ;

	virtual bool isSwitch() const {return true; };

	std::set<RegType> getDefs();

	void removeCase(uint32_t case_n);
};

class PhiPFLMIRNode: public StmtPFLMIRNode
{
	private:
//		std::vector<RegType> params; //!< vector containing the phi params
		std::map<uint16_t, RegType> params; //!< map of (source BBs, source variable)

	public:
		typedef std::map<uint16_t, RegType>::iterator params_iterator;

		PhiPFLMIRNode(uint32_t BBId, RegType defReg);

		PhiPFLMIRNode(const PhiPFLMIRNode& n);
			
		virtual ~PhiPFLMIRNode() {
		};

		virtual PFLMIRNode *copy() { return new PhiPFLMIRNode(*this); }

		void addParam(RegType param, uint16_t source_bb) {
			params[source_bb] = param;
//			params.push_back( param ); 
//			param_bb.push_back(source_bb);
		}

		params_iterator paramsBegin() { return params.begin(); }
		params_iterator paramsEnd() { return params.end(); }
		
		std::vector<uint16_t> param_bb;
		
		virtual bool isPhi() const {return true; };

		virtual std::ostream& printNode(std::ostream &os, bool SSAForm = false);

		virtual std::set<RegType> getUses();

		virtual std::set<RegType> getDefs();

		virtual std::set<RegType> getAllUses();

};

std::ostream& operator<<(std::ostream& os, PFLMIRNode& node);




#endif

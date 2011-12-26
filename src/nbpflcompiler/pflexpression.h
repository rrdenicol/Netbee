/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "defs.h"
#include "symbols.h"
#include "tree.h"
#include <list>
#include <iostream>

using namespace std;

/*!
	\brief Possible types for a boolean expression.

	Each element of the enumeration should match to a subclass of
	PFLExpression
*/
enum PFLExpressionType
{
	PFL_BINARY_EXPRESSION,	///< Binary Expression (for AND, OR operators)
	PFL_UNARY_EXPRESSION,	///< Unary Expression (for NOT operator)
	PFL_TERM_EXPRESSION		///< A term of a boolean expression
};



/*!
	\brief Possible operators for a boolean expression
*/
enum PFLOperator
{
	BINOP_BOOLAND,		///< Logical AND
	BINOP_BOOLOR,		///< Logical OR
	UNOP_BOOLNOT

};


/*!
	\brief Types for a PFL action
 */
enum PFLActionType
{
	PFL_RETURN_PACKET_ACTION,
	PFL_EXTRACT_FIELDS_ACTION,
	PFL_CLASSIFY_ACTION
};


class PFLExpression; //fw decl
class PFLAction; //fw decl
class PFLIndex;

/*!
	\brief This class represents a single PFL statement, which is composed of a boolean filtering expression
			and a corresponding action, like return packet or extract fields
*/ 

class PFLExpression 
{
private:
	static uint32	m_Count;
	uint32			m_ID;
protected:

	PFLExpressionType m_Type;		//!<Represents the kind of the expression tree node (one of the \ref PFLExpressionType enumeration values)
	bool			m_Const;		//!<Flag: if true the expression is constant

public:
	/*!
		\brief constructor
		\param type the kind of the expression tree node (one of the \ref PFLExpressionType enumeration values)
	*/
	PFLExpression(PFLExpressionType type)
		:m_Type(type), m_Const(false)
	{
		m_ID = m_Count++;
	}
	
	/*!
		\brief Default destructor
	*/
	virtual ~PFLExpression();

	
	/*!
		\brief It prints out the details of the expression to stderr.
		\param level Nesting level of the expression. Use to properly indent the text on screen.
	*/
	virtual void Printme(int level) = 0;

	/*!
		\brief Appends to outfile the current expression node in Dot format (useful to be 
		visualized by the dot program of the graphviz package).
		\param outFile Pointer to the output text file
	*/
	virtual void PrintMeDotFormat(ostream &outFile) = 0;

	/*!
		\brief Transforms binary expressions in the form "const <op> expr" into expressions 
		in the form  "expr <op> const"
	*/
	virtual void ToCanonicalForm() = 0;


	/*!
		\brief It returns the runtime type of the PFLExpression 
		\return The type of the expression, i.e. one of the \ref PFLExpressionType enumeration values
	*/
	PFLExpressionType GetType()
	{
		return m_Type;
	}


	/*!
		\brief It returns the ID of the expression
		\return the numeric ID of the expression
	*/

	uint32 GetID()
	{
		return m_ID;
	}

	/*!
		\brief It tells whether the expression is constant or not
		\return true if the expression is constant, false otherwise
	*/

	bool IsConst()
	{
		return m_Const;
	}
};

class PFLStatement
{
	PFLExpression	*m_Exp;		//The filtering expression
	PFLAction		*m_Action;	//The corresponding action
	PFLIndex		*m_HeaderIndex;	//The index header selected
	
public:
	/*!
		\brief constructor
		\param exp pointer to a filtering expression object
		\param action pointer to an action object
	*/
	PFLStatement(PFLExpression *exp, PFLAction *action, PFLIndex *headerindex);
	
	/*!
		\brief destructor
	*/
	~PFLStatement();
	
	/*!
		\brief Returns the filtering expression of this statement
	*/
	PFLExpression *GetExpression(void);
	
	/*!
		\brief Returns the action of this statement
	*/
	PFLAction *GetAction(void);

	PFLIndex *GetHeaderIndex(void);
};

class PFLIndex
{
	uint32 m_Index;

public:

	PFLIndex(uint32 num)
		:m_Index(num){}

	virtual ~PFLIndex();

	uint32 GetNum(void)
	{
		return m_Index;
	}
};

/*!
	\brief This class represents a generic PFL action
*/ 
class PFLAction
{
	PFLActionType m_Type;	//!< the kind of action
	
public:
	
	/*!
		\brief constructor
		\param type the kind of this action
	*/
	PFLAction(PFLActionType type)
		:m_Type(type){}
	
	/*!
		\brief destructor
	*/
	virtual ~PFLAction();

	/*!
		\brief It returns the runtime type of the PFLAction
	*/
	PFLActionType GetType(void)
	{
		return m_Type;
	}
};



/*!
 	\brief This class represents a Return Packet Action
*/
class PFLReturnPktAction: public PFLAction
{
	uint32 	m_Port;		//!< the port where accepted packets will be sent (NOTE: probably in future versions of the language this will be discarded)
public:
	
	/*!
		\brief constructor
		\param port the port where accepted packets will be sent
	*/
	PFLReturnPktAction(uint32 port)
		:PFLAction(PFL_RETURN_PACKET_ACTION), m_Port(port){}
};


/*!
 	\brief This class represents an Extract Fields Action
*/
class PFLExtractFldsAction: public PFLAction
{
	FieldsList_t 	m_FieldsList;	//!< the list of fields to be extracted

//NOTE: these are here for future enhancements of the language
//************************************************************
	bool 			m_InnerProto;
	bool			m_ProtoPath;
//************************************************************
	
public:
	
	/*!
		\brief constructor
	*/
	PFLExtractFldsAction()
		:PFLAction(PFL_EXTRACT_FIELDS_ACTION){}
	
	
	/*!
	 	\brief adds a field to the list
	 	\param field a pointer to a field symbol
	*/
	void AddField(SymbolField *field)
	{
		m_FieldsList.push_back(field);
	}
	
	
	/*!
	 	\brief returns a reference to the internal list of fields
	*/
	FieldsList_t &GetFields(void)
	{
		return m_FieldsList;
	}

};


/*!
	\brief This class represents a generic expression tree node.

	This class is not instantiable directly. You should use the derived
	classes.
*/


/*!
	\brief	It represents a binary expression. 
*/
class PFLBinaryExpression : public PFLExpression
{


private:
	
	/*!
		\brief The left operand (=left node).
	*/
	PFLExpression* m_LeftNode;

	/*!
		\brief The right operand (=right node).
	*/
	PFLExpression* m_RightNode;

	/*!
		\brief Operator of the expression.
	*/
	PFLOperator m_Operator;

	void SwapChilds();


public:
	virtual ~PFLBinaryExpression();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual void ToCanonicalForm();
	

	/*!
		\brief It creates a new binary expression.

		It creates a new binary expression, given the left and right operands, and the operator between them.
		
		\param LeftExpression Pointer to the left operand.
		\param RightExpression Pointer to the right operand.
		\param Operator Operator between the operands.
	*/
	PFLBinaryExpression(PFLExpression *LeftExpression, PFLExpression *RightExpression, PFLOperator Operator);
	
	/*!
		\brief	Returns the operator between the operands.

		\return	The operator between the operands.

	*/
	PFLOperator GetOperator()
	{
		return m_Operator;
	}
	
	/*!
		\brief	Returns the left operand (=left node).

		\return The left operand.
	*/
	PFLExpression* GetLeftNode()
	{
		return m_LeftNode;
	}

	/*!
		\brief Returns the right operand (=right node).

		\return The right operand.
	*/
	PFLExpression* GetRightNode()
	{
		return m_RightNode;
	}


	/*!
		\brief	Sets the left operand (=left node).
		\param left Pointer to the left operand.
		\return nothing
	*/
	void SetLeftNode(PFLExpression *left)
	{
		m_LeftNode = left;
	}

	/*!
		\brief Sets the right operand (=right node).
		\param right Pointer to the right operand.
		\return nothing
	*/
	void SetRightNode(PFLExpression *right)
	{
		m_RightNode = right;
	}

	
	
};

/*!
	\brief	It represents a unary expression. 
	
	For example "~ip" is a unary expression. "ip" represents the operand, 
	"~" represents the operator.
*/
class PFLUnaryExpression : public PFLExpression
{
public:

	virtual ~PFLUnaryExpression();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual void ToCanonicalForm();
	

	/*!
		\brief It creates a new unary expression.

		It creates a new unary expression, given the operand, and the operator prepended/postpended to it.
		
		\param Expression Pointer to the operand.
		\param RightExpression Pointer to the right operand.
		\param Operator Operator prepended/postpended to the operand.
	*/
	PFLUnaryExpression(PFLExpression *Expression, PFLOperator Operator);

	/*!
		\brief	Returns the operator prepended/postpended to the operand.

		\return	The operator of the expression.

	*/
	PFLOperator GetOperator()
	{
		return this->m_Operator;
	}

	
	/*!
		\brief Returns the operand of the expression

		\return Pointer to the operand of the expression.
	*/
	PFLExpression* GetChild()
	{
		return this->m_Node;
	}

	/*!
		\brief Sets the operand of the expression
		\param child Pointer to the operand.
		\return nothing
	*/
	void SetChild(PFLExpression *child)
	{
		m_Node = child;
	}

private:
	
	/*!
		\brief The operator of the expression.
	*/
	PFLOperator m_Operator;
	
	/*!
		\brief The operand of the expression.
	*/
	PFLExpression* m_Node;
};

/*!
	\brief	It represents a leaf node.
	
	Any leaf node represents a boolean term in the filter expression, i.e. a term that evaluates the presence of a specific protocol
	in the packet, or an IR expression that evaluates a condition on some protocol fields
*/
class PFLTermExpression : public PFLExpression
{

protected:
	/*!
		\brief The protocol (symbol) associated to the boolean terminal node
	*/
	SymbolProto *m_Protocol;
	Node		*m_IRExpr;
	uint32		m_Ind;

public:
	~PFLTermExpression();
	virtual void Printme(int level);
	virtual void PrintMeDotFormat(ostream &outFile);
	virtual void ToCanonicalForm();
	


	SymbolProto *GetProtocol()
	{
		return m_Protocol;
	}

	void SetProtocol(SymbolProto *protocol)
	{
		m_Protocol = protocol;
	}

	Node *GetIRExpr()
	{
		return m_IRExpr;
	}

	void SetIRExpr(Node *irExpr)
	{
		m_IRExpr = irExpr;
	}

	uint32 GetIndex()
	{
		return m_Ind;
	}

	void SetIndex(uint32 index)
	{
		m_Ind = index;
	}

	/*!
		\brief It creates a leaf node

		\param proto a protocol symbol
		\param an IR expression node that evaluates a condition
	*/
	PFLTermExpression(SymbolProto *proto, Node *expr = 0, uint32 index = 0);
};



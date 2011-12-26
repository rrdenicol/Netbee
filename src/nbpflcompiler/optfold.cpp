/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "optfold.h"



bool OptConstFolding::SimplifyTree(Node *&node)
{
	m_TreeChanged  = false;

	node = SimplifyExpr(node);
	
	return m_TreeChanged;
}

Node *OptConstFolding::SimplifyExpr(Node *node)
{
	if (node->IsBinOp())
		return SimplifyBinExpr(node);
	if (node->IsUnOp())
		return SimplifyUnExpr(node);
	
	return node;
}

Node *OptConstFolding::SimplifyUnExpr(Node *node)
{
	nbASSERT(node->IsUnOp(), "node MUST be a unary expression");

	Node *oldChild = node->GetLeftChild(); 

	nbASSERT(oldChild != NULL, "child cannot be NULL");

	Node *newChild = SimplifyExpr(oldChild);
	

	nbASSERT(newChild != NULL, "child cannot be NULL after simplification");

	if (oldChild != newChild)
	{
		delete oldChild;
		node->SetLeftChild(newChild);
		m_TreeChanged = true;
	}

	if (newChild->IsConst())
		return EvalUnExpr(node);

	return node;
}

Node *OptConstFolding::SimplifyBinExpr(Node *node)
{

	nbASSERT(node->IsBinOp(), "node MUST be a binary expression");

	Node *oldLeftChild(node->GetLeftChild());
	Node *oldRightChild(node->GetRightChild());

	nbASSERT(!(oldLeftChild == NULL || oldRightChild == NULL), "node kids cannot be both NULL");


	Node *newLeftChild = SimplifyExpr(oldLeftChild);
	
	nbASSERT(newLeftChild != NULL, "child cannot be NULL after simplification");

	if (oldLeftChild != newLeftChild)
	{
		delete oldLeftChild;
		node->SetLeftChild(newLeftChild);
		m_TreeChanged = true;
	}


	Node *newRightChild = SimplifyExpr(oldRightChild);
	
	nbASSERT(newRightChild != NULL, "child cannot be NULL after simplification");

	if (oldRightChild != newRightChild)
	{
		delete oldRightChild;
		node->SetRightChild(newRightChild);
		m_TreeChanged = true;
	}

	if (!newLeftChild->IsConst())
		return node;

	if (!newRightChild->IsConst())
		return node;

	//at this point both kids should be a constant terminal node
	return EvalBinExpr(node);
}


Node *OptConstFolding::EvalUnExpr(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	Node *child(node->GetLeftChild());
	nbASSERT(child != NULL, "child cannot be NULL");
	if (!child->IsConst())
		return node;

	if (node->Op < HIR_LAST_OP)
	{
		nbASSERT(child->Sym != NULL, "child symbol cannot be NULL");
	}

	uint32 intVal(0);
	switch(node->Op)
	{
		case IR_NOTB:
			nbASSERT(child->Sym->SymKind == SYM_INT_CONST, "child symbol must be an integer constant");
			intVal = ((SymbolIntConst *)child->Sym)->Value;
			return m_CodeGen.TermNode(IR_BCONST, m_CodeGen.ConstIntSymbol(!intVal));
			break;

		case IR_NOTI:
			nbASSERT(child->Sym->SymKind == SYM_INT_CONST, "child symbol must be an integer constant");
			intVal = ((SymbolIntConst *)child->Sym)->Value;
			return m_CodeGen.TermNode(IR_BCONST, m_CodeGen.ConstIntSymbol(~intVal));
			break;

		case IR_NEGI:
			nbASSERT(child->Sym->SymKind == SYM_INT_CONST, "child symbol must be an integer constant");
			intVal = ((SymbolIntConst *)child->Sym)->Value;
			return m_CodeGen.TermNode(IR_BCONST, m_CodeGen.ConstIntSymbol(-(int32)intVal));
			break;
		case IR_CINT:
			nbASSERT(child->Sym->SymKind == SYM_STR_CONST, "child symbol must be a string constant");
			nbASSERT(((SymbolStrConst *)child->Sym)->Size <= 4, "string constant must be of size < 4");
			if (str2int(((SymbolStrConst *)child->Sym)->Name.c_str(), &intVal, 10) != 0)
			{
				nbASSERT(false, "cannot convert string to integer");
			}
			return m_CodeGen.TermNode(IR_BCONST, m_CodeGen.ConstIntSymbol(intVal));
			break;
		case NOT:
			nbASSERT(child->Op == PUSH, "child must be an integer constant");
			intVal = child->Value;
			return m_CodeGen.TermNode(PUSH, ~intVal);
			break;
		case NEG:
			nbASSERT(child->Op == PUSH, "child  must be an integer constant");
			intVal = child->Value;
			return m_CodeGen.TermNode(PUSH, -intVal);

		default:
			//nbASSERT(false, "CANNOT BE HERE");
			return node;
			break;
	}
}


Node *OptConstFolding::EvalBinExpr(Node *node)
{
	Node *leftChild(node->GetLeftChild());
	Node *rightChild(node->GetRightChild());

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	if (!leftChild->IsConst())
		return node;

	if (!rightChild->IsConst())
		return node;

	uint32 leftVal(0);
	uint32 rightVal(0);

	if (node->Op < HIR_LAST_OP)
	{
		nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
		nbASSERT(rightChild->Sym != NULL, "right child symbol cannot be NULL");
		leftVal = ((SymbolIntConst *)leftChild->Sym)->Value;
		rightVal = ((SymbolIntConst *)rightChild->Sym)->Value;
	}
	else
	{
		nbASSERT(leftChild->Op == PUSH, "left child operator must be PUSH");
		nbASSERT(rightChild->Op == PUSH, "right child operator must be PUSH");
		leftVal = leftChild->Value;
		rightVal = rightChild->Value;
	}


	switch(node->Op)
	{
		case IR_ADDI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal + rightVal));
			break;

		case IR_SUBI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal - rightVal));
			break;

		case IR_DIVI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal / rightVal));
			break;

		case IR_MULI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal * rightVal));
			break;

		case IR_SHLI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal << rightVal));
			break;

		case IR_MODI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal % rightVal));
			break;

		case IR_SHRI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal >> rightVal));
			break;

		case IR_ANDI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal & rightVal));
			break;

		case IR_ORI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal | rightVal));
			break;

		case IR_XORI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal ^ rightVal));
			break;

		case IR_ANDB:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal && rightVal));
			break;

		case IR_ORB:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal || rightVal));
			break;

		case IR_EQI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal == rightVal));
			break;

		case IR_GEI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal >= rightVal));
			break;

		case IR_GTI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal > rightVal));
			break;

		case IR_LEI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal <= rightVal));
			break;

		case IR_LTI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal < rightVal));
			break;

		case IR_NEI:
			return m_CodeGen.TermNode(IR_ICONST, m_CodeGen.ConstIntSymbol(leftVal != rightVal));
			break;

		/***** NETVM IR OPERATORS *****/

		case ADD:
			return m_CodeGen.TermNode(PUSH, leftVal + rightVal);
			break;

		case SUB:
			return m_CodeGen.TermNode(PUSH, leftVal - rightVal);
			break;

		case IMUL:
			return m_CodeGen.TermNode(PUSH, leftVal * rightVal);
			break;

		case SHL:
			return m_CodeGen.TermNode(PUSH, leftVal << rightVal);;
			break;

		case MOD:
			return m_CodeGen.TermNode(PUSH, leftVal % rightVal);
			break;

		case SHR:
			return m_CodeGen.TermNode(PUSH, leftVal >> rightVal);
			break;

		case AND:
			return m_CodeGen.TermNode(PUSH, leftVal & rightVal);
			break;

		case OR:
			return m_CodeGen.TermNode(PUSH, leftVal | rightVal);
			break;

		case XOR:
			return m_CodeGen.TermNode(PUSH, leftVal ^ rightVal);
			break;
		default:
			//nbASSERT(false, "CANNOT BE HERE");
			return node;
			break;
	}

}



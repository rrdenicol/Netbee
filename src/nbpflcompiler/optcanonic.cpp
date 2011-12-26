/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "optcanonic.h"
#include "dump.h"


bool OptCanonic::SimplifyTree(Node *&node)
{
	m_TreeChanged  = false;

	node = SimplifyExpr(node);
	
	return m_TreeChanged;
}

Node *OptCanonic::SimplifyExpr(Node *node)
{
	if (node->IsBinOp())
		return SimplifyBinExpr(node);
	if (node->IsUnOp())
		return SimplifyUnExpr(node);
	
	return node;
}

Node *OptCanonic::SimplifyUnExpr(Node *node)
{
	nbASSERT(node->GetRightChild() == NULL && node->GetLeftChild() != NULL , "node MUST be a unary expression");

	Node *oldChild = node->GetLeftChild(); 

	nbASSERT(oldChild != NULL, "child cannot be NULL");

	node->SetLeftChild(SimplifyExpr(oldChild));

	return EvalUnExpr(node);
}

Node *OptCanonic::SimplifyBinExpr(Node *node)
{

	nbASSERT(node->GetRightChild() != NULL && node->GetLeftChild() != NULL , "node MUST be a binary expression");

	Node *oldLeftChild(node->GetLeftChild());
	Node *oldRightChild(node->GetRightChild());


	node->SetLeftChild( SimplifyExpr(oldLeftChild));
	
	node->SetRightChild(SimplifyExpr(oldRightChild));


	//at this point both kids should be a constant terminal node
	return EvalBinExpr(node);
}


Node *OptCanonic::EvalUnExpr(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");

	Node *child(node->GetLeftChild());
	nbASSERT(child != NULL, "child cannot be NULL");
	
	// performs the transformation not(not(x))==> x
	if (node->Op != IR_NOTB)
		return node;

	if (child->Op != IR_NOTI && child->Op != IR_NOTB)
		return node;
	
	//save the "grand-child" (i.e. x)
	Node *newNode = child->GetLeftChild();
	//unlink the "grand-child" from the child
	child->SetLeftChild(NULL);
	delete child;
	m_TreeChanged = true;
	return newNode;
}

Node *OptCanonic::EvalIdentity(Node *node)
{
	Node *leftChild(node->GetLeftChild());
	Node *rightChild(node->GetRightChild());

	if (leftChild->IsConst() && !rightChild->IsConst())
		return node;

	if (!leftChild->IsConst() && !rightChild->IsConst())
		return node;

	/*cout << "Eval IDENTITY - node: ";
	CodeWriter cw(cout);
	cw.DumpTree(node);
	cout <<endl;
	cout.flush();*/
	uint32 rightVal = 0;
	if (node->Op < HIR_LAST_OP)
	{
		rightVal = ((SymbolIntConst *)rightChild->Sym)->Value;
	}
	else
	{
		nbASSERT(rightChild->Op == PUSH, "operator should be PUSH");
		rightVal = rightChild->Value;
	}


	Node *result = node;

	switch(node->Op)
	{
		case IR_ADDI:
		case IR_SUBI:
		case IR_SHLI:
		case IR_SHRI:
		case IR_ORB:
		case IR_ORI:
		case ADD:
		case SUB:
		case SHL:
		case SHR:
		case OR:
			// i + 0 = i - 0 = i << 0 = i >> 0 = i | 0 = i
			if (rightVal == 0)
			{
				node->SetLeftChild(NULL);
				delete node;
				result = leftChild;
			}
			break;

		case IR_MULI:
		case IMUL:
			if (rightVal == 1)
			{
				// i * 1 = i
				node->SetLeftChild(NULL);
				delete node;
				result = leftChild;
			}
			else if (rightVal == 0)
			{
				// i * 0 = 0
				node->SetRightChild(NULL);
				delete node;
				result = rightChild;
			}
			break;

		case IR_DIVI:
			// i / 1 = i
			if (rightVal == 1)
			{
				node->SetLeftChild(NULL);
				delete node;
				result = leftChild;
			}
			break;


		case IR_ANDI:
		case AND:
			if (rightVal == 0)
			{
				// i & 0 = 0
				node->SetRightChild(NULL);
				delete node;
				result = rightChild;
			}
			break;


		case IR_ANDB:
			if (rightVal > 0)
			{
				// i && true = i
				node->SetLeftChild(NULL);
				delete node;
				result = leftChild;
			}
			else if (rightVal == 0)
			{
				// i && 0 = 0
				node->SetRightChild(NULL);
				delete node;
				result = rightChild;
			}
			break;
	}
	if (result != node)
		m_TreeChanged = true;
	
	return result;

}

Node *OptCanonic::EvalBinExpr(Node *node)
{
	/*
	CodeWriter cw(cout);
	cout << "EvalBinExpr - node: ";
	cw.DumpTree(node);
	cout << endl;
	*/
	CommuteExpr(node);
	/*
	cout << "  After Commuting: ";
	cw.DumpTree(node);
	cout << endl;
	cout.flush();
	*/
	//maybe there is the opportunity for other transformations
	return EvalIdentity(node);
}


void OptCanonic::CommuteExpr(Node *node)
{
	switch (node->Op)
	{
		case IR_ADDI:
		case IR_MULI:
		case ADD:
		case IMUL:
		{
			Node *leftChild(node->GetLeftChild());
			Node *rightChild(node->GetLeftChild());

			//!\todo om we should expand this function in order to better support commutative expressions

			//a const node must be in the right branch
			if (leftChild->IsConst() && !rightChild->IsConst())
			{
				if (leftChild->Op > HIR_LAST_OP)
				{
					nbASSERT(leftChild->Op == PUSH, "left child operator should be PUSH");
				}
				node->SwapChilds();
			}
		}break;
	}
}

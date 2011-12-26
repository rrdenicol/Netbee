/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "pflexpression.h"
#include "dump.h"




//!\ brief literal values corresponding to the elements of the \ref PFLOperator enumeration

char *PFLOperators[] = 
{
	"and",			///< Logical AND
	"or",			///< Logical OR
	"boolnot",		///< Boolean negation
};



uint32 PFLExpression::m_Count = 0;



PFLStatement::PFLStatement(PFLExpression *exp, PFLAction *action, PFLIndex *index)
	:m_Exp(exp), m_Action(action), m_HeaderIndex(index)
{
}
	
PFLStatement::~PFLStatement()
{
	delete m_Exp;
	delete m_Action;
	delete m_HeaderIndex;
}


PFLExpression *PFLStatement::GetExpression(void)
{
	return m_Exp;
}


PFLAction *PFLStatement::GetAction(void)
{
	return m_Action;
}

PFLIndex *PFLStatement::GetHeaderIndex(void)
{
	return m_HeaderIndex;
}

PFLAction::~PFLAction()
{

}

PFLBinaryExpression::PFLBinaryExpression(PFLExpression *LeftExpression, PFLExpression *RightExpression, PFLOperator Operator)
	:PFLExpression(PFL_BINARY_EXPRESSION), m_LeftNode(LeftExpression), m_RightNode(RightExpression), m_Operator(Operator)
{
}

PFLBinaryExpression::~PFLBinaryExpression()
{
	if (m_LeftNode != NULL)
		delete m_LeftNode;
	
	if (m_RightNode != NULL)
		delete m_RightNode;
}

void PFLBinaryExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLBinaryExpression\n");		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr," LeftChild\n");		

	if (m_LeftNode != NULL)
	{

		m_LeftNode->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr," RightChild\n");		

	if (m_RightNode != NULL)
	{

		m_RightNode->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		


	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLBinaryExpression\n");		
}

void PFLBinaryExpression::PrintMeDotFormat(ostream &outFile)
{
	outFile << "expr" << GetID() << "[label=\"" << PFLOperators[m_Operator] << "\"];" << endl;
	m_LeftNode->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_LeftNode->GetID() << ";" << endl;
	m_RightNode->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_RightNode->GetID() << ";" << endl;
	
}

void PFLBinaryExpression::SwapChilds()
{
	PFLExpression *tmp = m_LeftNode;
	m_LeftNode = m_RightNode;
	m_RightNode = tmp;
}

void PFLBinaryExpression::ToCanonicalForm()
{
	m_LeftNode->ToCanonicalForm();
	m_RightNode->ToCanonicalForm();

	if (m_LeftNode->IsConst() && !m_RightNode->IsConst())
		SwapChilds();
}

PFLUnaryExpression::PFLUnaryExpression(PFLExpression *Expression, PFLOperator Operator)
	:PFLExpression(PFL_UNARY_EXPRESSION), m_Operator(Operator), m_Node(Expression)
{
}

PFLUnaryExpression::~PFLUnaryExpression()
{
	if (m_Node != NULL)
		delete m_Node;
}

void PFLUnaryExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLUnaryExpression");		
	fprintf(stderr, " %s\n", PFLOperators[m_Operator]);
	
	if (m_Node != NULL)
	{
		m_Node->Printme(level+1);
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL\n");
	}		

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLUnaryExpression\n");		
}

void PFLUnaryExpression::PrintMeDotFormat(ostream &outFile)
{
	outFile << "expr" << GetID() << "[label=\"" << PFLOperators[m_Operator] << "\"];" << endl;
	m_Node->PrintMeDotFormat(outFile);
	outFile << "expr" << GetID() << "->expr" << m_Node->GetID() << ";" << endl;
}


void PFLUnaryExpression::ToCanonicalForm()
{
	m_Node->ToCanonicalForm();
	return;
}


PFLTermExpression::PFLTermExpression(SymbolProto *proto, Node *expr, uint32 index)
	:PFLExpression(PFL_TERM_EXPRESSION), m_Protocol(proto), m_IRExpr(expr), m_Ind(index)
{
}


PFLTermExpression::~PFLTermExpression()
{
}





void PFLTermExpression::Printme(int level)
{
	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"-->PFLTermExpression\n");		
	
	nbASSERT(m_Protocol != NULL, "Protocol cannot be NULL");

	if (m_Protocol != NULL)
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"%s\n", m_Protocol->Name.c_str());
	}
	else
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");
		fprintf(stderr,"NULL PROTO\n");
	}		

	if (m_IRExpr != NULL)
	{
		for (int i=0; i< level+1;i++) fprintf(stderr,"\t");

		//DumpTree(stderr, m_IRExpr, 0);
	}
	

	for (int i=0; i< level;i++) fprintf(stderr,"\t");
	fprintf(stderr,"<--PFLTermExpression\n");		
}

void PFLTermExpression::PrintMeDotFormat(ostream &outFile)
{
	nbASSERT(!(m_Protocol == NULL && m_IRExpr == NULL), "Terminal node should refer to at least one protocol or to a valid IR expression tree");
	outFile << "expr" << GetID() << "[label=\"";
	if (m_Protocol != NULL)
		outFile << "proto: " << m_Protocol->Name << "\\n";
	if (m_IRExpr != NULL)
	{
		outFile << "cond: ";
		CodeWriter codeWriter(outFile);
		codeWriter.DumpTree(m_IRExpr, 0);
	}
	outFile << "\"];" << endl;
}


void PFLTermExpression::ToCanonicalForm()
{
	return;
}


PFLExpression::~PFLExpression()
{
}







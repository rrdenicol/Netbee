/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "irlowering.h"
#include <math.h> //for log10()
#include "statements.h"
#include "../nbee/globals/debug.h"


void IRLowering::LowerHIRCode(CodeList *code, SymbolProto *proto, string comment)
{
	m_Protocol = proto;

	LowerHIRCode(code, comment);

	if(proto->proto_MultiField)
	{
		m_CodeGen.CommentStatement(string("MultiField - incremento variabile di estrazione"));
		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, proto->proto_Extract), m_CodeGen.TermNode(PUSH, 1)), proto->proto_Extract));
	}

	if(proto->ExAllfields)
	{	//store the number of fields extracted
		Node *position=m_CodeGen.TermNode(PUSH, proto->beginPosition);
		m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,proto->NExFields), position));
	}
}

void IRLowering::LowerHIRCode(CodeList *code, string comment)
{
	if (comment.compare("")!=0)
		m_CodeGen.CommentStatement(comment);
	LowerHIRCode(code);
}

void IRLowering::LowerHIRCode(CodeList *code)
{
	StmtBase *next = code->Front();

	while(next)
	{
		TranslateStatement(next);
		next = next->Next;
	}
}

void IRLowering::TranslateLabel(StmtLabel *stmt)
{
	SymbolLabel *label = ManageLinkedLabel((SymbolLabel*)stmt->Forest->Sym, "");
	m_CodeGen.LabelStatement(label);
	//StmtLabel *lblStmt = m_CodeGen.LabelStatement(label);
	//lblStmt->Comment = stmt->Comment;
}

void IRLowering::TranslateGen(StmtGen *stmt)
{
	Node *tree = stmt->Forest;

	nbASSERT(tree != NULL, "Forest cannot be NULL in a Gen Statement");
	switch (tree->Op)
	{
	case IR_DEFFLD:
		TranslateFieldDef(tree);
		break;

	case IR_DEFVARI:
		TranslateVarDeclInt(tree);
		break;

	case IR_DEFVARS:
		TranslateVarDeclStr(tree);
		break;

	case IR_ASGNI:
		TranslateAssignInt(tree);
		break;

	case IR_ASGNS:
		TranslateAssignStr(tree);
		break;

	case IR_LKADD:
		TranslateLookupAdd(tree);
		break;

	case IR_LKDEL:
		TranslateLookupDelete(tree);
		break;

	case IR_LKHIT:
	case IR_LKSEL:
		{
			SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)tree->Sym;
			nbASSERT(tree->GetLeftChild() != NULL, "Lookup select instruction should specify a keys list");
			SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)tree->GetLeftChild()->Sym;
			TranslateLookupSelect(entry, keys);
		}break;

	case IR_LKUPDS:
	case IR_LKUPDI:
		TranslateLookupUpdate(tree);
		break;

	case IR_DATA:
		{
			SymbolDataItem *data = (SymbolDataItem *)tree->Sym;
			m_CodeGen.GenStatement(m_CodeGen.TermNode(IR_DATA, data));
		}break;

	default:
		nbASSERT(false, "IRLOWERING::translateGen: CANNOT BE HERE");
		break;
	}
}


SymbolLabel *IRLowering::ManageLinkedLabel(SymbolLabel *label, string name)
{
	nbASSERT(label != NULL, "label cannot be NULL");

	if (label->LblKind != LBL_LINKED)
		return label;

	if (label->Linked != NULL)
		return label->Linked;

	label->Linked = m_CodeGen.NewLabel(LBL_CODE, name);
	return label->Linked;
}

void IRLowering::TranslateJump(StmtJump *stmt)
{
	nbASSERT(stmt->TrueBranch != NULL, "true branch should not be NULL");

	SymbolLabel *trueBranch = ManageLinkedLabel(stmt->TrueBranch, "jump_true");

	if (stmt->Forest == NULL)
	{
		m_CodeGen.JumpStatement(JUMPW, trueBranch);
		return;
	}

	nbASSERT(stmt->FalseBranch != NULL, "false branch should not be NULL");
	SymbolLabel *falseBranch = ManageLinkedLabel(stmt->FalseBranch, "jump_false");
	JCondInfo jcInfo(trueBranch, falseBranch);
	TranslateBoolExpr(stmt->Forest, jcInfo);
}


void IRLowering::TranslateRelOpInt(uint16 op, Node *relopExpr, JCondInfo &jcInfo)
{
	Node *leftExpr = TranslateTree(relopExpr->GetLeftChild());
	Node *rightExpr = TranslateTree(relopExpr->GetRightChild());

	if (leftExpr && rightExpr)
	{
		m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, m_CodeGen.BinOp(op, leftExpr, rightExpr));
	}
	else
	{
		m_CodeGen.CommentStatement(string("ERROR: One of two operands was not translated, jump to false by default."));
		m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
	}
}

void IRLowering::TranslateRelOpStr(uint16 op, Node *relopExpr, JCondInfo &jcInfo)
{
#ifdef USE_STRING_COMPARISON
	// operand nodes
	Node *leftExpr = 0, *rightExpr = 0;
	// constant sizes (if a size is known at run-time, the constant size will be 0)
	uint32 leftSize = 0, rightSize = 0;
	// node to load sizes
	Node *leftSizeNode = 0, *rightSizeNode = 0;

	this->TranslateRelOpStrOperand(relopExpr->GetLeftChild(), &leftExpr, &leftSizeNode, &leftSize);
	this->TranslateRelOpStrOperand(relopExpr->GetRightChild(), &rightExpr, &rightSizeNode, &rightSize);

	if (leftExpr!=NULL && rightExpr!=NULL)
	{
		switch (op)
		{
		case JFLDEQ:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					else
						m_CodeGen.JumpStatement(jcInfo.FalseLbl);
				}
				else
				{
					//	if ( size(string1)==size(string2) && EQ(string1, string2, size(string1)) )
					//		true;
					//	else
					//		false;
					SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.JCondStatement(JCMPEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, m_CodeGen.TermNode(LOCLD, size_a), rightSizeNode);
					m_CodeGen.LabelStatement(leftTrue);
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(LOCLD, size_a));
				}
			}break;
		case JFLDNEQ:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					else
						m_CodeGen.JumpStatement(jcInfo.TrueLbl);
				}
				else
				{
					// if ( size(string1)!=size(string2) || NEQ(string1, string2, size(string1)) )
					//		true;
					//	else
					//		false;
					SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.JCondStatement(JCMPNEQ, orJCInfo.TrueLbl, orJCInfo.FalseLbl, m_CodeGen.TermNode(LOCLD, size_a), rightSizeNode);
					m_CodeGen.LabelStatement(leftFalse);
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(LOCLD, size_a));
				}
			}break;
		case JFLDGT:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
					{
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					}
					else
					{
						uint32 minSize=min(leftSize, rightSize);

						//	if ( GT(string1, string2, mimSize) || ( EQ(string1, string2, mimSize) && size(string1)>size(string2) ) )
						//		true;
						//	else
						//		false;
						SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
						m_CodeGen.JFieldStatement(op, orJCInfo.TrueLbl, orJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftFalse);

						SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
						m_CodeGen.JFieldStatement(JFLDEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftTrue);
						m_CodeGen.JCondStatement(JCMPG, jcInfo.TrueLbl, jcInfo.FalseLbl, leftSizeNode, rightSizeNode);
					}
				}
				else
				{
					SymbolTemp *strMinSize=m_CodeGen.NewTemp("str_min_size", m_CompUnit.NumLocals);
					Node *loadStrMinSize=m_CodeGen.TermNode(LOCLD, strMinSize);
					SymbolLabel *minTrue=m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *minFalse=m_CodeGen.NewLabel(LBL_CODE, "if_false");
					SymbolLabel *minDone=m_CodeGen.NewLabel(LBL_CODE, "end_if");
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					SymbolTemp *size_b=m_CodeGen.NewTemp("str_size_b", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, rightSizeNode, size_b));
					m_CodeGen.JCondStatement(JCMPLE, minTrue, minFalse, m_CodeGen.TermNode(LOCLD, size_a), m_CodeGen.TermNode(LOCLD, size_b));
					m_CodeGen.LabelStatement(minTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_a), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_b), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minDone);
					m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, loadStrMinSize);
				}
			}break;
		case JFLDLT:
			{
				if (leftSize>0 && rightSize>0)
				{
					// both sizes are known at compile-time
					if (leftSize==rightSize)
					{
						m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, leftSizeNode);
					}
					else
					{
						uint32 minSize=min(leftSize, rightSize);

						//	if ( LT(string1, string2, mimSize) || ( EQ(string1, string2, mimSize) && size(string1)<size(string2) ) )
						//		true;
						//	else
						//		false;
						SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
						m_CodeGen.JFieldStatement(op, orJCInfo.TrueLbl, orJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftFalse);

						SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
						m_CodeGen.JFieldStatement(JFLDEQ, andJCInfo.TrueLbl, andJCInfo.FalseLbl, leftExpr, rightExpr, m_CodeGen.TermNode(PUSH, minSize));
						m_CodeGen.LabelStatement(leftTrue);
						m_CodeGen.JCondStatement(JCMPL, jcInfo.TrueLbl, jcInfo.FalseLbl, leftSizeNode, rightSizeNode);
					}
				}
				else
				{
					SymbolTemp *strMinSize=m_CodeGen.NewTemp("str_min_size", m_CompUnit.NumLocals);
					Node *loadStrMinSize=m_CodeGen.TermNode(LOCLD, strMinSize);
					SymbolLabel *minTrue=m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *minFalse=m_CodeGen.NewLabel(LBL_CODE, "if_false");
					SymbolLabel *minDone=m_CodeGen.NewLabel(LBL_CODE, "end_if");
					SymbolTemp *size_a=m_CodeGen.NewTemp("str_size_a", m_CompUnit.NumLocals);
					SymbolTemp *size_b=m_CodeGen.NewTemp("str_size_b", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, leftSizeNode, size_a));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, rightSizeNode, size_b));
					m_CodeGen.JCondStatement(JCMPLE, minTrue, minFalse, m_CodeGen.TermNode(LOCLD, size_a), m_CodeGen.TermNode(LOCLD, size_b));
					m_CodeGen.LabelStatement(minTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_a), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, size_b), strMinSize));
					m_CodeGen.JumpStatement(JUMPW, minDone);
					m_CodeGen.LabelStatement(minDone);
					m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JFieldStatement(op, jcInfo.TrueLbl, jcInfo.FalseLbl, leftExpr, rightExpr, loadStrMinSize);
				}
			}break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
	}
	else
	{
		this->GenerateWarning("One of the operand of the string comparison has not been defined.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
		m_CodeGen.CommentStatement("ERROR: One of the operand of the string comparison has not been defined.");
	}
#else
	this->GenerateWarning("String comparisons disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: String comparisons disabled.");
#endif
}

void IRLowering::TranslateRelOpStrOperand(Node *operand, Node **loadOperand, Node **loadSize, uint32 *size)
{
	switch(operand->Op)
	{
	case IR_SCONST:
		{
			SymbolTemp *memTemp=m_CodeGen.NewTemp(string("const_str_tmp_offs"), this->m_CompUnit.NumLocals);
			m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, memTemp, m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)operand->Sym)->MemOffset)));
			*loadOperand = m_CodeGen.TermNode(LOCLD, memTemp);
			*size = ((SymbolStrConst*)operand->Sym)->Size;
			*loadSize = m_CodeGen.TermNode(PUSH, *size);
		}break;
	case IR_FIELD:
		{
			switch (((SymbolField *)operand->Sym)->FieldType)
			{
			case PDL_FIELD_FIXED:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldFixed *)operand->Sym)->IndexTemp);
				*size = ((SymbolFieldFixed *)operand->Sym)->Size;
				*loadSize = m_CodeGen.TermNode(PUSH, *size);
				break;
			case PDL_FIELD_VARLEN:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_TOKEND:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_TOKWRAP:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_LINE:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)operand->Sym)->LenTemp);
				break;

			case PDL_FIELD_PATTERN:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)operand->Sym)->LenTemp);
				break;
			case PDL_FIELD_EATALL:
				*loadOperand = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)operand->Sym)->IndexTemp);
				*loadSize = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)operand->Sym)->LenTemp);
				break;
			default:
				this->GenerateWarning("Support for string comparisons with fields is supported only for fixed, variable length, tokenended, tokenwrapped, line, pattern or eatall fields.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
				break;
			}
		}break;
	case IR_SVAR:
		{
			SymbolVariable *var=(SymbolVariable *)operand->Sym;

			if (var->VarType==PDL_RT_VAR_REFERENCE)
			{
				SymbolVarBufRef *refVar=(SymbolVarBufRef *)var;
				if (refVar->Name.compare("$packet")==0)
				{
					Node *leftKid=operand->GetLeftChild();
					Node *rightKid=operand->GetRightChild();
					if (leftKid!=NULL && rightKid!=NULL)
					{
						*loadOperand=TranslateTree(leftKid);
						*loadSize=TranslateTree(rightKid);
						//// in this case, we have a offset...
						//switch (leftKid->Sym->SymKind)
						//{
						//case SYM_RT_VAR:
						//	if (((SymbolVariable *)leftKid->Sym)->VarType==PDL_RT_VAR_INTEGER && ((SymbolVariable *)leftKid->Sym)->Name.compare("$currentoffset")==0)
						//		*loadOperand=m_CodeGen.TermNode(LOCLD, m_CodeGen.CurrOffsTemp());
						//	break;
						//}

						//// ... and a size already specified as node
						//switch (rightKid->Sym->SymKind)
						//{
						//case SYM_INT_CONST:
						//	// if we have a constant size, let's store it, we can optimize the code
						//	*size=((SymbolIntConst *)rightKid->Sym)->Value;
						//	*loadSize=m_CodeGen.TermNode(PUSH, *size);
						//	break;
						//default:
						//	*loadSize=m_CodeGen.TermNode(LOCLD, rightKid->Sym);
						//	break;
						//}
					}
					else
					{
						this->GenerateWarning("Support for string comparisons using $packet is supported only with both starting offset and size specified.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
						break;
					}
				}
				else
				{
					if (refVar->IndexTemp==NULL || refVar->LenTemp==NULL)
					{
						this->GenerateWarning("A variable to be used as string should define offset and size.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
						break;
					}

					*loadOperand=m_CodeGen.TermNode(LOCLD, refVar->IndexTemp);

					if (refVar->GetFixedSize()==0)
					{
						// size is not known at compile-time, we should use the symbol to refer to the length
						*loadSize = m_CodeGen.TermNode(LOCLD, refVar->LenTemp);
					}
					else
					{
						// size is fixed, we can use this value
						*size=refVar->GetFixedSize();
						*loadSize = m_CodeGen.TermNode(PUSH, *size);
					}
				}
			}
			else
			{
				this->GenerateWarning("Support for string comparisons with variables is supported only for buffer references.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
				break;
			}
			break;
		}break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

void IRLowering::TranslateBoolExpr(Node *expr, JCondInfo &jcInfo)
{
	switch(expr->Op)
	{
	case IR_NOTB:
		{
			JCondInfo notJCInfo(jcInfo.FalseLbl, jcInfo.TrueLbl);
			TranslateBoolExpr(expr->GetLeftChild(), notJCInfo);
		};break;
	case IR_ANDB:
		{
			SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			JCondInfo andJCInfo(leftTrue, jcInfo.FalseLbl);
			TranslateBoolExpr(expr->GetLeftChild(), andJCInfo);
			m_CodeGen.LabelStatement(leftTrue);
			TranslateBoolExpr(expr->GetRightChild(), jcInfo);
		};break;
	case IR_ORB:
		{
			SymbolLabel *leftFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			JCondInfo orJCInfo(jcInfo.TrueLbl, leftFalse);
			TranslateBoolExpr(expr->GetLeftChild(), orJCInfo);
			m_CodeGen.LabelStatement(leftFalse);
			TranslateBoolExpr(expr->GetRightChild(), jcInfo);
		}break;

	case IR_EQI:
		TranslateRelOpInt(JCMPEQ, expr, jcInfo);
		break;
	case IR_GEI:
		TranslateRelOpInt(JCMPGE, expr, jcInfo);
		break;
	case IR_GTI:
		TranslateRelOpInt(JCMPG, expr, jcInfo);
		break;
	case IR_LEI:
		TranslateRelOpInt(JCMPLE, expr, jcInfo);
		break;
	case IR_LTI:
		TranslateRelOpInt(JCMPL, expr, jcInfo);
		break;
	case IR_NEI:
		TranslateRelOpInt(JCMPNEQ, expr, jcInfo);
		break;

	case IR_EQS:
		TranslateRelOpStr(JFLDEQ, expr, jcInfo);
		break;
	case IR_NES:
		TranslateRelOpStr(JFLDNEQ, expr, jcInfo);
		break;
	case IR_GTS:
		TranslateRelOpStr(JFLDGT, expr, jcInfo);
		break;
	case IR_LTS:
		TranslateRelOpStr(JFLDLT, expr, jcInfo);
		break;

	case IR_ICONST:
		{
			SymbolIntConst *sym = (SymbolIntConst*)expr->Sym;
			if (sym->Value)
				m_CodeGen.JumpStatement(JUMPW, jcInfo.TrueLbl);
			else
				m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
		};break;

	case IR_IVAR:
		{
			SymbolVariable *sym = (SymbolVariable *)expr->Sym;
			switch (sym->VarType)
			{
			case PDL_RT_VAR_INTEGER:
				{
					SymbolVarInt *symInt=(SymbolVarInt *)sym;

					if (symInt->Temp!=NULL) {
						Node *jump=m_CodeGen.BinOp(JCMPGE, m_CodeGen.TermNode(LOCLD, symInt->Temp), m_CodeGen.TermNode(PUSH, 1));
						m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
					} else {
						this->GenerateWarning(string("The int variable ")+sym->Name+string(" is unassigned"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
						m_CodeGen.CommentStatement(string("ERROR: The int variable ")+sym->Name+string(" is unassigned"));
					}
				}break;
			default:
				{
					this->GenerateWarning(string("The type of the variable ")+sym->Name+string(" is not supported in a boolean expression"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
					m_CodeGen.CommentStatement(string("ERROR: The type of the variable ")+sym->Name+string(" is not supported in a boolean expression"));
				}
			}

		};break;

	case IR_CINT:
		{
			Node *cint=TranslateCInt(expr);

			if (cint!=NULL)
			{
				Node *jump=m_CodeGen.BinOp(JCMPGE, cint, m_CodeGen.TermNode(PUSH, 1));
				m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
			}
		}break;

	case IR_LKSEL:
		TranslateRelOpLookup(IR_LKSEL, expr, jcInfo);
		break;

	case IR_LKHIT:
		TranslateRelOpLookup(IR_LKHIT, expr, jcInfo);
		break;

	case IR_REGEXFND:
		TranslateRelOpRegEx(expr, jcInfo);
		break;

	default:
		this->GenerateWarning(string("The node type is not supported"), __FILE__, __FUNCTION__, __LINE__, 1, 5);
		m_CodeGen.CommentStatement(string("ERROR: The node type is not supported"));
		break;
	}
}


void IRLowering::TranslateRelOpRegEx(Node *expr, JCondInfo &jcInfo)
{
//#ifdef USE_REGEX
	nbASSERT(expr->Sym != NULL, "The RegExp operand should define a valid RegExp symbol");

	SymbolRegEx *regExp = (SymbolRegEx * )expr->Sym;

	// set pattern id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, regExp->Id),
		new SymbolLabel(LBL_ID, 0, string("regexp")),
		REGEX_OUT_PATTERN_ID));

	// set buffer offset
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		TranslateTree(regExp->Offset),
		new SymbolLabel(LBL_ID, 0, string("regexp")),
		REGEX_OUT_BUF_OFFSET));

	// calculate and set buffer length
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
		new SymbolLabel(LBL_ID, 0, string("regexp")),
		REGEX_OUT_BUF_LENGTH));


	// find the pattern
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

	// check the result
	Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

	Node *leftExpr = validNode;
	Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
	m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));
//#endif
}

void IRLowering::TranslateRelOpLookup(uint16 opCode, Node *expr, JCondInfo &jcInfo)
{
//#ifdef USE_LOOKUPTABLE
	nbASSERT(expr->Sym != NULL, "The lookup selection node should specify a valid lookup table entry symbol");
	nbASSERT(expr->Sym->SymKind == SYM_RT_LOOKUP_ENTRY, "The lookup selection node should specify a valid lookup table entry symbol");
	nbASSERT(expr->GetLeftChild() != NULL && expr->GetLeftChild()->Sym != NULL, "The lookup selection node should specify a valid keys list symbol.");
	nbASSERT(expr->GetLeftChild()->Sym->SymKind == SYM_RT_LOOKUP_KEYS, "The lookup selection node should specify a valid keys list symbol.");

	SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)expr->Sym;
	SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)expr->GetLeftChild()->Sym;

	if (this->TranslateLookupSelect(entry, keys)==false)
	{
		m_CodeGen.JumpStatement(JUMPW, jcInfo.FalseLbl);
	}

	// jump to true if coprocessor returns valid
	Node *validNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALID);

	Node *jump=m_CodeGen.BinOp(JCMPEQ, m_CodeGen.TermNode(PUSH, 1), validNode);
	
	if (opCode == IR_LKSEL)
		m_CodeGen.JCondStatement(jcInfo.TrueLbl, jcInfo.FalseLbl, jump);
	else
	{
		SymbolLabel *leftTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
		m_CodeGen.JCondStatement(leftTrue, jcInfo.FalseLbl, jump);
		m_CodeGen.LabelStatement(leftTrue);

		{
			/*
			Here the entry to update has been found:
			- if the entry has been added to be kept max time:
			get the timestamp value
			get the lifespan value
			if timestamp+lifespan > current timestamp -> delete the entry
			- if the entry has been added to be updated on hit:
			get the timestamp value
			set the timestamp to the current value
			- if the entry has been added to be replaced on hit:
			create a copy of the masked entry to update
			remove the mask and set the correct value
			remove the entry to update
			add the new entry
			- if the entry has been added to be added on hit:
			create a copy of the masked entry to update
			remove the mask and set the correct value
			add the new entry
			*/

			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, entry->Table->Id),
				entry->Table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 flagOffset = entry->Table->GetHiddenValueOffset(HIDDEN_FLAGS_INDEX);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, flagOffset),
				entry->Table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

			Node *flagNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

			StmtSwitch *swStmt = m_CodeGen.SwitchStatement(flagNode);
			swStmt->SwExit = m_CodeGen.NewLabel(LBL_LINKED);
			SymbolLabel *swExit = ManageLinkedLabel(swStmt->SwExit, "switch_exit");


			// create the keepmaxtime case
			SymbolLabel *caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			StmtCase *newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_KEEPMAXTIME), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Check if the entry should be deleted");
				uint32 offset = entry->Table->GetHiddenValueOffset(HIDDEN_TIMESTAMP_INDEX);
				SymbolTemp *timestampS = m_CodeGen.NewTemp("timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *timestampUs = m_CodeGen.NewTemp("timestamp_us", m_CompUnit.NumLocals);
				// put the timestamp on the stack
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampS, m_CodeGen.TermNode(TSTAMP_S)));
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampUs, m_CodeGen.TermNode(TSTAMP_US)));

				SymbolTemp *entryTimestampS = m_CodeGen.NewTemp("entry_timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *lifespan = m_CodeGen.NewTemp("lifespan", m_CompUnit.NumLocals);

				// get lifespan
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->GetHiddenValueOffset(HIDDEN_LIFESPAN_INDEX)),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

				Node *lifespanValueNode = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, lifespan, lifespanValueNode));

				// get entry timestamp (seconds)
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_GET_VALUE));

				Node *entryTimestampSValue = m_CodeGen.TermNode(COPIN, entry->Table->Label, LOOKUP_EX_IN_VALUE);

				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, entryTimestampS, entryTimestampSValue));


				SymbolLabel *toDelete = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *dontDelete = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				SymbolLabel *done = m_CodeGen.NewLabel(LBL_CODE, "end_if");
				Node *currentTime = m_CodeGen.TermNode(LOCLD, timestampS);
				Node *packetLimitTime = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, entryTimestampS), m_CodeGen.TermNode(LOCLD, lifespan));
				m_CodeGen.JCondStatement(JCMPGE, toDelete, dontDelete, packetLimitTime, currentTime);
				m_CodeGen.LabelStatement(toDelete);

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_DELETE));

				m_CodeGen.JumpStatement(JUMPW, done);
				m_CodeGen.LabelStatement(dontDelete);
				m_CodeGen.JumpStatement(JUMPW, done);
				m_CodeGen.LabelStatement(done);

			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the updateonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_UPDATEONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);

			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Update the timestamp of the selected entry");
				uint32 offset = entry->Table->GetHiddenValueOffset(HIDDEN_TIMESTAMP_INDEX);
				SymbolTemp *timestampS = m_CodeGen.NewTemp("timestamp_s", m_CompUnit.NumLocals);
				SymbolTemp *timestampUs = m_CodeGen.NewTemp("timestamp_ms", m_CompUnit.NumLocals);
				// put the timestamp on the stack
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampS, m_CodeGen.TermNode(TSTAMP_S)));
				m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, timestampUs, m_CodeGen.TermNode(TSTAMP_US)));

				// save new timestamp
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(LOCLD, timestampS),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_UPD_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, offset+1),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE_OFFSET));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(LOCLD, timestampUs),
					entry->Table->Label,
					LOOKUP_EX_OUT_VALUE));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_UPD_VALUE));
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the replaceonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_REPLACEONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Replace the masked entry");
				// do the actual work for this case
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// create the addonhit case
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			newCase = new StmtCase(m_CodeGen.TermNode(PUSH, LOOKUPTABLE_ENTRY_VALIDITY_ADDONHIT), caseLabel);
			CHECK_MEM_ALLOC(newCase);
			swStmt->Cases->PushBack(newCase);
			swStmt->NumCases++;

			m_CodeGen.LabelStatement(caseLabel);
			{
				m_CodeGen.CommentStatement("Add an entry related to the masked selected one");
				// do the actual work for this case
			}
			m_CodeGen.JumpStatement(JUMPW, swExit);


			// by default, go away
			caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
			StmtCase *defCase = new StmtCase(NULL, caseLabel);
			CHECK_MEM_ALLOC(defCase);
			swStmt->Default = defCase;
			m_CodeGen.LabelStatement(caseLabel);
			m_CodeGen.JumpStatement(JUMPW, swExit);


			//generate the exit label
			m_CodeGen.LabelStatement(swExit);
			//reset the switch exit label;
			swStmt->SwExit->Linked = NULL;

		}


		m_CodeGen.JumpStatement(JUMPW, jcInfo.TrueLbl);
	}
/*#else
	this->GenerateWarning(string("Lookup table conditions have not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table conditions have not yet been implemented.");
#endif*/
}

bool IRLowering::TranslateLookupSelect(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *keys)
{
//#ifdef USE_LOOKUPTABLE
	m_CodeGen.CommentStatement(string("Lookup an entry in the table ").append(entry->Table->Name));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// reset coprocessor
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_RESET));

	if (this->TranslateLookupSetValue(entry, keys, true)==false)
		return false;

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// select the entry with the specified key
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_SELECT));

/*#else
	this->GenerateWarning(string("Lookup table selection has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table selection has not yet been implemented.");
	return false;
#endif*/
	return true;
}

void IRLowering::TranslateLookupAdd(Node *node)
{
//#ifdef USE_LOOKUPTABLE
	SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)node->Sym;
	SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)node->GetLeftChild()->Sym;
	SymbolLookupTableValuesList *values = (SymbolLookupTableValuesList *)node->GetRightChild()->Sym;

	m_CodeGen.CommentStatement(string("Add entry in the table ").append(entry->Table->Name));


	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, entry->Table->Id),
		entry->Table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// reset coprocessor
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_RESET));
	
	// set keys list
	if (this->TranslateLookupSetValue(entry, keys, true)==false)
		return;

	// set values list
	if (this->TranslateLookupSetValue(entry, values, false)==false)
		return;

/*#else
	this->GenerateWarning(string("Lookup table instructions (add) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (add) has not yet been implemented.");
#endif*/
	
}

void IRLowering::TranslateLookupDelete(Node *node)
{
#ifdef USE_LOOKUPTABLE
	nbASSERT(node->Sym != NULL, "The lookup delete node should specify a valid lookup table entry symbol");
	nbASSERT(node->Sym->SymKind == SYM_RT_LOOKUP, "The lookup delete node should specify a valid lookup table entry symbol");

	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;

	m_CodeGen.CommentStatement(string("Delete selected entry in the table '").append(table->Name).append("'"));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// select the entry with the specified key
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_DELETE));

#else
	this->GenerateWarning(string("Lookup table instructions (delete) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (delete) has not yet been implemented.");
#endif
}

void IRLowering::TranslateLookupUpdate(Node *node)
{
#ifdef USE_LOOKUPTABLE
	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;
	Node *offsetNode = node->GetLeftChild();
	Node *newValueNode = node->GetRightChild();

	nbASSERT(table != NULL, "The update instruction should specify a lookup table");
	nbASSERT(newValueNode != NULL, "The update instruction should specify a valid value");
	nbASSERT(offsetNode != NULL, "The update instruction should specify a valid offset");


	m_CodeGen.CommentStatement(string("Update a value of the selected entry in the table ").append(table->Name));

	//SymbolTemp *temp;
	Node *toUpdate = TranslateTree(newValueNode);
	SymbolTemp *valueToUpdate = m_CodeGen.NewTemp("value_to_update", m_CompUnit.NumLocals);
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, toUpdate, valueToUpdate));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// set value offset
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, ((SymbolIntConst *)offsetNode->Sym)->Value),
		table->Label,
		LOOKUP_EX_OUT_VALUE_OFFSET));

	// set value
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(LOCLD, valueToUpdate),
		table->Label,
		LOOKUP_EX_OUT_VALUE));

	// update entry
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_UPD_VALUE));

#else
	this->GenerateWarning(string("Lookup table instructions (update int) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (update int) has not yet been implemented.");
#endif
}

void IRLowering::TranslateLookupInit(Node *node)
{
#ifdef USE_LOOKUPTABLE
	// set tables number
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, node->Value),
		new SymbolLabel(LBL_ID, 0, "lookup_ex"),
		LOOKUP_EX_OUT_TABLE_ID));

	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OP_INIT));

#else
	this->GenerateWarning(string("Lookup table instructions (init) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (init) has not yet been implemented.");
#endif
}

void IRLowering::TranslateLookupInitTable(Node *node)
{
#ifdef USE_LOOKUPTABLE
	SymbolLookupTable *table = (SymbolLookupTable *)node->Sym;

	m_CodeGen.CommentStatement(string("Initialize the table ").append(table->Name));

	// set table id
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->Id),
		table->Label,
		LOOKUP_EX_OUT_TABLE_ID));

	// set entries number
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->MaxExactEntriesNr+table->MaxMaskedEntriesNr),
		table->Label,
		LOOKUP_EX_OUT_ENTRIES));

	// set data size
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, table->KeysListSize),
		table->Label,
		LOOKUP_EX_OUT_KEYS_SIZE));

	// set value size
	uint32 valueSize = table->ValuesListSize;
	if (table->Validity==TABLE_VALIDITY_DYNAMIC)
	{
		// hidden values
		for (int i=0; i<HIDDEN_LAST_INDEX; i++)
			valueSize += (table->HiddenValuesList[i]->Size/sizeof(uint32));
	}
	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
		m_CodeGen.TermNode(PUSH, valueSize),
		table->Label,
		LOOKUP_EX_OUT_VALUES_SIZE));

	m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_INIT_TABLE));

#else
	this->GenerateWarning(string("Lookup table instructions (init) has not yet been implemented."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
	m_CodeGen.CommentStatement("ERROR: Lookup table instructions (init) has not yet been implemented.");
#endif

}

bool IRLowering::TranslateLookupSetValue(SymbolLookupTableEntry *entry, SymbolLookupTableValuesList *values, bool isKeysList)
{
//#ifdef USE_LOOKUPTABLE
	// load the value in the coprocessor
	Node *value=NULL;

	SymbolLookupTableItemsList_t::iterator itemIter;
	if (isKeysList)
		itemIter=entry->Table->KeysList.begin();
	else
		itemIter=entry->Table->ValuesList.begin();

	NodesList_t::iterator maskIter;
	if (isKeysList)
		maskIter=((SymbolLookupTableKeysList *)values)->Masks.begin();
	NodesList_t::iterator valueIter = values->Values.begin();

	while (valueIter != values->Values.end())
	{
		uint32 size = (*itemIter)->Size;

		if (isKeysList && (*maskIter)!=NULL)
		{
			// if you have a not null mask, add a dummy value (0)
			uint32 refLoaded = 0;
			Node *pieceOffset = m_CodeGen.TermNode(PUSH, (uint32)(0));
			uint32 bytes2Load = 0;
			// put value in steps of max 4 bytes
			while (refLoaded<size)
			{
				bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));

				// set table id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

					//add value
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					GenMemLoad(pieceOffset, bytes2Load),
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));
							
				if (isKeysList)
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
				else
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

				refLoaded += bytes2Load;
			}
		}
		else
		{
			// in this case you have a value or a key without mask to set
			switch ((*valueIter)->Op)
			{
			case IR_SCONST:
				this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
				m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables.");
				return false;
				break;
			case IR_FIELD:
				{
					nbASSERT((*valueIter)->Sym != NULL, "Field IR should contain a valid symbol.");
					//SymbolFieldFixed *field = (SymbolFieldFixed *)(*valueIter)->Sym;

					this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
					m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables.");
					return false;
				}break;
			case IR_SVAR:
				{
					nbASSERT((*valueIter)->Sym != NULL, "node symbol cannot be NULL");
					Node *fieldOffset = (*valueIter)->GetLeftChild();

					if (fieldOffset != NULL)
					{
						fieldOffset = TranslateTree(fieldOffset);
						Node *lenNode = (*valueIter)->GetRightChild();
						nbASSERT(lenNode->Op == IR_ICONST, "lenNode should be an IR_ICONST");
						size = ((SymbolIntConst*)lenNode->Sym)->Value;
					}

					SymbolVarBufRef *ref = (SymbolVarBufRef*)(*valueIter)->Sym;

					if (ref->RefType == REF_IS_PACKET_VAR && ref->Name.compare("$packet")==0)
					{
#if 0
						this->GenerateWarning(string("Conditions with lookup table instructions are implemented only with string variables (references to fields)."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
						m_CodeGen.CommentStatement("ERROR: Conditions with lookup table instructions are implemented only with string variables (references to fields).");
						return false;
#else
						uint32 refLoaded = 0;
						Node *pieceOffset = NULL;
						uint32 bytes2Load = 0;
						// SymbolTemp *indexTemp = ref->IndexTemp;
						// put value in steps of max 4 bytes
						while (refLoaded<size)
						{
							if (size>refLoaded)
							{
								//pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
								//if (fieldOffset!=0)
								//	pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, fieldOffset);
								//if (refLoaded>0)
								//	pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								if (refLoaded>0)
									pieceOffset = m_CodeGen.BinOp(ADD, fieldOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								else
									pieceOffset = fieldOffset;

								bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));
							}

							// set table id
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
								m_CodeGen.TermNode(PUSH, entry->Table->Id),
								entry->Table->Label,
								LOOKUP_EX_OUT_TABLE_ID));

							if (size>refLoaded)
							{
								// add value
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
								GenMemLoad(pieceOffset, bytes2Load),
								entry->Table->Label,
								LOOKUP_EX_OUT_GENERIC));
							
							}
							else
							{
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (uint32)0),
									entry->Table->Label,
									LOOKUP_EX_OUT_GENERIC));
							}

							if (isKeysList)
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
							else
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

							refLoaded += bytes2Load;
						}
#endif
					} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
						nbASSERT(ref->GetFixedMaxSize() <= size, "The reference variable should define a valid max size.");

						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						uint32 refLoaded = 0;
						Node *pieceOffset = NULL;
						uint32 bytes2Load = 0;
						// put value in steps of max 4 bytes
						while (refLoaded<size)
						{
							if (ref->GetFixedSize()==0)
							{
								// check if ( refLoaded < ref_len)
								SymbolLabel *jumpTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
								SymbolLabel *jumpFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
								SymbolLabel *jumpExit = m_CodeGen.NewLabel(LBL_CODE, "end_if");
								m_CodeGen.JCondStatement(JCMPL, jumpTrue, jumpFalse, m_CodeGen.TermNode(PUSH, refLoaded), m_CodeGen.TermNode(LOCLD, ref->LenTemp));

								m_CodeGen.LabelStatement(jumpTrue);
								//
								// if true, load the current step
								//
								pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
								if (fieldOffset!=0)
									pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, fieldOffset);
								if (refLoaded>0)
									pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));
								bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));

								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));

								if (ref->GetFixedMaxSize()>refLoaded)
								{
									// add value
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										GenMemLoad(pieceOffset, bytes2Load),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));
									
								}
								else
								{
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										m_CodeGen.TermNode(PUSH, (uint32)0),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));
								}

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

								m_CodeGen.JumpStatement(JUMPW, jumpExit);


								m_CodeGen.LabelStatement(jumpFalse);
								//
								// if false, load 0
								//

								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));
						
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (uint32)0),
									entry->Table->Label,
									LOOKUP_EX_OUT_GENERIC));

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else

									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
								m_CodeGen.JumpStatement(JUMPW, jumpExit);

								m_CodeGen.LabelStatement(jumpExit);
							}
							else
							{
								if (ref->GetFixedMaxSize()>refLoaded)
								{
									pieceOffset = m_CodeGen.TermNode(LOCLD, indexTemp);
									if (fieldOffset!=0)
										pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, fieldOffset);
									if (refLoaded>0)
										pieceOffset = m_CodeGen.BinOp(ADD, pieceOffset, m_CodeGen.TermNode(PUSH, refLoaded));

									bytes2Load = (size-refLoaded)>=sizeof(uint32) ? sizeof(uint32) : ((size-refLoaded)%sizeof(uint32));
								}

								// set table id
								m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, entry->Table->Id),
									entry->Table->Label,
									LOOKUP_EX_OUT_TABLE_ID));

								if (ref->GetFixedMaxSize()>refLoaded)
								{
									// add value
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										GenMemLoad(pieceOffset, bytes2Load),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));

									// Field_size
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
									m_CodeGen.TermNode(PUSH, (*itemIter)->Size),
									entry->Table->Label,
									LOOKUP_EX_OUT_FIELD_SIZE));
									
								}
								else
								{
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
										m_CodeGen.TermNode(PUSH, (uint32)0),
										entry->Table->Label,
										LOOKUP_EX_OUT_GENERIC));
								}

								if (isKeysList)
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_KEY));
								else
									m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
							}
							refLoaded += bytes2Load;
						}
					}
				}break;
			default:
				// set table id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, entry->Table->Id),
					entry->Table->Label,
					LOOKUP_EX_OUT_TABLE_ID));

				// add value
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					TranslateTree(*valueIter),
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));

				break;
			}
		}

		itemIter++;
		valueIter++;
		if (isKeysList)
			maskIter++;
	}

	if (isKeysList==false)
	{
		// add hidden values
		for (int valueIter=0; valueIter<HIDDEN_LAST_INDEX; valueIter++)
		{
			// set table id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, entry->Table->Id),
				entry->Table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			// [ds] note that this loop has to be used only with the timestamp (2 int, not initialized)
			//      in other cases, you should change this implementation
			for (uint32 size=0; size < (entry->Table->HiddenValuesList[valueIter]->Size/sizeof(uint32)); size++)
			{
				if (entry->HiddenValues[valueIter]!=NULL)
					value = TranslateTree(entry->HiddenValues[valueIter]);
				else
					value = m_CodeGen.TermNode(PUSH, (uint32)-1);

				//add value
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					value,
					entry->Table->Label,
					LOOKUP_EX_OUT_GENERIC));

				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, entry->Table->Label, LOOKUP_EX_OP_ADD_VALUE));
			}
		}
	}

	return true;
/*#else
	return false;
#endif*/
}

void IRLowering::TranslateCase(StmtSwitch *newSwitchSt, StmtCase *caseSt, SymbolLabel *swExit, bool IsDefault)
{
	//if the code for the current case is empty we drop it
	//! \todo manage the number of cases!
	if (caseSt->Code->Code->Empty())
		return;

	SymbolLabel *caseLabel = m_CodeGen.NewLabel(LBL_CODE, "case");
	if (IsDefault)
	{
		StmtCase *defCase = new StmtCase(NULL, caseLabel);
		CHECK_MEM_ALLOC(defCase);
		newSwitchSt->Default = defCase;
	}
	else
	{
		StmtCase *newCase = new StmtCase(TranslateTree(caseSt->Forest), caseLabel);
		CHECK_MEM_ALLOC(newCase);
		newSwitchSt->Cases->PushBack(newCase);

		// [ds] no. of cases will be computed by actually adding case statements
		newSwitchSt->NumCases++;
	}
	m_CodeGen.LabelStatement(caseLabel);
	LowerHIRCode(caseSt->Code->Code);
	m_CodeGen.JumpStatement(JUMPW, swExit);
}


void IRLowering::TranslateCases(StmtSwitch *newSwitchSt, CodeList *cases, SymbolLabel *swExit)
{
	StmtBase *caseSt = cases->Front();
	while(caseSt)
	{
		nbASSERT(caseSt->Kind == STMT_CASE, "statement must be a STMT_CASE");
		TranslateCase(newSwitchSt, (StmtCase*)caseSt, swExit, false);
		caseSt = caseSt->Next;
	}
}

void IRLowering::TranslateSwitch(StmtSwitch *stmt)
{
	StmtSwitch *swStmt = m_CodeGen.SwitchStatement(TranslateTree(stmt->Forest));
	// [ds] no. of cases will be computed by actually adding case statements
	//swStmt->NumCases = stmt->NumCases;
	//if the exit label was not already created we must create it
	SymbolLabel *swExit = ManageLinkedLabel(stmt->SwExit, "switch_exit");

	TranslateCases(swStmt, stmt->Cases, swExit);
	if (stmt->Default != NULL)
	{
		TranslateCase(swStmt, stmt->Default, swExit, true);
	}
	else if (stmt->ForceDefault)
	{
		StmtCase *defCase = new StmtCase(NULL, m_FilterFalse);
		CHECK_MEM_ALLOC(defCase);
		swStmt->Default = defCase;
	}
	else
	{
		StmtCase *defCase = new StmtCase(NULL, swExit);
		CHECK_MEM_ALLOC(defCase);
		swStmt->Default = defCase;
	}

	//generate the exit label
	m_CodeGen.LabelStatement(swExit);
	//reset the switch exit label;
	stmt->SwExit->Linked = NULL;
}


void IRLowering::TranslateSwitch2(StmtSwitch *stmt)
{
	SymbolTemp *tmp = m_CodeGen.NewTemp("", m_CompUnit.NumLocals);
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, TranslateTree(stmt->Forest), tmp));

	//if the exit label was not already created we must create it
	SymbolLabel *swExit = ManageLinkedLabel(stmt->SwExit, "switch_exit");

	StmtCase *caseSt = (StmtCase*)stmt->Cases->Front();
	while(caseSt)
	{
		nbASSERT(caseSt->Kind == STMT_CASE, "statement must be a STMT_CASE");


		if (caseSt->Code->Code->Empty())
			return;
		SymbolLabel *nextCase = m_CodeGen.NewLabel(LBL_CODE, "next");
		SymbolLabel *code = m_CodeGen.NewLabel(LBL_CODE, "code");
		m_CodeGen.JCondStatement(nextCase, code, m_CodeGen.BinOp(JCMPNEQ, m_CodeGen.TermNode(LOCLD, tmp), TranslateTree(caseSt->Forest)));
		m_CodeGen.LabelStatement(code);
		LowerHIRCode(caseSt->Code->Code);
		m_CodeGen.JumpStatement(JUMPW, swExit);
		m_CodeGen.LabelStatement(nextCase);
		caseSt = (StmtCase*)caseSt->Next;
	}
	if (stmt->Default != NULL)
	{
		LowerHIRCode(stmt->Default->Code->Code);
	}
	else if (stmt->ForceDefault)
	{
		m_CodeGen.JumpStatement(JUMPW, m_FilterFalse);
	}
	else
	{
		m_CodeGen.JumpStatement(JUMPW, swExit);
	}

	//generate the exit label
	m_CodeGen.LabelStatement(swExit);
	//reset the switch exit label;
	stmt->SwExit->Linked = NULL;
}


void IRLowering::EnterLoop(SymbolLabel *start, SymbolLabel *exit)
{
	LoopInfo loopInfo(start, exit);
	m_LoopStack.push_back(loopInfo);
}

void IRLowering::ExitLoop(void)
{
	nbASSERT(!m_LoopStack.empty(), "The loop stack is empty");
	m_LoopStack.pop_back();
}

void IRLowering::TranslateIf(StmtIf *stmt)
{
	/*
	If both true and false branches are present, it generates:

	cmp (!expr) goto if_N_false
	if_N_true:
	;code for the true branch
	goto if_N_end
	if_N_false:
	;code for the false branch
	if_N_end:

	...

	otherwise it generates:

	cmp (!expr) goto if_N_false
	if_N_true:
	;code for the true branch
	if_N_false:
	...

	*/

	Node *expr = stmt->Forest;
	ReverseCondition(&expr);

	SymbolLabel *trueLabel = m_CodeGen.NewLabel(LBL_CODE, "if_true");
	SymbolLabel *falseLabel = m_CodeGen.NewLabel(LBL_CODE, "if_false");
	SymbolLabel *exitLabel = m_CodeGen.NewLabel(LBL_CODE, "end_if");

	JCondInfo jcInfo(falseLabel, trueLabel);

	TranslateBoolExpr(expr, jcInfo);

	//if_N_true:
	m_CodeGen.LabelStatement(trueLabel);

	if (stmt->TrueBlock->Code->Empty()==false)
	{
		LowerHIRCode(stmt->TrueBlock->Code/*, "true branch"*/);
		//goto if_N_end
		m_CodeGen.JumpStatement(JUMPW, exitLabel);

		//if_N_false:
		m_CodeGen.LabelStatement(falseLabel);
		if(stmt->FalseBlock->Code->Empty()==false)
		{
			//code for the false branch
			LowerHIRCode(stmt->FalseBlock->Code/*, "false branch"*/);
		}
		//goto if_N_end
		m_CodeGen.JumpStatement(JUMPW, exitLabel);
	}
	else
	{
		m_CodeGen.JumpStatement(JUMPW, exitLabel);
	}

	//if_N_end:
	m_CodeGen.LabelStatement(exitLabel);
}


void IRLowering::GenTempForVar(Node *node)
{
	nbASSERT(node != NULL, "node cannot be NULL");
	nbASSERT(node->Sym != NULL, "node symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind == SYM_RT_VAR, "node symbol must be a SYM_RT_VAR");

	SymbolVariable *varSym = (SymbolVariable*)node->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "node symbol must be an integer variable");

	SymbolVarInt *intVar = (SymbolVarInt*)varSym;
	SymbolTemp *temp = m_CodeGen.NewTemp(varSym->Name, m_CompUnit.NumLocals);
	intVar->Temp = temp;
}

void IRLowering::TranslateLoop(StmtLoop *stmt)
{
	/* generates the following code:

	$index = $initVal
	goto test
	start:

	loop body

	<inc_statement>
	test:
	if (condition) goto start
	exit:
	...
	*/

	GenTempForVar(stmt->Forest);
	Node *index = TranslateTree(stmt->Forest);
	Node *initVal = TranslateTree(stmt->InitVal);
	//index = initval
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, initVal, index->Sym));
	SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
	EnterLoop(start, exit);
	//jump test
	m_CodeGen.JumpStatement(JUMPW, test);
	//start:
	m_CodeGen.LabelStatement(start);
	//loop body
	LowerHIRCode(stmt->Code->Code/*, "loop body"*/);
	//test:
	m_CodeGen.LabelStatement(test);
	//<inc_statement>
	TranslateStatement(stmt->IncStmt);
	//if (condition) goto start else goto exit
	JCondInfo jcInfo(start, exit);
	TranslateBoolExpr(stmt->TermCond, jcInfo);
	//exit:
	m_CodeGen.LabelStatement(exit);
	ExitLoop();
}

void IRLowering::TranslateWhileDo(StmtWhile *stmt)
{
	/* generates the following code:

	goto test
	start:

	;loop body

	test:
	if (condition) goto start else goto exit
	exit:
	...
	*/

#ifdef OPTIMIZE_SIZED_LOOPS
//#if 0
	bool skip=true;
	for (StmtList_t::iterator i=m_Protocol->SizedLoopToPreserve.begin(); i!=m_Protocol->SizedLoopToPreserve.end(); i++)
	{
		if ((*i)==stmt)
			skip=false;
	}

	Node *sentinel = stmt->Forest->GetRightChild();

	if (skip == false || sentinel == NULL)
	{
#endif

		SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
		SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
		SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
		EnterLoop(start, exit);
		//jump test
		m_CodeGen.JumpStatement(JUMPW, test);
		//start:
		m_CodeGen.LabelStatement(start);
		//loop body
		LowerHIRCode(stmt->Code->Code, "loop body");
		//test:
		m_CodeGen.LabelStatement(test);
		//if (condition) goto start else goto exit
		JCondInfo jcInfo(start, exit);
		TranslateBoolExpr(stmt->Forest, jcInfo);
		//exit:
		m_CodeGen.LabelStatement(exit);
		ExitLoop();

#ifdef OPTIMIZE_SIZED_LOOPS
//#if 0
	}
	else
	{
		SymbolVarInt *sentinelVar = (SymbolVarInt *)sentinel->Sym;
		SymbolTemp *currOffsTemp = m_CodeGen.CurrOffsTemp();

		m_CodeGen.TermNode(LOCLD, currOffsTemp);
		// Node *offset = m_CodeGen.TermNode(LOCLD, currOffsTemp);
		Node *newOffset = m_CodeGen.TermNode(LOCLD, sentinelVar->Temp);

		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, newOffset, currOffsTemp));

		//SymbolLabel *test = m_CodeGen.NewLabel(LBL_CODE);
		//SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
		//SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
		//EnterLoop(start, exit);
		////jump test
		//m_CodeGen.JumpStatement(JUMPW, test);
		////start:
		//m_CodeGen.LabelStatement(start);
		////loop body
		//LowerHIRCode(&stmt->Code.Code, "loop body");
		////test:
		//m_CodeGen.LabelStatement(test);
		////if (condition) goto start else goto exit
		//JCondInfo jcInfo(start, exit);
		//TranslateBoolExpr(stmt->Forest, jcInfo);
		////exit:
		//m_CodeGen.LabelStatement(exit);
		//ExitLoop();
	}
#endif
}

void IRLowering::TranslateDoWhile(StmtWhile *stmt)
{
	/* generates the following code:

	start:

	;loop body

	if (condition) goto start else goto exit
	exit:
	...
	*/
	SymbolLabel *start = m_CodeGen.NewLabel(LBL_CODE);
	SymbolLabel *exit = m_CodeGen.NewLabel(LBL_CODE);
	EnterLoop(start, exit);
	//start:
	m_CodeGen.LabelStatement(start);
	//loop body
	LowerHIRCode(stmt->Code->Code, "loop body");
	//if (condition) goto start else goto exit
	JCondInfo jcInfo(start, exit);
	TranslateBoolExpr(stmt->Forest, jcInfo);
	//exit:
	m_CodeGen.LabelStatement(exit);
	ExitLoop();
}

void IRLowering::TranslateBreak(StmtCtrl *stmt)
{
	nbASSERT(!m_LoopStack.empty(), "empty loop stack");
	SymbolLabel *exit = m_LoopStack.back().LoopExit;
	m_CodeGen.JumpStatement(JUMPW, exit);
}

void IRLowering::TranslateContinue(StmtCtrl *stmt)
{
	nbASSERT(!m_LoopStack.empty(), "empty loop stack");
	SymbolLabel *start = m_LoopStack.back().LoopStart;
	m_CodeGen.JumpStatement(JUMPW, start);
}

void IRLowering::TranslateFieldDef(Node *node)
{
	nbASSERT(node->Op == IR_DEFFLD, "node must be an IR_DEFFLD");
	Node *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Op == IR_FIELD, "left child must be an IR_FIELD");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_FIELD, "left child symbol must be a SYM_FIELD");
	SymbolField *fieldSym = (SymbolField*)leftChild->Sym;
	SymbolTemp *currOffsTemp = m_CodeGen.CurrOffsTemp();
	SymbolVarInt *protoOffsVar = m_GlobalSymbols.GetProtoOffsVar(m_Protocol);
	nbASSERT(currOffsTemp != NULL, "currOffsTemp cannot be NULL");
	nbASSERT(protoOffsVar != NULL, "m_Protocol->ProtoOffsVar cannot be NULL");
	nbASSERT(protoOffsVar->Temp != NULL, "m_Protocol->ProtoOffsVar->Temp cannot be NULL");

	SymbolTemp *protoOffsTemp = protoOffsVar->Temp;

	// [ds] we will use the symbol that defines different version of the same field
	SymbolField *sym=this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, fieldSym);

	bool defined=false;
	bool usedAsInt=false;
	bool usedAsArray=false;
	bool usedAsString=false;
	bool intCompatible=true;

	if (sym->IntCompatible==false)
		intCompatible=false;
	if (sym->UsedAsInt)
		usedAsInt=true;
	if (sym->UsedAsString)
		usedAsString=true;
	if (sym->UsedAsArray)
		usedAsArray=true;

	Node *offset=NULL;

	// offset = $currentoffset
	offset=m_CodeGen.TermNode(LOCLD, currOffsTemp);

	if ((sym->FieldType==PDL_FIELD_FIXED || sym->FieldType==PDL_FIELD_VARLEN || sym->FieldType==PDL_FIELD_TOKEND|| sym->FieldType==PDL_FIELD_TOKWRAP
				|| sym->FieldType==PDL_FIELD_LINE || sym->FieldType==PDL_FIELD_PATTERN || sym->FieldType==PDL_FIELD_EATALL ) && sym->DependsOn!=NULL)
	{
		// shift the offset according to previous defined fields in this container
		SymbolFieldContainer *container=sym->DependsOn;
		switch (sym->FieldType)
		{
		case PDL_FIELD_FIXED:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldFixed *)sym->DependsOn)->IndexTemp);
			break;
		case PDL_FIELD_VARLEN:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldVarLen *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_TOKEND:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokEnd *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_TOKWRAP:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldTokWrap *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_LINE:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldLine *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_PATTERN:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldPattern *)sym->DependsOn)->IndexTemp);
			break;

		case PDL_FIELD_EATALL:
			offset = m_CodeGen.TermNode(LOCLD, ((SymbolFieldEatAll *)sym->DependsOn)->IndexTemp);
			break;

		default:
			break;

		}

		uint32 iOffset=0;
		FieldsList_t::iterator innerField =  container->InnerFields.begin();
		while (innerField != container->InnerFields.end() && *innerField!=sym)
		{
			// offset = offset + innerField.size
			switch ((*innerField)->FieldType)
			{
			case PDL_FIELD_FIXED:
				iOffset += ((SymbolFieldFixed *)*innerField)->Size;
				break;
			case PDL_FIELD_VARLEN:
				{
					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldVarLen *)sym)->LenTemp = lenTemp;
					Node *lenExpr = TranslateTree( ((SymbolFieldVarLen *)sym)->LenExpr );
					Node *len = m_CodeGen.UnOp(LOCST, lenExpr, lenTemp);
					offset = m_CodeGen.BinOp(ADD, offset, len);
				}
				break;
			case PDL_FIELD_BITFIELD:
				break;
			case PDL_FIELD_PADDING:
				break;
			case PDL_FIELD_TOKEND:
				{
#ifdef USE_REGEX
					Node *len = m_CodeGen.TermNode(PUSH, (uint32) 0);

					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldTokEnd *)sym)->LenTemp = lenTemp;


					if(((SymbolFieldTokEnd *)sym)->EndTok!=NULL)
					{

	                    SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokEnd *)sym)->EndTok));

						SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,offset), lenTemp);
					}
					else if(((SymbolFieldTokEnd *)sym)->EndRegEx!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokEnd *)sym)->EndRegEx));

						SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,offset), lenTemp);
					}
					else
						break;

					if(((SymbolFieldTokEnd *)sym)->EndOff!=NULL)
					{
						Node *offExpr = TranslateTree( ((SymbolFieldTokEnd *)sym)->EndOff );
						len = m_CodeGen.UnOp(LOCST, offExpr, lenTemp);
					}

					offset=m_CodeGen.BinOp(ADD,offset,len);

					if(((SymbolFieldTokEnd *)sym)->EndDiscard!=NULL)
					{
						SymbolTemp *discTemp = m_CodeGen.NewTemp(sym->Name + string("_disc"), m_CompUnit.NumLocals);
						((SymbolFieldTokEnd *)sym)->DiscTemp = discTemp;
						Node *discExpr = TranslateTree( ((SymbolFieldTokEnd *)sym)->EndDiscard );
						Node *disc = m_CodeGen.UnOp(LOCST, discExpr, discTemp);
						offset=m_CodeGen.BinOp(ADD,offset,disc);
					}

#else
	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
	break;
#endif
				}
				break;

			case PDL_FIELD_TOKWRAP:
				{
#ifdef USE_REGEX
					Node *begin=offset;
					Node *len = m_CodeGen.TermNode(PUSH, (uint32) 0);
					Node *new_begin = m_CodeGen.TermNode(PUSH, (uint32) 0);
					Node *offsetNode=offset;
					Node *lenRegEx;


					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldTokWrap *)sym)->LenTemp = lenTemp;


					if(((SymbolFieldTokWrap *)sym)->BeginTok!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->BeginTok));

						SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32) 0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;

						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
							//if the field doesn't start from $current_offset, the field doesn't exist so don't update the offset
							SymbolLabel *labelBeginTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
							SymbolLabel *labelBeginFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
							m_CodeGen.JCondStatement(labelBeginTrue, labelBeginFalse, m_CodeGen.BinOp(JCMPEQ, offset,m_CodeGen.BinOp(SUB,offsetNode,lenRegEx)));
							m_CodeGen.LabelStatement(labelBeginFalse);
								break;
							m_CodeGen.LabelStatement(labelBeginTrue);
								begin=m_CodeGen.BinOp(SUB,offsetNode, lenRegEx);

					}
					else if(((SymbolFieldTokWrap *)sym)->BeginRegEx!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->BeginRegEx));

						SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32) 0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
							begin=m_CodeGen.BinOp(SUB,offsetNode, lenRegEx);
					}
					else
						break;

					if(((SymbolFieldTokWrap *)sym)->EndTok!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->EndTok));
						//Search the endtoken from the first byte after th end of begintoken
						SymbolRegEx *regExp = new SymbolRegEx(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH,(uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,begin), lenTemp);
					}
					else if(((SymbolFieldTokWrap *)sym)->EndRegEx!=NULL)
					{
						SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldTokWrap *)sym)->EndRegEx));

						SymbolRegEx *regExp = new SymbolRegEx(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
						m_GlobalSymbols.StoreRegExEntry(regExp);

						//set id
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.TermNode(PUSH, regExp->Id),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_PATTERN_ID));

						// set buffer offset
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						TranslateTree(regExp->Offset),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_OFFSET));

						// calculate and set buffer length
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
						m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
						new SymbolLabel(LBL_ID, 0, string("regexp")),
						REGEX_OUT_BUF_LENGTH));


						// find the pattern
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

						// check the result
						Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

						Node *leftExpr = validNode;
						Node *rightExpr = m_CodeGen.TermNode(PUSH,(uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

						// if there aren't any matches the field doesn't exist, don't update the offset
						m_CodeGen.LabelStatement(labelFalse);
							break;
						m_CodeGen.LabelStatement(labelTrue);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
							offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
							len=m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(SUB,offsetNode,begin), lenTemp);
					}
					else
						break;

					if(((SymbolFieldTokWrap *)sym)->BeginOff!=NULL)
					{
					new_begin = TranslateTree( ((SymbolFieldTokWrap *)sym)->BeginOff );
					begin=m_CodeGen.BinOp(ADD,offset,new_begin);
					}

					if(((SymbolFieldTokWrap *)sym)->EndOff!=NULL)
					{
						Node *offExpr = TranslateTree( ((SymbolFieldTokWrap *)sym)->EndOff );
						len = m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB,m_CodeGen.BinOp(ADD,offExpr,offset),begin), lenTemp);

					}

					offset=m_CodeGen.BinOp(ADD,begin,len);

					if(((SymbolFieldTokWrap *)sym)->EndDiscard!=NULL)
					{
						SymbolTemp *discTemp = m_CodeGen.NewTemp(sym->Name + string("_disc"), m_CompUnit.NumLocals);
						((SymbolFieldTokWrap *)sym)->DiscTemp = discTemp;
						Node *discExpr = TranslateTree( ((SymbolFieldTokWrap *)sym)->EndDiscard );
						Node *disc = m_CodeGen.UnOp(LOCST, discExpr, discTemp);
						offset=m_CodeGen.BinOp(ADD,offset,disc);
					}
#else
	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
	break;
#endif
				}
				break;


			case PDL_FIELD_LINE:
				{
//#ifdef USE_REGEX
					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldLine *)sym)->LenTemp = lenTemp;


                    SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldLine *)sym)->EndTok));

					SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
					m_GlobalSymbols.StoreRegExEntry(regExp);

					//set id
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, regExp->Id),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_PATTERN_ID));

					// set buffer offset
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					TranslateTree(regExp->Offset),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_BUF_OFFSET));

					// calculate and set buffer length
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_BUF_LENGTH));


					// find the pattern
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

					// check the result
					Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

					Node *leftExpr = validNode;
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));

					// if there aren't any matches the field doesn't exist, don't update the offset
					m_CodeGen.LabelStatement(labelFalse);
						break;

					m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					Node *lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
					offset=m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCST,lenRegEx, lenTemp),offset);


//#else
//	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
//	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
//	break;
//#endif

				}
				break;

				case PDL_FIELD_PATTERN:
				{
//#ifdef USE_REGEX
					SymbolTemp *lenTemp = m_CodeGen.NewTemp(sym->Name + string("_len"), m_CompUnit.NumLocals);
					((SymbolFieldPattern *)sym)->LenTemp = lenTemp;


                    SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)((SymbolFieldPattern *)sym)->Pattern));

					SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
					m_GlobalSymbols.StoreRegExEntry(regExp);

					//set id
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.TermNode(PUSH, regExp->Id),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_PATTERN_ID));

					// set buffer offset
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					TranslateTree(regExp->Offset),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_BUF_OFFSET));

					// calculate and set buffer length
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
					m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
					new SymbolLabel(LBL_ID, 0, string("regexp")),
					REGEX_OUT_BUF_LENGTH));


					// find the pattern
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

					// check the result
					Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

					Node *leftExpr = validNode;
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));

					// if there aren't any matches the field doesn't exist, don't update the offset
					m_CodeGen.LabelStatement(labelFalse);
						break;

					m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
						//Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
						Node *lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
							offset=m_CodeGen.BinOp(ADD,m_CodeGen.UnOp(LOCST,lenRegEx, lenTemp),offset);

//#else
//	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
//	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
//	break;
//#endif

				}
				break;
			//the eatall field must be the last innerfield
			case PDL_FIELD_EATALL:
				nbASSERT(false, "CANNOT BE HERE");
				break;
			default:
				break;
			}

			innerField++;
		}

		if (iOffset>0)
			offset = m_CodeGen.BinOp(ADD, offset, m_CodeGen.TermNode(PUSH, iOffset));
	}

	switch(fieldSym->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixedFieldSym = (SymbolFieldFixed*)sym;
			
			if (usedAsInt)
			{
				if (intCompatible==false)
				{
					this->GenerateWarning(string("Field ")+fieldSym->Name+(" is used as integer, but is not compatible to integer type; definition is aborted"), __FILE__, __FUNCTION__, __LINE__, 1, 4);
					m_CodeGen.CommentStatement(string("ERROR: Field ")+fieldSym->Name+(" is used as integer, but is not compatible to integer type; definition is aborted"));

					if (sym->DependsOn==NULL)
					{
						//$add = $currentoffset + size
						Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Size));
						//$currentoffset = $add
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
					}
					return;
				}

				if (fixedFieldSym->ValueTemp==NULL)
				{
					SymbolTemp *valueTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_value"), m_CompUnit.NumLocals);
					fixedFieldSym->ValueTemp = valueTemp;
				}

				//$fld_value = load($currentoffset)
				Node *field_ind=offset;
				Node *field_value=this->GenMemLoad(field_ind, fixedFieldSym->Size);
				if (fixedFieldSym->Size==3)
				{
					// the field size is 3 bytes, you should align the value to 4 bytes (xx.xx.xx.00 -> 00.xx.xx.xx)
					field_value=m_CodeGen.BinOp(SHR, field_value, m_CodeGen.TermNode(PUSH, 8));
				}
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, field_value, fixedFieldSym->ValueTemp));
				
				if (fixedFieldSym->IndexTemp==NULL)
				{
					SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
					fixedFieldSym->IndexTemp = indexTemp;
				}

				//$fld_index = $currentoffset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, currOffsTemp), fixedFieldSym->IndexTemp));
				defined=true;
			}

			if (usedAsString || usedAsArray || fixedFieldSym->InnerFields.size())
			{
				if (fixedFieldSym->IndexTemp==NULL)
				{
					SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
					fixedFieldSym->IndexTemp = indexTemp;
				}
				
				//$fld_index = $currentoffset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, currOffsTemp), fixedFieldSym->IndexTemp));
				defined=true;
			}

			if (defined==false)
			{
				if (fixedFieldSym->IndexTemp==NULL)
				{
					SymbolTemp *indexTemp = m_CodeGen.NewTemp(fixedFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
					fixedFieldSym->IndexTemp = indexTemp;
				}

				//$fld_index = $currentoffset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, fixedFieldSym->IndexTemp));
			}

			if (fixedFieldSym->DependsOn==NULL)
			{
				//$add = $currentoffset + size
				Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Size));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if (fixedFieldSym->ToExtract)
			{
				if(fixedFieldSym->MultiProto)
				{
					m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per header ") + fixedFieldSym->Protocol->Name);
					Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, fixedFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, fixedFieldSym->Position), m_CodeGen.TermNode(PUSH, 4)));
					SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH, fixedFieldSym->Size), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));

					m_CodeGen.CommentStatement(string("Incremento HeaderCounter per header ") + fixedFieldSym->Name);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, 1)), fixedFieldSym->HeaderCount));
					FieldsList_t::iterator f = fixedFieldSym->MultiFields.begin();
					for (; f != fixedFieldSym->MultiFields.end(); f++)
					{	//store the number of fields extracted
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, (*f)->Position)));
					}
				}
					
				else if(fixedFieldSym->MultiField)
				{
					m_CodeGen.CommentStatement(string("MultiField - test estrazione"));
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, fixedFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per MultiField"));
					Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, fixedFieldSym->Position), m_CodeGen.TermNode(PUSH, 4)));
					SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, fixedFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH, fixedFieldSym->Size), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
					m_CodeGen.CommentStatement(string("Incremento variabile FieldCount per MultiField"));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), fixedFieldSym->FieldCount));

					m_CodeGen.LabelStatement(labelFalse);
				}

				else if (fixedFieldSym->HeaderIndex > 0)
				{
					m_CodeGen.CommentStatement(string("Test HeaderIndexing"));
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, fixedFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)fixedFieldSym->HeaderIndex - 1);//sottraggo 1 perch� l'indice Extract parte da 0
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.CommentStatement(string("HeaderIndex - header ") + fixedFieldSym->Protocol->Name + string(" da estrarre"));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,fixedFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Position)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->Size), m_CodeGen.TermNode(PUSH, (fixedFieldSym->Position)+2)));

					m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.CommentStatement(string("HeaderIndex - header ") + fixedFieldSym->Protocol->Name + string(" da ignorare"));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->ExtractG), m_CodeGen.TermNode(PUSH, 1)), fixedFieldSym->ExtractG));
				}	

				else
				{
					m_CodeGen.CommentStatement(string("Test FirstExtract per il protocollo ") + fixedFieldSym->Protocol->Name);
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, fixedFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)1);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.CommentStatement(string("Extract - estrarre prima occorrenza header ") + fixedFieldSym->Protocol->Name);
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,fixedFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, fixedFieldSym->Position)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->Size), m_CodeGen.TermNode(PUSH, (fixedFieldSym->Position)+2)));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(PUSH,(uint32)1), fixedFieldSym->ExtractG)); //Extract = 1

					m_CodeGen.LabelStatement(labelTrue);
				}
			}

			if(fixedFieldSym->Protocol->ExAllfields)
			{  
				  //if the field hasn,t valid PDLFieldInfo is a bit_union field and it contains bitfields
				if(fixedFieldSym->PDLFieldInfo!=NULL)
				{  
					Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->ID), positionId));
					Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,fixedFieldSym->IndexTemp), positionOff));
					Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,fixedFieldSym->Size), positionLen));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),fixedFieldSym->Protocol->position));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),fixedFieldSym->Protocol->NExFields));
				  }
			}

		}break;

	case PDL_FIELD_VARLEN:
		{
			SymbolFieldVarLen *varlenFieldSym = (SymbolFieldVarLen*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(varlenFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			varlenFieldSym->IndexTemp = indexTemp;
			if (varlenFieldSym->LenTemp==NULL)
			{
				SymbolTemp *lenTemp = m_CodeGen.NewTemp(varlenFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				varlenFieldSym->LenTemp = lenTemp;
			}

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
			Node *lenExpr = TranslateTree(varlenFieldSym->LenExpr);
			//$fld_len = lenExpr
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, lenExpr, varlenFieldSym->LenTemp));

			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, varlenFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(varlenFieldSym->ToExtract)
			{
				if(varlenFieldSym->MultiProto)
				{
					m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per header ") + varlenFieldSym->Protocol->Name);
					Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, varlenFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, varlenFieldSym->Position), m_CodeGen.TermNode(PUSH, 4)));
					SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));

					m_CodeGen.CommentStatement(string("Incremento HeaderCounter per header ") + varlenFieldSym->Name);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, 1)), varlenFieldSym->HeaderCount));
					FieldsList_t::iterator f = varlenFieldSym->MultiFields.begin();
					for (; f != varlenFieldSym->MultiFields.end(); f++)
					{	//store the number of fields extracted
						m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->HeaderCount), m_CodeGen.TermNode(PUSH, (*f)->Position)));
					}
				}

				else if(varlenFieldSym->MultiField)
				{
					m_CodeGen.CommentStatement(string("MultiField - test estrazione"));
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, varlenFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per MultiField"));
					Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
						m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, varlenFieldSym->Position), m_CodeGen.TermNode(PUSH, 4)));
					SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->IndexTemp), m_CodeGen.TermNode(LOCLD, temp)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD, varlenFieldSym->LenTemp), m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, temp), m_CodeGen.TermNode(PUSH, 2))));
					m_CodeGen.CommentStatement(string("Incremento variabile FieldCount per MultiField"));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->FieldCount), m_CodeGen.TermNode(PUSH, 1)), varlenFieldSym->FieldCount));

					m_CodeGen.LabelStatement(labelFalse);
				}

				else if(varlenFieldSym->HeaderIndex > 0)
				{
					m_CodeGen.CommentStatement(string("Test HeaderIndexing"));
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, varlenFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)varlenFieldSym->HeaderIndex - 1);//sottraggo 1 perch� l'indice Extract parte da 0
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.CommentStatement(string("HeaderIndex - header ") + varlenFieldSym->Protocol->Name + string(" da estrarre"));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, varlenFieldSym->Position)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (varlenFieldSym->Position)+2)));

					m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.CommentStatement(string("HeaderIndex - header ") + varlenFieldSym->Protocol->Name + string(" da ignorare"));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->ExtractG), m_CodeGen.TermNode(PUSH, 1)), varlenFieldSym->ExtractG));
				}

				else // FirstExtract
				{
					m_CodeGen.CommentStatement(string("Test FirstExtract per il protocollo ") + varlenFieldSym->Protocol->Name);
					Node *leftExpr = m_CodeGen.TermNode(LOCLD, varlenFieldSym->ExtractG);
					Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)1);
					SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_true");
					SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_false");
					m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

					m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.CommentStatement(string("Extract - estrarre prima occorrenza header ") + varlenFieldSym->Protocol->Name);
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, varlenFieldSym->Position)));
					m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (varlenFieldSym->Position)+2)));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(PUSH,(uint32)1), varlenFieldSym->ExtractG)); //Extract = 1

					m_CodeGen.LabelStatement(labelTrue);
				}
			}

			if(varlenFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,varlenFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,varlenFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),varlenFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),varlenFieldSym->Protocol->NExFields));
			}
		}break;

	case PDL_FIELD_BITFIELD:
		{
			SymbolFieldBitField* bitField=(SymbolFieldBitField*)sym;

			if(bitField->ToExtract)
			{
				nbASSERT(bitField->DependsOn != NULL, "depends-on pointer should be != NULL");
				Node *memOffs(0);
				uint32 mask=bitField->Mask;
				SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;

				// search the fixed field root to get the size
				while (container->FieldType!=PDL_FIELD_FIXED)
				{
					nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

					// and the mask with the parent mask
					mask=mask & ((SymbolFieldBitField *)container)->Mask;

					container=((SymbolFieldBitField *)container)->DependsOn;
				}

				SymbolFieldContainer *fieldContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
				SymbolFieldFixed *fixed=(SymbolFieldFixed *)fieldContainer;

				if(fixed->IndexTemp != NULL)
				{
					// after applying the bit mask, you should also shift the field to align the actual value
					uint8 shift=0;
					uint32 tMask=mask;

					while ((tMask & 1)==0)
					{
						shift++;
						tMask=tMask>>1;
					}

					memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
					Node *memLoad = GenMemLoad(memOffs, fixed->Size);
					Node *_field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));

					if (bitField->MultiProto) 
					{
						m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per header ") + bitField->Protocol->Name);
						Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, bitField->HeaderCount), m_CodeGen.TermNode(PUSH, 4)),
							m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, bitField->Position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
					
						if (shift>0)
						{   
							Node *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
						}
						else
						{
							Node *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
						}
						m_CodeGen.CommentStatement(string("Incremento HeaderCounter per header ") + bitField->Name);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->HeaderCount), m_CodeGen.TermNode(PUSH, 1)), bitField->HeaderCount));
						FieldsList_t::iterator f = bitField->MultiFields.begin();
						for (; f != bitField->MultiFields.end(); f++)
						{	//store the number of fields extracted
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.TermNode(LOCLD, bitField->HeaderCount), m_CodeGen.TermNode(PUSH, (*f)->Position)));
						}
					}

					else if (bitField->MultiField)
					{
						m_CodeGen.CommentStatement(string("MultiField - test estrazione"));
						Node *leftExpr = m_CodeGen.TermNode(LOCLD, bitField->ExtractG);
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "MultiField_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "MultiField_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("Calcolo offset Info-Partition per MultiField"));
						Node *slot_offs = m_CodeGen.BinOp(ADD, m_CodeGen.BinOp(IMUL, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 4)),
							m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(PUSH, bitField->Position), m_CodeGen.TermNode(PUSH, 4)));
						SymbolTemp *temp = m_CodeGen.NewTemp("temp", m_CompUnit.NumLocals);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, slot_offs, temp));
						if (shift>0)
						{   
							Node *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
						}
						else
						{
							Node *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(LOCLD, temp)));
						}
						m_CodeGen.CommentStatement(string("Incremento variabile FieldCount per MultiField"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->FieldCount), m_CodeGen.TermNode(PUSH, 1)), bitField->FieldCount));

						m_CodeGen.LabelStatement(labelFalse);
					}

					else if(bitField->HeaderIndex > 0)
					{
						m_CodeGen.CommentStatement(string("Test HeaderIndexing"));
						Node *leftExpr = m_CodeGen.TermNode(LOCLD, bitField->ExtractG);
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)bitField->HeaderIndex - 1);//sottraggo 1 perch� l'indice Extract parte da 0
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "HeaderIndex_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelTrue);
						m_CodeGen.CommentStatement(string("HeaderIndex - header ") + bitField->Protocol->Name + string(" da estrarre"));
						if (shift>0)
						{   
							Node *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, bitField->Position)));
						}
						else
						{
							Node *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, bitField->Position)));
						}

						m_CodeGen.LabelStatement(labelFalse);
						m_CodeGen.CommentStatement(string("HeaderIndex - header ") + bitField->Protocol->Name + string(" da ignorare"));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->ExtractG), m_CodeGen.TermNode(PUSH, 1)), bitField->ExtractG));
					}
					else
					{
						m_CodeGen.CommentStatement(string("Test FirstExtract per il protocollo ") + bitField->Protocol->Name);
						Node *leftExpr = m_CodeGen.TermNode(LOCLD, bitField->ExtractG);
						Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)1);
						SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_true");
						SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "FirstExtract_false");
						m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPEQ, leftExpr, rightExpr));

						m_CodeGen.LabelStatement(labelFalse);
						m_CodeGen.CommentStatement(string("Extract - estrarre prima occorrenza header ") + bitField->Protocol->Name);
						if (shift>0)
						{   
							Node *validation=m_CodeGen.BinOp(ADD,m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)),m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, bitField->Position)));
						}
						else
						{
							Node *validation=m_CodeGen.BinOp(ADD,_field,m_CodeGen.TermNode(PUSH,0x80000000));
							m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, validation, m_CodeGen.TermNode(PUSH, bitField->Position)));
						}
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(PUSH,(uint32)1), bitField->ExtractG)); //Extract = 1

						m_CodeGen.LabelStatement(labelTrue);
						
					}
				}
			}

			if(bitField->Protocol->ExAllfields)
			{			
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,bitField->ID), positionId));
				Node *positionValue= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				
				Node *memOffs(0);
				uint32 mask=bitField->Mask;
				SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;

				// search the fixed field root to get the size
				while (container->FieldType!=PDL_FIELD_FIXED)
				{
					nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

					// and the mask with the parent mask
					mask=mask & ((SymbolFieldBitField *)container)->Mask;

					container=((SymbolFieldBitField *)container)->DependsOn;
				}

								SymbolFieldContainer *fieldContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
								SymbolFieldFixed *fixed=(SymbolFieldFixed *)fieldContainer;

				if(fixed->IndexTemp != NULL)
				{
					// after applying the bit mask, you should also shift the field to align the actual value
					uint8 shift=0;
					uint32 tMask=mask;

					while ((tMask & 1)==0)
					{
						shift++;
						tMask=tMask>>1;
					}

					memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
					Node *memLoad = GenMemLoad(memOffs, fixed->Size);
					Node *_field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));
					if (shift>0)
					{   m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, m_CodeGen.BinOp(SHR, _field, m_CodeGen.TermNode(PUSH, shift)), positionValue));
					}
					else
					{	m_CodeGen.GenStatement(m_CodeGen.BinOp(IISTR, _field, positionValue));
					}
				}

				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),bitField->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, bitField->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),bitField->Protocol->NExFields));
								
			}
		
		}
		break;
	case PDL_FIELD_PADDING:
		{
			/*
			From NetPDLProtoDecoder.cpp in the NetBee libray sources:
			FieldLen= (StartingOffset - m_protoStartingOffset) % ( ((struct _nbNetPDLElementFieldPadding *)FieldElement)->Align);
			In other words:
			$fld_len = ($currentoffset-$protooffset) MOD $fld_align
			*/

			SymbolFieldPadding *paddingFieldSym = (SymbolFieldPadding*)fieldSym;
			Node *currOffsNode = m_CodeGen.TermNode(LOCLD, currOffsTemp);
			Node *protoOffsNode = m_CodeGen.TermNode(LOCLD, protoOffsTemp);
			//$fld_len = ($currentoffset-$protooffset) MOD $fld_align
			Node *lenExpr = m_CodeGen.BinOp(MOD, m_CodeGen.BinOp(SUB, currOffsNode, protoOffsNode), m_CodeGen.TermNode(PUSH, paddingFieldSym->Align));
			//$add = $currentoffset + $len
			Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), lenExpr);
			//$currentoffset = $add
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
		}break;

	case PDL_FIELD_TOKEND:
		{
#ifdef USE_REGEX
			SymbolFieldTokEnd *tokendFieldSym = (SymbolFieldTokEnd*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			tokendFieldSym->IndexTemp = indexTemp;
			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
			SymbolTemp *lenTemp;
			Node *len = m_CodeGen.TermNode(PUSH, (uint32) 0);

			if (tokendFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				tokendFieldSym->LenTemp = lenTemp;
			}



			if(tokendFieldSym->EndTok!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokendFieldSym->EndTok));

				SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,offset);
			}
			else if(tokendFieldSym->EndRegEx!=NULL)
			{
				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokendFieldSym->EndRegEx));

				SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,offset);
			}
			else
				break;

			SymbolLabel *len_true = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			SymbolLabel *len_false = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			Node *zero = m_CodeGen.TermNode(PUSH, (uint32) 0);

			//if the field exists, "len" is defined -> EndTok found.
			m_CodeGen.JCondStatement(len_true, len_false, m_CodeGen.BinOp(JCMPNEQ, len, zero));

			m_CodeGen.LabelStatement(len_true);

			if(tokendFieldSym->EndOff!=NULL)
			{
				len = TranslateTree( tokendFieldSym->EndOff );
			}

			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));

			if(tokendFieldSym->EndDiscard!=NULL)
			{
				if (tokendFieldSym->DiscTemp==NULL)
				{
				SymbolTemp *discTemp = m_CodeGen.NewTemp(tokendFieldSym->Name + string("_disc"), m_CompUnit.NumLocals);
				tokendFieldSym->DiscTemp = discTemp;
				}
			Node *discExpr = TranslateTree(tokendFieldSym->EndDiscard);
			//$fld_disc = discExpr
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, discExpr, tokendFieldSym->DiscTemp));
			}

			if (sym->DependsOn==NULL)
			{	Node *addNode;
				//$add = $currentoffset + $len

				addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), len);

				//$add = $add +$discard
				if(tokendFieldSym->DiscTemp!=NULL)
				{
				addNode=m_CodeGen.BinOp(ADD,addNode,m_CodeGen.TermNode(LOCLD, tokendFieldSym->DiscTemp));
				}
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(tokendFieldSym->ToExtract)
			{
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, tokendFieldSym->Position)));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (tokendFieldSym->Position)+2)));	
			}

			if(tokendFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,tokendFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokendFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),tokendFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),tokendFieldSym->Protocol->NExFields));
			}
			m_CodeGen.LabelStatement(len_false);
			// do nothing
#else
	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
	break;
#endif
		}break;


	case PDL_FIELD_TOKWRAP:
		{
#ifdef USE_REGEX
			SymbolFieldTokWrap *tokwrapFieldSym = (SymbolFieldTokWrap*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			tokwrapFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp;
			Node* offsetNode=offset;
			Node* lenRegEx;
			Node* begin=offset;
			Node *len=m_CodeGen.TermNode(PUSH,(uint32)0);
			Node* new_begin;


			if (tokwrapFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				tokwrapFieldSym->LenTemp = lenTemp;
			}

			if(tokwrapFieldSym->BeginTok!=NULL)
			{
				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->BeginTok));

				SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
					SymbolLabel *labelBeginTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
					SymbolLabel *labelBeginFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
					m_CodeGen.JCondStatement(labelBeginTrue, labelBeginFalse, m_CodeGen.BinOp(JCMPEQ, offset,m_CodeGen.BinOp(SUB,offsetNode,lenRegEx)));
					m_CodeGen.LabelStatement(labelBeginFalse);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
						break;
					m_CodeGen.LabelStatement(labelBeginTrue);
						//$begin = $offsetNode - $lenRegEx
						begin=m_CodeGen.BinOp(SUB,offsetNode,lenRegEx);

			}
			else if(tokwrapFieldSym->BeginRegEx!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->BeginRegEx));

				SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
					//$begin = $offsetNode - $lenRegEx
					begin=m_CodeGen.BinOp(SUB,offsetNode,lenRegEx);
			}
			else
				break;


			if(tokwrapFieldSym->EndTok!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->EndTok));

				SymbolRegEx *regExp = new SymbolRegEx(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,begin);
			}
			else if(tokwrapFieldSym->EndRegEx!=NULL)
			{

				SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)tokwrapFieldSym->EndRegEx));

				SymbolRegEx *regExp = new SymbolRegEx(offsetNode, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
				m_GlobalSymbols.StoreRegExEntry(regExp);

				//set id
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_PATTERN_ID));

				// set buffer offset
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				TranslateTree(regExp->Offset),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_OFFSET));

				// calculate and set buffer length
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
				new SymbolLabel(LBL_ID, 0, string("regexp")),
				REGEX_OUT_BUF_LENGTH));


				// find the pattern
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

				// check the result
				Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

				Node *leftExpr = validNode;
				Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
				SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
				SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
				m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

				// if there aren't any matches the field doesn't exist, don't update the offset
				m_CodeGen.LabelStatement(labelFalse);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));
					break;
				m_CodeGen.LabelStatement(labelTrue);
					m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
					offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
					len=m_CodeGen.BinOp(SUB,offsetNode,begin);
			}
			else
				break;


			if(tokwrapFieldSym->BeginOff!=NULL)
			{
				new_begin = TranslateTree( tokwrapFieldSym->BeginOff );
				begin=m_CodeGen.BinOp(ADD,offset,new_begin);
			}
			//$fld_index = $begin
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, begin, indexTemp));

			if(tokwrapFieldSym->EndOff!=NULL)
			{
				Node *offExpr = TranslateTree( tokwrapFieldSym->EndOff );
				len =m_CodeGen.BinOp(SUB,m_CodeGen.BinOp(ADD,offExpr,offset),begin);
			}
			//$fld_len = $len
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, len, lenTemp));

			if(tokwrapFieldSym->EndDiscard!=NULL)
			{
				if (tokwrapFieldSym->DiscTemp==NULL)
				{
				SymbolTemp *discTemp = m_CodeGen.NewTemp(tokwrapFieldSym->Name + string("_disc"), m_CompUnit.NumLocals);
				tokwrapFieldSym->DiscTemp = discTemp;
				}
			Node *discExpr = TranslateTree(tokwrapFieldSym->EndDiscard);
			//$fld_disc = discExpr
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, discExpr, tokwrapFieldSym->DiscTemp));
			}

			if (sym->DependsOn==NULL)
			{	Node *addNode;

				//$add = $begin + $len
				if(tokwrapFieldSym->LenTemp!=NULL)
				{
				addNode = m_CodeGen.BinOp(ADD, begin, len);
				}
				//$add = $add +$discard
     			if(tokwrapFieldSym->DiscTemp!=NULL)
				{
				addNode=m_CodeGen.BinOp(ADD,addNode,m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->DiscTemp));
				}

				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(tokwrapFieldSym->ToExtract)
			{
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, tokwrapFieldSym->Position)));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (tokwrapFieldSym->Position)+2)));	
			}

			if(tokwrapFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,tokwrapFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,tokwrapFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),tokwrapFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),tokwrapFieldSym->Protocol->NExFields));
			}

#else
	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
	break;
#endif
		}break;


		case PDL_FIELD_LINE:
			{
//#ifdef USE_REGEX

			SymbolFieldLine *lineFieldSym = (SymbolFieldLine*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(lineFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			lineFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp = NULL;
			if (lineFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(lineFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				lineFieldSym->LenTemp = lenTemp;
			}

			m_CodeGen.TermNode(PUSH, (uint32) 0);

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset->Clone(), indexTemp));

			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)lineFieldSym->EndTok));

			SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
			m_GlobalSymbols.StoreRegExEntry(regExp);

			//set id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.TermNode(PUSH, regExp->Id),
				new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_PATTERN_ID));

			// set buffer offset
			// m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, TranslateTree(regExp->Offset),
			//	new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_OFFSET));
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, regExp->Offset->Clone(),
					new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_OFFSET));

			// calculate and set buffer length
			// m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), TranslateTree(regExp->Offset) ),
			//	new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), regExp->Offset->Clone() ),
					new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));

			// find the pattern
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

			// check the result
			Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

			Node *leftExpr = validNode;
			Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
			SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPG, leftExpr, rightExpr));

			// if there aren't any matches the field doesn't exist, don't update the offset
			//m_CodeGen.LabelStatement(labelFalse);
			//	break;
			m_CodeGen.LabelStatement(labelTrue);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
				Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
				//Node *lenRegEx = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offsetNode, lenTemp));
			m_CodeGen.LabelStatement(labelFalse);

			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, lineFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(lineFieldSym->ToExtract)
			{
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, lineFieldSym->Position)));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (lineFieldSym->Position)+2)));	
			}

			if(lineFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,lineFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,lineFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),lineFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),lineFieldSym->Protocol->NExFields));
			}

//#else
//	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
//	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
//	break;
//#endif
			}
			break;
		case PDL_FIELD_PATTERN:
		{
//#ifdef USE_REGEX
			SymbolFieldPattern *patternFieldSym = (SymbolFieldPattern*)sym;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(patternFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			patternFieldSym->IndexTemp = indexTemp;
			SymbolTemp *lenTemp = NULL;
			if (patternFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(patternFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				patternFieldSym->LenTemp = lenTemp;
			}

			m_CodeGen.TermNode(PUSH, (uint32) 0);

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset->Clone(), indexTemp));

			SymbolStrConst *pattern = (SymbolStrConst *)m_CodeGen.ConstStrSymbol(string((const char*)patternFieldSym->Pattern));

			SymbolRegEx *regExp = new SymbolRegEx(offset, pattern, true, m_GlobalSymbols.GetRegExEntriesCount());
			m_GlobalSymbols.StoreRegExEntry(regExp);

			//set id
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.TermNode(PUSH, regExp->Id),
					new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_PATTERN_ID));

			// set buffer offset - comment to implement 'set behaviour'
			//m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, regExp->Offset->Clone(),
			//		new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_OFFSET));

			// calculate and set buffer length - comment to implement set behaviour, see below
			//m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.BinOp( SUB, m_CodeGen.TermNode(PBL), regExp->Offset->Clone() ),
			//		new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));
			// uncomment this
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT, m_CodeGen.TermNode(PBL),
					new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_OUT_BUF_LENGTH));

			// find the pattern
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_MATCH_WITH_OFFSET));

			// check the result
			Node *validNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_MATCHES_FOUND);

			Node *leftExpr = validNode;
			Node *rightExpr = m_CodeGen.TermNode(PUSH, (uint32)0);
			SymbolLabel *labelTrue = m_CodeGen.NewLabel(LBL_CODE, "if_true");
			SymbolLabel *labelFalse = m_CodeGen.NewLabel(LBL_CODE, "if_false");
			m_CodeGen.JCondStatement(labelTrue, labelFalse, m_CodeGen.BinOp(JCMPGE, leftExpr, rightExpr));

			m_CodeGen.LabelStatement(labelTrue);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_GET_RESULT));
				Node *offsetNode = m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_OFFSET_FOUND);
				Node *lenRegEx= m_CodeGen.TermNode(COPIN, new SymbolLabel(LBL_ID, 0, string("regexp")), REGEX_IN_LENGTH_FOUND);
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, lenRegEx->Clone(), lenTemp));
				// uncomment next line to implement set behaviour
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp( SUB, offsetNode->Clone(), lenRegEx->Clone()), indexTemp));
			// if there aren't any matches the field doesn't exist, don't update the offset
			m_CodeGen.LabelStatement(labelFalse);

			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, patternFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(patternFieldSym->ToExtract)
			{
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, patternFieldSym->Position)));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (patternFieldSym->Position)+2)));	
			}

			if(patternFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,patternFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,patternFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),patternFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),patternFieldSym->Protocol->NExFields));
			}

//#else
//	this->GenerateWarning("Regular Expressions disabled.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
//	m_CodeGen.CommentStatement("ERROR: Regular Expressions disabled.");
//	break;
//#endif
			}break;
		case PDL_FIELD_EATALL:
		{

			SymbolFieldEatAll *eatallFieldSym = (SymbolFieldEatAll*)fieldSym;
			Node *currOffsNode = m_CodeGen.TermNode(LOCLD, currOffsTemp);
			SymbolTemp *lenTemp = NULL;
			SymbolTemp *indexTemp = m_CodeGen.NewTemp(eatallFieldSym->Name + string("_ind"), m_CompUnit.NumLocals);
			eatallFieldSym->IndexTemp = indexTemp;

			if (eatallFieldSym->LenTemp==NULL)
			{
				lenTemp = m_CodeGen.NewTemp(eatallFieldSym->Name + string("_len"), m_CompUnit.NumLocals);
				eatallFieldSym->LenTemp = lenTemp;
			}

			//$fld_index = $currentoffset
			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, offset, indexTemp));

			if(sym->DependsOn==NULL)
			{
				//$fld_len = ($packet_end-$currentoffset)
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB, m_CodeGen.TermNode(PBL), currOffsNode), lenTemp));
			}
			else
			{
				//$fld_len = ($parent_field_offset_end-$currentoffset)
				Node *p_fieldOffsNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,((SymbolFieldEatAll *)sym->DependsOn)->IndexTemp), m_CodeGen.TermNode(LOCLD,((SymbolFieldEatAll*)sym->DependsOn)->LenTemp));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.BinOp(SUB, p_fieldOffsNode, offset), lenTemp));
			}



			if (sym->DependsOn==NULL)
			{
				//$add = $currentoffset + $len
				Node *addNode = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, currOffsTemp), m_CodeGen.TermNode(LOCLD, eatallFieldSym->LenTemp));
				//$currentoffset = $add
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, addNode, currOffsTemp));
			}

			if(eatallFieldSym->ToExtract)
			{
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->IndexTemp), m_CodeGen.TermNode(PUSH, eatallFieldSym->Position)));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), m_CodeGen.TermNode(PUSH, (eatallFieldSym->Position)+2)));	
			}

			if(eatallFieldSym->Protocol->ExAllfields)
			{
				Node *positionId= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD,eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 2));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(PUSH,eatallFieldSym->ID), positionId));
				Node *positionOff= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 4));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->IndexTemp), positionOff));
				Node *positionLen= m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position), m_CodeGen.TermNode(PUSH, 6));
				m_CodeGen.GenStatement(m_CodeGen.BinOp(ISSTR, m_CodeGen.TermNode(LOCLD,eatallFieldSym->LenTemp), positionLen));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->position) ,m_CodeGen.TermNode(PUSH, 6)),eatallFieldSym->Protocol->position));
				m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallFieldSym->Protocol->NExFields) ,m_CodeGen.TermNode(PUSH, 1)),eatallFieldSym->Protocol->NExFields));
			}

		}break;

	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

void IRLowering::TranslateVarDeclInt(Node *node)
{
	nbASSERT(node->Op == IR_DEFVARI, "node must be an IR_DEFVARI");
	Node *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	Node *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "left child symbol must be an integer variable");

	SymbolVarInt *intVar = (SymbolVarInt*)varSym;
	SymbolTemp *temp = m_CodeGen.NewTemp(varSym->Name, m_CompUnit.NumLocals);
	intVar->Temp = temp;
	Node *initializer(0);
	if (intVar->Name.compare("$pbl") == 0)
		initializer = m_CodeGen.TermNode(PBL);
	else
		initializer = TranslateTree(rightChild);
	nbASSERT(initializer != NULL, "initializer cannot be NULL");
	//$var = init_expr
	m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, initializer, temp));

}

void IRLowering::TranslateVarDeclStr(Node *node)
{
	nbASSERT(node->Op == IR_DEFVARS, "node must be an IR_DEFVARS");
	Node *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");


	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	switch (varSym->VarType)
	{
	case PDL_RT_VAR_REFERENCE:
		//do nothing
		break;
	case PDL_RT_VAR_BUFFER:
		//!\todo support buffer variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	case PDL_RT_VAR_PROTO:
		//!\todo support proto variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

SymbolTemp *IRLowering::AssociateVarTemp(SymbolVarInt *var)
{
	//if the var has not already an associated temporary we create it
	//such case is when a variable has been generated by the compiler
	if(var->Temp == NULL)
	{
		SymbolTemp *temp = m_CodeGen.NewTemp(var->Name, m_CompUnit.NumLocals);
		var->Temp = temp;
	}
	return var->Temp;
}

void IRLowering::TranslateAssignInt(Node *node)
{
	nbASSERT(node->Op == IR_ASGNI, "node must be an IR_ASGNI");
	Node *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	Node *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	nbASSERT(varSym->VarType == PDL_RT_VAR_INTEGER, "left child symbol must be an integer variable");
	SymbolVarInt *intVar = (SymbolVarInt*)varSym;

	if (rightChild->Op==IR_SVAR && rightChild->Sym!=NULL && rightChild->Sym->SymKind==SYM_RT_LOOKUP_ITEM)
	{
		SymbolLookupTableItem *item = (SymbolLookupTableItem *)rightChild->Sym;
		SymbolLookupTable *table = item->Table;

		if (item->Size <= 4)
		{
			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, table->Id),
				table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 itemOffset = table->GetValueOffset(item->Name);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, itemOffset),
				table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));

			Node *itemNode = m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);

			m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, itemNode, AssociateVarTemp(intVar)));
		}
		else
		{
			this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
			m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
		}
	}
	else
	{
		//$var = init_expr
		m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, TranslateTree(rightChild), AssociateVarTemp(intVar)));
	}
}

void IRLowering::TranslateAssignRef(SymbolVarBufRef *refVar, Node *right)
{
	nbASSERT(false, "CANNOT BE HERE");
#if 0
	if (refVar->IndexTemp == NULL)
	{
		nbASSERT(refVar->LenTemp == NULL, "index temp is NULL, lenTemp should be NULL");
		refVar->IndexTemp = m_CodeGen.NewTemp(refVar->Name + string("_ind"), m_CompUnit.NumLocals);
		refVar->LenTemp = m_CodeGen.NewTemp(refVar->Name + string("_len"), m_CompUnit.NumLocals);
	}

	//!\todo manage packet reference
#endif
}

void IRLowering::TranslateAssignStr(Node *node)
{
	nbASSERT(node->Op == IR_ASGNS, "node must be an IR_ASGNS");
	Node *leftChild = node->GetLeftChild();
	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(leftChild->Sym != NULL, "left child symbol cannot be NULL");
	nbASSERT(leftChild->Sym->SymKind == SYM_RT_VAR, "left child symbol must be a SYM_RT_VAR");

	Node *rightChild = node->GetRightChild();
	nbASSERT(rightChild != NULL, "right child cannot be NULL");
	nbASSERT(rightChild->IsString(), "right child should be of type string");
	nbASSERT(rightChild->Op == IR_FIELD || rightChild->Op == IR_SVAR, "right child should be a string");
	SymbolVariable *varSym = (SymbolVariable*)leftChild->Sym;
	
	
	switch (varSym->VarType)
	{

	case PDL_RT_VAR_REFERENCE:
		{
			SymbolVarBufRef *ref=(SymbolVarBufRef *)varSym;
			SymbolField *referee=(SymbolField *)node->Sym;
			
			switch (referee->FieldType)
			{
			case PDL_FIELD_FIXED:
				{
					SymbolFieldFixed *fixedField=(SymbolFieldFixed *)referee;

					if (fixedField->UsedAsInt)
					{
						if (ref->IntCompatible)
						{
							// the reference is used as integer, preload the value

							if (ref->ValueTemp == NULL)
								ref->ValueTemp=m_CodeGen.NewTemp(ref->Name + string("_ref_temp"), m_CompUnit.NumLocals);

							m_CodeGen.CommentStatement(ref->Name + string(" = ") + fixedField->Name + string(" (preload of ") + ref->ValueTemp->Name + string(")"));

							Node *cint=TranslateFieldToInt(m_CodeGen.TermNode(IR_FIELD, fixedField), NULL, fixedField->Size);
							m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, cint, ref->ValueTemp));
						}
						else
						{
							// the reference is used as integer, but is not compatible
							// (e.g. there's an assignment of a non-int field)
							this->GenerateWarning(string("The variable ")+ref->Name+string(" is used as integer, and it must be compatible to integer;")+
								string(" assignment of ")+fixedField->Name+string(" is not legal"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
							m_CodeGen.CommentStatement(string("ERROR: The variable ")+ref->Name+string(" is used as integer, and it must be compatible to integer;")+
								string(" assignment of ")+fixedField->Name+string(" is not legal"));
						}
					}

					if (fixedField->UsedAsArray || fixedField->UsedAsString)
					{
						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						SymbolTemp *lenTemp = ref->LenTemp;
						if (lenTemp == 0)
							lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

						m_CodeGen.CommentStatement(ref->Name + string(" = ") + fixedField->Name + string(" (") + indexTemp->Name + string(")"));

						//$fld_index = $referee_index
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp), indexTemp));

						//$fld_len = var_size
						m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(PUSH, fixedField->Size), lenTemp));
					}

					if (!fixedField->UsedAsInt && !fixedField->UsedAsArray && !fixedField->UsedAsString)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is not used, assignment will be omitted"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						m_CodeGen.CommentStatement(string("INFO: The variable ")+ref->Name+string(" is not used, assignment will be omitted"));
					}

				}break;


			case PDL_FIELD_VARLEN:
				{
					SymbolFieldVarLen *varLenField=(SymbolFieldVarLen *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + varLenField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, varLenField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, varLenField->LenExpr, lenTemp));
				}break;

			case PDL_FIELD_TOKEND:
				{
					SymbolFieldTokEnd *tokEndField=(SymbolFieldTokEnd *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + tokEndField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokEndField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(LOCLD, tokEndField->LenTemp), lenTemp));

				}break;

			case PDL_FIELD_TOKWRAP:
				{
					SymbolFieldTokWrap *tokWrapField=(SymbolFieldTokWrap *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);


					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);


					m_CodeGen.CommentStatement(ref->Name + string(" = ") + tokWrapField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokWrapField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, tokWrapField->LenTemp), lenTemp));

				}break;


			case PDL_FIELD_LINE:
				{
					SymbolFieldLine *lineField=(SymbolFieldLine *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + lineField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, lineField->IndexTemp), indexTemp));

					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST,m_CodeGen.TermNode(LOCLD, lineField->LenTemp), lenTemp));

				}break;

			case PDL_FIELD_PATTERN:
				{
					SymbolFieldPattern *patternField=(SymbolFieldPattern *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + patternField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, patternField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD,patternField->LenTemp), lenTemp));
				}break;

			case PDL_FIELD_EATALL:
				{
					SymbolFieldEatAll *eatallField=(SymbolFieldEatAll *)referee;
					SymbolTemp *indexTemp = ref->IndexTemp;
					if (indexTemp == 0)
						indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

					SymbolTemp *lenTemp = ref->LenTemp;
					if (lenTemp == 0)
						lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

					m_CodeGen.CommentStatement(ref->Name + string(" = ") + eatallField->Name + string(" (") + indexTemp->Name + string(")"));

					//$fld_index = $referee_index
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD, eatallField->IndexTemp), indexTemp));
					//$fld_len = $referee_len
					m_CodeGen.GenStatement(m_CodeGen.UnOp(LOCST, m_CodeGen.TermNode(LOCLD,eatallField->LenTemp), lenTemp));
				}break;

			default:
				nbASSERT(false, "A reference variable should refer to fixed , variable length, tokenended, tokenwrapped, line, pattern or eatall field");
			}
		}

		//!\todo support reference variables
		//TranslateAssignRef(((SymbolVarBufRef*)varSym)->Referee, rightChild);
		break;
	case PDL_RT_VAR_BUFFER:
		//!\todo support buffer variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	case PDL_RT_VAR_PROTO:
		//!\todo support proto variables
		nbASSERT(false, "CANNOT BE HERE");
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}


Node *IRLowering::TranslateIntVarToInt(Node *node)
{
	nbASSERT(node->Op == IR_IVAR, "node must be an IR_IVAR");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_RT_VAR, "contained symbol must be an RT_VAR");
	nbASSERT(((SymbolVariable*)node->Sym)->VarType == PDL_RT_VAR_INTEGER, "contained symbol must be an integer variable");
	SymbolVarInt *intVar = (SymbolVarInt*)node->Sym;

	if (intVar->Name.compare("$pbl") == 0)
		return m_CodeGen.TermNode(PBL);
	else
		return m_CodeGen.TermNode(LOCLD, intVar->Temp);
}

Node *IRLowering::TranslateStrVarToInt(Node *node, Node* offset, uint32 size)
{
	//!\todo Implement lowering of CINT(SVAR)
	nbASSERT(node->Op == IR_SVAR, "node must be an IR_VAR");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");

	if (node->Sym->SymKind==SYM_RT_VAR)
	{
		SymbolVariable *varSym = (SymbolVariable*)node->Sym;

		switch(varSym->VarType)
		{
		case PDL_RT_VAR_REFERENCE:
			{
				SymbolVarBufRef *ref = (SymbolVarBufRef*)varSym;

				if (ref->RefType == REF_IS_PACKET_VAR) {
					nbASSERT(offset != NULL, "offsNode should be != NULL");
					return GenMemLoad(offset, size);
				} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
					if (ref->IntCompatible==false)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is not compatible to integer, it cannot be converted;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32) -1);
					}
					if (ref->UsedAsString)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" is used as string in the NetPDL database, it cannot be converted;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32)-1);
					}
					if (ref->ValueTemp == NULL)
					{
						this->GenerateWarning(string("The variable ")+ref->Name+string(" has not been preloaded, and it cannot be used;")+
							string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
						return m_CodeGen.TermNode(PUSH, (uint32)-1);
					}

					return m_CodeGen.TermNode(LOCLD, ref->ValueTemp);
				}


			}
			break;

		case PDL_RT_VAR_BUFFER:
			nbASSERT(false, "TO BE IMPLEMENTED");
			return NULL;
			break;

		default:
			nbASSERT(false, "CANNOT BE HERE");
			return NULL;
		}
	} else if (node->Sym->SymKind==SYM_RT_LOOKUP_ITEM)
	{
		SymbolLookupTableItem *item = (SymbolLookupTableItem *)node->Sym;
		SymbolLookupTable *table = item->Table;

		if (item->Size <= 4)
		{
			// get the entry flag
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, table->Id),
				table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			uint32 itemOffset = table->GetValueOffset(item->Name);
			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
				m_CodeGen.TermNode(PUSH, itemOffset),
				table->Label,
				LOOKUP_EX_OUT_VALUE_OFFSET));

			m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));

			Node *itemNode = m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);

			return itemNode;
		}
		else
		{
			this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
			m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
		}
	} else
	{
		nbASSERT(false, "CANNOT BE HERE");
	}
	//!\todo maybe we should manage this somewhere else!!!

	return NULL;
}


Node *IRLowering::TranslateFieldToInt(Node *node, Node* offset, uint32 size)
{
	nbASSERT(node->Op == IR_FIELD, "node must be an IR_FIELD");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind == SYM_FIELD, "contained symbol should be a field");

	SymbolField *fieldSym = (SymbolField*)node->Sym;

	// [ds] we will use the symbol that defines different version of the same field
	SymbolField *sym = this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, fieldSym);


	if (sym->IntCompatible==false && offset==NULL && size==0)
	{
		this->GenerateWarning(string("The field ")+sym->Name+string(" is not compatible to integer, it cannot be converted;")+
			string(" -1 will be used."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		return m_CodeGen.TermNode(PUSH, (uint32)-1);
	}

	Node *memOffs(0);
	switch(fieldSym->FieldType)
	{
	case PDL_FIELD_FIXED:
		{
			SymbolFieldFixed *fixedField = (SymbolFieldFixed*)sym;

			if (fixedField->ValueTemp != NULL)
			{
				return m_CodeGen.TermNode(LOCLD, fixedField->ValueTemp);
			}
			else if (offset != NULL)
			{
				memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp), offset);
			}
			else
			{
				memOffs = m_CodeGen.TermNode(LOCLD, fixedField->IndexTemp);
				size = fixedField->Size;
			}
			return GenMemLoad(memOffs, size);
		}break;
	case PDL_FIELD_VARLEN:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because var-len fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldVarLen *varlenField = (SymbolFieldVarLen*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, varlenField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;
	case PDL_FIELD_BITFIELD:
		{
			SymbolFieldBitField *bitField = (SymbolFieldBitField*)sym;
			nbASSERT(bitField->DependsOn != NULL, "depends-on pointer should be != NULL");

			uint32 mask=bitField->Mask;
			SymbolFieldContainer *container=(SymbolFieldContainer *)bitField->DependsOn;

			// search the fixed field root to get the size
			while (container->FieldType!=PDL_FIELD_FIXED)
			{
				nbASSERT(container->FieldType==PDL_FIELD_BITFIELD, "Only a bit field can have a parent field");

				// and the mask with the parent mask
				mask=mask & ((SymbolFieldBitField *)container)->Mask;

				container=((SymbolFieldBitField *)container)->DependsOn;
			}

			SymbolFieldContainer *protoContainer=(SymbolFieldContainer *)this->m_GlobalSymbols.LookUpProtoField(this->m_Protocol, container);
			SymbolFieldFixed *fixed=(SymbolFieldFixed *)protoContainer;

			nbASSERT((fixed->Size == 1)||(fixed->Size == 2)||(fixed->Size == 4), "size can be only 1, 2, or 4!");
			nbASSERT(fixed->IndexTemp != NULL, string("the container of the field '").append(fieldSym->Name).append("' was not defined").c_str());

			// after applying the bit mask, you should also shift the field to align the actual value
			uint8 shift=0;
			uint32 tMask=mask;
			while ((tMask & 1)==0)
			{
				shift++;
				tMask=tMask>>1;
			}

			memOffs = m_CodeGen.TermNode(LOCLD, fixed->IndexTemp);
			Node *memLoad = GenMemLoad(memOffs, fixed->Size);
			Node *field = m_CodeGen.BinOp(AND, memLoad, m_CodeGen.TermNode(PUSH, mask));
			if (shift>0)
				return m_CodeGen.BinOp(SHR, field, m_CodeGen.TermNode(PUSH, shift));
			else
				return field;
		}break;

	case PDL_FIELD_TOKEND:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-end fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldTokEnd *tokendField = (SymbolFieldTokEnd*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokendField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_TOKWRAP:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-wrap fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldTokWrap *tokwrapField = (SymbolFieldTokWrap*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, tokwrapField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;


	case PDL_FIELD_LINE:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because tok-end fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldLine *lineField = (SymbolFieldLine*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, lineField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_PATTERN:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because pattern fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldPattern *patternField = (SymbolFieldPattern*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, patternField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_EATALL:
		{
			nbASSERT(offset != NULL, "offset should be != NULL, because eatall fields can be used as integers only referencing them with [offs:len]");
			nbASSERT(size > 0, "size should be > 0");
			SymbolFieldEatAll *eatallField = (SymbolFieldEatAll*)sym;
			memOffs = m_CodeGen.BinOp(ADD, m_CodeGen.TermNode(LOCLD, eatallField->IndexTemp), offset);
			return GenMemLoad(memOffs, size);
		}
		break;

	case PDL_FIELD_PADDING:
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	nbASSERT(false, "CANNOT BE HERE");
	return NULL;
}



Node *IRLowering::GenMemLoad(Node *offsNode, uint32 size)
{
	uint16 memOp(0);
	switch(size)
	{
	case 1:
		memOp = PBLDU; //pload.8
		break;
	case 2:
		memOp = PSLDU;
		break;
	case 3:
	case 4:
		memOp = PILD;
		break;
	default:
		this->GenerateWarning(string("Size should be 1, 2, 3 or 4 bytes"), __FILE__, __FUNCTION__, __LINE__, 1, 3);
		break;
	}

	Node *memLoadNode = m_CodeGen.UnOp(memOp,offsNode);
	if (size == 3)
		return m_CodeGen.BinOp(AND, memLoadNode, m_CodeGen.TermNode(PUSH, 0xFFF0));

	return memLoadNode;
}

Node *IRLowering::TranslateCInt(Node *node)
{
	Node *child = node->GetLeftChild();
	nbASSERT(child != NULL, "left child cannot be NULL");

	if (child->Op == IR_CHGBORD)
	{
		// change byte order
		Node *node = child->GetLeftChild();

		switch (node->Op)
		{
		case IR_FIELD:
			{   SymbolField* field = (SymbolField*)node->Sym;
				switch(field->FieldType)
				{
				case PDL_FIELD_FIXED:
					{
						SymbolFieldFixed *fixed =(SymbolFieldFixed*)field;
						switch(fixed->Size)
						{
						case 1:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 24)));
							}break;
						case 2:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 16)));
							}break;
						case 3:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, m_CodeGen.TermNode(LOCLD,fixed->ValueTemp), m_CodeGen.TermNode(PUSH, 8)));
							}break;
						case 4:
							{
							return m_CodeGen.UnOp(IESWAP,m_CodeGen.TermNode(LOCLD, fixed->ValueTemp));
							}break;
						default:
							nbASSERT(false, "changebyteorder can accept only members with size le than 4");
							break;
						}
					}break;
				default:
					nbASSERT(false, "changebyteorder can accept only members with constant size");
					break;
				}
			}break;
		case IR_SVAR:
			{
				Node *toInvert = TranslateTree(node);
				switch (node->Sym->SymKind)
				{
				case SYM_RT_VAR:
					{
						SymbolVarBufRef *ref = (SymbolVarBufRef*)node->Sym;
						Node *sizeNode = node->GetRightChild();
						nbASSERT(sizeNode->Op==IR_ICONST, "changebyteorder can accept only members with constant size");
						uint32 size = ((SymbolIntConst *)sizeNode->Sym)->Value;


							if (ref->RefType == REF_IS_PACKET_VAR) {
								switch(size)
								{
								case 1:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 24)));
									}break;
								case 2:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 24)));
									}break;
								case 3:
									{
									return m_CodeGen.UnOp(IESWAP,m_CodeGen.BinOp(SHL, toInvert, m_CodeGen.TermNode(PUSH, 8)));
									}break;
								case 4:
									{
									return m_CodeGen.UnOp(IESWAP,toInvert);
									}break;
								default:
									nbASSERT(false, "changebyteorder can accept only members with size le than 4");
									break;

								}
							}

					}break;
				case SYM_RT_LOOKUP_ITEM:
					{
					}break;
				default:
					nbASSERT(false, "String variable unknown");
					break;
				}
			}break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			break;
		}
	}
	else
	{
		// other nodes...
		nbASSERT(child->Sym != NULL, "child symbol cannot be NULL");

		uint32 size(0);
		Node *offsNode = child->GetLeftChild();

		if (offsNode != NULL)
		{
			offsNode = TranslateTree(offsNode);
			Node *lenNode = child->GetRightChild();
			if (lenNode->Op != IR_ICONST)
			{
				this->GenerateWarning(string("A buf2int expression must be used with an ICONST value."), __FILE__, __FUNCTION__, __LINE__, 1, 3);
				return NULL;
			}
			size = ((SymbolIntConst*)lenNode->Sym)->Value;
		}

		switch (child->Op)
		{
		case IR_IVAR:
			return TranslateIntVarToInt(child);
			break;
		case IR_SVAR:
			return TranslateStrVarToInt(child, offsNode, size);
			break;
		case IR_FIELD:
			return TranslateFieldToInt(child, offsNode, size);
			break;
		default:
			nbASSERT(false, "CANNOT BE HERE");
			return NULL;
			break;
		}
	}
	return NULL;
}

Node *IRLowering::TranslateConstInt(Node *node)
{
	nbASSERT(node->Op == IR_ICONST || node->Op == IR_BCONST, "node must be an IR_ICONST or and IR_BCONST");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_INT_CONST, "contained symbol must be an INT_CONST");
	SymbolIntConst *intConst = (SymbolIntConst*)node->Sym;

	return m_CodeGen.TermNode(PUSH, intConst->Value);

}

Node *IRLowering::TranslateConstStr(Node *node)
{
	/*nbASSERT(node->Op == IR_SCONST, "node must be an IR_SCONST");
	nbASSERT(node->Sym != NULL, "contained symbol cannot be NULL");
	nbASSERT(node->Sym->SymKind = SYM_STR_CONST, "contained symbol must be an STR_CONST");
	SymbolStrConst *strConst = (SymbolStrConst*)node->Sym;

	return m_CodeGen.UnOp(strConst->MemOffset, strConst->Size);*/

	return NULL;
}

Node *IRLowering::TranslateConstBool(Node *node)
{
	//at the moment bool constants are handled like integer variables
	//indeed they can only be generated by constant folding...
	return TranslateConstInt(node);

}

Node *IRLowering::TranslateArithBinOp(uint16 op, Node *node)
{
	nbASSERT(node->IsBinOp(), "node must be a binary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	Node *leftChild = node->GetLeftChild();
	Node *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	//nbASSERT(leftChild->IsInteger(), "left child must be integer");
	//nbASSERT(rightChild->IsInteger(), "right child must be integer");
	Node *left = TranslateTree(leftChild);
	Node *right = TranslateTree(rightChild);
	return m_CodeGen.BinOp(op, left, right);
}

Node *IRLowering::TranslateArithUnOp(uint16 op, Node *node)
{
	nbASSERT(node->IsUnOp(), "node must be a unary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	Node *leftChild = node->GetLeftChild();
	Node *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild == NULL, "right child should be NULL");

	nbASSERT(leftChild->IsInteger(), "left child must be integer");

	return m_CodeGen.UnOp(op, TranslateTree(leftChild));
}

uint32 log2(uint32 X)
{
	return (uint32)(log((double)X)/log((double)2));
}

Node *IRLowering::TranslateDiv(Node *node)
{
	nbASSERT(node->IsBinOp(), "node must be a binary operator");
	nbASSERT(node->IsInteger(), "node must be an integer operator");

	Node *leftChild = node->GetLeftChild();
	Node *rightChild = node->GetRightChild();

	nbASSERT(leftChild != NULL, "left child cannot be NULL");
	nbASSERT(rightChild != NULL, "right child cannot be NULL");

	nbASSERT(leftChild->IsInteger(), "left child must be integer");
	nbASSERT(rightChild->IsInteger(), "right child must be integer");
	nbASSERT(rightChild->Sym->SymKind == SYM_INT_CONST, "right operand symbol should be an integer constant");
	nbASSERT((((SymbolIntConst*)rightChild->Sym)->Value % 2) == 0, "right operand should be a power of 2")
		uint32 shiftVal = log2(((SymbolIntConst*)rightChild->Sym)->Value);
	return m_CodeGen.BinOp(SHR, TranslateTree(leftChild), m_CodeGen.TermNode(PUSH, shiftVal));
}

Node *IRLowering::TranslateTree(Node *node)
{
	switch(node->Op)
	{

	case IR_IVAR:
		return TranslateIntVarToInt(node);
		break;
		/*these should be handled by TranslateRelOpStr
		case IR_SVAR:
		break;
		case IR_FIELD:
		break;
		case IR_SCONST:
		return TranslateConstStr(node);
		break;
		*/
	case IR_ITEMP:
		//we should not find a Temp inside the High Level IR
		nbASSERT(false, "CANNOT BE HERE");
		return NULL;
		break;
	case IR_ICONST:
		return TranslateConstInt(node);
		break;
	case IR_BCONST:
		return TranslateConstBool(node);
		break;
	case IR_CINT:
		return TranslateCInt(node);
		break;
	case IR_ADDI:
		return TranslateArithBinOp(ADD, node);
		break;
	case IR_SUBI:
		return TranslateArithBinOp(SUB, node);
		break;
	case IR_DIVI:
		return TranslateDiv(node);
		break;
	case IR_MULI:
		return TranslateArithBinOp(IMUL, node);
		break;
	case IR_NEGI:
		return TranslateArithUnOp(NEG, node);
		break;
	case IR_SHLI:
		return TranslateArithBinOp(SHL, node);
		break;
	case IR_MODI:
		return TranslateArithBinOp(MOD, node);
		break;
	case IR_SHRI:
		return TranslateArithBinOp(SHR, node);
		break;
	case IR_XORI:
		return TranslateArithBinOp(XOR, node);
		break;
	case IR_ANDI:
		return TranslateArithBinOp(AND, node);
		break;
	case IR_ORI:
		return TranslateArithBinOp(OR, node);
		break;
	case IR_NOTI:
		return TranslateArithUnOp(NOT, node);
		break;
	case IR_SCONST:
		{
			SymbolTemp *offset=m_CodeGen.NewTemp(((SymbolStrConst*)node->Sym)->Name + string("_offs"), m_CompUnit.NumLocals);
			Node *offsetNode=m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)node->Sym)->MemOffset);
			m_CodeGen.GenStatement(m_CodeGen.TermNode(LOCST, offset, offsetNode));
			return m_CodeGen.TermNode(LOCLD,
				offset,
				m_CodeGen.TermNode(PUSH, ((SymbolStrConst*)node->Sym)->Size));
		}break;
	case IR_FIELD:
		this->GenerateWarning("Support for string comparisons is not yet implemented.", __FILE__, __FUNCTION__, __LINE__, 1, 4);
		m_CodeGen.CommentStatement("ERROR: Support for string comparisons is not yet implemented.");
		return NULL;
		break;
	case IR_SVAR:
		{
			nbASSERT(node->Sym != NULL, "node symbol cannot be NULL");

			uint32 size(0);
			Node *offsNode = node->GetLeftChild();

			if (offsNode != NULL)
			{
				offsNode = TranslateTree(offsNode);
				Node *lenNode = node->GetRightChild();
				nbASSERT(lenNode->Op == IR_ICONST, "lenNode should be an IR_ICONST");
				size = ((SymbolIntConst*)lenNode->Sym)->Value;
			}

			switch (node->Sym->SymKind)
			{
			case SYM_RT_VAR:
				{
					SymbolVarBufRef *ref = (SymbolVarBufRef*)node->Sym;

					if (ref->RefType == REF_IS_PACKET_VAR) {
						nbASSERT(offsNode != NULL, "offsNode should be != NULL");
						return GenMemLoad(offsNode, size);
					} else if (ref->RefType == REF_IS_REF_TO_FIELD) {
						SymbolTemp *indexTemp = ref->IndexTemp;
						if (indexTemp == 0)
							indexTemp = ref->IndexTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_ind"), m_CompUnit.NumLocals);

						SymbolTemp *lenTemp = ref->LenTemp;
						if (lenTemp == 0)
							lenTemp = ref->LenTemp = m_CodeGen.NewTemp(ref->Name + string("_ref_len"), m_CompUnit.NumLocals);

						return m_CodeGen.TermNode(LOCLD, ref->IndexTemp);
					}
				}break;
			case SYM_RT_LOOKUP_ITEM:
				{
					SymbolLookupTableItem *item = (SymbolLookupTableItem *)node->Sym;
					SymbolLookupTable *table = item->Table;

					if (item->Size <= 4)
					{
						// get the entry flag
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
							m_CodeGen.TermNode(PUSH, table->Id),
							table->Label,
							LOOKUP_EX_OUT_TABLE_ID));

						uint32 itemOffset = table->GetValueOffset(item->Name);
						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPOUT,
							m_CodeGen.TermNode(PUSH, itemOffset),
							table->Label,
							LOOKUP_EX_OUT_VALUE_OFFSET));

						m_CodeGen.GenStatement(m_CodeGen.UnOp(COPRUN, table->Label, LOOKUP_EX_OP_GET_VALUE));
						return m_CodeGen.TermNode(COPIN, table->Label, LOOKUP_EX_IN_VALUE);
					}
					else
					{
						this->GenerateWarning(string("A lookup table item should be assigned to an integer variable, but the size is > 4."), __FILE__, __FUNCTION__, __LINE__, 1, 4);
						m_CodeGen.CommentStatement(string("ERROR: A lookup table item should be assigned to an integer variable, but the size is > 4."));
					}
				}break;
			default:
				nbASSERT(false, "String variable unknown");
				break;
			}
		}break;

	case IR_LKSEL:
		{
			//SymbolLookupTableEntry *entry = (SymbolLookupTableEntry *)node->Sym;
			//nbASSERT(node->GetLeftChild() != NULL, "Lookup select instruction should specify a keys list");
			//SymbolLookupTableKeysList *keys = (SymbolLookupTableKeysList *)node->GetLeftChild()->Sym;
			//TranslateLookupSelect(entry, keys);
		}break;

	case IR_LKHIT:
		//TranslateRelOpLookup(IR_LKHIT, expr, jcInfo);
		break;

	case IR_REGEXFND:
		//TranslateRelOpRegEx(expr, jcInfo);
		break;

		/*
		The following cases are managed by TranslateBoolExpr()

		case IR_ANDB:

		case IR_ORB:

		case IR_NOTB:

		case IR_EQI:

		case IR_GEI:

		case IR_GTI:

		case IR_LEI:

		case IR_LTI:

		case IR_NEI:

		case IR_EQS:

		case IR_NES:
		*/
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
	return NULL;
}


void IRLowering::TranslateStatement(StmtBase *stmt)
{
	m_TreeDepth = 0;
	switch(stmt->Kind)
	{
	case STMT_LABEL:
		TranslateLabel((StmtLabel*)stmt);
		break;

	case STMT_GEN:
		TranslateGen((StmtGen*)stmt);
		break;

	case STMT_JUMP:
		TranslateJump((StmtJump*)stmt);
		break;

	case STMT_SWITCH:
		TranslateSwitch((StmtSwitch*)stmt);
		break;
	case STMT_COMMENT:
		m_CodeGen.CommentStatement(stmt->Comment);
		break;
	case STMT_IF:
		TranslateIf((StmtIf*)stmt);
		break;
	case STMT_LOOP:
		TranslateLoop((StmtLoop*)stmt);
		break;
	case STMT_WHILE:
		TranslateWhileDo((StmtWhile*)stmt);
		break;
	case STMT_DO_WHILE:
		TranslateDoWhile((StmtWhile*)stmt);
		break;
	case STMT_BREAK:
		TranslateBreak((StmtCtrl*)stmt);
		break;
	case STMT_CONTINUE:
		TranslateContinue((StmtCtrl*)stmt);
		break;
	case STMT_FINFOST:
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}


void IRLowering::GenerateInfo(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)
			message=string("(")+this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_INFO, file, function, line, indentation);
	}
}


void IRLowering::GenerateWarning(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)
			message=string("(")+this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_WARNING, file, function, line, indentation);
	}
}


void IRLowering::GenerateError(string message, char *file, char *function, int line, int requiredDebugLevel, int indentation)
{
	if (m_GlobalSymbols.GetGlobalInfo().Debugging.DebugLevel >= requiredDebugLevel)
	{
		if (m_Protocol!=NULL)
			message=string("(") + this->m_Protocol->Name+string(") ").append(message);
		nbPrintDebugLine((char *)message.c_str(), DBG_TYPE_ERROR, file, function, line, indentation);
	}
}

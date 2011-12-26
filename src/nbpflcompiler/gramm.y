/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


%{


//#define YYDEBUG 1

/*
hack to overcome the  "warning C4273: 'malloc' : inconsistent dll linkage" in visual studio when linking the dll C runtime
*/
#ifdef WIN32
  #define YYMALLOC malloc
  #define YYFREE free
#endif

#include <stdlib.h>
#include "defs.h"
#include "compile.h"
#include "utils.h"
#include "pflexpression.h"
#include "tree.h"
#include "symbols.h"
#include "typecheck.h"



%}

/* create a reentrant parser */
%pure-parser

%parse-param {struct ParserInfo *parserInfo}

%union{
    FieldsList_t    *FieldsList;
    SymbolField		*Field;
    PFLStatement    *stmt;
    PFLAction       *act;
    PFLExpression	*exp;
    uint32			index;
    Node			*IRExp;
    char			id[256];
    uint32			number;	
}



%{

int yylex (YYSTYPE *lvalp);

PFLExpression *GenUnOpNode(PFLOperator op, PFLExpression *kid);
PFLExpression *GenBinOpNode(PFLOperator op, PFLExpression *kid1, PFLExpression *kid2);
PFLExpression *GenTermNode(SymbolProto *protocol, Node *irExpr, uint32 index);

PFLExpression *MergeNotInTermNode(ParserInfo *parserInfo, PFLExpression *node1);
PFLExpression *MergeTermNodes(ParserInfo *parserInfo, uint16 op, PFLExpression *node1, PFLExpression *node2);

PFLExpression *GenProtoBytesRef(ParserInfo *parserInfo, char *protocol, uint32 offs, uint32 len);
PFLExpression *GenNumber(struct ParserInfo *parserInfo, uint32 value);
bool CheckOperandTypes(ParserInfo *parserInfo, PFLExpression *expr);
SymbolField *CheckField(ParserInfo *parserInfo, char * protoName);

%}
%token <id>		PFL_RETURN_PACKET
%token <id>		PFL_EXTRACT_FIELDS
%token <id>		PFL_ON
%token <id>		PFL_PORT
%token <id>		PFL_NUMBER
%token <id>		PFL_HEXNUMBER
%token <id>		PFL_PROTOCOL
%token <id>		PFL_PROTOFIELD
%token <id>		PFL_PROTOCOL_INDEX
%token <id>		PFL_PROTOFIELD_INDEX
%token <id>		PFL_MULTIPROTOFIELD
%token <id>		PFL_PROTOMULTIFIELD
%token <id>		PFL_IPV4ADDR
%token <id>		PFL_IPV6ADDR
%token			PFL_ADD
%token			PFL_SUB
%token			PFL_MUL
%token			PFL_BWAND
%token			PFL_BWOR
%token			PFL_BWXOR
%token			PFL_BWNOT
%token			PFL_SHL
%token			PFL_SHR
%token			PFL_AND
%token			PFL_OR
%token			PFL_NOT
%token			PFL_EQ
%token			PFL_NE
%token			PFL_LT
%token			PFL_LE
%token			PFL_GT
%token	 		PFL_GE

%left	PFL_OR 
%left   PFL_AND
%left   PFL_BWAND PFL_BWOR
%left	PFL_SHL PFL_SHR
%left	PFL_ADD PFL_SUB
%left	PFL_MUL
%nonassoc	PFL_NOT PFL_BWNOT

%type <stmt> Statement
%type <act> Action
%type <act> ExtractFields
%type <act> ReturnPkt
%type <exp> BoolExpression
%type <exp> UnaryExpression
%type <exp> OrExpression
%type <exp> AndExpression
%type <exp> BoolOperand
%type <exp> RelExpression
%type <exp>	Protocol
%type <exp> BitwiseExpression
%type <exp> ShiftExpression
%type <exp> AddExpression
%type <exp> MulExpression
%type <exp> Term
%type <number> GenericNumber
%type <FieldsList> FieldsList
%type <Field> FieldsListItem

%destructor {delete $$;} BoolExpression BoolOperand RelExpression Protocol Term
%destructor {delete $$;} BitwiseExpression ShiftExpression AddExpression MulExpression

%%


Statement: BoolExpression
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement($1, new PFLReturnPktAction(1), NULL);
									}
									$$ = parserInfo->Filter;
								}
								
			|Action								
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement(NULL, (PFLAction*)$1, NULL);
									}
									
									$$ = parserInfo->Filter;
								}
		   |BoolExpression Action	
								{
									if (yynerrs > 0)
									{									
										parserInfo->Filter = NULL;
									}
									else
									{ 
										parserInfo->Filter = new PFLStatement($1,(PFLAction*)$2, NULL);
									}
									
									$$ = parserInfo->Filter;
								}
          	
                        ;				


Action:		ReturnPkt
		   |ExtractFields	
								{
									$$ = $1;
								}
                        ;			

ExtractFields: PFL_EXTRACT_FIELDS '('FieldsList')'
								{
									PFLExtractFldsAction *action= new PFLExtractFldsAction();
									for(FieldsList_t::iterator i = ($3)->begin(); i != ($3)->end(); i++)
										action->AddField((*i));
									$$=action;
								}
						;

FieldsList: FieldsListItem
								{
									FieldsList_t *list = new FieldsList_t();
									list->push_back($1);
									$$ = list;
								}
           |FieldsList ',' FieldsListItem
		   						{
									FieldsList_t *list = $1;
									list->push_back($3);
									$$ = list;
								}
						;
						
		
FieldsListItem: PFL_PROTOFIELD
								{
									SymbolField *field = CheckField(parserInfo,$1);
									if (field == NULL)
									{
										yyerror(parserInfo, "Unknown field in extraction statement");
										YYERROR;
									}
									$$ = field;
								}
				|PFL_MULTIPROTOFIELD
								{
									string field_name($1);
									//we remove the "*" symbol from the field name, so we can check it properly
									string field_no_star = remove_chars(field_name, "*", "");
									SymbolField *field = CheckField(parserInfo, (char*)field_no_star.c_str());
									if (field == NULL)
									{
										yyerror(parserInfo, "Unknown field in extraction statement");
										YYERROR;
									}
									field->MultiProto = true;
									field->MultiFields.push_back(field);
									$$ = field;
																
								}
				|PFL_PROTOMULTIFIELD
								{
									string field_name($1);
									//we remove the "*" symbol from the field name, so we can check it properly
									string field_no_star = remove_chars(field_name, "*", "");
									SymbolField *field = CheckField(parserInfo, (char*)field_no_star.c_str());
									if (field == NULL)
									{
										yyerror(parserInfo, "Unknown field in extraction statement");
										YYERROR;
									}
									field->MultiField = true;
									field->MultiFields.push_back(field);
									field->Protocol->proto_MultiField = true;
									field->Protocol->proto_MultiFields.push_back(field);
									$$ = field;
																
								}
				|PFL_PROTOFIELD_INDEX {
									char *fieldName, *index;
									uint32 num = 0;
									fieldName = strchr($1, '.');
									if(fieldName != NULL)
									{
										*fieldName = '\0';
										fieldName++;
									}

									index = strchr($1, '%');
									if(index != NULL)
									{
										*index = '\0';
										index++;
										str2int(index, &num, 10);
									}
									
									char *field_name($1);
									
									strcat(field_name, (char*)("."));
									strcat(field_name, fieldName);
									
									string field_ok = field_name;
									SymbolField *field = CheckField(parserInfo, (char*)field_ok.c_str());

									if (field == NULL)
									{
										yyerror(parserInfo, "Unknown field in extraction statement");
										YYERROR;
									}
									
									field->HeaderIndex = num;
									field->MultiFields.push_back(field);
									$$ = field;
								}
						;
										

ReturnPkt: PFL_RETURN_PACKET PFL_ON PFL_PORT GenericNumber
									{
										
										PFLReturnPktAction *action= new PFLReturnPktAction($4);
										$$=action;
																			
									}
						;

UnaryExpression:	'(' BoolExpression ')' {$$ = $2}
					|BoolOperand
					|PFL_NOT UnaryExpression
								{
									PFLExpression *child = $2;
									if (child->GetType() == PFL_TERM_EXPRESSION)
									{
										$$ = MergeNotInTermNode(parserInfo, child);
									}
									else
									{
										PFLUnaryExpression *expr = new PFLUnaryExpression(child, UNOP_BOOLNOT);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										
										$$ = expr;
									}
								}	
					;

BoolExpression:		OrExpression
	
					;


OrExpression:		AndExpression
					|OrExpression PFL_OR AndExpression
								{
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ORB, $1, $3);
									if (termNode != NULL)
									{
										$$ = termNode;
									}
									else
									{
										PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLOR);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										$$ = expr;
									}
								}

					;
					
					
AndExpression:		UnaryExpression
					|AndExpression PFL_AND AndExpression
								{
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ANDB, $1, $3);
									if (termNode != NULL)
									{
										$$ = termNode;
									}
									else
									{
										PFLBinaryExpression *expr = new PFLBinaryExpression($1, $3, BINOP_BOOLAND);
										if (expr == NULL)
											throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
										$$ = expr;
									}
								}
					;
				
			
BoolOperand:		Protocol
					|RelExpression
					
					;


RelExpression:		BitwiseExpression PFL_EQ BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_EQI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_NE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_NEI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_GT BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_GTI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_GE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_GEI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_LT BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_LTI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_LE BitwiseExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_LEI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
								
					;


Protocol:			PFL_PROTOCOL 
								{
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
									SymbolProto *protoSymbol = globalSymbols.LookUpProto($1);
									
									if (protoSymbol == NULL)
									{
										yyerror(parserInfo, "Invalid PROTOCOL identifier");
										YYERROR;
									}
									
								
									PFLTermExpression *expr = new PFLTermExpression(protoSymbol);
									if (expr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
									
									$$ = expr;
								}
								
					|PFL_PROTOCOL_INDEX
								{
									char *index;
									uint32 num = 0;
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
									
									index = strchr($1, '%');
									if (index != NULL)
									{
										*index = '\0';
										index++;
										str2int(index, &num, 10);
									}
									
									SymbolProto *protoSymbol = globalSymbols.LookUpProto($1);
									
									if (protoSymbol == NULL)
									{
										yyerror(parserInfo, "Invalid PROTOCOL identifier");
										YYERROR;
									}
									
								
									PFLTermExpression *expr = new PFLTermExpression(protoSymbol);
									if (expr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
									
									//protoSymbol->HeaderIndex = num;
									
									$$ = expr;
								}
					;

BitwiseExpression:	ShiftExpression
					|BitwiseExpression PFL_BWAND ShiftExpression	
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ANDI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_BWOR ShiftExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ORI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|BitwiseExpression PFL_BWXOR ShiftExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_XORI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|PFL_BWNOT ShiftExpression
								{
									PFLTermExpression *termNode = (PFLTermExpression*)$2;
									IRCodeGen &codeGen = parserInfo->CodeGen;
									termNode->SetIRExpr(codeGen.UnOp(IR_NOTI, termNode->GetIRExpr()));
									$$ = termNode;
								}
					|'('BitwiseExpression')'
								{
									$$ = $2;
								}
					
					;

ShiftExpression:	AddExpression
					|ShiftExpression PFL_SHL AddExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SHLI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|ShiftExpression PFL_SHR AddExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SHRI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}

					;
					
AddExpression:		MulExpression
					|AddExpression PFL_ADD MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_ADDI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
					|AddExpression PFL_SUB MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_SUBI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}

					;

MulExpression:		Term
					|MulExpression PFL_MUL MulExpression
								{
									
									PFLExpression *termNode = MergeTermNodes(parserInfo, IR_MULI, $1, $3);
									if (termNode == NULL)
									{
										yyerror(parserInfo, "Operations between fields of different protocols are not supported");
										YYERROR;
									}
									if (!CheckOperandTypes(parserInfo, termNode))
									{
										yyerror(parserInfo, "type mismatch in expression");
										YYERROR;
									}
									$$ = termNode;
								}
						;
					
Term:				GenericNumber	
								{									
									$$ = GenNumber(parserInfo, $1);
								}
								
								
					|PFL_IPV4ADDR	{
									uint32 add = 0;
									if (IPaddr2int($1, &add) != 0)
									{
										yyerror(parserInfo, "IP address not valid!");
										YYERROR;
									}
									$$ = GenNumber(parserInfo, add);
								}
								
					|PFL_PROTOFIELD {
									char *protoName = $1, *fieldName;
									GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
									SymbolField *fieldSym(0);
									fieldName = strchr($1, '.');

									if (fieldName != NULL)
									{
										*fieldName = '\0';
										fieldName++;
									}

									SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
									
									if (protoSymbol == NULL)
									{
										yyerror(parserInfo, "Invalid PROTOCOL identifier");
										YYERROR;
									}
									
									fieldSym = globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
									
									if (fieldSym == NULL)
									{
										yyerror(parserInfo, "Field not valid for the specified protocol");
										YYERROR;
									}	
									
									switch(fieldSym->FieldType)
									{
										case PDL_FIELD_FIXED:
										{
											SymbolFieldFixed *fixedField = (SymbolFieldFixed*)fieldSym;
											if ((fixedField->Size != 1) && (fixedField->Size != 2) && (fixedField->Size != 3) && (fixedField->Size != 4))
											{
												yyerror(parserInfo, "Only integer fields are currently supported");
												YYERROR;
											}
										};break;
										case PDL_FIELD_BITFIELD:
											break;
										default:
											yyerror(parserInfo, "Only fixed length fields and bitfields are currently supported");
											YYERROR;
									}
									IRCodeGen &codeGen = parserInfo->CodeGen;
									Node *irExpr = codeGen.TermNode(IR_FIELD, fieldSym);
									if (irExpr == NULL)
										throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");									
									$$ = GenTermNode(protoSymbol, irExpr, NULL);
								}
								
/*								
					|PFL_IPV6ADDR
*/
					|PFL_PROTOCOL'['GenericNumber']'
								{
									PFLExpression *expr = GenProtoBytesRef(parserInfo, $1, $3, 1);
									if (expr == NULL)
										YYERROR;
									$$ = expr;
								
								}
					|PFL_PROTOCOL'['GenericNumber':'GenericNumber']' 
								{
									PFLExpression *expr = GenProtoBytesRef(parserInfo, $1, $3, $5);
									if (expr == NULL)
										YYERROR;
									$$ = expr;
								
								}
						
					;
					
					
GenericNumber:		PFL_NUMBER
								{
									uint32 num = 0;
									if (str2int($1, &num, 10) != 0)
									{
										yyerror(parserInfo, "Decimal number out of range");
										YYERROR;
									}
									
									$$ = num;
								}
									
					|PFL_HEXNUMBER		
								{
									uint32 num = 0;
									if (str2int($1, &num, 16) != 0)
									{
										yyerror(parserInfo, "Hexadecimal number out of range");
										YYERROR;
									}
									
									$$ = num;
								}	
					
%%


SymbolField* CheckField(ParserInfo *parserInfo, char * protoName)
{
	char *fieldName;
	GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;									
	//SymbolField *fieldSym(0);
	fieldName = strchr(protoName, '.');

	if (fieldName != NULL)
	{
		*fieldName = '\0';
		fieldName++;
	}

	SymbolProto *protoSymbol = globalSymbols.LookUpProto(protoName);
	
	if (protoSymbol == NULL)
	{
		return NULL;
	}

	if(strcmp(fieldName,"allfields")==0)
   	{	
   		SymbolField *sym = new SymbolField(fieldName,PDL_FIELD_ALLFIELDS,NULL);
       	sym->Protocol=protoSymbol;
		return sym;
   	}
    else
    {
		return globalSymbols.LookUpProtoFieldByName(protoName, fieldName);
	}
	
}

PFLExpression *GenUnOpNode(PFLOperator op, PFLExpression *kid)
{
	PFLUnaryExpression *expr = new PFLUnaryExpression(kid, op);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}


PFLExpression *GenBinOpNode(PFLOperator op, PFLExpression *kid1, PFLExpression *kid2)
{
	PFLBinaryExpression *expr = new PFLBinaryExpression(kid1, kid2, op);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}

PFLExpression *GenTermNode(SymbolProto *protocol, Node *irExpr, uint32 index)
{
	PFLTermExpression *expr = new PFLTermExpression(protocol, irExpr, index);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}

PFLExpression *MergeNotInTermNode(ParserInfo *parserInfo, PFLExpression *node1)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node1 != NULL, "node1 cannot be NULL");
	nbASSERT(node1->GetType() == PFL_TERM_EXPRESSION, "node1 should be a terminal node");
	PFLTermExpression *kid1 = (PFLTermExpression*)node1;

	// save the IR trees of the terminal node
	Node *irExpr1 = kid1->GetIRExpr();
	
	if (irExpr1 != NULL)
	{
		IRCodeGen &codeGen = parserInfo->CodeGen;
		kid1->SetIRExpr(codeGen.UnOp(IR_NOTB, irExpr1));
		return kid1;
	}
	
	PFLUnaryExpression *expr = new PFLUnaryExpression(kid1, UNOP_BOOLNOT);
	if (expr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");
	return expr;
}


Node *ConvToInt(ParserInfo *parserInfo, Node *node)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node != NULL, "Node cannot be NULL");
	if (CheckIntOperand(node))
	{
		switch(node->Op)
		{
			case IR_ICONST:
			case IR_IVAR:
				return node;
				break;
			case IR_SVAR:
			case IR_FIELD:
			{
				IRCodeGen &codeGen = parserInfo->CodeGen;
				node->NRefs--; //otherwise it will appear as a dag and this is not the case
				return codeGen.UnOp(IR_CINT, node);
			}break;
			default:
				return node;
				break;
		}
	}
	return NULL;
}

bool CheckOperandTypes(ParserInfo *parserInfo, PFLExpression *expr)
{
	nbASSERT(expr->GetType() == PFL_TERM_EXPRESSION, "expression should be a terminal node");
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(expr != NULL, "expr cannot be NULL");
	nbASSERT(expr->GetType() == PFL_TERM_EXPRESSION, "expr should be a terminal node");
	Node *node = ((PFLTermExpression*)expr)->GetIRExpr();
	nbASSERT(node != NULL, "Contained Node cannot be NULL");
	if (GET_OP_TYPE(node->Op) == IR_TYPE_INT)
	{
		Node *leftChild = node->GetLeftChild();
		Node *rightChild = node->GetRightChild();
		if (leftChild)
		{
			leftChild = ConvToInt(parserInfo, leftChild);
			if(leftChild == NULL)
			{
				return false;
			} 
			node->SetLeftChild(leftChild);
		}
		if (rightChild)
		{		
			rightChild = ConvToInt(parserInfo, rightChild);
			if(rightChild == NULL)
			{
				return false;
			} 
			node->SetRightChild(rightChild);
		}
			
		return true;
	}
	nbASSERT(false, "SHOULD NOT BE HERE");
	return true;
} 

PFLExpression *MergeTermNodes(ParserInfo *parserInfo, uint16 op, PFLExpression *node1, PFLExpression *node2)
{
	nbASSERT(parserInfo != NULL, "parserInfo cannot be NULL");
	nbASSERT(node1 != NULL, "node1 cannot be NULL");
	nbASSERT(node2 != NULL, "node2 cannot be NULL");
	//node1 should be a terminal node
	if(node1->GetType() != PFL_TERM_EXPRESSION)
		return NULL;
	//node2 should be a terminal node
	if(node2->GetType() != PFL_TERM_EXPRESSION)	
		return NULL;
	nbASSERT(node2->GetType() == PFL_TERM_EXPRESSION, "node2 should be a terminal node");
	PFLTermExpression *kid1 = (PFLTermExpression*)node1;
	PFLTermExpression *kid2 = (PFLTermExpression*)node2;

	// save protocol information about the two terminal nodes
	SymbolProto *proto1 = kid1->GetProtocol();
	SymbolProto *proto2 = kid2->GetProtocol();
	
	// save the IR trees of the two terminal nodes
	Node *irExpr1 = kid1->GetIRExpr();
	Node *irExpr2 = kid2->GetIRExpr();
	
	if (proto1 != proto2)
	{
		// the two terminal nodes refer to different protocols
		if (proto2 == NULL)
		{
			nbASSERT(irExpr2 != NULL, "irExpr2 cannot be NULL");
			// if the second has a NULL protocol (i.e. it contains a constant expression) we delete it
			kid2->SetIRExpr(NULL);
			delete kid2;
		}
		else if (proto1 == NULL)
		{
			nbASSERT(irExpr1 != NULL, "irExpr1 cannot be NULL");
			// if the first has a NULL protocol (i.e. it contains a constant expression) we copy the protocol 
			// information from the second, then we delete the second
			kid1->SetProtocol(proto2);
			kid2->SetIRExpr(NULL);
			delete kid2;
		}
		else
		{
			//The two terminal nodes cannot be merged in a single PFLTermExpression node
			return NULL;
		}
	}
	else
	{
			// the two terminal nodes refer to the same protocol
			// so we simply delete the second
			kid2->SetIRExpr(NULL);
			delete kid2;
	}
	
	//handle the cases such as (ip and (ip.src == 10.0.0.1)) where one operand is redundant
	
	if (irExpr1 == NULL)
	{
		kid1->SetIRExpr(irExpr2);
		return kid1;
	}
	if (irExpr2 == NULL)
	{
		kid1->SetIRExpr(irExpr1);
		return kid1;
	}
	
	// we create a new Node as an op between the two subexpressions
	// and we append it to the first terminal node

	IRCodeGen &codeGen = parserInfo->CodeGen;
	kid1->SetIRExpr(codeGen.BinOp(op, irExpr1, irExpr2));
	return kid1;
}

PFLExpression *GenNumber(struct ParserInfo *parserInfo, uint32 value)
{
	IRCodeGen &codeGen = parserInfo->CodeGen;
	Node *irExpr = codeGen.TermNode(IR_ICONST, codeGen.ConstIntSymbol(value));
	return GenTermNode(NULL, irExpr, NULL);
}


PFLExpression *GenProtoBytesRef(ParserInfo *parserInfo, char *protocol, uint32 offs, uint32 len)
{
	GlobalSymbols &globalSymbols = *parserInfo->GlobalSyms;
	IRCodeGen &codeGen = parserInfo->CodeGen;
	SymbolProto *protoSymbol = globalSymbols.LookUpProto(protocol);

	if (protoSymbol == NULL)
	{
		yyerror(parserInfo, "Invalid PROTOCOL identifier");
		return NULL;
	}

	Node *irExpr = codeGen.ProtoBytesRef(protoSymbol, offs, len);

	if (irExpr == NULL)
		throw ErrorInfo(ERR_FATAL_ERROR, "MEMORY ALLOCATION FAILURE");

	return GenTermNode(protoSymbol, irExpr, NULL);
}

int yyerror(struct ParserInfo *parserInfo, const char *s)
{
	parserInfo->ErrRecorder->PFLError(s);
	//fprintf(stderr, "%s\n", s);
	return 1;
}


void compile(ParserInfo *parserInfo, const char *filter, int debug)
{

#ifdef YYDEBUG
#if YYDEBUG != 0
	pfl_debug = debug;
#endif
#endif
	parserInfo->ResetFilter();
	pflcompiler_lex_init(filter);
	pfl_parse(parserInfo);
	pflcompiler_lex_cleanup();

} 

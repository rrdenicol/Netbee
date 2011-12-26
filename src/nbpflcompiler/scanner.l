/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


%{

#include "defs.h"
#include "pflexpression.h"
#include "parser.hpp"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern int pfl_error(const char *s);


#define YY_DECL int yylex (YYSTYPE *lvalp)
//#define YY_NO_UNPUT
static YY_BUFFER_STATE src_buffer = 0;




%}

%option noyywrap
%option nounput
%option never-interactive

ID			[a-z][_a-z0-9]*
CHAR		[^\n]
DECNUMBER	[0-9]+
HEXNUMBER	0x[0-9a-fA-F]+
ADDRv4		[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+
HINDEX		[1-9][0-9]*
and			"&&"
or			"||"
not			"!"
plus		"+"
minus		"-"
mult		"*"
bwand		"&"
bwor		"|"
bwxor		"^"
bwnot		"~"
eq			"=="
ne			"!="
gt			">"
lt			"<"
ge			">="
le			"<="
shl			"<<"
shr			">>"


%%

{DECNUMBER}		{strncpy(lvalp->id, yytext, 255);return PFL_NUMBER;};
				
{HEXNUMBER}		{strncpy(lvalp->id, yytext, 255);return PFL_HEXNUMBER;};

"extractfields" {return PFL_EXTRACT_FIELDS;};
"returnpacket"	{return PFL_RETURN_PACKET;};
"retpkt"		{return PFL_RETURN_PACKET;};
"port"			{return PFL_PORT;};	
"p"				{return PFL_PORT;}
"on"			{return PFL_ON;};	
"and"			{return PFL_AND;};
"or"			{return PFL_OR;};
"not"			{return PFL_NOT;};	
{ID}			{strncpy(lvalp->id, yytext, 255); return PFL_PROTOCOL;};
{ID}\.{ID}		{strncpy(lvalp->id, yytext, 255); return PFL_PROTOFIELD;};
{ID}\%{HINDEX}	{strncpy(lvalp->id, yytext, 255); return PFL_PROTOCOL_INDEX;};
{ID}\%{HINDEX}\.{ID}	{strncpy(lvalp->id, yytext, 255); return PFL_PROTOFIELD_INDEX;};

{ID}\*\.{ID}    {strncpy(lvalp->id, yytext, 255); return PFL_MULTIPROTOFIELD;};
{ID}\.{ID}\*	{strncpy(lvalp->id, yytext, 255); return PFL_PROTOMULTIFIELD;};

{ADDRv4}		{strncpy(lvalp->id, yytext, 255); return PFL_IPV4ADDR;};
{plus}			{return PFL_ADD;};
{minus}			{return PFL_SUB;};
{mult}			{return PFL_MUL;};
{bwand}			{return PFL_BWAND;};
{bwor}			{return PFL_BWOR;};
{bwxor}			{return PFL_BWXOR;};
{bwnot}			{return PFL_BWNOT;};
{shl}			{return PFL_SHL;}
{shr}			{return PFL_SHR;}
{and}			{return PFL_AND;};
{or}			{return PFL_OR;};
{not}			{return PFL_NOT;};
{eq}			{return PFL_EQ;};
{ne}			{return PFL_NE;};
{gt}			{return PFL_GT;};
{lt}			{return PFL_LT;};
{ge}			{return PFL_GE;};
{le}			{return PFL_LE;};
\:				{return ':';};
\[				{return '[';};
\]				{return ']';};
\(				{return '(';};
\)				{return ')';};
\n				{return '\n';};
\,				{return ',';};
" "|\t			;
.				printf("unrecognized character: %s", yytext);

%%


void pflcompiler_lex_init(const char *buf)
{
	src_buffer = yy_scan_string(buf);
}


void pflcompiler_lex_cleanup()
{
	if (src_buffer != NULL)
		yy_delete_buffer(src_buffer);
	src_buffer = NULL;
}

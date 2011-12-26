/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


#pragma once


#include "netpflfrontend.h"
#include "pflexpression.h"
#include "globalsymbols.h"
#include "errors.h"

struct ParserInfo
{
	PFLStatement	*Filter;
	GlobalSymbols   *GlobalSyms;
	ErrorRecorder	*ErrRecorder;
	EncapGraph	*ProtoGraph;
	IRCodeGen		CodeGen;

	ParserInfo(GlobalSymbols &globalSyms, ErrorRecorder &errRecorder)
		:Filter(0),  GlobalSyms(&globalSyms), ErrRecorder(&errRecorder), CodeGen(globalSyms){}


	void ResetFilter(void)
	{
		Filter = NULL;
	}

};

extern int pfl_parse(struct ParserInfo *parserInfo);
void pflcompiler_lex_init(const char *buf);
void pflcompiler_lex_cleanup();
int pfl_error(struct ParserInfo *parserInfo, const char *s);

extern void compile(ParserInfo *parserInfo, const char *filter, int debug);


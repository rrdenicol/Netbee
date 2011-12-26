/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/




#include "compunit.h"
#include "dump.h"
#include "pfl_trace_builder.h"


void CompilationUnit::GenerateNetIL(ostream &stream)
{
	stream << "segment .ports" << endl;
	stream << "  push_input in" << endl;
	stream << "  push_output out" << OutPort << endl;
	stream << "ends" << endl;
	stream << endl << endl;

	if (this->UsingCoproLookupEx || this->UsingCoproRegEx)
	{
		// declare lookup coprocessor
		stream << "segment .metadata" << endl;
		if (this->UsingCoproLookupEx)
			stream << "  use_coprocessor lookup_ex" << endl;
		if (this->UsingCoproRegEx)
			stream << "  use_coprocessor regexp" << endl;
		stream << "ends" << endl;
		stream << endl << endl;

		if (this->UsingCoproRegEx && this->DataItems->size()>0)
		{
			// declare data segment for regex
			stream << "segment .data" << endl;
			stream << "  .byte_order little_endian" << endl;
			//CodeWriter cw(stream);
			//cw.DumpNetIL(&DataSegmentCode);
			for (DataItemList_t::iterator i=this->DataItems->begin(); i!=this->DataItems->end(); i++)
			{
				SymbolDataItem *data=(SymbolDataItem *)(*i);
				char type[3];
				if (data->Type==DATA_TYPE_BYTE)
					strncpy(type, "db", 3);
				else
					strncpy(type, "dw", 3);

				stream << "  " << data->Name << "  " << type << "  " << data->Value << endl;
			}
			stream << "ends" << endl;
			stream << endl << endl;
		}
	}

	stream << "segment .init" << endl;
	if (this->UsingCoproRegEx)
		stream << "  .maxstacksize 10240" << endl;
	else
		stream << "  .maxstacksize 5" << endl;

	//NetILTraceBuilder traceBuilder(stream);
	//traceBuilder.CreateTrace(InitCfg);
	//TODO!!!!!
	PFLTraceBuilder ptb(InitCfg);
	ptb.build_trace();
	for(PFLTraceBuilder::trace_iterator_t i = ptb.begin(); i != ptb.end(); i++)
	{
		CodeWriter cw(stream);
		cw.DumpNetIL((*i)->getMIRNodeCode());
	}
	stream << "  ret" << endl;
	stream << "ends" << endl;
	stream << endl << endl;

	stream << "segment .pull" << endl;
	stream << "  ret" << endl;
	stream << "ends" << endl;
	stream << endl << endl;

	if (PFLSource.size() > 0)
	{
		stream << "; Code for filter: \"" << PFLSource << "\"" << endl << endl;
		stream << "segment .push" << endl;
		stream << ".maxstacksize " << 30 << endl;
		stream << ".locals " << NumLocals << endl;
		stream << "  pop  ; discard the \"calling\" port id" << endl << endl;

		if (this->UsingCoproRegEx)
		{
			stream << "  copro.exbuf  regexp, 0" << endl << endl;
		}

		//NetILTraceBuilder traceBuilder(stream);
		//traceBuilder.CreateTrace(Cfg);
		//TODO!!!!!
		PFLTraceBuilder ptb(Cfg);
		ptb.build_trace();
		for(PFLTraceBuilder::trace_iterator_t i = ptb.begin(); i != ptb.end(); i++)
		{
			CodeWriter cw(stream);
			cw.DumpNetIL((*i)->getMIRNodeCode());
		}

		stream << "ends" << endl;
	}
	else
	{
		stream << "; Code for the empty filter (accept all):"<< endl << endl;
		stream << "segment .push" << endl;
		stream << ".maxstacksize " << 30 << endl;
		stream << "  pop  ; discard the \"calling\" port id" << endl;
		stream << "  pkt.send out" << OutPort << endl;
		stream << "  ret" << endl;
		stream << "ends" << endl;
	}
	stream << endl << endl;
}

CompilationUnit::~CompilationUnit()
{
}



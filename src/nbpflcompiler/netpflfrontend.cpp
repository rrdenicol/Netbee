/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



#include "netpflfrontend.h"
#include "statements.h"
#include "ircodegen.h"
#include "compile.h"
#include "dump.h"
#include "compilerconsts.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "cfgwriter.h"
#include "../nbee/globals/debug.h"
#include "protographwriter.h"
#include "filtersubgraph.h"
#include "protographshaper.h"
#include "cfg_edge_splitter.h"
#include "cfgdom.h"
#include "cfg_ssa.h"
#include "deadcode_elimination_2.h"
#include "constant_propagation.h"
#include "copy_propagation.h"
#include "constant_folding.h"
#include "controlflow_simplification.h"
#include "redistribution.h"
#include "reassociation.h"
#include "irnoderegrenamer.h"
#include "../nbnetvm/jit/copy_folding.h"
#include "register_mapping.h"
#include "reassociation_fixer.h"
#include "optimizer_statistics.h"
#include "nbpflcompiler.h"


#ifdef ENABLE_PFLFRONTEND_PROFILING
#include <nbee_profiler.h>
#endif


using namespace std;


void NetPFLFrontEnd::SetupCompilerConsts(CompilerConsts &compilerConsts, uint16 linkLayer)
{
	compilerConsts.AddConst("$linklayer", linkLayer);
}

//TODO OM: maybe it would be better to keep a smaller constructor and to provide some "Set" methods for
// setting all those filenames. Moreover, the unsigned char flags (like dumpHIRCode) are useless if we check
// the presence of the corresponding filename

NetPFLFrontEnd::NetPFLFrontEnd(_nbNetPDLDatabase &protoDB, nbNetPDLLinkLayer_t LinkLayer,
										 const unsigned int DebugLevel,
										 const char *dumpHIRCodeFilename,
										 const char *dumpLIRNoOptGraphFilename,
										 const char *dumpProtoGraphFilename)
										 :m_ProtoDB(&protoDB), m_GlobalInfo(protoDB), m_GlobalSymbols(m_GlobalInfo),
										  m_CompUnit(0), m_LIRCodeGen(0), m_NetVMIRGen(0),  m_StartProtoGenerated(0),
									      NetPDLParser(protoDB, m_GlobalSymbols, m_ErrorRecorder),
									      HIR_FilterCode(0), NetVMIR_FilterCode(0), PFL_Tree(0),

										 NetIL_FilterCode("")
#ifdef OPTIMIZE_SIZED_LOOPS
										 , m_ReferredFieldsInFilter(0), m_ReferredFieldsInCode(0), m_ContinueParsingCode(false)
#endif
{
	m_GlobalInfo.Debugging.DebugLevel= DebugLevel;

	if (dumpHIRCodeFilename!=NULL)
		m_GlobalInfo.Debugging.DumpHIRCodeFilename=(char *)dumpHIRCodeFilename;
	if (dumpLIRNoOptGraphFilename!=NULL)
		m_GlobalInfo.Debugging.DumpLIRNoOptGraphFilename=(char *)dumpLIRNoOptGraphFilename;
	if (dumpProtoGraphFilename!=NULL)
		m_GlobalInfo.Debugging.DumpProtoGraphFilename=(char *)dumpProtoGraphFilename;

	if (m_GlobalInfo.Debugging.DebugLevel > 0)
		nbPrintDebugLine("Initializing NetPFL Front End...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	if (!m_GlobalInfo.IsInitialized())
		throw ErrorInfo(ERR_FATAL_ERROR, "startproto protocol not found!!!");

#ifdef ENABLE_PFLFRONTEND_PROFILING
	uint64_t TicksBefore, TicksAfter, TicksTotal, MeasureCost;

	TicksTotal= 0;
	MeasureCost= nbProfilerGetMeasureCost();
#endif

	CompilerConsts &compilerConsts = m_GlobalInfo.GetCompilerConsts();
	SetupCompilerConsts(compilerConsts, LinkLayer);

	ofstream EncapCode;

	if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
		EncapCode.open(m_GlobalInfo.Debugging.DumpHIRCodeFilename);

	CodeWriter cw(EncapCode);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksBefore= nbProfilerGetTime();
#endif

	IRCodeGen ircodegen(m_GlobalSymbols, &(m_GlobalSymbols.GetProtoCode(m_GlobalInfo.GetStartProtoSym())));
	NetPDLParser.ParseStartProto(ircodegen, PARSER_VISIT_ENCAP);
	SymbolProto **protoList = m_GlobalInfo.GetProtoList();

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	TicksTotal= TicksTotal + (TicksAfter - TicksBefore - MeasureCost);

	protoList[0]->ParsingTime= TicksAfter - TicksBefore - MeasureCost;
#endif


	//create the graph
	for (uint32 i = 1; i < m_GlobalInfo.GetNumProto(); i++)
	{
#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksBefore= nbProfilerGetTime();
#endif

		IRCodeGen protocodegen(m_GlobalSymbols, &(m_GlobalSymbols.GetProtoCode(protoList[i])));
		NetPDLParser.ParseProtocol(*protoList[i], protocodegen, PARSER_VISIT_ENCAP);

#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksAfter= nbProfilerGetTime();
		TicksTotal= TicksTotal + (TicksAfter - TicksBefore - MeasureCost);

		protoList[i]->ParsingTime= TicksAfter - TicksBefore - MeasureCost;
#endif

		//if (!protoList[i]->IsSupported)
		//	if (m_GlobalInfo.Debugging.FlowInformation > 0)
		//		cerr << ";Protocol " << protoList[i]->Name << " is not supported" << endl;
		//if (!protoList[i]->IsDefined)
		//	cerr << ";Protocol " << protoList[i]->Name << " is not defined" << endl;

		if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
		{
			cw.DumpCode(&m_GlobalSymbols.GetProtoCode(protoList[i]));
		}
	}


#ifdef STATISTICS
	ofstream fieldStats;
		fieldStats.open("fieldstats.txt");
		m_GlobalSymbols.DumpProtoFieldStats(fieldStats);
	printf("\nTotal protocols number: %u", m_GlobalInfo.GetNumProto());

	uint32 notSupportedProto=0;
	FILE *f=fopen("statistics.csv", "w");
	if (f!=NULL)
	{
		fprintf(f, "Protocol name;Supported;Encapsulable;Parsing time (ticks);" \
			"ParsedFields;Inner fields;Field unsupported;Ref field unknown;" \
			"Expression unsupported;Var declared;Var unsupported;Var occurrences;Var tot occurrences;" \
			"Encap declared;Encap tentative;Encap successful;" \
			"Lookup declared;Lookup occurrences" \
			"\n");
		for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
		{
			fprintf(f, "%s", protoList[i]->Name.c_str());

			if (protoList[i]->IsDefined==false || protoList[i]->IsSupported==false)
				fprintf(f, ";no");
			else
				fprintf(f, ";yes");

			if (protoList[i]->VerifySectionSupported==false || protoList[i]->BeforeSectionSupported==false)
				fprintf(f, ";no");
			else
				fprintf(f, ";yes");

			fprintf(f, ";-");
			fprintf(f, ";%u", protoList[i]->DeclaredFields);
			fprintf(f, ";%u", protoList[i]->InnerFields);
			fprintf(f, ";%u", protoList[i]->UnsupportedFields);
			fprintf(f, ";%u", protoList[i]->UnknownRefFields);
			fprintf(f, ";%u", protoList[i]->UnsupportedExpr);
			fprintf(f, ";%u", protoList[i]->VarDeclared);
			fprintf(f, ";%u", protoList[i]->VarUnsupported);
			fprintf(f, ";%u", protoList[i]->VarOccurrences);
			fprintf(f, ";%u", protoList[i]->VarTotOccurrences);
			fprintf(f, ";%u", protoList[i]->EncapDecl);
			fprintf(f, ";%u", protoList[i]->EncapTent);
			fprintf(f, ";%u", protoList[i]->EncapSucc);
			fprintf(f, ";%u", protoList[i]->LookupDecl);
			fprintf(f, ";%u", protoList[i]->LookupOccurrences);
			fprintf(f, "\n");
			if (protoList[i]->IsDefined==false || protoList[i]->IsSupported==false)
				notSupportedProto++;
		}
	}

	printf("\n\tTotal unsupported protocols number: %u\n", notSupportedProto);
#endif

#ifdef ENABLE_PFLFRONTEND_PROFILING
	printf("\n\n\tProtocol name  Parsing time (ticks)\n");
	for (uint32 i = 0; i < m_GlobalInfo.GetNumProto(); i++)
	{
		printf("\t%s  %ld\n", protoList[i]->Name.c_str(), protoList[i]->ParsingTime);
	}
	printf("\n\tTotal protocol parsing time %ld (avg per protocol %ld)\n\n", TicksTotal, TicksTotal/m_GlobalInfo.GetNumProto());
#endif

#ifndef ENABLE_PFLFRONTEND_PROFILING
	//ProtoGraphLayers protoLayers(m_GlobalInfo.GetProtoGraph(), *m_GlobalInfo.GetStartProtoSym()->GraphNode);
	//protoLayers.FindProtoLevels();
	if (m_GlobalInfo.Debugging.DumpHIRCodeFilename)
		EncapCode.close();

	if (m_GlobalInfo.Debugging.DumpProtoGraphFilename)
	{
		ofstream protograph(m_GlobalInfo.Debugging.DumpProtoGraphFilename);
		m_GlobalInfo.DumpProtoGraph(protograph);
		protograph.close();
	}
#endif

	if (m_GlobalInfo.Debugging.DebugLevel > 0)
		nbPrintDebugLine("Initialized NetPFL Front End", DBG_TYPE_INFO, __FILE__,
		__FUNCTION__, __LINE__, 1);

}


NetPFLFrontEnd::~NetPFLFrontEnd()
{
	if (m_CompUnit != NULL)
	{
		delete m_CompUnit;
		m_CompUnit = NULL;
	}
}




void NetPFLFrontEnd::DumpPFLTreeDotFormat(PFLExpression *expr, ostream &stream)
{
	stream << "Digraph PFL_Tree{" << endl;
	expr->PrintMeDotFormat(stream);
	stream << endl << "}" << endl;
}


bool NetPFLFrontEnd::CheckFilter(string filter)
{
	m_ErrorRecorder.Clear();

	ParserInfo parserInfo(m_GlobalSymbols, m_ErrorRecorder);
	compile(&parserInfo, filter.c_str(), 0);

	if (parserInfo.Filter == NULL)
		return false;
	else
		return true;
}


PFLStatement *NetPFLFrontEnd::ParseFilter(string filter)
{
	
	if (filter.size() == 0)
	{
		PFLReturnPktAction *retpkt = new PFLReturnPktAction(1);
		CHECK_MEM_ALLOC(retpkt);
		PFLStatement *stmt = new PFLStatement(NULL, retpkt, NULL);
		CHECK_MEM_ALLOC(stmt);
		return stmt;
	}
	ParserInfo parserInfo(m_GlobalSymbols, m_ErrorRecorder);
	compile(&parserInfo, filter.c_str(), 0);
	return parserInfo.Filter;
}

bool NetPFLFrontEnd::CompileFilter(string filter, bool optimizationCycles)
{
	m_ErrorRecorder.Clear();
	if (m_CompUnit != NULL)
	{
		delete m_CompUnit;
		m_CompUnit = NULL;
	}

	PFLStatement *filterStmt = ParseFilter(filter);
	if (filterStmt == NULL)
		return false;
	m_CompUnit = new CompilationUnit(filter);
	CHECK_MEM_ALLOC(m_CompUnit);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	int64_t TicksBefore, TicksAfter, TicksTotal, MeasureCost;

	TicksTotal= 0;
	MeasureCost= nbProfilerGetMeasureCost();
#endif

	if (filter.size() > 0) //i.e. if the filter is not empty
	{
#ifdef ENABLE_PFLFRONTEND_PROFILING
		TicksBefore= nbProfilerGetTime();
#endif

		IRCodeGen lirCodeGen(m_GlobalSymbols, &m_CompUnit->IRCode);
		m_LIRCodeGen = &lirCodeGen;
		m_CompUnit->OutPort = 1;
		if(filterStmt->GetAction()!=NULL)
			VisitAction(filterStmt->GetAction());
		
		try
		{
			GenCode(filterStmt);
		}
		catch(ErrorInfo &e)
		{
			if (e.Type != ERR_FATAL_ERROR)
			{
				m_ErrorRecorder.PDLError(e.Msg);
				return false;
			}
			else
			{
				throw e;
			}
		}

		delete filterStmt;

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	printf("\n\tGenCode() used %ld ticks", TicksAfter - TicksBefore - MeasureCost);
	TicksBefore= nbProfilerGetTime();
#endif

#ifdef ENABLE_PFLFRONTEND_DBGFILES
                {
		// FULVIO No really idea what we're dumping here
		// Is it similar to what we dump on lir_code.asm?
		ofstream ir("lir_code2.asm");
		CodeWriter cw(ir);
		cw.DumpCode(&m_CompUnit->IRCode);
		ir.close();
                }
#endif
		CFGBuilder builder(m_CompUnit->Cfg);
		builder.Build(m_CompUnit->IRCode);

#ifdef ENABLE_PFLFRONTEND_DBGFILES
		{
			ofstream irCFG("cfg_ir_no_opt.dot");
			DumpCFG(irCFG, false, false);
			irCFG.close();
		}
#endif


		GenRegexEntries();


		// generate initialization code
		IRCodeGen lirInitCodeGen(m_GlobalSymbols, &m_CompUnit->InitIRCode);
		m_LIRCodeGen = &lirInitCodeGen;
		try
		{
			int locals = m_CompUnit->NumLocals;
			GenInitCode();
			m_CompUnit->NumLocals = locals;
		}
		catch(ErrorInfo &e)
		{
			if (e.Type != ERR_FATAL_ERROR)
			{
				m_ErrorRecorder.PDLError(e.Msg);
				return false;
			}
			else
			{
				throw e;
			}
		}

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	printf("\n\tIR preparation %ld ticks", TicksAfter - TicksBefore - MeasureCost);
	TicksBefore= nbProfilerGetTime();
#endif

		CFGBuilder initBuilder(m_CompUnit->InitCfg);
		initBuilder.Build(m_CompUnit->InitIRCode);

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	printf("\n\tActual IR generation %ld ticks", TicksAfter - TicksBefore - MeasureCost);
	TicksBefore= nbProfilerGetTime();
#endif


		//*******************************************************************
		//*******************************************************************
		//		HERE IT BEGINS												*
		//*******************************************************************

#ifdef _DEBUG_OPTIMIZER
		std::cout << "Running setCFG" << std::endl;
#endif
		PFLMIRNode::setCFG(&m_CompUnit->Cfg);

		if (optimizationCycles)
		{
#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running NodeRenamer" << std::endl;
#endif
			IRNodeRegRenamer inrr(m_CompUnit->Cfg);
			inrr.run();

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running edgeSPlitter" << std::endl;
#endif
			CFGEdgeSplitter<PFLMIRNode> cfges(m_CompUnit->Cfg);
			cfges.run();

#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_edge_splitting.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}
#endif

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running ComputeDominance" << std::endl;
#endif
			jit::ComputeDominance<PFLCFG> cd(m_CompUnit->Cfg);
			cd.run();

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running SSA" << std::endl;
#endif
			jit::SSA<PFLCFG> ssa(m_CompUnit->Cfg);
			ssa.run();

#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_after_SSA.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}
#endif
			bool changed = true;
			bool reass_changed = true;
#ifdef ENABLE_PFLFRONTEND_DBGFILES
			int counter = 0;
#endif
			while(reass_changed)
			{
				reass_changed = false;
				changed = true;

				while (changed)
				{
					changed = false;

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Copy propagation" << std::endl;
#endif
					jit::opt::CopyPropagation<PFLCFG> copyp(m_CompUnit->Cfg);
					changed |= copyp.run();
#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Constant propagation" << std::endl;
#endif
					jit::opt::ConstantPropagation<PFLCFG> cp(m_CompUnit->Cfg);
					changed |= cp.run();

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running ConstantFolding" << std::endl;
#endif
					jit::opt::ConstantFolding<PFLCFG> cf(m_CompUnit->Cfg);
					changed |= cf.run();

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running DeadCodeElimination" << std::endl;
#endif
					jit::opt::DeadcodeElimination<PFLCFG> dce_b(m_CompUnit->Cfg);
					changed |= dce_b.run();


#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Basic block elimination" << std::endl;
#endif
					jit::opt::BasicBlockElimination<PFLCFG> bbs(m_CompUnit->Cfg);
					bbs.start(changed);

#ifdef _DEBUG_OPTIMIZER
					std::cout << "Running Redistribution" << std::endl;
#endif
					jit::opt::Redistribution<PFLCFG> rdst(m_CompUnit->Cfg);
					rdst.start(changed);

				}
#ifdef _DEBUG_OPTIMIZER
				std::cout << "Running reassociation" << std::endl;
#endif
				//{
				//	ostringstream filename;
				//	filename << "cfg_before_reass_" << counter << ".dot";
				//	ofstream ir(filename.str().c_str());
				//	DumpCFG(ir, false, false);
				//	ir.close();
				//}
				{
					jit::Reassociation<PFLCFG> reass(m_CompUnit->Cfg);
					reass_changed = reass.run();
				}
#ifdef ENABLE_PFLFRONTEND_DBGFILES
				{
					ostringstream filename;
					filename << "cfg_after_reass_" << counter++ << ".dot";
					ofstream ir(filename.str().c_str());
					DumpCFG(ir, false, false);
					ir.close();
				}
#endif
			}

			ReassociationFixer rf(m_CompUnit->Cfg);
			rf.run();


#ifdef ENABLE_PFLFRONTEND_DBGFILES
			{
				ofstream ir("cfg_after_opt.dot");
				DumpCFG(ir, false, false);
				ir.close();
			}
#endif

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running UndoSSA" << std::endl;
#endif
			jit::UndoSSA<PFLCFG> ussa(m_CompUnit->Cfg);
			ussa.run();

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running Foldcopies" << std::endl;
#endif
			jit::Fold_Copies<PFLCFG> fc(m_CompUnit->Cfg);
			fc.run();

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running KillRedundant" << std::endl;
#endif
			jit::opt::KillRedundantCopy<PFLCFG> krc(m_CompUnit->Cfg);
			krc.run();

#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running ControlFlow simplification" << std::endl;
#endif
			jit::opt::ControlFlowSimplification<PFLCFG> cfs(m_CompUnit->Cfg);
			cfs.start(changed);


#ifdef _DEBUG_OPTIMIZER
			std::cout << "Running Basic block elimination" << std::endl;
#endif
			jit::opt::BasicBlockElimination<PFLCFG> bbs(m_CompUnit->Cfg);
			bbs.start(changed);

			jit::opt::JumpToJumpElimination<PFLCFG> j2je(m_CompUnit->Cfg);
			jit::opt::EmptyBBElimination<PFLCFG> ebbe(m_CompUnit->Cfg);
			jit::opt::RedundantJumpSimplification<PFLCFG> rds(m_CompUnit->Cfg);

			bool cfs_changed = true;
			while(cfs_changed)
			{
				cfs_changed = false;
				cfs_changed |= ebbe.run();
				bbs.start(changed);
				cfs_changed |= rds.run();
				bbs.start(changed);
				cfs_changed |= j2je.run();
				bbs.start(changed);
			}

		}
#ifdef _DEBUG_OPTIMIZER
		std::cout << "Running RegisterMapping" << std::endl;
#endif
		std::set<uint32_t> ign_set;
		jit::Register_Mapping<PFLCFG> rm(m_CompUnit->Cfg, 1, ign_set);
		rm.run();

		m_CompUnit->NumLocals = rm.get_manager().getCount();
		//std::cout << "Il regmanager ha " << rm.get_manager().getCount() << " registri " << std::endl;
		//changed = true;
		//while(changed)
		//{
		//	changed = false;
		//	jit::opt::DeadcodeElimination<PFLCFG> dce_b(m_CompUnit->Cfg);
		//	changed |= dce_b.run();
		//}
		//*******************************************************************
		//*******************************************************************
		//		HERE IT ENDS												*
		//*******************************************************************

#ifdef ENABLE_PFLFRONTEND_DBGFILES
		{
			ofstream irCFG("cfg_ir_opt.dot");
			DumpCFG(irCFG, false, false);
			irCFG.close();

		}
#endif

#ifdef ENABLE_COMPILER_PROFILING
		jit::opt::OptimizerStatistics<PFLCFG> optstats("After PFL");
		std::cout << optstats << std::endl;
#endif


	}
#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	printf("\n\tOptimizations %ld ticks", (TicksAfter - TicksBefore - MeasureCost));
	TicksBefore= nbProfilerGetTime();
#endif

	ostringstream netIL;
	if (this->m_GlobalSymbols.GetLookupTablesList().size()>0)
		m_CompUnit->UsingCoproLookupEx=true;
	else
		m_CompUnit->UsingCoproLookupEx=false;

	if (this->m_GlobalSymbols.GetRegExEntriesCount()>0)
		m_CompUnit->UsingCoproRegEx=true;
	else
		m_CompUnit->UsingCoproRegEx=false;

	m_CompUnit->DataItems=this->m_GlobalSymbols.GetDataItems();

	m_CompUnit->GenerateNetIL(netIL);

	NetIL_FilterCode = netIL.str();

#ifdef ENABLE_PFLFRONTEND_PROFILING
	TicksAfter= nbProfilerGetTime();
	printf("\n\tNetIL generation %ld ticks\n\n", TicksAfter - TicksBefore - MeasureCost);
#endif

	if (m_GlobalInfo.Debugging.DebugLevel > 1)
		nbPrintDebugLine("Generated NetIL code", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 2);

	m_LIRCodeGen = NULL;
	m_NetVMIRGen = NULL;
	return true;
}


void NetPFLFrontEnd::DumpCFG(ostream &stream, bool graphOnly, bool netIL)
{
	CFGWriter cfgWriter(stream);
	cfgWriter.DumpCFG(m_CompUnit->Cfg, graphOnly, netIL);
}

void NetPFLFrontEnd::DumpFilter(ostream &stream, bool netIL)
{
	if (netIL)
	{
		stream << NetIL_FilterCode;
	}
	else
	{
		CodeWriter cw(stream);
		cw.DumpCode(&m_CompUnit->IRCode);
	}
}

void NetPFLFrontEnd::GenCode(PFLStatement *filterExpression)
{
	stmts = 0;
	SymbolLabel *filterFalseLbl = m_LIRCodeGen->NewLabel(LBL_CODE, "DISCARD");
	SymbolLabel *sendPktLbl = m_LIRCodeGen->NewLabel(LBL_CODE, "SEND_PACKET");
	SymbolLabel *actionLbl = m_LIRCodeGen->NewLabel(LBL_CODE, "ACTION");

	// [ds] created to bypass the if inversion bug
	SymbolLabel *filterExitLbl = m_LIRCodeGen->NewLabel(LBL_CODE, "EXIT");


	PFLAction *action = filterExpression->GetAction();
	nbASSERT(action, "action cannot be null")

	m_StartProtoGenerated = false;

	if (filterExpression->GetExpression())
	{
		VisitFilterExpression(filterExpression->GetExpression(), actionLbl, filterFalseLbl);
	}

	m_LIRCodeGen->LabelStatement(actionLbl);

	if (action->GetType() == PFL_EXTRACT_FIELDS_ACTION)
	{
		PFLExtractFldsAction *exFldAct = (PFLExtractFldsAction*) action;
		FieldsList_t &fields = exFldAct->GetFields();
		FieldsList_t::iterator f = fields.begin();

		NodeList_t targetNodes;

		EncapGraph &protograph = m_GlobalInfo.GetProtoGraph();

		for (; f != fields.end(); f++)
		{
			SymbolField *field = *f;
			targetNodes.push_back(protograph.GetNode(field->Protocol));
			targetNodes.sort();
			targetNodes.unique();
		}

		GenFieldExtract(targetNodes, sendPktLbl, sendPktLbl);
	}

	m_LIRCodeGen->LabelStatement(sendPktLbl);
    // default outport == 1
	m_LIRCodeGen->GenStatement(m_LIRCodeGen->TermNode(SNDPKT, 1));

	// [ds] created to bypass the if inversion bug
	m_LIRCodeGen->JumpStatement(JUMPW, filterExitLbl);

	m_LIRCodeGen->LabelStatement(filterFalseLbl);

	// [ds] created to bypass the if inversion bug
	m_LIRCodeGen->LabelStatement(filterExitLbl);

	m_LIRCodeGen->GenStatement(m_LIRCodeGen->TermNode(RET));
	//fprintf(stderr, "number of statements: %u\n", stmts);
}


void NetPFLFrontEnd::VisitFilterBinExpr(PFLBinaryExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	switch(expr->GetOperator())
	{
	case BINOP_BOOLAND:
		{
			SymbolLabel *leftTrue = m_LIRCodeGen->NewLabel(LBL_CODE);
			VisitFilterExpression(expr->GetLeftNode(), leftTrue, falseLabel);
			m_LIRCodeGen->LabelStatement(leftTrue);
			VisitFilterExpression(expr->GetRightNode(), trueLabel, falseLabel);
		}break;
	case BINOP_BOOLOR:
		{
			SymbolLabel *leftFalse = m_LIRCodeGen->NewLabel(LBL_CODE);
			VisitFilterExpression(expr->GetLeftNode(), trueLabel, leftFalse);
			m_LIRCodeGen->LabelStatement(leftFalse);
			VisitFilterExpression(expr->GetRightNode(), trueLabel, falseLabel);
		}break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}
}

void NetPFLFrontEnd::VisitFilterUnExpr(PFLUnaryExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	switch(expr->GetOperator())
	{
	case UNOP_BOOLNOT:
		VisitFilterExpression(expr->GetChild(), falseLabel, trueLabel);
		break;
	default:
		nbASSERT(false, "CANNOT BE HERE");
		break;
	}

}


void NetPFLFrontEnd::GenFieldExtract(NodeList_t &protocols, SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	bool result = false;
	IRLowering NetVMIRGen(*m_CompUnit, *m_LIRCodeGen, falseLabel);
	m_NetVMIRGen = &NetVMIRGen;

	//Info-Partition Initialization
	for (FieldsList_t::iterator i = m_FieldsList.begin(); i != m_FieldsList.end(); i++)
	{
		switch((*i)->FieldType)
		{
			case PDL_FIELD_ALLFIELDS:
				m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(ISSTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)(*i)->Position)));
			break;

			case PDL_FIELD_BITFIELD:
				m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(IISTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)(*i)->Position)));
			break;
			
			case PDL_FIELD_FIXED:
				if((*i)->MultiProto)
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(IISTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)(*i)->Position)));
				else if((*i)->MultiField)
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(IISTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)(*i)->Position)));
				else
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(IISTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)((*i)->Position+2))));
			break;

			default:
				m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(ISSTR, m_LIRCodeGen->TermNode(PUSH,(uint32)0), m_LIRCodeGen->TermNode(PUSH,(uint32)((*i)->Position+2))));
			break;
		}	
	}

	EncapGraph &protoGraph = m_GlobalInfo.GetProtoGraph();

	m_GlobalSymbols.UnlinkProtoLabels();

	//create the subgraph for the extractfields statement
	FilterSubGraph filterSubGraph(protoGraph, protocols);

	FilterCodeGenInfo protoFilterInfo(filterSubGraph, trueLabel, falseLabel);

	EncapGraph &subGraph = filterSubGraph.GetGraph();
	//Since the protocol symbol keeps a reference to the corresponding node into the protocol graph
	//we MUST get the mapping of such node into the subgraph with FilterSubGraph::GetTargetNode()
	NodeList_t &targetNodes = filterSubGraph.GetTargetNodes();

	filterSubGraph.GetStartNode();
	// EncapGraph::GraphNode *startProtoNode = &filterSubGraph.GetStartNode();
	//remove unsupported protocols
	result = filterSubGraph.RemoveUnsupportedNodes();

	if (!filterSubGraph.IsConnected())
		throw ErrorInfo(ERR_PDL_ERROR, string("There is not a valid path between the startproto and the protocols used in extractfields"));

	//sort the subgraph in reverse postorder
	subGraph.SortRevPostorder(targetNodes);
	EncapGraph::SortedIterator i = subGraph.FirstNodeSorted();
	for (; i != subGraph.LastNodeSorted(); i++)
	{

		//if(*i == startProtoNode)
			//if (m_StartProtoGenerated)
				//continue;

		//do a reverse postorder traversal of the subgraph while generating the code
		SymbolProto *currentProto = (*i)->NodeInfo;

		if (currentProto->IsSupported)
		{
#ifdef OPTIMIZE_SIZED_LOOPS
			m_Protocol = currentProto;

			// get referred fields in ExtractField list
			if(m_GlobalSymbols.GetFieldExtractCode(m_Protocol).Front() != NULL)
				this->GetReferredFieldsExtractField(m_Protocol);

			// get referred fields in the protocol code
			this->GetReferredFields( &(m_GlobalSymbols.GetProtoCode(m_Protocol)) );

			// set sized loops to preserve
			for (FieldsList_t::iterator f = m_ReferredFieldsInFilter.begin(); f != m_ReferredFieldsInFilter.end(); f++)
			{
				StmtBase *loop = currentProto->Field2SizedLoopMap[*f];

				if (loop != NULL)
				{
					currentProto->SizedLoopToPreserve.push_front(loop);

					StmtBase *extLoop = currentProto->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						currentProto->SizedLoopToPreserve.push_front(loop);
						extLoop = currentProto->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}
			// set sized loops to preserve
			for (FieldsList_t::iterator f = m_ReferredFieldsInCode.begin(); f != m_ReferredFieldsInCode.end(); f++)
			{
				StmtBase *loop = currentProto->Field2SizedLoopMap[*f];

				if (loop != NULL)
				{
					currentProto->SizedLoopToPreserve.push_front(loop);

					StmtBase *extLoop = currentProto->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						currentProto->SizedLoopToPreserve.push_front(loop);
						extLoop = currentProto->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}
#endif
			EncapGraph::GraphNode *current = (*i);
			bool genEncap = ((current->GetSuccessors().size() > 0) ? true : false);
			GenProtoCode(currentProto, protoFilterInfo, genEncap, true);
			if (!genEncap)
				m_LIRCodeGen->JumpStatement(JUMPW, trueLabel);
		}
	}

	m_NetVMIRGen = NULL;
}

void NetPFLFrontEnd::VisitFilterTermExpr(PFLTermExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	bool result = false;
	Node *irExpr = expr->GetIRExpr();

	IRLowering NetVMIRGen(*m_CompUnit, *m_LIRCodeGen, falseLabel);
	m_NetVMIRGen = &NetVMIRGen;

	EncapGraph &protoGraph = m_GlobalInfo.GetProtoGraph();

	SymbolProto *protocol = expr->GetProtocol();
	nbASSERT(protocol != NULL, "protocol should not be null");
	m_GlobalSymbols.UnlinkProtoLabels();

	//!\todo generate a shortcut to the right output label when the term expression is a constant

	NodeList_t targets;
	targets.push_back(protoGraph.GetNode(protocol));

	//create the subgraph of the current filter
	FilterSubGraph filterSubGraph(protoGraph, targets);

	//This section is still experimental so we should keep it commented!!!
	/*
	ProtoGraphLayers protoLayers(protoGraph, *m_GlobalInfo.GetStartProtoSym()->GraphNode);
	protoLayers.PruneFilterGraph(filterSubGraph, *protocol->GraphNode);
	*/

	FilterCodeGenInfo protoFilterInfo(filterSubGraph, trueLabel, falseLabel);

	EncapGraph &subGraph = filterSubGraph.GetGraph();
	//Since the protocol symbol keeps a reference to the corresponding node into the protocol graph
	//we MUST get the mapping of such node into the subgraph with FilterSubGraph::GetTargetNode()
	NodeList_t &targetNodes = filterSubGraph.GetTargetNodes();

	nbASSERT(targetNodes.size() == 1, "There should be only 1 target node");
	//remove unsupported protocols
	result = filterSubGraph.RemoveUnsupportedNodes();

	//This section is still experimental so we should keep it commented!!!
	/*
	ProtoGraphShaper filterShaper(filterSubGraph);
	filterShaper.RemoveTargetTunnels();
	*/

	/*
	Since we have mangled in several ways the filter sub-graph (unsupported nodes removal, tunnels removal, etc.)
	we must check if the subgraph is always connected (i.e. there is at least a path between startproto and the
	target protocol)
	*/
	if (!filterSubGraph.IsConnected())
		throw ErrorInfo(ERR_PDL_ERROR, string("There is not a valid path between the startproto and the ") + protocol->Name + string(" protocol"));

	//sort the subgraph in reverse postorder
	subGraph.SortRevPostorder(targetNodes);
	EncapGraph::SortedIterator i = subGraph.FirstNodeSorted();

	for (; i != subGraph.LastNodeSorted(); i++)
	{
		//do a reverse postorder traversal of the subgraph while generating the code
		SymbolProto *currentProto = (*i)->NodeInfo;
		if (currentProto != protocol && currentProto->IsSupported)
		{
#ifdef OPTIMIZE_SIZED_LOOPS
			m_Protocol = currentProto;
			// get referred fields in ExtractField list
			if(m_GlobalSymbols.GetFieldExtractCode(m_Protocol).Front() != NULL)
				this->GetReferredFieldsExtractField(m_Protocol);

			// get referred fields in the protocol code
			this->GetReferredFields( &(m_GlobalSymbols.GetProtoCode(m_Protocol)) );

			// set sized loops to preserve
			for (FieldsList_t::iterator i = m_ReferredFieldsInFilter.begin(); i != m_ReferredFieldsInFilter.end(); i++)
			{
				StmtBase *loop = protocol->Field2SizedLoopMap[*i];

				if (loop != NULL)
				{
					protocol->SizedLoopToPreserve.push_front(loop);

					StmtBase *extLoop = protocol->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						protocol->SizedLoopToPreserve.push_front(loop);
						extLoop = protocol->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}
			// set sized loops to preserve
			for (FieldsList_t::iterator i = m_ReferredFieldsInCode.begin(); i != m_ReferredFieldsInCode.end(); i++)
			{
				StmtBase *loop = protocol->Field2SizedLoopMap[*i];

				if (loop != NULL)
				{
					protocol->SizedLoopToPreserve.push_front(loop);

					StmtBase *extLoop = protocol->SizedLoop2SizedLoopMap[loop];
					while (extLoop != NULL)
					{
						protocol->SizedLoopToPreserve.push_front(loop);
						extLoop = protocol->SizedLoop2SizedLoopMap[extLoop];
					}
				}
			}
#endif
			GenProtoCode(currentProto, protoFilterInfo);
		}
	}

	if (irExpr != NULL)
	{
#ifdef OPTIMIZE_SIZED_LOOPS
		m_Protocol = protocol;
		// get referred fields
		if(irExpr != NULL )
			this->GetReferredFieldsInBoolExpr(irExpr);
		// get referred fields in ExtractField list
		if(m_GlobalSymbols.GetFieldExtractCode(protocol).Front() != NULL)
			this->GetReferredFieldsExtractField(m_Protocol);

		// get referred fields in the protocol code
		this->GetReferredFields( &(m_GlobalSymbols.GetProtoCode(m_Protocol)) );

		// set sized loops to preserve
		for (FieldsList_t::iterator i = m_ReferredFieldsInFilter.begin(); i != m_ReferredFieldsInFilter.end(); i++)
		{
			StmtBase *loop = protocol->Field2SizedLoopMap[*i];

			if (loop != NULL)
			{
				protocol->SizedLoopToPreserve.push_front(loop);

				StmtBase *extLoop = protocol->SizedLoop2SizedLoopMap[loop];
				while (extLoop != NULL)
				{
					protocol->SizedLoopToPreserve.push_front(loop);
					extLoop = protocol->SizedLoop2SizedLoopMap[extLoop];
				}
			}
		}
		for (FieldsList_t::iterator i = m_ReferredFieldsInCode.begin(); i != m_ReferredFieldsInCode.end(); i++)
		{
			StmtBase *loop = protocol->Field2SizedLoopMap[*i];

			if (loop != NULL)
			{
				protocol->SizedLoopToPreserve.push_front(loop);

				StmtBase *extLoop = protocol->SizedLoop2SizedLoopMap[loop];
				while (extLoop != NULL)
				{
					protocol->SizedLoopToPreserve.push_front(loop);
					extLoop = protocol->SizedLoop2SizedLoopMap[extLoop];
				}
			}
		}
#endif

		//Lower the target protocol format code
		m_NetVMIRGen->LowerHIRCode(&(m_GlobalSymbols.GetProtoCode(protocol)), protocol, "filter target protocol format");

		if(irExpr != NULL)
		{
			//Generate a test on the protocol fields
			CodeList *cond = m_GlobalSymbols.NewCodeList();
			IRCodeGen condGen(m_GlobalSymbols, cond);
			EncapGraph::GraphNode *targetNode = targetNodes.back();
			if (targetNode->GetSuccessors().size() > 0)
			{
				SymbolLabel *encapLabel = condGen.NewLabel(LBL_CODE, protocol->Name + "_encap");
				condGen.JCondStatement(trueLabel, encapLabel, irExpr);
				condGen.LabelStatement(encapLabel);
				NetPDLParser.ParseEncapsulation(*protocol, filterSubGraph, falseLabel, condGen);
			}
			else
			{
				condGen.JCondStatement(trueLabel, falseLabel, irExpr);
			}

			m_NetVMIRGen->LowerHIRCode(cond, "condition");
		}
		else
		{

#ifndef EXEC_BEFORE_AS_SOON_AS_FOUND
			// execute-before is parsed for each next-proto candidate (to ensure the execution even if
			// the candidate is not the filtered one)
			CodeList *execBeforeCode = m_GlobalSymbols.NewCodeList(true);
			IRCodeGen execBeforeCodeGen(m_GlobalSymbols, execBeforeCode);
			NetPDLParser.ParseExecBefore(*protocol, protoFilterInfo.SubGraph, protoFilterInfo.FilterFalseLbl, execBeforeCodeGen);
			m_NetVMIRGen->LowerHIRCode(execBeforeCode, ""/*, "code for protocol " + proto->Name + " execBeforesulation"*/);
#endif

			m_LIRCodeGen->JumpStatement(JUMPW, trueLabel);

		}
	}
	else
	{
		SymbolLabel *startLabel = m_GlobalSymbols.GetProtoStartLbl(protocol);
		if (startLabel->Linked!=NULL)
		{
			m_LIRCodeGen->LabelStatement(startLabel->Linked);
		}

#ifndef EXEC_BEFORE_AS_SOON_AS_FOUND
		// execute-before is parsed for each next-proto candidate (to ensure the execution even if
		// the candidate is not the filtered one)
		CodeList *execBeforeCode = m_GlobalSymbols.NewCodeList(true);
		IRCodeGen execBeforeCodeGen(m_GlobalSymbols, execBeforeCode);
		NetPDLParser.ParseExecBefore(*protocol, protoFilterInfo.SubGraph, protoFilterInfo.FilterFalseLbl, execBeforeCodeGen);
		m_NetVMIRGen->LowerHIRCode(execBeforeCode, ""/*, "code for protocol " + proto->Name + " execBeforesulation"*/);
#endif

		m_LIRCodeGen->JumpStatement(JUMPW, trueLabel);
	}
	m_NetVMIRGen = NULL;
}

void NetPFLFrontEnd::VisitFilterExpression(PFLExpression *expr, SymbolLabel *trueLabel, SymbolLabel *falseLabel)
{
	switch(expr->GetType())
	{
	case PFL_BINARY_EXPRESSION:
		VisitFilterBinExpr((PFLBinaryExpression*)expr, trueLabel, falseLabel);
		break;
	case PFL_UNARY_EXPRESSION:
		VisitFilterUnExpr((PFLUnaryExpression*)expr, trueLabel, falseLabel);
		break;
	case PFL_TERM_EXPRESSION:
		VisitFilterTermExpr((PFLTermExpression*)expr, trueLabel, falseLabel);
		break;
	}
}


void NetPFLFrontEnd::ParseExtractField(FieldsList_t *fldList)
{
	uint32 count=0;
	for(FieldsList_t::iterator i = fldList->begin(); i!= fldList->end();i++)
	{
		if((*i)->FieldType==PDL_FIELD_ALLFIELDS)
		{	StmtFieldInfo *fldInfoStmt= new StmtFieldInfo((*i),count);
			m_GlobalSymbols.AddFieldExtractStatment((*i)->Protocol,fldInfoStmt);
			(*i)->Protocol->ExAllfields=true;
			(*i)->Protocol->position= m_LIRCodeGen->NewTemp(string("allfields_position"), m_CompUnit->NumLocals);
			(*i)->Protocol->beginPosition=count;
			this->m_LIRCodeGen->CommentStatement(string("INITIALIZATION ALLFIELDS POSITION"));
			this->m_LIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_allfields")));
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(LOCST,m_LIRCodeGen->TermNode(PUSH,count),(*i)->Protocol->position));
			(*i)->Protocol->NExFields= m_LIRCodeGen->NewTemp(string("allfields_count"), m_CompUnit->NumLocals);
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(LOCST,m_LIRCodeGen->TermNode(PUSH,(uint32)0),(*i)->Protocol->NExFields));
			break;
		}
		else
		{
			SymbolField *fld=m_GlobalSymbols.LookUpProtoField((*i)->Protocol,(*i));
			if(fld!=NULL)
			{
				StmtFieldInfo *fldInfoStmt= new StmtFieldInfo(fld,count);
				m_GlobalSymbols.AddFieldExtractStatment((*i)->Protocol,fldInfoStmt);
				fld->ToExtract=true;
				fld->Position=count;
				
				if(fld->ExtractG == NULL)
				{
					//Protocol->Extract conta il numero di header del protocollo incontrati nel pacchetto durante l'elaborazione
					fld->ExtractG = m_LIRCodeGen->NewTemp(fld->Name + string("_ExtractG"), m_CompUnit->NumLocals);
					this->m_LIRCodeGen->CommentStatement(string("Initializing Extract counter for header ") + fld->Name);
					this->m_LIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_") + fld->Protocol->Name + fld->Name + string("_ExtractG_counter")));
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(LOCST,m_LIRCodeGen->TermNode(PUSH,(uint32)0), fld->ExtractG)); //ExtractG = 0

					fld->Protocol->proto_Extract = fld->ExtractG;
				}

				if (fld->MultiProto)
				{
					fld->HeaderCount = m_LIRCodeGen->NewTemp(fld->Protocol->Name + string("_counter"), m_CompUnit->NumLocals);
					this->m_LIRCodeGen->CommentStatement(string("Initializing MultiProto header counter for header ") + fld->Protocol->Name);
					this->m_LIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_") + fld->Protocol->Name /* fld->Name */ + string("_headercount")));
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(LOCST,m_LIRCodeGen->TermNode(PUSH,(uint32)0), fld->HeaderCount)); //header_counter = 0
				 
					fld->MultiProto = true;
					count += nbNETPFLCOMPILER_INFO_FIELDS_SIZE * (1 + nbNETPFLCOMPILER_MAX_PROTO_INSTANCES);
				}

				else if (fld->MultiField)
				{
					fld->FieldCount = m_LIRCodeGen->NewTemp(fld->Name + string("_counter"), m_CompUnit->NumLocals);
					this->m_LIRCodeGen->CommentStatement(string("Initializing MultiField counter for field ") + fld->Name);
					this->m_LIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init_") + fld->Protocol->Name /* fld->Name */ + string("_fieldcount")));
					m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(LOCST,m_LIRCodeGen->TermNode(PUSH,(uint32)0), fld->FieldCount)); //field_counter = 0

					fld->Protocol->proto_FieldCount = fld->FieldCount;
				 
					fld->MultiField = true;
					fld->Protocol->proto_MultiField = true;
					count += nbNETPFLCOMPILER_INFO_FIELDS_SIZE * (1 + nbNETPFLCOMPILER_MAX_FIELD_INSTANCES);
				}

				else
				{
					count += nbNETPFLCOMPILER_INFO_FIELDS_SIZE;
				}
			}
		}
	}
}


void NetPFLFrontEnd::VisitAction(PFLAction *action)
{
	switch(action->GetType())
	{
	case PFL_EXTRACT_FIELDS_ACTION:
		{
		PFLExtractFldsAction * ExFldAction = (PFLExtractFldsAction *)action;
		m_FieldsList = ExFldAction->GetFields();
		ParseExtractField(&m_FieldsList);
		}
		break;
	case PFL_CLASSIFY_ACTION:
		//not yet implemented
		break;
	case PFL_RETURN_PACKET_ACTION:
		break;
	}
}


void NetPFLFrontEnd::GenRegexEntries(void)
{

	int regexEntries=this->m_GlobalSymbols.GetRegExEntriesCount();
	if (regexEntries>0)
	{
		int index = 0;
		char bufEntriesCount[10];
		char bufIndex[10];
		char bufOptLen[10];
		char bufOpt[10]={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		char bufPatLen[10];

		//itoa(regexEntries, bufEntriesCount, 10);
		snprintf(bufEntriesCount, 9, "%d", regexEntries);

		SymbolDataItem *regexData = new SymbolDataItem(string("regexp_data"), DATA_TYPE_WORD, string(bufEntriesCount), index);
		//m_CodeGen->GenStatement(m_CodeGen->TermNode(IR_DATA, regexData));
		this->m_GlobalSymbols.StoreDataItem(regexData);

		RegExList_t list = this->m_GlobalSymbols.GetRegExList();

		index++;
		for (RegExList_t::iterator i=list.begin(); i!=list.end(); i++, index++)
		{
			snprintf(bufIndex, 9, "%d", index);

			int optLenValue=( (*i)->CaseSensitive? 0 : 1 );
			//int optLenValue=( (*i)->CaseSensitive? 2 : 3 );
			snprintf(bufOptLen, 9, "%d", optLenValue);

			//int escapes=0;
			int slashes=0;
			int length=(*i)->Pattern->Size;
			for (int cIndex=0; cIndex<length; cIndex++)
			{
				if ((*i)->Pattern->Name[cIndex]=='\\')
				{
					slashes++;
				}
			}

			length-=slashes/2;
			snprintf(bufPatLen, 9, "%d", length);

			if ((*i)->CaseSensitive)
				//strncpy(bufOpt, "sm", 3);
				strncpy(bufOpt, "", 0);
			else
				//strncpy(bufOpt, "smi", 3);
				strncpy(bufOpt, "i", 1);

			SymbolDataItem *optLen = new SymbolDataItem(string("opt_len_").append(bufIndex), DATA_TYPE_WORD, string(bufOptLen), index);
			this->m_GlobalSymbols.StoreDataItem(optLen);

			SymbolDataItem *opt = new SymbolDataItem(string("opt_").append(bufIndex), DATA_TYPE_BYTE, string("\"").append(string(bufOpt)).append("\""), index);
			this->m_GlobalSymbols.StoreDataItem(opt);

			SymbolDataItem *patternLen = new SymbolDataItem(string("pat_len_").append(bufIndex), DATA_TYPE_WORD, string(bufPatLen), index);
			this->m_GlobalSymbols.StoreDataItem(patternLen);

			SymbolDataItem *pattern = new SymbolDataItem(string("pat_").append(bufIndex), DATA_TYPE_BYTE, string("\"").append(string((*i)->Pattern->Name)).append("\""), index);
			this->m_GlobalSymbols.StoreDataItem(pattern);
		}
	}
}



void NetPFLFrontEnd::GenInitCode()
{
	this->m_LIRCodeGen->CommentStatement(string("INITIALIZATION"));
	this->m_LIRCodeGen->LabelStatement(new SymbolLabel(LBL_CODE, 0, string("init")));

	if (this->m_GlobalSymbols.GetRegExEntriesCount()>0)
	{
		this->m_LIRCodeGen->CommentStatement(string("Initialize Regular Expression coprocessor"));
		m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(POP,
			m_LIRCodeGen->UnOp(COPINIT, new SymbolLabel(LBL_ID, 0, "regexp"), 0, new SymbolLabel(LBL_ID, 0, "regexp_data"))));
	}

	LookupTablesList_t lookupTablesList = this->m_GlobalSymbols.GetLookupTablesList();
	m_LIRCodeGen->CommentStatement(string("Initialize Lookup_ex coprocessor"));
	if (lookupTablesList.size() > 0)
	{
		m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPOUT,
			m_LIRCodeGen->TermNode(PUSH, lookupTablesList.size()),
			new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OUT_TABLE_ID));

		m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPRUN, new SymbolLabel(LBL_ID, 0, "lookup_ex"), LOOKUP_EX_OP_INIT));

		// initialize lookup coprocessor
		for (LookupTablesList_t::iterator i = lookupTablesList.begin(); i!=lookupTablesList.end(); i++)
		{
			SymbolLookupTable *table = (*i);

			m_LIRCodeGen->CommentStatement(string("Initialize the table ").append(table->Name));

			// set table id
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPOUT,
				m_LIRCodeGen->TermNode(PUSH, table->Id),
				table->Label,
				LOOKUP_EX_OUT_TABLE_ID));

			// set entries number
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPOUT,
				m_LIRCodeGen->TermNode(PUSH, table->MaxExactEntriesNr+table->MaxMaskedEntriesNr),
				table->Label,
				LOOKUP_EX_OUT_ENTRIES));

			// set data size
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPOUT,
				m_LIRCodeGen->TermNode(PUSH, table->KeysListSize),
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
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPOUT,
				m_LIRCodeGen->TermNode(PUSH, valueSize),
				table->Label,
				LOOKUP_EX_OUT_VALUES_SIZE));

			m_LIRCodeGen->GenStatement(m_LIRCodeGen->UnOp(COPRUN, table->Label, LOOKUP_EX_OP_INIT_TABLE));
		}
	}

}


//**********************

void NetPFLFrontEnd::GenProtoCode(SymbolProto *proto, FilterCodeGenInfo &protoFilterInfo, bool encap, bool fieldExtract)
{
	//cerr << "Generating code for protocol " << proto->Name << endl;

	//the code for a protocol is generated by parsing the fixed code and the encapsulation sections

	m_NetVMIRGen->LowerHIRCode(&(m_GlobalSymbols.GetProtoCode(proto)), proto);

	if(proto->proto_MultiField)
	{
		m_LIRCodeGen->CommentStatement(string("MultiField - memorizzazione numero campi estratti ") + proto->Name);
		FieldsList_t::iterator f = proto->proto_MultiFields.begin();
		for (; f != proto->proto_MultiFields.end(); f++)
		{	//store the number of fields extracted
			m_LIRCodeGen->GenStatement(m_LIRCodeGen->BinOp(IISTR, m_LIRCodeGen->TermNode(LOCLD, proto->proto_FieldCount), m_LIRCodeGen->TermNode(PUSH, (*f)->Position)));
		}
	}

	if (encap)	{
		CodeList *encapCode = m_GlobalSymbols.NewCodeList(true);
		IRCodeGen encapCodeGen(m_GlobalSymbols, encapCode);
		NetPDLParser.ParseEncapsulation(*proto, protoFilterInfo.SubGraph, protoFilterInfo.FilterFalseLbl, encapCodeGen);

		m_NetVMIRGen->LowerHIRCode(encapCode, ""/*, "code for protocol " + proto->Name + " encapsulation"*/);
	}

	//The current node is the StartProto, set the StartProtoGenerated flag
	if (proto == m_GlobalInfo.GetStartProtoSym())
		m_StartProtoGenerated = true;
}

//**********************


void NetPFLFrontEnd::GenProtoHIR(EncapGraph::GraphNode &node, FilterCodeGenInfo &protoFilterInfo, bool encap, bool fieldExtract)
{
	SymbolProto *proto = node.NodeInfo;
	cout << "Generating code for protocol " << proto->Name << endl;

	CodeList *protoHIRCode = protoFilterInfo.SubGraph.NewProtoCodeList(node);
	IRCodeGen protoCodeGen(m_GlobalSymbols, protoHIRCode);

	//the code for a protocol is generated by parsing the fixed code and the encapsulation sections
	CloneHIR(m_GlobalSymbols.GetProtoCode(proto), protoCodeGen);

	if (fieldExtract)
	{
		if(m_GlobalSymbols.GetFieldExtractCode(proto).Front()!=NULL)
			CloneHIR(m_GlobalSymbols.GetFieldExtractCode(proto), protoCodeGen);
	}

	if (encap)
	{

		NetPDLParser.ParseEncapsulation(*proto, protoFilterInfo.SubGraph, protoFilterInfo.FilterFalseLbl, protoCodeGen);
	}
}



void NetPFLFrontEnd::CloneHIR(CodeList &code, IRCodeGen &codegen)
{
	StmtBase *next = code.Front();

	while(next)
	{
		StmtBase *stmt = next->Clone();
		nbASSERT(stmt != NULL, "problems cloning statement");
		codegen.Statement(next->Clone());
		next = next->Next;
	}
}

FieldsList_t NetPFLFrontEnd::GetExField(void)
{
  return m_FieldsList;
}


SymbolField* NetPFLFrontEnd::GetFieldbyId(const string protoName, int index)
{
	return m_GlobalSymbols.LookUpProtoFieldById(protoName,(uint32)index);
}

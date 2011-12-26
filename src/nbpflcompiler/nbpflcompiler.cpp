/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/





#include "nbpflcompiler.h"
#include "netpflfrontend.h"
#include "errors.h"
#include "../nbee/globals/globals.h"
#include "../nbee/globals/debug.h"




nbNetPFLCompiler *nbAllocateNetPFLCompiler(_nbNetPDLDatabase *NetPDLProtoDB)
{
	if (!NetPDLProtoDB)
		return NULL;

	return new nbNetPFLCompiler(*NetPDLProtoDB);
}

void nbDeallocateNetPFLCompiler(nbNetPFLCompiler *NetPFLCompiler)
{
	if (NetPFLCompiler)
		delete NetPFLCompiler;
}


nbNetPFLCompiler::nbNetPFLCompiler(_nbNetPDLDatabase &protoDB)
: NumMsg(0), MsgList(0), PDLInited(0), ProtoDB(protoDB), PFLFrontEnd(0), GenCode(0), CodeSize(0)
{
#ifdef _DEBUG
	m_debugLevel= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL;
	dumpHIRCodeFilename= nbNETPFLCOMPILER_DEBUG_HIR_CODE_FILENAME;
	dumpLIRCodeFilename= NULL;
	dumpLIRGraphFilename= NULL;
	dumpLIRNoOptGraphFilename= nbNETPFLCOMPILER_DEBUG_LIR_NOOPT_GRAPH_FILENAME;
	dumpNetILCodeFilename= NULL;
	dumpNetILGraphFilename= NULL;
	dumpNoCodeGraphFilename= NULL;
	dumpProtoGraphFilename= nbNETPFLCOMPILER_DEBUG_PROTOGRAH_DUMP_FILENAME;
#else
	m_debugLevel= nbNETPFLCOMPILER_DEBUG_DETAIL_LEVEL;
	dumpHIRCodeFilename= NULL;
	dumpLIRCodeFilename= NULL;
	dumpLIRGraphFilename= NULL;
	dumpLIRNoOptGraphFilename= NULL;
	dumpNetILCodeFilename= NULL;
	dumpNetILGraphFilename= NULL;
	dumpNoCodeGraphFilename= NULL;
	dumpProtoGraphFilename= NULL;
#endif

	Descriptors= NULL;
}


nbNetPFLCompiler::~nbNetPFLCompiler()
{
	if (GenCode)
		delete []GenCode;
	if (Descriptors)
		delete Descriptors;
	NetPDLCleanup();
}

void nbNetPFLCompiler::NetPDLCleanup(void)
{
	if (this->m_debugLevel > 2)
		nbPrintDebugLine("Cleaning NetPFL Front End", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 2);

	if (PFLFrontEnd)
		delete PFLFrontEnd;

	PFLFrontEnd= NULL;
	PDLInited= false;
}


int nbNetPFLCompiler::FillMsgList(ErrorRecorder &errRecorder)
{
	NumMsg = 0;
	if (errRecorder.TotalErrors() > 0)
	{
		list<ErrorInfo> &compErrList = errRecorder.GetErrorList();
		list<ErrorInfo>::iterator i = compErrList.begin();
		MsgList = 0;
		_nbNetPFLCompilerMessages *current=0;
		char errType[20];

		while (i != compErrList.end())
		{
			_nbNetPFLCompilerMessages *message = new _nbNetPFLCompilerMessages;
			if (message == NULL)
			{
				errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
				return nbFAILURE;
			}
			memset(message->MessageString, 0, nbNETPFLCOMPILER_MAX_MESSAGE);
			switch ((*i).Type) {
				case ERR_PFL_ERROR:
					sprintf(errType, "PFL error");
					break;
				case ERR_PDL_ERROR:
					sprintf(errType, "PDL error");
					break;
				case ERR_PFL_WARNING:
					sprintf(errType, "PFL warning");
					break;
				case ERR_PDL_WARNING:
					sprintf(errType, "PDL warning");
					break;
				case ERR_FATAL_ERROR:
					sprintf(errType, "Fatal error");
					break;
			}
			snprintf(message->MessageString, nbNETPFLCOMPILER_MAX_MESSAGE - 1, "[%s] %s", errType, (*i).Msg.c_str());
			message->Next = 0;
			if (MsgList == NULL)
				MsgList = message;
			else
				current->Next=message;

			current=message;

			NumMsg++;
			i++;
		}
	}
	return nbSUCCESS;
}


void nbNetPFLCompiler::ClearMsgList(void)
{
	NumMsg = 0;
	_nbNetPFLCompilerMessages *next = MsgList;
	while(next)
	{
		_nbNetPFLCompilerMessages *current = next;
		next = current->Next;
		delete current;
	}
	MsgList = NULL;
}


int nbNetPFLCompiler::IsInitialized(void)
{
	if (PDLInited)
		return nbSUCCESS;

	return nbFAILURE;
}

void nbNetPFLCompiler::SetDebugLevel(const unsigned int DebugLevel)
{
	m_debugLevel= DebugLevel;
}

void nbNetPFLCompiler::SetNetILCodeFilename(const char *DumpNetILCodeFilename)
{
	dumpNetILCodeFilename= (char*)DumpNetILCodeFilename;
}

void nbNetPFLCompiler::SetHIRCodeFilename(const char *DumpHIRCodeFilename)
{
	dumpHIRCodeFilename= (char*)DumpHIRCodeFilename;
}

void nbNetPFLCompiler::SetLIRGraphFilename(const char *DumpLIRGraphFilename)
{
	dumpLIRGraphFilename= (char*)DumpLIRGraphFilename;
}

void nbNetPFLCompiler::SetLIRCodeFilename(const char *DumpLIRCodeFilename)
{
	dumpLIRCodeFilename= (char*) DumpLIRCodeFilename;
}

void nbNetPFLCompiler::SetLIRNoOptGraphFilename(const char *DumpLIRNoOptGraphFilename)
{
	dumpLIRNoOptGraphFilename= (char*)DumpLIRNoOptGraphFilename;
}

void nbNetPFLCompiler::SetNoCodeGraphFilename(const char *DumpNoCodeGraphFilename)
{
	dumpNoCodeGraphFilename= (char*)DumpNoCodeGraphFilename;
}

void nbNetPFLCompiler::SetNetILGraphFilename(const char *DumpNetILGraphFilename)
{
	dumpNetILGraphFilename= (char*)DumpNetILGraphFilename;
}

void nbNetPFLCompiler::SetProtoGraphFilename(const char *DumpProtoGraphFilename)
{
	dumpProtoGraphFilename= (char*)DumpProtoGraphFilename;
}


int nbNetPFLCompiler::NetPDLInit(nbNetPDLLinkLayer_t LinkLayer)
{
	ClearMsgList();

	if (PDLInited || PFLFrontEnd)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has already been initialized, please use NetPDLCleanup()");
		return nbFAILURE;
	}

	try
	{
		PFLFrontEnd = new NetPFLFrontEnd(ProtoDB, LinkLayer, m_debugLevel, dumpHIRCodeFilename, dumpLIRNoOptGraphFilename, dumpProtoGraphFilename);
		if (PFLFrontEnd == 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
			NetPDLCleanup();
			return nbFAILURE;
		}
		ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();

		if (FillMsgList(errRecorder) != nbSUCCESS)
		{
			NetPDLCleanup();
			return nbFAILURE;
		}

		if (errRecorder.NumErrors() > 0)
		{
			errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPDL description");
			NetPDLCleanup();
			return nbFAILURE;
		}
		PDLInited = true;
		return nbSUCCESS;
	}

	catch (ErrorInfo &e)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Compiler Internal Error: ", e.Msg.c_str());
		NetPDLCleanup();
		return nbFAILURE;
	}
}

_nbNetPFLCompilerMessages *nbNetPFLCompiler::GetCompMessageList(void)
{
	return MsgList;
}

int nbNetPFLCompiler::CheckFilter(const char *NetPFLFilterString)
{
bool RetVal;

	if (m_debugLevel > 1)
		nbPrintDebugLine("Checking filter syntax...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	ClearMsgList();

	if (!(PDLInited && PFLFrontEnd))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has not been initialized, please use NetPDLInit()");
		return nbFAILURE;
	}

	if (NetPFLFilterString)
		RetVal= PFLFrontEnd->CheckFilter("");
	else
		RetVal= PFLFrontEnd->CheckFilter(NetPFLFilterString);

	ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
	FillMsgList(errRecorder);

	if ((errRecorder.NumErrors() > 0) || !RetVal)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPFL filter");
		return nbFAILURE;
	}

	return nbSUCCESS;
}


int nbNetPFLCompiler::CompileFilter(const char *NetPFLFilterString, char **NetILCode, bool optimizationCycles)
{
bool RetVal;

	if (m_debugLevel > 1)
		nbPrintDebugLine("Compiling filter...", DBG_TYPE_INFO, __FILE__, __FUNCTION__, __LINE__, 1);

	ClearMsgList();
	if (GenCode != NULL)
	{
		delete []GenCode;
		GenCode = NULL;
	}
	*NetILCode= NULL;

	if (!(PDLInited && PFLFrontEnd))
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "NetPDL Compiler Engine has not been initialized, please use NetPDLInit().");
		return nbFAILURE;
	}

	if (NetPFLFilterString == NULL)
		RetVal= PFLFrontEnd->CompileFilter("", optimizationCycles);
	else
		RetVal= PFLFrontEnd->CompileFilter(NetPFLFilterString, optimizationCycles);


	ErrorRecorder &errRecorder = PFLFrontEnd->GetErrRecorder();
	FillMsgList(errRecorder);

	if ((errRecorder.NumErrors() > 0) || !RetVal)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), "Failed to compile the NetPFL filter");
		return nbFAILURE;
	}

	string &netIL = PFLFrontEnd->GetNetILFilter();
	unsigned int codeStrLen = netIL.size() + 1;
	GenCode = new char[codeStrLen];
	if (GenCode == NULL)
	{
		errorsnprintf(__FILE__, __FUNCTION__, __LINE__, m_errbuf, sizeof(m_errbuf), ERROR_ALLOC_FAILED);
		return nbFAILURE;
	}
	strncpy(GenCode, netIL.c_str(), codeStrLen - 1);
	GenCode[codeStrLen-1] = '\0';

	if (this->dumpNetILCodeFilename!=NULL)
	{
		if (this->dumpNetILCodeFilename[0] == 0)
			printf("%s", GenCode);
		else
		{
			ofstream netILFile(this->dumpNetILCodeFilename);
			PFLFrontEnd->DumpFilter(netILFile, true);
			netILFile.close();
		}
	}

#ifdef ENABLE_PFLFRONTEND_DBGFILES
	if (this->dumpLIRCodeFilename!=NULL)
	{
		ofstream irFile(this->dumpLIRCodeFilename);
		PFLFrontEnd->DumpFilter(irFile, false);
		irFile.close();
	}
	if (this->dumpNetILGraphFilename!=NULL)
	{
		ofstream netILCFG(this->dumpNetILGraphFilename);
		PFLFrontEnd->DumpCFG(netILCFG, false, true);
		netILCFG.close();
	}
	if (this->dumpNoCodeGraphFilename!=NULL)
	{
		ofstream cfgNoCode(this->dumpNoCodeGraphFilename);
		PFLFrontEnd->DumpCFG(cfgNoCode, true, true);
		cfgNoCode.close();
	}
	if (this->dumpLIRGraphFilename!=NULL)
	{
		ofstream irCFG(this->dumpLIRGraphFilename);
		PFLFrontEnd->DumpCFG(irCFG, false, false);
		irCFG.close();
	}
#endif

	*NetILCode = GenCode;
	return nbSUCCESS;
}


_nbExtractedFieldsDescriptorVector* nbNetPFLCompiler::GetExtractField()
{
int j=0;

	exFieldList= PFLFrontEnd->GetExField();
	Descriptors= new _nbExtractedFieldsDescriptorVector(exFieldList.size());
	for (FieldsList_t::iterator i = exFieldList.begin(); i != exFieldList.end(); i++,j++)
	{
		SymbolField *field=(*i);
		int fldSize = 0;
		if(field->FieldType!=PDL_FIELD_ALLFIELDS)
		{
			if (field->FieldType == PDL_FIELD_FIXED)
				fldSize = ((SymbolFieldFixed*)field)->Size;
			else if (field->FieldType == PDL_FIELD_BITFIELD)
				fldSize = 4;
			Descriptors->FieldDescriptor[j].Position = field->Position;
			if (field->MultiProto)
			{
				Descriptors->FieldDescriptor[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO;
				Descriptors->FieldDescriptor[j].Name = field->Name.c_str();
				Descriptors->FieldDescriptor[j].Proto = field->Protocol->Name.c_str();
				Descriptors->FieldDescriptor[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
				Descriptors->FieldDescriptor[j].DVct = new _nbExtractedFieldsDescriptorVector(nbNETPFLCOMPILER_MAX_PROTO_INSTANCES);
				for (uint32 k = 0; k < nbNETPFLCOMPILER_MAX_PROTO_INSTANCES; k++)
				{
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].Name = Descriptors->FieldDescriptor[j].Name;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].Proto = Descriptors->FieldDescriptor[j].Proto;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].FieldType = Descriptors->FieldDescriptor[j].FieldType;
				}
			}
			else if (field->MultiField)
			{
				Descriptors->FieldDescriptor[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD;
				Descriptors->FieldDescriptor[j].Name = field->Name.c_str();
				Descriptors->FieldDescriptor[j].Proto = field->Protocol->Name.c_str();
				Descriptors->FieldDescriptor[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
				Descriptors->FieldDescriptor[j].DVct = new _nbExtractedFieldsDescriptorVector(nbNETPFLCOMPILER_MAX_FIELD_INSTANCES);
				for (uint32 k = 0; k < nbNETPFLCOMPILER_MAX_FIELD_INSTANCES; k++)
				{
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].Name = Descriptors->FieldDescriptor[j].Name;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].Proto = Descriptors->FieldDescriptor[j].Proto;
					Descriptors->FieldDescriptor[j].DVct->FieldDescriptor[k].FieldType = Descriptors->FieldDescriptor[j].FieldType;
				}
			}
			else
			{

				Descriptors->FieldDescriptor[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_FIELD;
				Descriptors->FieldDescriptor[j].Name = field->Name.c_str();
				Descriptors->FieldDescriptor[j].Proto = field->Protocol->Name.c_str();
				Descriptors->FieldDescriptor[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
				Descriptors->FieldDescriptor[j].Length = fldSize;
			}
		}
		else
		{
			Descriptors->FieldDescriptor[j].DataFormatType = nbNETPFLCOMPILER_DATAFORMAT_FIELDLIST;
			Descriptors->FieldDescriptor[j].Name = "allfields";
			Descriptors->FieldDescriptor[j].Proto = field->Protocol->Name.c_str();
			Descriptors->FieldDescriptor[j].FieldType = (nbExtractedFieldsFieldType_t) field->FieldType;
			Descriptors->FieldDescriptor[j].DVct = new _nbExtractedFieldsDescriptorVector(nbNETPFLCOMPILER_MAX_ALLFIELDS);
		}
	}
	return Descriptors;
}

int nbNetPFLCompiler::GetFieldInfo(const string protoName, uint32_t id,_nbExtractedFieldsDescriptor* des)
{
	SymbolField* field=PFLFrontEnd->GetFieldbyId(protoName,(int)id);

	if ((field==NULL) || (field->IsDefined==false))
		return nbFAILURE;

	des->FieldType= (nbExtractedFieldsFieldType_t) field->FieldType;
	des->Name= field->Name.c_str();
	des->Proto= protoName.c_str();
	des->DataFormatType= nbNETPFLCOMPILER_DATAFORMAT_FIELD;

	return nbSUCCESS;
}

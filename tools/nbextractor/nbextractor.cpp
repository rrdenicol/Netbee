/*
* Copyright (c) 2002 - 2011
* NetGroup, Politecnico di Torino (Italy)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following condition
* is met:
*
* Neither the name of the Politecnico di Torino nor the names of its
* contributors may be used to endorse or promote products derived from
* this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
* OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pcap.h>
#include <nbee.h>
#include <nbsockutils.h>

#include "configparams.h"
#include "anonimize-ip.h"
#include "fieldprinter.h"
#include "../utils/utils.h"

#ifdef ENABLE_SQLITE3
#include "dump-sqlite.h"
#endif

#if defined(_WIN32) || defined(_WIN64)
#define snprintf _snprintf
#define strdup _strdup
#endif

#ifdef PROFILING
// dark 2011-02-07: I chose this number arbitrarly. Feel free to tweak it
#define PROFILER_MAX_PACKETS 2000
nbProfiler* ProfilerFormatFields;
#endif


// Global variable for configuration
extern ConfigParams_t ConfigParams;
bool AbortSignalCaught;
bool ForceRotation;

#define MAX_FIELD_SIZE 2048		/* We do not expect a field larger than this value */


// Function prototypes
void SignalHandler(int Signal);
#ifndef _MSC_VER
void CatchRotationRequest(int Signal);
#endif

int main(int argc, char* argv[])
{
char ErrBuf[PCAP_ERRBUF_SIZE + 1] = "";
int RetVal;	// Generic Variable for return code

// Pcap related structures
char pcapSourceName[2048] = "";
pcap_t *PcapHandle = NULL;

// Data for parsing packets and extracting fields
nbPacketEngine *PacketEngine;
nbExtractedFieldsReader* FieldReader;
_nbExtractedFieldsDescriptorVector *DescriptorVector;
nbNetPDLUtils* NetPDLUtils;
nbNetVMCreationFlag_t CreationFlag;
int PacketCounter;
int AcceptedPkts;
char CurrentFileName[1024];
int CurrentFileNumber = 0; // this variable is used also when dumping to SQL databases
FILE *OutputFile = NULL;
CFieldPrinter FieldPrinter;

#ifdef ENABLE_SQLITE3
sqlite3* pSQLite3DB= NULL;
char *SQLDB_FilenameFormat = NULL; // stores the format of the subsequent SQLDBCurrentFilename afetr the first
char *SQLDBCurrentFilename = NULL; // this can be NULL or a malloc'd string
char SQLCommandBuffer[SQLCOMMAND_MAX_LEN];
int SQLCommandBufferOccupancy= 0;
#endif

// Defines IP anonymization data structures and generate pseudo-random seed for IP anonymization masks
AnonymizationMapTable_t AnonymizationIPTable;
AnonymizationArgumentList_t AnonymizationIPArgumentList;


	srand((unsigned int) time(NULL));

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	fprintf(stderr, "\nLoading NetPDL protocol database...\n");

	if (ConfigParams.NetPDLFileName)
	{
		RetVal= nbInitialize(ConfigParams.NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf) );

		if (RetVal == nbFAILURE)
		{
			fprintf(stderr, "Error initializing the NetBee Library: %s\n", ErrBuf);
			fprintf(stderr, "Trying to use the NetPDL database embedded in the NetBee library instead.\n");
		}
	}

	// In case the NetBee library has not been initialized
	// initialize right now with the embedded NetPDL protocol database instead
	// Previous initialization may fail because the NetPDL file has errors, or because it is missing,
	// or because the user didn't specify a NetPDL file
	if (nbIsInitialized() == nbFAILURE)
	{
		if (nbInitialize(NULL, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf)) == nbFAILURE)
		{
			fprintf(stderr, "Error initializing the NetBee Library: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	fprintf(stderr, "NetPDL Protocol database loaded.\n");
	
	PacketEngine= nbAllocatePacketEngine(ConfigParams.UseJit, ErrBuf, sizeof(ErrBuf));
	if (PacketEngine == NULL)
	{
		fprintf(stderr, "Error retrieving the PacketEngine: %s", ErrBuf);
		return nbFAILURE;
	}

	fprintf(stderr, "Compiling filter '%s'...\n", ConfigParams.FilterString);
	RetVal= PacketEngine->Compile(ConfigParams.FilterString, nbNETPDL_LINK_LAYER_ETHERNET, ConfigParams.Backends[0].Optimization);
		if (RetVal == nbFAILURE)
	{
		fprintf(stderr, "Error compiling the filter '%s': %s",ConfigParams.FilterString,PacketEngine->GetLastError());
		return nbFAILURE;
	}

	if ((ConfigParams.StopAfterDumpCode) && (ConfigParams.Backends[1].Id == -1))
	{
		printf("\n\n\nNetIL code\n===== ===== ===== =====\n\n");
		printf("%s", PacketEngine->GetCompiledCode());

		printf("\n===== ===== ===== =====\n");
		return nbSUCCESS;
	}

	if (ConfigParams.Backends[1].Id >= 0)
		// in this case, you should just compile the code and quit
		CreationFlag= nbNETVM_CREATION_FLAG_COMPILEONLY;
	else
		// in this case, you should compile the code and execute it
		CreationFlag= nbNETVM_CREATION_FLAG_COMPILEANDEXECUTE;

	if (PacketEngine->InitNetVM(CreationFlag)== nbFAILURE)
	{
		fprintf(stderr, "Error initializing the netVM : %s", PacketEngine->GetLastError());
		return nbFAILURE;
	}

	if (CreationFlag == nbNETVM_CREATION_FLAG_COMPILEONLY)
	{
		if (PacketEngine->GenerateBackendCode(ConfigParams.Backends[1].Id,
			ConfigParams.Backends[1].Optimization,
			ConfigParams.Backends[1].Inline,
			ConfigParams.DumpCodeFilename))
		{
			printf("NetVM Runtime - Error initializing the runtime environment: %s\n", PacketEngine->GetLastError());
			nbCleanup();
			return nbFAILURE;
		}
		else
		{
			printf("\n\n\nBackend code\n===== ===== ===== =====\n\n");
		}
	}

	// prefix+namepe(netpe0)+.push.s
	if ((ConfigParams.StopAfterDumpCode) && (ConfigParams.Backends[1].Id >= 0))
	{
		char filename[255];

		if (ConfigParams.Backends[1].Inline==0)
			sprintf(filename, "%sNetPE0.push.s", ConfigParams.DumpCodeFilename);
		else
			sprintf(filename, "%sglobalCFG.s", ConfigParams.DumpCodeFilename);
		FILE *f = fopen(filename, "r");

		if (f)
		{
			char line[255];
			while (1)
			{
				fgets(line, 255, f);

				if (feof(f))
					break;

				printf("%s", line);
			}
		}
		printf("\n===== ===== ===== =====\n");

		return nbSUCCESS;
	}


	if (ConfigParams.CaptureFileName)
	{
		// Let's set the source file
		if ((PcapHandle= pcap_open_offline(ConfigParams.CaptureFileName, ErrBuf)) == NULL)
		{
			fprintf(stderr, "Cannot open the capture source file: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}
	else
	{
		// Let's set the device
		if ((PcapHandle= pcap_open_live(ConfigParams.AdapterName, 65535 /*snaplen*/,
			ConfigParams.PromiscuousMode, 1000 /* read timeout, ms */, ErrBuf)) == NULL)
		{
			fprintf(stderr, "Cannot open the capture interface: %s\n", ErrBuf);
			return nbFAILURE;
		}
	}

	if (ConfigParams.CaptureFileName == NULL)
		fprintf(stderr, "Starting packet capture from interface: %s \n\n", ConfigParams.AdapterName);
	else
		fprintf(stderr, "Reading packets from file: %s \n\n", ConfigParams.CaptureFileName);

	fprintf(stderr, "===============================================================================\n");


	// Registering a signal handler, so that if we press ctrl+C 
	// we'll exit from the application gracefully
	signal(SIGINT, &SignalHandler);			// Ctrl+C

#ifndef _MSC_VER
	// register an handler for a forced output file/sql database rotation
	signal(SIGHUP, &CatchRotationRequest);
#endif

	// Initialize packet counters
	PacketCounter= 0;
	AcceptedPkts= 0;

	// Initialize classes for packet processing
	NetPDLUtils= nbAllocateNetPDLUtils(ErrBuf, sizeof(ErrBuf));
	FieldReader= PacketEngine->GetExtractedFieldsReader();

	// Get the list of fields we need to extract
	DescriptorVector= FieldReader->GetFields();

	// Loop across the fields that have to be extracted, and check if they are associated to a fast printing code
	// In that case, store that code for later use
	for (int j= 0; j < DescriptorVector->NumEntries; j++)
	{
		int RetVal;
		int FastPrintingFunctionCode;

		RetVal= NetPDLUtils->GetFastPrintingFunctionCode(DescriptorVector->FieldDescriptor[j].Proto, DescriptorVector->FieldDescriptor[j].Name, &FastPrintingFunctionCode);

		switch (RetVal)
		{
		case nbSUCCESS:
			{
				DescriptorVector->FieldDescriptor[j].UserExtension= (void*) FastPrintingFunctionCode;
			}; break;

		case nbWARNING:
			// Do nothing; the field is not associated to any fast printing function
			break;

		case nbFAILURE:
			{
				fprintf(stderr, "%s", NetPDLUtils->GetLastError());
			}; break;
		}
	}

#ifdef PROFILING
	// We allocate there three profiler instances:
	// - the first measures processing time
	// - the second measures the printing time
	// - the third measures only the time required to format a field (from the hext dump to a printable form)
	nbProfiler* ProfilerProcess= nbAllocateProfiler(ErrBuf, sizeof(ErrBuf));
	nbProfiler* ProfilerPrint= nbAllocateProfiler(ErrBuf, sizeof(ErrBuf));
	ProfilerFormatFields= nbAllocateProfiler(ErrBuf, sizeof(ErrBuf));
	int64_t StartTime, EndTime;

#ifdef ENABLE_SQLITE3
        // variables used in SQLITE3 code
        uint64_t beforeTransaction = 0;
        uint64_t afterTransaction = 0;
#endif

        // variables related to IP anonymization
        uint64_t beforeIPInit = 0;
        uint64_t afterIPInit = 0;

	// We process max PROFILER_MAX_PACKETS packets, repeating the measure (for each packet) one time,
	//   and discarding no samples at the beginning, and none among biggest/smallest samples
	if (ProfilerProcess->Initialize(PROFILER_MAX_PACKETS) == nbFAILURE)
		printf("\n%s\n", ProfilerProcess->GetLastError());

	if (ProfilerPrint->Initialize(PROFILER_MAX_PACKETS) == nbFAILURE)
		printf("\n%s\n", ProfilerPrint->GetLastError());

	if (ProfilerFormatFields->Initialize(PROFILER_MAX_PACKETS) == nbFAILURE)
		printf("\n%s\n", ProfilerFormatFields->GetLastError());
#endif

#ifdef ENABLE_SQLITE3
	if (ConfigParams.SQLDatabaseFileBasename)
	{
          // store the current name...
          SQLDBCurrentFilename= strdup(ConfigParams.SQLDatabaseFileBasename);
          // and the format for the future ones (base_%05d.ext)
          const int format_length = strlen(ConfigParams.SQLDatabaseFileBasename)+5+1;  // strlen() + strlen("_%05d") + strlen("\0")
          SQLDB_FilenameFormat = (char*) malloc( format_length );
          char *lastDot = strrchr(ConfigParams.SQLDatabaseFileBasename, '.');
          if(lastDot) {
            memcpy(SQLDB_FilenameFormat, ConfigParams.SQLDatabaseFileBasename, lastDot - ConfigParams.SQLDatabaseFileBasename);
            snprintf(SQLDB_FilenameFormat + (lastDot - ConfigParams.SQLDatabaseFileBasename),
                     format_length - (lastDot - ConfigParams.SQLDatabaseFileBasename),
                     "_%%05d%s",
                     lastDot);
          }
          else
            snprintf(SQLDB_FilenameFormat, format_length, "%s_%%05d", ConfigParams.SQLDatabaseFileBasename);

          RetVal = SQL_open(SQLDBCurrentFilename, &ConfigParams, &pSQLite3DB, DescriptorVector);
          if (RetVal == nbFAILURE)
            {
              fprintf(stderr, "Failed to create new table and no existing table (same schema) in database.\n");
              goto cleanup;
            }
	}
#endif

	if (ConfigParams.IPAnonFileName != NULL)
	{
#ifdef PROFILING
          fprintf(stderr, "Initialization of the IP anonymizer...\n");
          beforeIPInit = nbProfilerGetMicro();
#endif
          int IPAnonymizerResult = InitializeIPAnonymizer(AnonymizationIPTable, AnonymizationIPArgumentList, DescriptorVector, ErrBuf, sizeof(ErrBuf));
#ifdef PROFILING
          afterIPInit = nbProfilerGetMicro();
          fprintf(stderr, "IP anonymizer initialization complete\n\n");
#endif
		if (IPAnonymizerResult  == nbFAILURE)
		{
			fprintf(stderr, "Failed initialize the anonymizer module: %s.\n", ErrBuf);
			goto cleanup;
		}
	}

	// Set output file descriptor for packets
	if (ConfigParams.SaveFileName == NULL)
		OutputFile= stdout;
	else
	{
		if (ConfigParams.RotateFiles)
		{
                  // I prefer to always begin from #1 if the rotation is somehow forcefully enforced
                  CurrentFileNumber = 1;

			snprintf(CurrentFileName, sizeof(CurrentFileName)/sizeof(char), "%s%d", ConfigParams.SaveFileName, CurrentFileNumber);
			CurrentFileName[sizeof(CurrentFileName)/sizeof(char) - 1 ] = 0;

			OutputFile= fopen(CurrentFileName, "w");
		}
		else
			OutputFile= fopen(ConfigParams.SaveFileName, "w");

		if (OutputFile == NULL)
		{
			fprintf(stderr, "\n\nError opening the dump file for saving the extracted fields");
			return nbFAILURE;
		}
		else
		{
			// Print metainfo to describe thoroughly the capture environment
			char hostname[1024];
			hostname[1023] = '\0';

			// get the host name from the operating system
			gethostname(hostname, 1023);
			fprintf(OutputFile, "# Capturing node: %s\n", hostname);

			time_t rawtime;
			time(&rawtime);
			fprintf(OutputFile, "# Start time: %s", ctime(&rawtime));

			if (ConfigParams.CaptureFileName)
				fprintf(OutputFile, "# Capture file: %s\n", ConfigParams.CaptureFileName);
			else
				fprintf(OutputFile, "# Capture interface: %s\n", ConfigParams.AdapterName);
			if (ConfigParams.NetPDLFileName)
				fprintf(OutputFile, "# NetPDL database: %s\n", ConfigParams.NetPDLFileName);
			else
				fprintf(OutputFile, "# NetPDL database: embedded\n");
			if (ConfigParams.FilterString)
				fprintf(OutputFile, "# Filter string: %s\n", ConfigParams.FilterString);
			else
				fprintf(OutputFile, "# Filter string: ip extractfields(ip.src,ip.dst)\n");

			fprintf(OutputFile, "\n");
		}
	}

	// Write the header in the output file
	switch (ConfigParams.PrintingMode)
	{
	case CP:
		{
			fprintf(OutputFile, "\n#Packet#, Timestamp, Field1, Field2, ... \n");
			fprintf(OutputFile, "#-------------------------------------------------------------------------------");
		}; break;

	case SCP:
		{
			fprintf(OutputFile, "\n#Field1, Field2, ... \n");
			fprintf(OutputFile, "#-------------------------------------------------------------------------------");
		}; break;

	case SCPT:
		{
			fprintf(OutputFile, "\n#Timestamp, Field1, Field2, ... \n");
			fprintf(OutputFile, "#-------------------------------------------------------------------------------");
		}; break;

	case SCPN:
		{
			fprintf(OutputFile, "\n#Packet#, Field1, Field2, ... \n");
			fprintf(OutputFile, "#-------------------------------------------------------------------------------");
		}; break;

	case DEFAULT:
	default:
		break;
	}


	// Initialize some data that will be used later when processing packets
	FieldPrinter.Initialize(NetPDLUtils, ConfigParams.PrintingMode, AnonymizationIPTable, AnonymizationIPArgumentList, OutputFile);

#ifdef ENABLE_SQLITE3
	RetVal= PrepareAddNewDataRecordSQLCommand(pSQLite3DB, ConfigParams.SQLTableName, DescriptorVector, SQLCommandBuffer, sizeof(SQLCommandBuffer), SQLCommandBufferOccupancy);
	if (RetVal == nbFAILURE)
	{
		fprintf(stderr, "Internal error: the buffer that contains SQL commands is too small.\n");
		goto cleanup;
	}
#endif

#ifdef ENABLE_SQLITE3
	// Start first transaction
	if (ConfigParams.SQLTransactionSize)
		sqlite3_exec(pSQLite3DB, "BEGIN", NULL, NULL, NULL);

        // generate an identifier that is unique (... sort of) for this run
        char run_id[100];
        snprintf(run_id, sizeof(run_id), "%ld", time(NULL));
#endif

        ForceRotation = false;
	while (!AbortSignalCaught)
	{
	struct pcap_pkthdr *PktHeader;
	const unsigned char *PktData;
	int RetVal;

		RetVal= pcap_next_ex(PcapHandle, &PktHeader, &PktData);

		if (RetVal == -2)
			break;		// capture file ended

		if (RetVal < 0)
		{
			fprintf(stderr, "Cannot read packet: %s\n", pcap_geterr(PcapHandle));
			return nbFAILURE;
		}

		// Timeout expired
		if (RetVal == 0)
			continue;

		PacketCounter++;
#ifdef PROFILING
		StartTime= nbProfilerGetTime();
#endif
		RetVal= PacketEngine->ProcessPacket(PktData, PktHeader->len);
#ifdef PROFILING
		EndTime= nbProfilerGetTime();
		ProfilerProcess->StoreSample(StartTime, EndTime);
#endif
		if (RetVal != nbFAILURE)
		{
#ifdef PROFILING
			StartTime= nbProfilerGetTime();
#endif

			AcceptedPkts++;

#ifdef ENABLE_SQLITE3
			if (SQLDBCurrentFilename)
			{
			int SQLCommandBufferNewOccupancy;
			char Timestamp[128];
			int i;

				// Move SQLCommandBufferOccupancy into a new variable, so that the original value won't be changed
				// and it will be used also in the next rounds
				SQLCommandBufferNewOccupancy= SQLCommandBufferOccupancy;
				
				// Format timestamp
				ssnprintf(Timestamp, sizeof(Timestamp), "%ld.%ld", PktHeader->ts.tv_sec, PktHeader->ts.tv_usec);

				sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, "\"");
				sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, Timestamp);
				sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, "\",");

				for (i= 0; i < DescriptorVector->NumEntries; i++)
				{
					FieldPrinter.PrintField(DescriptorVector->FieldDescriptor[i], i, PktData);

					sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, "\"");
					sstrncatescape_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, '"', '"', FieldPrinter.GetFormattedField() );
					sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, "\"");

					if (i < (DescriptorVector->NumEntries - 1))
						sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, ",");
					else
						sstrncat_ex(SQLCommandBuffer, sizeof(SQLCommandBuffer), &SQLCommandBufferNewOccupancy, ")");
				}

				AddNewDataRecord(pSQLite3DB, SQLCommandBuffer);

				// Check if transaction has to be committed
				if ((ConfigParams.SQLTransactionSize) && (AcceptedPkts % ConfigParams.SQLTransactionSize == 0))
				{
					sqlite3_exec(pSQLite3DB, "COMMIT", NULL, NULL, NULL);
					sqlite3_exec(pSQLite3DB, "BEGIN", NULL, NULL, NULL);
				}
			}
			else
			{
#endif
				switch (ConfigParams.PrintingMode)
				{
				case DEFAULT:
					{
						fprintf(OutputFile, "Packet %d Timestamp %ld.%ld\n", PacketCounter, PktHeader->ts.tv_sec, PktHeader->ts.tv_usec);
					}; break;

				case CP:
					{
						fprintf(OutputFile, "\n%d, %ld.%ld, ", PacketCounter, PktHeader->ts.tv_sec, PktHeader->ts.tv_usec);
					}; break;

				case SCP:
					{
						fprintf(OutputFile, "\n");
					}; break;

				case SCPT:
					{
						fprintf(OutputFile, "\n%ld.%ld, ", PktHeader->ts.tv_sec, PktHeader->ts.tv_usec);
					}; break;

				case SCPN:
					{
						fprintf(OutputFile, "\n%d, ", PacketCounter);
					}; break;

				default:
					break;
				}

				// Do not write to database then write to screen
				for (int j= 0; j < DescriptorVector->NumEntries; j++)
					FieldPrinter.PrintField(DescriptorVector->FieldDescriptor[j], j, PktData);
#ifdef ENABLE_SQLITE3
			}
#endif

#ifdef PROFILING
			EndTime= nbProfilerGetTime();
			ProfilerPrint->StoreSample(StartTime, EndTime);
#endif
		}

		// Check if the user wanted to capture max N packets
		if ((ConfigParams.NPackets != 0) && (PacketCounter == ConfigParams.NPackets))
			break;

		if ( ForceRotation ||
                     ((ConfigParams.RotateFiles) && (AcceptedPkts % ConfigParams.RotateFiles == 0)) )
		{
			CurrentFileNumber++;
                        ForceRotation = false;
                        if (ConfigParams.SaveFileName) {
                          snprintf(CurrentFileName, sizeof(CurrentFileName)/sizeof(char), "%s%d", ConfigParams.SaveFileName, CurrentFileNumber);
                          CurrentFileName[sizeof(CurrentFileName)/sizeof(char) -1 ] = 0;

                          fclose(OutputFile);
                          OutputFile = fopen(CurrentFileName, "w");
                          if (OutputFile == NULL)
                            {
                              fprintf(stderr, "\n\nError opening the dump file for saving the extracted fields");
                              return nbFAILURE;
                            }
                        }
#ifdef ENABLE_SQLITE3
                        else if (SQLDBCurrentFilename) {
                          // ... argh ... we need to do a number of things

                          // 1) commit the outstanding transaction, if needed 
                          if (ConfigParams.SQLTransactionSize)
                            sqlite3_exec(pSQLite3DB, "COMMIT", NULL, NULL, NULL);

                          // 2) close the old file
                          UpdateExtraDBTable(pSQLite3DB, AcceptedPkts, ConfigParams, SQLDBCurrentFilename, run_id);
                          sqlite3_close(pSQLite3DB);

                          // 3) generate a new filename
                          free(SQLDBCurrentFilename);
                          int len = strlen(ConfigParams.SQLDatabaseFileBasename);
                          SQLDBCurrentFilename = (char*) malloc ( len + 6 + 1 ); // len + strlen("_XXXXX") + strlen("\0")
                          snprintf(SQLDBCurrentFilename, len+6+1, SQLDB_FilenameFormat, CurrentFileNumber);

                          // 4) open the new file and handle errors
                          RetVal = SQL_open(SQLDBCurrentFilename, &ConfigParams, &pSQLite3DB, DescriptorVector);
                          if (RetVal == nbFAILURE) {
                            fprintf(stderr, "Failed to create new table and no existing table (same schema) in database.\n");
                            goto cleanup;
                          }
                          
                          // 5) prepare add
                          SQLCommandBufferOccupancy = 0;
                          RetVal= PrepareAddNewDataRecordSQLCommand(pSQLite3DB, ConfigParams.SQLTableName, DescriptorVector, SQLCommandBuffer, sizeof(SQLCommandBuffer), SQLCommandBufferOccupancy);
                          if (RetVal == nbFAILURE) {
                            fprintf(stderr, "Internal error: the buffer that contains SQL commands is too small.\n");
                            goto cleanup;
                          }

                          // 6) start transaction, if needed
                          if (ConfigParams.SQLTransactionSize)
                            sqlite3_exec(pSQLite3DB, "BEGIN", NULL, NULL, NULL);
                        }
#endif // #ifdef ENABLE_SQLITE3
		}
	}

#ifdef ENABLE_SQLITE3
#ifdef PROFILING
	beforeTransaction = nbProfilerGetMicro();
#endif // ifdef PROFILING

	// Commit last transaction
	if (ConfigParams.SQLTransactionSize)
		sqlite3_exec(pSQLite3DB, "COMMIT", NULL, NULL, NULL);

#ifdef PROFILING
	afterTransaction = nbProfilerGetMicro();
#endif // ifdef PROFILING
#endif

	fprintf(OutputFile, "\n");

	fprintf(OutputFile, "\n\n#The filter accepted %d out of %d packets\n", AcceptedPkts, PacketCounter);
	// Print this info also on the screen, just in case
	if (OutputFile != stdout)
		fprintf(stderr, "\n\nThe filter accepted %d out of %d packets\n", AcceptedPkts, PacketCounter);

	fflush(OutputFile);

	// If we are writing on file, close file descriptor
	if (OutputFile != stdout)
		fclose(OutputFile);

#ifdef PROFILING
	char ResultBuffer[10000];

	ProfilerPrint->ProcessProfilerData();
	ProfilerProcess->ProcessProfilerData();
	ProfilerFormatFields->ProcessProfilerData();

	if (ProfilerProcess->ProcessProfilerData() == nbFAILURE)
		printf("\n%s\n", ProfilerProcess->GetLastError());
	if (ProfilerPrint->ProcessProfilerData() == nbFAILURE)
		printf("\n%s\n", ProfilerPrint->GetLastError());
	if (ProfilerFormatFields->ProcessProfilerData() == nbFAILURE)
		printf("\n%s\n", ProfilerFormatFields->GetLastError());

	printf("\n==========================================================");
	printf("\nProfiling results: packet processing time \n");
	printf("==========================================================\n");
	if (ProfilerProcess->FormatResults(ResultBuffer, sizeof(ResultBuffer), true) == nbSUCCESS)
		printf("\n%s\n", ResultBuffer);
	else
		printf("\n%s\n", ProfilerProcess->GetLastError());

#ifdef PROFILING_DUMP_RAW_DATA
	// Print all samples, if needed
	int64_t *StartTicks, *EndTicks;
	ProfilerProcess->GetRawSamples(StartTicks, EndTicks);
	printf("\nDump of the measured samples (processing time)\n");
	for (int j= 0; j < ProfilerProcess->GetNumberOfSamples(); j++)
	{
		//        printf("%.5d: %llu\n", j, EndTicks[j] - StartTicks[j]);
		printf("%.5d: %lu\n", j, EndTicks[j] - StartTicks[j]);
	}
#endif

	printf("\n==========================================================");
	printf("\nProfiling results: formatting processing time\n");
	printf("==========================================================\n");
	if (ProfilerFormatFields->FormatResults(ResultBuffer, sizeof(ResultBuffer), true) == nbSUCCESS)
		printf("\n%s\n", ResultBuffer);
	else
		printf("\n%s\n", ProfilerFormatFields->GetLastError());

	printf("\n==========================================================");
	printf("\nProfiling results: formatting and printing processing time\n");
	printf("==========================================================\n");
	if (ProfilerPrint->FormatResults(ResultBuffer, sizeof(ResultBuffer), true) == nbSUCCESS)
		printf("\n%s\n", ResultBuffer);
	else
		printf("\n%s\n", ProfilerPrint->GetLastError());

	printf("\n==========================================================");
	printf("\nProfiling results about anonymization\n");
	printf("==========================================================\n");
        printf("Init globally took %lu usecs\n", afterIPInit - beforeIPInit );

#ifdef ENABLE_SQLITE3
        printf("\n==========================================================");
        printf("\nProfiling results about SQL usage\n");
        printf("==========================================================\n");
        printf("SQL Transaction took %lu usecs\n", afterTransaction - beforeTransaction );
#endif

	nbDeallocateProfiler(ProfilerProcess);
	nbDeallocateProfiler(ProfilerPrint);
	nbDeallocateProfiler(ProfilerFormatFields);
#endif

#ifdef ENABLE_SQLITE3

	if (SQLDBCurrentFilename)
	{
          UpdateExtraDBTable(pSQLite3DB, AcceptedPkts, ConfigParams, SQLDBCurrentFilename, run_id);
		sqlite3_close(pSQLite3DB);
	}
#endif

cleanup:
	nbDeallocatePacketEngine(PacketEngine);
	nbCleanup();

	return nbSUCCESS;
}



// Called when a signal is caught. We set a boolean flag that is
// tested each new packet, so that in this case we terminate gracefully.
void SignalHandler(int Signal)
{
	AbortSignalCaught= true;
}


#ifndef _MSC_VER
void CatchRotationRequest(int Signal)
{
  ForceRotation= true;
  signal(SIGHUP, &CatchRotationRequest); // for portability issues
}
#endif

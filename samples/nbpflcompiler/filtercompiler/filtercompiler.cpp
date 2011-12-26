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


#include <nbpflcompiler.h>
// If we want to use the NetPFL filter compiler module, we need to include
// also some primitives defined in the NetPDL protocol dababase module, which
// creates the protocol database (that is one of the input of the
// NetPFL compiler)
#include <nbprotodb.h>


#define DEFAULT_NETPDL_DATABASE "./netpdl.xml"
#define DEFAULT_INPUT_STRING "ip.src == 10.0.0.1"
#define DEFAULT_OUTPUT_FILE "stdout"


const char *NetPDLFileName= DEFAULT_NETPDL_DATABASE;
const char *FilterString= DEFAULT_INPUT_STRING;
const char *OutputFilename= DEFAULT_OUTPUT_FILE;
FILE *OutputFile= stdout;
bool CompileOnly = false;


void Usage()
{
char string[]= \
	"nUsage:\n"
	"  filtercompiler [options] [filterstring]\n\n"
	"Example:\n"
	"  filtercompiler -netpdl netpdl.xml \"ip.src == 10.0.0.1\"\n\n"
	"Options:\n"
	" -netpdl FileName: name (and *path*) of the file containing the NetPDL\n"
	"     description. In case it is omitted, the NetPDL description embedded\n"
	"     within the NetBee library will be used.\n"
	" -compileonly: in this case only the NetPFL syntax is checked, but no NetIL\n"
	"     code is actually generated. It can be used to check if the first step\n"
	"     of the compiler behaves correctly and if the NetPFL string is correct.\n"
	" -o Filename: write the netil code to FileName (default: stdout)\n"
	" filterstring: a string containing the filter to be compiled, using the\n"
	"     NetPFL syntax. Default: we use the '" DEFAULT_INPUT_STRING "' filter.\n"
	" -h: prints this help message.\n\n"
	"Description\n"
	"============================================================================\n"
	"This program can be used to see how to interact with the NetPFLCompiler\n"
	"module. It prints to console the NetIL code generated from NetPFL filtering\n"
	"rule, e.g. '" DEFAULT_INPUT_STRING "', by exploiting the protocol format\n"
	"information provided in the given NetPDL protocol database.\n\n";

	fprintf(stderr, "%s", string);
}


int ParseCommandLine(int argc, char *argv[])
{
int CurrentItem;
	
	CurrentItem= 1;

	while (CurrentItem < argc)
	{
		if (strcmp(argv[CurrentItem], "-netpdl") == 0)
		{
			NetPDLFileName= argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-o") == 0)
		{
			OutputFilename = argv[CurrentItem+1];
			CurrentItem+= 2;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-compileonly") == 0)
		{
			CompileOnly= true;
			CurrentItem++;
			continue;
		}

		if (strcmp(argv[CurrentItem], "-h") == 0)
		{
			Usage();
			return nbFAILURE;
		}

		// This should be the filter string
		if (argv[CurrentItem][0] != '-')
		{
			FilterString= argv[CurrentItem];
			CurrentItem++;
			continue;
		}

		printf("Error: parameter '%s' is not valid.\n", argv[CurrentItem]);
		return nbFAILURE;
	}

	return nbSUCCESS;
}




int main(int argc, char *argv[])
{
char ErrBuf[2048];
int RetVal;
char *GeneratedCode= NULL;
nbNetPFLCompiler *NetPFLCompiler;

	if (ParseCommandLine(argc, argv) == nbFAILURE)
		return nbFAILURE;

	if (strcmp(OutputFilename, DEFAULT_OUTPUT_FILE) != 0)
	{
		if ((OutputFile = fopen(OutputFilename, "w")) == NULL)
		{
			fprintf(stderr, "\nError opening the output file %s\n", OutputFilename);
			return 1;
		}

	}

	fprintf(stderr, "\n\nLoading NetPDL protocol database '%s'...\n", NetPDLFileName);

	// Get the object containing the NetPDL protocol database
	// This function belongs to the NetPDL Protocol Database module
	struct _nbNetPDLDatabase *NetPDLProtoDB= nbProtoDBXMLLoad(NetPDLFileName, nbPROTODB_FULL, ErrBuf, sizeof(ErrBuf));
	if (NetPDLProtoDB == NULL)
	{
		fprintf(stderr, "Error loading the NetPDL protocol Database: %s\n", ErrBuf);
		return 0;
	}

	fprintf(stderr, "NetPDL Protocol database loaded.\n");

	// Instantiates the NetPFL compiler using the previously loaded protocol database
	NetPFLCompiler= nbAllocateNetPFLCompiler(NetPDLProtoDB);

	// Initialize the compiler assuming we are filtering ethernet frames
	RetVal= NetPFLCompiler->NetPDLInit(nbNETPDL_LINK_LAYER_ETHERNET);
	
	if (RetVal == nbSUCCESS)
	{
		fprintf(stderr, "NetPFL Compiler successfully initialized\n");
	}
	else
	{
	int i=1;
	_nbNetPFLCompilerMessages *Message= NetPFLCompiler->GetCompMessageList();

		fprintf(stderr, "Unable to initialize the NetPFL compiler: %s\n\tError messages:\n", NetPFLCompiler->GetLastError());

		while (Message)
		{
			printf("\t%3d. %s\n", i, Message->MessageString);
			i++;
			Message= Message->Next;
		}
		return nbFAILURE;
	}

	fprintf(stderr, "Compiling filter '%s'...\n", FilterString);

	if (CompileOnly)
	{
		// Compiles only the filter, without actually generating the NetIL code
		RetVal= NetPFLCompiler->CheckFilter(FilterString);
	}
	else
	{
		// Compiles the filter and generates NetIL code
		RetVal= NetPFLCompiler->CompileFilter(FilterString, &GeneratedCode);
	}
	
	if (RetVal == nbSUCCESS)
	{
		fprintf(stderr, "NetPFL filter successfully compiled\n");
	}
	else
	{
	int i=1;
	_nbNetPFLCompilerMessages *Message= NetPFLCompiler->GetCompMessageList();

		fprintf(stderr, "Unable to initialize the NetPFL compiler: %s\n\tError messages:\n", NetPFLCompiler->GetLastError());

		while (Message)
		{
			fprintf(stderr, "\t%3d. %s\n", i++, Message->MessageString);
			Message= Message->Next;
		}
		return nbFAILURE;
	}

	if (!CompileOnly)
	{
		if (strcmp(OutputFilename, DEFAULT_OUTPUT_FILE) == 0)
			fprintf(stdout, "\nDumping the generated NetIL code:\n");
		fprintf(OutputFile, "%s", GeneratedCode);
	}

	nbDeallocateNetPFLCompiler(NetPFLCompiler);
	// This function belongs to the NetPDL Protocol Database module
	nbProtoDBXMLCleanup();

	fclose(OutputFile);
	return nbSUCCESS;
}

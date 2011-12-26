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

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>

#define MAX_FIELD_SIZE 2048		/* We do not expect a field larger than this value */

// The following allows to print some global statistics about processing time and such
//#define PROFILING
// The following allows to print the time taken by all samples
// Take care: when you define PROFILING_DUMP_RAW_DATA, the previous DEFINE musts be active
//#define PROFILING_DUMP_RAW_DATA

// Global variables for configuration
struct _Backend
{
	int		Id;
	bool	Optimization;
	bool	Inline;
};
typedef struct _Backend Backend_t;


// Defines the type of compact printing we want
typedef enum
{
#ifdef ENABLE_SQLITE3
	DEFAULT = 0,		//!< Default printing (fields names, offset, value)
	CP,					//!< Compact printing (one line per packet).
	SCP,				//!< Super-compact printing (one line per packet, no other data)
	SCPT,				//!< Super-compact printing, but with the timestamp
	SCPN,				//!< Super-compact printing, but with the packet number
	SQLITE3				//!< Print on a SQLite3 database
#else
	DEFAULT = 0,		//!< Default printing (fields names, offset, value)
	CP,					//!< Compact printing (one line per packet).
	SCP,				//!< Super-compact printing (one line per packet, no other data)
	SCPT,				//!< Super-compact printing, but with the timestamp
	SCPN				//!< Super-compact printing, but with the packet number
#endif
} PrintingMode_t;


struct _ConfigParams
{
	char*		NetPDLFileName;
	char*		CaptureFileName;
	char*		SaveFileName;
	char*		FilterString;
	char		AdapterName[1024];
	int			PromiscuousMode;
	int			NPackets;
	bool		UseJit;
	int			RotateFiles;
	PrintingMode_t			PrintingMode;

	u_char		DumpCode;
	char		DumpCodeFilename[255];
	u_char		StopAfterDumpCode;
	u_int32_t	NBackends;
	Backend_t	Backends[2];

	char*		IPAnonFileName;
	char*		IPAnonFieldsList;

#ifdef ENABLE_SQLITE3
        char*		SQLDatabaseFileBasename; // this is only a template, optional chars might be appended by nbextractor, see its code
	char*		SQLTableName;
	int			SQLTransactionSize;
#endif

};
typedef struct _ConfigParams ConfigParams_t;


// Prototypes
void Usage();
int ParseCommandLine(int argc, char *argv[]);

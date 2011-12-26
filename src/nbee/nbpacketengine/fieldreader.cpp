/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/

#include "nbeefieldreader.h"
#include <iostream>


nbeeFieldReader::nbeeFieldReader(_nbExtractedFieldsDescriptorVector* ndescriptorVct, unsigned char* dataInfo, nbNetPFLCompiler *compiler):
FieldDescriptorsVector(ndescriptorVct), DataInfo(dataInfo), m_Compiler(compiler), FieldVector(FieldDescriptorsVector->NumEntries, 0)
{
string FieldName;

	for (int i=0; i < FieldDescriptorsVector->NumEntries; i++)
	{
		FieldName= string(FieldDescriptorsVector->FieldDescriptor[i].Proto)+"."+ string(FieldDescriptorsVector->FieldDescriptor[i].Name);
		FieldNamesTable[FieldName]= i;
		ProtocolFieldNames.push_back(FieldName);
		FieldName.erase(0, FieldName.length());


		FieldVector[i] = 1 << FieldDescriptorsVector->FieldDescriptor[i].DataFormatType;

		if (FieldDescriptorsVector->FieldDescriptor[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO)
			FieldVector[i] = FieldVector[i] | nbNETPFLCOMPILER_MAX_PROTO_INSTANCES << 12;

		else if(FieldDescriptorsVector->FieldDescriptor[i].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD)
			FieldVector[i] = FieldVector[i] | nbNETPFLCOMPILER_MAX_FIELD_INSTANCES << 8;
	}
}


nbeeFieldReader::~nbeeFieldReader()
{
	delete FieldDescriptorsVector;
}


_nbExtractedFieldsDescriptor* nbeeFieldReader::GetField(string FieldName)
{
int Index;

	Index= FieldNamesTable.count(FieldName);

	if (Index == 1)
		return &(FieldDescriptorsVector->FieldDescriptor[FieldNamesTable[FieldName]]);

	return NULL;
}


_nbExtractedFieldsDescriptor* nbeeFieldReader::GetField(int Index)
{
	if ((Index >= 0) && (Index < FieldDescriptorsVector->NumEntries))
		return &(FieldDescriptorsVector->FieldDescriptor[Index]);

	return NULL;
}


_nbExtractedFieldsDescriptorVector* nbeeFieldReader::GetFields()
{
	return FieldDescriptorsVector;
}


void nbeeFieldReader::FillDescriptors()
{	
	for (int index=0; index < FieldDescriptorsVector->NumEntries; index++)
	{
		if (FieldDescriptorsVector->FieldDescriptor[index].FieldType == PDL_FIELD_TYPE_ALLFIELDS)
		{
			int n = *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
			FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
			FieldDescriptorsVector->FieldDescriptor[index].DVct->NumEntries= n;

			if ((n > 0) && (FieldDescriptorsVector->FieldDescriptor[index].DVct != NULL))
			{
				
				for (int i=0; i < n; i++)
				{
					int id= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 2 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE_ALL];

					int res= m_Compiler->GetFieldInfo(FieldDescriptorsVector->FieldDescriptor[index].Proto, id, &FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i]);
					if (res == nbSUCCESS)
					{
						if (FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].FieldType == PDL_FIELD_TYPE_BIT)
						{
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].BitField_Value= *(uint16_t *) &(DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE_ALL]);
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid=true;
						}
						else
						{
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Offset=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE_ALL];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Length=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 6 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE_ALL];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid=true;
						}
					}
				}
			}
		}
		else
		{	
			if (FieldDescriptorsVector->FieldDescriptor[index].FieldType == PDL_FIELD_TYPE_FIXED)
			{
				if ((FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO) ||
					(FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD))
				{
					int n = *(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
					FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
					FieldDescriptorsVector->FieldDescriptor[index].DVct->NumEntries= n;
					if ((n > 0) && (FieldDescriptorsVector->FieldDescriptor[index].DVct))
					{
						for (int i=0; i < n; i++)
						{
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Offset=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Length=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 6 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid=false;
							if (FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Length>0)
								FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid=true;
						}
					}
				}
				else
				{
					FieldDescriptorsVector->FieldDescriptor[index].Offset=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
					FieldDescriptorsVector->FieldDescriptor[index].Length=*(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position+2];
					FieldDescriptorsVector->FieldDescriptor[index].Valid=false;
					if (FieldDescriptorsVector->FieldDescriptor[index].Length > 0)
						FieldDescriptorsVector->FieldDescriptor[index].Valid=true;
				}
			}

			else if(FieldDescriptorsVector->FieldDescriptor[index].FieldType == PDL_FIELD_TYPE_BIT)
			{
				if ((FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO) ||
					(FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD))
				{
					int n = *(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
					FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
					FieldDescriptorsVector->FieldDescriptor[index].DVct->NumEntries= n;
					if ((n > 0) && (FieldDescriptorsVector->FieldDescriptor[index].DVct))
					{
						for (int i=0; i < n; i++)
						{
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].BitField_Value= (*(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE])-0x80000000;
							if(((*(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE]) & 0x80000000) == 0x80000000)
								FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid= true;
						}
					}
				}
				else
				{	
					FieldDescriptorsVector->FieldDescriptor[index].BitField_Value= (*(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position])-0x80000000;
					if (((*(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position]) & 0x80000000) == 0x80000000)
						FieldDescriptorsVector->FieldDescriptor[index].Valid=true;
				}
			}

			else if(FieldDescriptorsVector->FieldDescriptor[index].FieldType == PDL_FIELD_TYPE_VARLEN)
			{
				if ((FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIPROTO) || (FieldDescriptorsVector->FieldDescriptor[index].DataFormatType == nbNETPFLCOMPILER_DATAFORMAT_MULTIFIELD))
				{
					int n = *(uint32_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
					FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
					FieldDescriptorsVector->FieldDescriptor[index].DVct->NumEntries= n;
					if ((n > 0) && (FieldDescriptorsVector->FieldDescriptor[index].DVct))
					{
						for (int i=0;i<n;i++)
						{
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Offset= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 4 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Length= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 6 + i * nbNETPFLCOMPILER_INFO_FIELDS_SIZE];
							FieldDescriptorsVector->FieldDescriptor[index].DVct->FieldDescriptor[i].Valid= true;
						}
					}
				}
				else
				{
					FieldDescriptorsVector->FieldDescriptor[index].Offset= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
					FieldDescriptorsVector->FieldDescriptor[index].Length= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 2];
					FieldDescriptorsVector->FieldDescriptor[index].Valid= false;
					if (FieldDescriptorsVector->FieldDescriptor[index].Length > 0)
						FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
				}
			}

			else
			{
				FieldDescriptorsVector->FieldDescriptor[index].Offset= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position];
				FieldDescriptorsVector->FieldDescriptor[index].Length= *(uint16_t *) &DataInfo[FieldDescriptorsVector->FieldDescriptor[index].Position + 2];
				FieldDescriptorsVector->FieldDescriptor[index].Valid= false;
				if (FieldDescriptorsVector->FieldDescriptor[index].Length > 0)
					FieldDescriptorsVector->FieldDescriptor[index].Valid= true;
			}
		}
	}
}


_nbExtractedFieldsNameList nbeeFieldReader::GetFieldNames()
{
		return ProtocolFieldNames;
}


int nbeeFieldReader::IsValid(_nbExtractedFieldsDescriptor *FieldDescriptor)
{
	if (FieldDescriptor->Valid)
			return nbSUCCESS;

	return nbFAILURE;
}


int nbeeFieldReader::IsComplete()
{
	for (int i=0; i < FieldDescriptorsVector->NumEntries; i++)
	{
	 if (IsValid(&FieldDescriptorsVector->FieldDescriptor[i]) == nbFAILURE)
		 return nbFAILURE;
	}
	return nbSUCCESS;
}


vector<uint16_t> nbeeFieldReader::GetFieldVector()
{
	return FieldVector;
}


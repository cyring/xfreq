/*
 * xfreq-smbios.c by CyrIng
 *
 * Copyright (C) 2012-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "xfreq-smbios.h"

struct STRING *SMB_Dig_Strings(struct PACKED *packed, int ID)
{
	char  *buffer=NULL;
	size_t rbyte=0;

	if(getdelim(&buffer, &rbyte, STRING_DELIMITER, packed->File) > 1)
	{
		unsigned int NodeSize=sizeof(struct STRING);
		struct STRING *pstr=malloc(NodeSize);

		pstr->Buffer=buffer;
		pstr->ID=++ID;
		pstr->Node.MemSize=NodeSize + strlen(pstr->Buffer) + 1;
/* Verify sum */
		if((pstr->Link=SMB_Dig_Strings(packed, ID)) == NULL)
			pstr->Node.MemSum=pstr->Node.MemSize;
		else
			pstr->Node.MemSum=pstr->Node.MemSize + pstr->Link->Node.MemSum;
/* End of sum */
		return(pstr);
	}
	else {
		free(buffer);
		return(NULL);
	}
}

struct STRING *SMB_Read_Strings(struct PACKED *packed)
{
	return((packed->Length == 0) ? SMB_Dig_Strings(packed, 0) : NULL);
}

int SMB_Read_Length(struct PACKED *packed)
{
	char    pathName[]="/sys/firmware/dmi/entries/999-99/file1234567890";

	sprintf(pathName, "/sys/firmware/dmi/entries/%d-%d/length", packed->Type, packed->Instance);
	if((packed->File=fopen(pathName, "r")) != NULL)
	{
		fscanf(packed->File, "%zd\n", &packed->Length);
		fclose(packed->File);
		packed->Length-=sizeof(struct HEADER);
		return(0);
	}
	else
		return(errno);
}

int SMB_Open_Structure(struct PACKED *packed)
{
	char    pathName[]="/sys/firmware/dmi/entries/999-99/file1234567890";

	sprintf(pathName, "/sys/firmware/dmi/entries/%d-%d/raw", packed->Type, packed->Instance);
	if((packed->File=fopen(pathName, "rb")) == NULL)
		return(errno);
	else
		return(0);
}

int SMB_Close_Structure(struct PACKED *packed)
{
	if(packed->File)
	{
		fclose(packed->File);
		packed->File=NULL;
	}
	return(errno);
}

struct STRUCTINFO *SMB_Read_Structure(struct PACKED *packed)
{
	struct STRUCTINFO *smb=NULL;
	unsigned int StructSize=sizeof(struct STRUCTINFO), HeaderSize=sizeof(struct HEADER), AttribSize=0;

	if((smb=calloc(1, StructSize)) != NULL)
	{
		unsigned int head=0;

		fread(&smb->Header, HeaderSize, 1, packed->File);

		while(packed->Tape[head] != _EOT_)
		{
			unsigned long long int poly=0;

			AttribSize=(head + 1) * sizeof(unsigned long long int);
			smb->Attrib=realloc(smb->Attrib, AttribSize);
			fread(&poly, 0b0001 << packed->Tape[head], 1, packed->File);
			smb->Attrib[head]=poly;
			packed->Length-=0b0001 << packed->Tape[head];
			poly=0;
			head++ ;
		}
		smb->Dimension=head;
	}
/* Verify sum */
	smb->Node.MemSize=StructSize + AttribSize;
/* End of sum */
	return(smb);
}

void SMB_Read_Extension(struct PACKED *packed, struct STRUCTINFO *smb)
{
	unsigned int head=0;
	unsigned int StructSize=sizeof(struct STRUCTINFO), AttribSize=0;

	while((smb != NULL) && (packed->Tape[head] != _EOT_))
	{
		unsigned long long int poly=0;

		AttribSize=(smb->Dimension + head + 1) * sizeof(unsigned long long int);
		smb->Attrib=realloc(smb->Attrib, AttribSize);
		fread(&poly, 0b0001 << packed->Tape[head], 1, packed->File);
		smb->Attrib[smb->Dimension + head]=poly;
		packed->Length-=0b0001 << packed->Tape[head];
		poly=0;
		head++ ;
	}
	smb->Dimension+=head;
/* Verify sum */
	smb->Node.MemSize=StructSize + AttribSize;
/* End of sum */
}

void BIOS_Free_Structure(struct STRUCTINFO *smb)
{
	if(smb != NULL)
	{
		struct STRING *Link=NULL, *pstr=NULL;

		pstr=smb->String;
		while(pstr != NULL)
		{
			Link=pstr->Link;
			free(pstr->Buffer);
			free(pstr);
			pstr=Link;
		};
		smb->String=NULL;
		if(smb->Attrib)
		{
			free(smb->Attrib);
			smb->Attrib=NULL;
		}
		free(smb);
		smb=NULL;
	}
}

struct BIOSINFO *BIOS_Read_Info(void)
{
	const int         tape[]=SMBIOS_BIOSINFO_PACKED;
	struct PACKED     packed={SMBIOS_BIOSINFO_TYPE, SMBIOS_BIOSINFO_INSTANCE, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct BIOSINFO*) smb);
}

struct BOARDINFO *BOARD_Read_Info(void)
{
	const int         tape[]=SMBIOS_BOARDINFO_PACKED,
	                  extens[]=SMBIOS_BOARDINFO_EXTENS;
	struct PACKED     packed={SMBIOS_BOARDINFO_TYPE, SMBIOS_BOARDINFO_INSTANCE, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		if(smb->Attrib[9] > 0)
		{
			packed.Tape=&extens[0];
			while(packed.Length > 0)
				SMB_Read_Extension(&packed, smb);
		}
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct BOARDINFO*) smb);
}

struct PROCINFO *PROC_Read_Info(void)
{
	const int         tape[]=SMBIOS_PROCINFO_PACKED,
	                  extens[]=SMBIOS_PROCINFO_EXTENS;
	struct PACKED     packed={SMBIOS_PROCINFO_TYPE, SMBIOS_PROCINFO_INSTANCE, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		if(packed.Length > 0)
		{
			packed.Tape=&extens[0];
			SMB_Read_Extension(&packed, smb);
		}
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct PROCINFO*) smb);
}

struct CACHEINFO *CACHE_Read_Info(int instance)
{
	const int         tape[]=SMBIOS_CACHEINFO_PACKED;
	struct PACKED     packed={SMBIOS_CACHEINFO_TYPE, instance, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct CACHEINFO*) smb);
}

struct MEMARRAY *MEM_Read_Array(void)
{
	const int         tape[]=SMBIOS_MEMARRAY_PACKED,
	                  extens[]=SMBIOS_MEMARRAY_EXTENS;
	struct PACKED     packed={SMBIOS_MEMARRAY_TYPE, SMBIOS_MEMARRAY_INSTANCE, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		if(packed.Length > 0)
		{
			packed.Tape=&extens[0];
			SMB_Read_Extension(&packed, smb);
		}
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct MEMARRAY*) smb);
}

struct MEMDEV *MEM_Read_Device(int instance)
{
	const int         tape[]=SMBIOS_MEMDEV_PACKED,
	                  extens[]=SMBIOS_MEMDEV_EXTENS;
	struct PACKED     packed={SMBIOS_MEMDEV_TYPE, instance, NULL, 0, &tape[0]};
	struct STRUCTINFO *smb=NULL;

	if(!SMB_Read_Length(&packed) && !SMB_Open_Structure(&packed))
	{
		smb=SMB_Read_Structure(&packed);
		if(packed.Length > 0)
		{
			packed.Tape=&extens[0];
			SMB_Read_Extension(&packed, smb);
		}
		smb->String=SMB_Read_Strings(&packed);
		SMB_Close_Structure(&packed);
/* Verify sum */
		if(smb->String == NULL)
			smb->Node.MemSum=smb->Node.MemSize;
		else
			smb->Node.MemSum=smb->Node.MemSize + smb->String->Node.MemSum;
/* End of sum */
	}
	return((struct MEMDEV*) smb);
}

struct MEMDEV **MEM_ReadAll_Devices(struct MEMARRAY *memArray)
{
	struct MEMDEV **memory=NULL;

	if(memArray != NULL)
	{
		unsigned int arraySize=sizeof(struct MEMDEV*) * memArray->Attrib->Number_Devices;
		memory=malloc(arraySize);

		int stick=0;
		for(stick=0; stick < memArray->Attrib->Number_Devices; stick++)
			memory[stick]=MEM_Read_Device(stick);
	}
	return(memory);
}

void MEM_FreeAll_Devices(struct MEMDEV **memory, struct MEMARRAY *memArray)
{
	if(memArray != NULL)
	{
		int stick=0;
		for(stick=0; stick < memArray->Attrib->Number_Devices; stick++)
			BIOS_Free_Structure((struct STRUCTINFO*) memory[stick]);

		free(memory);
		memory=NULL;
	}
}

Bool32 Init_SMBIOS(SMBIOS_TREE *Smb)
{
	if(Smb != NULL)
	{
/* Verify sum */
		Smb->Node.MemSize=sizeof(SMBIOS_TREE);
/* End of sum */
		// Read the full SMBIOS tree.
		if((Smb->Bios=BIOS_Read_Info())
		&& (Smb->Board=BOARD_Read_Info())
		&& (Smb->Proc=PROC_Read_Info())
		&& (Smb->Cache[0]=CACHE_Read_Info(0)) && (Smb->Cache[1]=CACHE_Read_Info(1)) && (Smb->Cache[2]=CACHE_Read_Info(2))
		&& (Smb->MemArray=MEM_Read_Array())
		&& (Smb->Memory=MEM_ReadAll_Devices(Smb->MemArray)))
		{
/* Verify sum */
			Smb->Node.MemSize+=sizeof(struct MEMDEV*) * Smb->MemArray->Attrib->Number_Devices;
			Smb->Node.MemSum= Smb->Node.MemSize \
					+ Smb->Bios->Node.MemSum \
					+ Smb->Board->Node.MemSum \
					+ Smb->Proc->Node.MemSum \
					+ Smb->Cache[0]->Node.MemSum \
					+ Smb->Cache[1]->Node.MemSum \
					+ Smb->Cache[2]->Node.MemSum \
					+ Smb->MemArray->Node.MemSum;
			int stick=0;
			for(stick=0; stick <  Smb->MemArray->Attrib->Number_Devices; stick++)
				Smb->Node.MemSum+=Smb->Memory[stick]->Node.MemSum;
/* End of sum */
			return(TRUE);
		}
		else
			return(FALSE);
	}
	else
		return(FALSE);
}

Bool32 Close_SMBIOS(SMBIOS_TREE *Smb)
{
	if(Smb != NULL)
	{	// Release the SMBIOS tree.
		MEM_FreeAll_Devices(Smb->Memory, Smb->MemArray);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->MemArray);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Cache[2]);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Cache[1]);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Cache[0]);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Proc);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Board);
		BIOS_Free_Structure((struct STRUCTINFO*) Smb->Bios);
		return(TRUE);
	}
	else
		return(FALSE);
}

PADDR Copy_SmbString(PADDR pSrc, PADDR pAddr)
{
	struct STRING	*pStrSrc=pSrc,
			*pStrDest=pAddr;
	PADDR retAddr=pAddr;

	const unsigned int op1=sizeof(struct STRING);

	pStrDest->ID=pStrSrc->ID;

	retAddr+=op1;

	if(pStrSrc->Buffer != NULL)
	{
		const unsigned int op2=strlen(pStrSrc->Buffer) + 1;

		pStrDest->Buffer=retAddr;
		strncpy(pStrDest->Buffer, pStrSrc->Buffer, op2);

		retAddr+=op2;
	}
	else
	{
		pStrDest->Buffer=NULL;
	}
/* Verify sum */
	pStrDest->Node.MemSize=retAddr - pAddr;
/* End of sum */
	if(pStrSrc->Link != NULL)
	{
		pStrDest->Link=retAddr;
		retAddr=Copy_SmbString(pStrSrc->Link, retAddr);
/* Verify sum */
		pStrDest->Node.MemSum=pStrDest->Node.MemSize + pStrDest->Link->Node.MemSum;
/* End of sum */
	}
	else
	{
		pStrDest->Link=NULL;
/* Verify sum */
		pStrDest->Node.MemSum=pStrDest->Node.MemSize;
/* End of sum */
	}
	return(retAddr);
}

PADDR Copy_SmbStruct(PADDR pSrc, PADDR pAddr)
{
	struct STRUCTINFO	*pSiSrc=pSrc,
				*pSiDest=pAddr;
	PADDR retAddr=pAddr;

	const unsigned int op1=sizeof(struct STRUCTINFO);

	pSiDest->Header.Type=pSiSrc->Header.Type;
	pSiDest->Header.Length=pSiSrc->Header.Length;
	pSiDest->Header.Handle=pSiSrc->Header.Handle;
	pSiDest->Dimension=pSiSrc->Dimension;

	retAddr+=op1;

	if(pSiSrc->Attrib != NULL)
	{
		const unsigned int op2=sizeof(PADDR), op3=pSiSrc->Dimension * op2;

		pSiDest->Attrib=retAddr;

		unsigned int ix=0;
		for(ix=0; ix < pSiSrc->Dimension; ix++)
			pSiDest->Attrib[ix]=pSiSrc->Attrib[ix];

		retAddr+=op3;
	}
	else
	{
		pSiDest->Attrib=NULL;
	}
/* Verify sum */
	pSiDest->Node.MemSize=retAddr - pAddr;
/* End of sum */
	if(pSiSrc->String != NULL)
	{
		pSiDest->String=retAddr;
		retAddr=Copy_SmbString(pSiSrc->String, retAddr);
/* Verify sum */
		pSiDest->Node.MemSum=pSiDest->Node.MemSize + pSiDest->String->Node.MemSum;
/* End of sum */
	}
	else
	{
		pSiDest->String=NULL;
/* Verify sum */
		pSiDest->Node.MemSum=pSiDest->Node.MemSize;
/* End of sum */
	}
	return(retAddr);
}

Bool32 Copy_SmbTree(SMBIOS_TREE *SmbSrc, SMBIOS_TREE *SmbDest)
{
	if((SmbSrc != NULL) && (SmbDest != NULL))
	{
		PADDR pAddr=(PADDR) &SmbDest[1];

/* Verify sum */
		SmbDest->Node.MemSize=pAddr - (PADDR) SmbDest;
/* End of sum */

		SmbDest->Bios=(struct BIOSINFO *) pAddr;
		pAddr=Copy_SmbStruct((PADDR) SmbSrc->Bios, pAddr);

		SmbDest->Board=(struct BOARDINFO *) pAddr;
		pAddr=Copy_SmbStruct((PADDR) SmbSrc->Board, pAddr);

		SmbDest->Proc=(struct PROCINFO *) pAddr;
		pAddr=Copy_SmbStruct((PADDR) SmbSrc->Proc, pAddr);

		SmbDest->Cache[0]=(struct CACHEINFO *) pAddr;
		pAddr=Copy_SmbStruct((struct STRUCTINFO *) SmbSrc->Cache[0], pAddr);

		SmbDest->Cache[1]=(struct CACHEINFO *) pAddr;
		pAddr=Copy_SmbStruct((struct STRUCTINFO *) SmbSrc->Cache[1], pAddr);

		SmbDest->Cache[2]=(struct CACHEINFO *) pAddr;
		pAddr=Copy_SmbStruct((struct STRUCTINFO *) SmbSrc->Cache[2], pAddr);

		SmbDest->MemArray=(struct MEMARRAY *) pAddr;
		pAddr=Copy_SmbStruct((struct STRUCTINFO *) SmbSrc->MemArray, pAddr);

		SmbDest->Memory=(struct MEMDEV **) pAddr;
		if(SmbSrc->MemArray->Attrib->Number_Devices > 0)
		{
			const unsigned int op1=sizeof(PADDR), op2=op1 * SmbSrc->MemArray->Attrib->Number_Devices;
			pAddr+=op2;

			unsigned int stick=0;
			for(stick=0; stick < SmbSrc->MemArray->Attrib->Number_Devices; stick++)
			{
				SmbDest->Memory[stick]=(struct MEMDEV *) pAddr;
				pAddr=Copy_SmbStruct((struct STRUCTINFO *) SmbSrc->Memory[stick], pAddr);
			}
/* Verify sum */
		SmbDest->Node.MemSize+=op2;
/* End of sum */
		}
/* Verify sum */
		SmbDest->Node.MemSum =	SmbDest->Node.MemSize \
				+	SmbDest->Bios->Node.MemSum \
				+	SmbDest->Board->Node.MemSum \
				+	SmbDest->Proc->Node.MemSum \
				+	SmbDest->Cache[0]->Node.MemSum \
				+	SmbDest->Cache[1]->Node.MemSum \
				+	SmbDest->Cache[2]->Node.MemSum \
				+	SmbDest->MemArray->Node.MemSum;
		int stick=0;
		for(stick=0; stick <  SmbDest->MemArray->Attrib->Number_Devices; stick++)
			SmbDest->Node.MemSum+=SmbDest->Memory[stick]->Node.MemSum;
/* End of sum */
		return(SmbDest->Node.MemSum == SmbSrc->Node.MemSum);
	}
	else
		return(FALSE);
}

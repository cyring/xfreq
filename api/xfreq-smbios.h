/*
 * xfreq-smbios.h by CyrIng
 *
 * XFreq
 * Copyright (C) 2012-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#include <stdio.h>

#include "xfreq-types.h"

// System Management BIOS (SMBIOS) Reference Specification
// Version: 2.7.1 § 7.5

struct HEADER
{
		unsigned int
				Type	:  8,
				Length	:  8,
				Handle	: 16;
};

struct NODE
{
		unsigned int	MemSize,
				MemSum;
};

#define STRING_DELIMITER 0

struct STRING
{
		long int	ID;
		char		*Buffer;
		struct STRING	*Link;
		struct NODE	Node;
};

struct STRUCTINFO
{
	struct HEADER		Header;
	unsigned int		Dimension;
	unsigned long long int	*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};

struct PACKED
{
		const	int	Type;
		const	int	Instance;
		FILE		*File;
		size_t		Length;
		const	int	*Tape;
};

#define _B_		0
#define _W_		1
#define _D_		2
#define _Q_		3
#define _S_		,
#define _EOT_		-1

#define SMBIOS_BIOSINFO_TYPE		0
#define SMBIOS_BIOSINFO_INSTANCE	0

#define SMBIOS_BIOSINFO_PACKED {_B_ _S_ _B_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _Q_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _EOT_}

struct SMBIOS0
{
		unsigned long long int
				Vendor,
				Version,
				Address,
				Release_Date,
				ROM_Size,
				Characteristics,
				Extension_Bytes,
				Major_Release,
				Minor_Release,
				Firmware_Major,
				Firmware_Minor;
};

struct BIOSINFO
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS0		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


#define SMBIOS_BOARDINFO_TYPE		2
#define SMBIOS_BOARDINFO_INSTANCE	0
/*
* Spec. for a multi motherboard
#define SMBIOS_BOARDINFO_PACKED {_B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _EOT_}

* The ASUS Rampage II Gene is a single motherboard
*/
#define SMBIOS_BOARDINFO_PACKED {_B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _EOT_}
#define SMBIOS_BOARDINFO_EXTENS {_W_ _S_ _EOT_}

struct SMBIOS2
{
		unsigned long long int
				Manufacturer,
				Product,
				Version,
				Serial,
				AssetTag,
				Feature,
				Location,
				Chassis_Handle,
				Board_Type,
				Number_Object,
				Object_Handles;
			//	Use Attrib[10] to Attrib[265] to read the list of the 255 object handles
};

struct BOARDINFO
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS2		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


#define SMBIOS_PROCINFO_TYPE		4
#define SMBIOS_PROCINFO_INSTANCE	0
/*
* Spec. Version 2.6+
#define SMBIOS_PROCINFO_PACKED {_B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _Q_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _EOT_}

* The SMBIOS version of the ASUS Rampage II Gene is a 2.5
*/
#define SMBIOS_PROCINFO_PACKED {_B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _Q_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _EOT_}
#define SMBIOS_PROCINFO_EXTENS {_W_ _S_ _EOT_}

struct SMBIOS4
{
		unsigned long long int
				Socket,
				ProcType,
				Family,
				Manufacturer;
				struct
				{
		unsigned int
					EAX	: 32-0,
					EDX	: 64-32;
				} CPUID_0x01;
		unsigned long long int
				Version;
				struct
				{
		unsigned long int
					Tension	:  7-0,
					Mode	:  8-7,
					Unused	: 64-8;
				} Voltage;
		unsigned long long int
				Clock,
				MaxSpeed,
				CurrentSpeed,
				Populated,
				Upgrade,
				L1_Cache_Handle,
				L2_Cache_Handle,
				L3_Cache_Handle,
				Serial,
				AssetTag,
				PartNumber,
				CoreCount,
				CoreEnabled,
				ThreadCount,
				Characteristics,
				Family2;
};

struct PROCINFO
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS4		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


#define SMBIOS_CACHEINFO_TYPE		7

#define SMBIOS_CACHEINFO_PACKED {_B_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _EOT_}

struct SMBIOS7
{
		unsigned long long int
				Socket,
				Configuration,
				Maximum_Size,
				Installed_Size,
				Supported_SRAM,
				Current_SRAM,
				Cache_Speed,
				Error_Correction,
				System_Cache,
				Associativity;
};

struct CACHEINFO
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS7		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


#define SMBIOS_MEMARRAY_TYPE		16
#define SMBIOS_MEMARRAY_INSTANCE	0

#define SMBIOS_MEMARRAY_PACKED {_B_ _S_ _B_ _S_ _B_ _S_ _D_ _S_ _W_ _S_ _W_ _S_ _EOT_}
#define SMBIOS_MEMARRAY_EXTENS {_Q_ _S_ _EOT_}

struct SMBIOS16
{
		unsigned long long int
				Location,
				Use,
				Error_Correction,
				Maximum_Capacity,
				Error_Handle,
				Number_Devices,
				Extended_Capacity;
};

struct MEMARRAY
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS16		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


#define SMBIOS_MEMDEV_TYPE		17

#define SMBIOS_MEMDEV_PACKED {_W_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _W_ _S_ _W_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _B_ _S_ _EOT_}
#define SMBIOS_MEMDEV_EXTENS {_B_ _S_ _D_ _S_ _W_ _S_ _EOT_}

struct SMBIOS17
{
		unsigned long long int
				MemArray_Handle,
				Error_Handle,
				Total_Width,
				Data_Width,
				Size,
				Form_Factor,
				Set,
				Socket,
				Bank,
				Mem_Type,
				Mem_Detail,
				Speed,
				Manufacturer,
				Serial,
				AssetTag,
				PartNumber,
				Attributes,
				Extended_Size,
				Clock_Speed;
};

struct MEMDEV
{
	struct HEADER		Header;
	unsigned int		Dimension;
	struct SMBIOS17		*Attrib;
	struct STRING		*String;
	struct NODE		Node;
};


typedef struct
{
	struct BIOSINFO		*Bios;
	struct BOARDINFO	*Board;
	struct PROCINFO		*Proc;
	struct CACHEINFO	*Cache[3];
	struct MEMARRAY		*MemArray;
	struct MEMDEV		**Memory;
	struct NODE		Node;
} SMBIOS_TREE;

typedef	void*	PADDR;

extern Bool32 Init_SMBIOS(SMBIOS_TREE *Smb);
extern Bool32 Close_SMBIOS(SMBIOS_TREE *Smb);
extern Bool32 Copy_SmbTree(SMBIOS_TREE *SmbSrc, SMBIOS_TREE *SmbDest);

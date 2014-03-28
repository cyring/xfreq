/*
 * XFreq.h #0.23 SR1 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define _MAJOR   "0"
#define _MINOR   "23"
#define _NIGHTLY "1"
#define AutoDate "X-Freq "_MAJOR"."_MINOR"-"_NIGHTLY" (C) CYRIL INGENIERIE "__DATE__"\n"


#define MAX(M, m)	((M) > (m) ? (M) : (m))
#define MIN(m, M)	((m) < (M) ? (m) : (M))

typedef enum {false=0, true=1} bool;

typedef struct
{
	struct
	{
		struct SIGNATURE
		{
			unsigned
			Stepping	:  4-0,
			Model		:  8-4,
			Family		: 12-8,
			ProcType	: 14-12,
			Unused1		: 16-14,
			ExtModel	: 20-16,
			ExtFamily	: 28-20,
			Unused2		: 32-28;
		} EAX;
		struct
		{
			unsigned
			Brand_ID	:  8-0,
			CLFSH_Size	: 16-8,
			MaxThread	: 24-16,
			APIC_ID		: 32-24;
		} EBX;
		struct
		{
			unsigned
			SSE3	:  1-0,
			PCLMULDQ:  2-1,
			DTES64	:  3-2,
			MONITOR	:  4-3,
			DS_CPL	:  5-4,
			VMX	:  6-5,
			SMX	:  7-6,
			EIST	:  8-7,
			TM2	:  9-8,
			SSSE3	: 10-9,
			CNXT_ID	: 11-10,
			Unused1	: 12-11,
			FMA	: 13-12,
			CX16	: 14-13,
			xTPR	: 15-14,
			PDCM	: 16-15,
			Unused2	: 17-16,
			PCID	: 18-17,
			DCA	: 19-18,
			SSE41	: 20-19,
			SSE42	: 21-20,
			x2APIC	: 22-21,
			MOVBE	: 23-22,
			POPCNT	: 24-23,
			TSCDEAD	: 25-24,
			AES	: 26-25,
			XSAVE	: 27-26,
			OSXSAVE	: 28-27,
			AVX	: 29-28,
			F16C	: 30-29,
			RDRAND	: 31-30,
			Unused3	: 32-31;
		} ECX;
		struct
		{
			unsigned
			FPU	:  1-0,
			VME	:  2-1,
			DE	:  3-2,
			PSE	:  4-3,
			TSC	:  5-4,
			MSR	:  6-5,
			PAE	:  7-6,
			MCE	:  8-7,
			CX8	:  9-8,
			APIC	: 10-9,
			Unused1	: 11-10,
			SEP	: 12-11,
			MTRR	: 13-12,
			PGE	: 14-13,
			MCA	: 15-14,
			CMOV	: 16-15,
			PAT	: 17-16,
			PSE36	: 18-17,
			PSN	: 19-18,
			CLFSH	: 20-19,
			Unused2	: 21-20,
			DS_PEBS	: 22-21,
			ACPI	: 23-22,
			MMX	: 24-23,
			FXSR	: 25-24,
			SSE	: 26-25,
			SSE2	: 27-26,
			SS	: 28-27,
			HTT	: 29-28,
			TM1	: 30-29,
			Unused3	: 31-30,
			PBE	: 32-31;
		} EDX;
	} Std;
	unsigned	ThreadCount;
	struct
	{
		struct
		{
			unsigned
			DTS	:  1-0,
			Turbo	:  2-1,
			ARAT	:  3-2,
			Unused1	:  4-3,
			PLN	:  5-4,
			ECMD	:  6-5,
			PTM	:  7-6,
			Unused2	: 32-7;
		} EAX;
		struct
		{
			unsigned
			Threshld:  4-0,
			Unused1	: 32-4;
		} EBX;
		struct
		{
			unsigned
			HCF_Cap	:  1-0,
			ACNT_Cap:  2-1,
			Unused1	:  3-2,
			PEB_Cap	:  4-3,
			Unused2	: 32-4;
		} ECX;
		struct
		{
			unsigned
			Unused1	: 32-0;
		} EDX;
	} Thermal_Power_Leaf;
	struct
	{
		struct
		{
			unsigned
			Version	:  8-0,
			MonCtrs	: 16-8,
			MonWidth: 24-16,
			VectorSz: 32-24;
		} EAX;
		struct
		{
			unsigned
			CoreCycl:  1-0,
			InRetire:  2-1,
			RefCycle:  3-2,
			LLC_Ref	:  4-3,
			LLC_Miss:  5-4,
			BrInRet	:  6-5,
			BrMispre:  7-6,
			Unused1	: 32-7;
		} EBX;
		struct
		{
			unsigned
			Unused1	: 32-0;
		} ECX;
		struct
		{
			unsigned
			FixCtrs	:  5-0,
			FixWidth: 13-5,
			Unused1	: 32-13;
		} EDX;
	} Perf_Monitoring_Leaf;
	struct
	{
		struct
		{
			unsigned
			MaxSubLeaf	: 32-0;
		} EAX;
		struct
		{
			unsigned
			FSGSBASE	:  1-0,
			TSC_ADJUST	:  2-1,
			Unused1		:  3-2,
			BMI1		:  4-3,
			HLE		:  5-4,
			AVX2		:  6-5,
			Unused2		:  7-6,
			SMEP		:  8-7,
			BMI2		:  9-8,
			FastStrings	: 10-9,
			INVPCID		: 11-10,
			RTM		: 12-11,
			QM		: 13-12,
			FPU_CS_DS	: 14-13,
			Unused3		: 32-14;
		} EBX;
			unsigned
		ECX			: 32-0,
		EDX			: 32-0;

	} ExtFeature;
	unsigned	LargestExtFunc;
	struct
	{
		struct
		{
			unsigned
			LAHFSAHF:  1-0,
			Unused1	: 32-1;
		} ECX;
		struct
		{
			unsigned
			Unused1	: 11-0,
			SYSCALL	: 12-11,
			Unused2	: 20-12,
			XD_Bit	: 21-20,
			Unused3	: 26-21,
			PG_1GB	: 27-26,
			RDTSCP	: 28-27,
			Unused4	: 29-28,
			IA64	: 30-29,
			Unused5	: 32-30;
		} EDX;
	} ExtFunc;
	char		BrandString[48+1];
	bool		HTT_enabled;
} FEATURES;

typedef	struct {
		union {
			struct
			{
				unsigned
				SHRbits	:  5-0,
				Unused1	: 32-5;
			};
			unsigned Register;
		} EAX;
		union {
			struct
			{
				unsigned
				Threads	: 16-0,
				Unused1	: 32-16;
			};
			unsigned Register;
		} EBX;
		union {
			struct
			{
				unsigned
				Level	:  8-0,
				Type	: 16-8,
				Unused1 : 32-16;
			};
			unsigned Register;
		} ECX;
		union {
			struct
			{
				unsigned
				 x2APIC_ID: 32-0;
			};
			unsigned Register;
		} EDX;
} CPUID_TOPOLOGY;

// Memory address of the Base Clock in the ROM of the ASUS Rampage II GENE.
#define	BCLK_ROM_ADDR	0xf08d9 + 0x12

// Read data from the PCI bus.
#define PCI_CONFIG_ADDRESS(bus, dev, fn, reg) \
	(0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

struct IMCINFO
{
	unsigned ChannelCount;
	struct CHANNEL
	{
		struct
		{
			unsigned
			tCL,
			tRCD,
			tRP,
			tRAS,
			tRRD,
			tRFC,
			tWR,
			tRTPr,
			tWTPr,
			tFAW,
			B2B;
		} Timing;
	}	*Channel;
};


//	Read, Write a Model Specific Register.
#define	Read_MSR(FD, offset, msr)  pread(FD, msr, sizeof(*msr), offset)
#define	Write_MSR(FD, offset, msr) pwrite(FD, msr, sizeof(*msr), offset)

#define	IA32_TIME_STAMP_COUNTER		0x10
#define	MSR_FSB_FREQ			0xcd
#define	IA32_MPERF			0xe7
#define	IA32_APERF			0xe8
#define	IA32_PERF_STATUS		0x198
#define	IA32_CLOCK_MODULATION		0x19a
#define	IA32_THERM_INTERRUPT		0x19b
#define IA32_THERM_STATUS		0x19c
#define	IA32_MISC_ENABLE		0x1a0
#define	IA32_ENERGY_PERF_BIAS		0x1b0
#define	IA32_PKG_THERM_INTERRUPT 	0x1b2
#define	IA32_FIXED_CTR1			0x30a
#define	IA32_FIXED_CTR2			0x30b
#define	IA32_FIXED_CTR_CTRL		0x38d
#define	IA32_PERF_GLOBAL_STATUS		0x38e
#define	IA32_PERF_GLOBAL_CTRL		0x38f
#define	IA32_PERF_GLOBAL_OVF_CTRL	0x390
#define	MSR_CORE_C3_RESIDENCY		0x3fc
#define	MSR_CORE_C6_RESIDENCY		0x3fd
#define	MSR_PLATFORM_INFO		0xce
#define	MSR_TURBO_RATIO_LIMIT		0x1ad
#define	MSR_TEMPERATURE_TARGET		0x1a2

#define	SMBIOS_PROCINFO_STRUCTURE	4
#define	SMBIOS_PROCINFO_INSTANCE	0
#define	SMBIOS_PROCINFO_EXTCLK		0x12
#define	SMBIOS_PROCINFO_CORES		0x23
#define	SMBIOS_PROCINFO_THREADS		0x25

typedef	struct
{
	unsigned long long
		Bus_Speed	:  3-0,
		ReservedBits	: 64-3;
} FSB_FREQ;

typedef	struct
{
	unsigned long long
		Ratio		: 16-0,
		ReservedBits1	: 31-16,
		XE		: 32-31,
		ReservedBits2	: 40-32,
		MaxBusRatio	: 45-40,
		ReservedBits3	: 46-45,
		NonInt_BusRatio	: 47-46,
		ReservedBits4	: 64-47;
} PERF_STATUS;

typedef struct
{
	unsigned long long
		ReservedBits1	:  8-0,
		MaxNonTurboRatio: 16-8,
		ReservedBits2	: 28-16,
		Ratio_Limited	: 29-28,
		TDC_TDP_Limited	: 30-29,
		ReservedBits3	: 40-30,
		MinimumRatio	: 48-40,
		ReservedBits4	: 64-48;
} PLATFORM;

typedef struct
{
	unsigned long long
		MaxRatio_1C	:  8-0,
		MaxRatio_2C	: 16-8,
		MaxRatio_3C	: 24-16,
		MaxRatio_4C	: 32-24,
		MaxRatio_5C	: 40-32,
		MaxRatio_6C	: 48-40,
		MaxRatio_7C	: 56-48,
		MaxRatio_8C	: 64-56;
} TURBO;

typedef	struct
{
	unsigned long long
		FastStrings	:  1-0,
		ReservedBits1	:  3-1,
		TCC		:  4-3,
		ReservedBits2	:  7-4,
		PerfMonitoring	:  8-7,
		ReservedBits3	: 11-8,
		BTS		: 12-11,
		PEBS		: 13-12,
		ReservedBits4	: 16-13,
		EIST		: 17-16,
		ReservedBits5	: 18-17,
		FSM		: 19-18,
		ReservedBits6	: 22-19,
		CPUID_MaxVal	: 23-22,
		xTPR		: 24-23,
		ReservedBits7	: 34-24,
		XD_Bit		: 35-34,
		ReservedBits8	: 38-35,
		Turbo		: 39-38,
		ReservedBits9	: 64-39;
} MISC_PROC_FEATURES;

typedef struct
{
	unsigned long long
		EN_PMC0		:  1-0,
		EN_PMC1		:  2-1,
		ReservedBits1	: 32-2,
		EN_FIXED_CTR0	: 33-32,
		EN_FIXED_CTR1	: 34-33,
		EN_FIXED_CTR2	: 35-34,
		ReservedBits2	: 64-35;
} GLOBAL_PERF_COUNTER;

typedef struct
{
	unsigned long long
		EN0_OS		:  1-0,
		EN0_Usr		:  2-1,
		AnyThread_EN0	:  3-2,
		EN0_PMI		:  4-3,
		EN1_OS		:  5-4,
		EN1_Usr		:  6-5,
		AnyThread_EN1	:  7-6,
		EN1_PMI		:  8-7,
		EN2_OS		:  9-8,
		EN2_Usr		: 10-9,
		AnyThread_EN2	: 11-10,
		EN2_PMI		: 12-11,
		ReservedBits	: 64-12;
} FIXED_PERF_COUNTER;

typedef struct
{
	unsigned long long
		Overflow_PMC0	:  1-0,
		Overflow_PMC1	:  2-1,
		Overflow_PMC2	:  3-2,
		Overflow_PMC3	:  4-3,
		ReservedBits1	: 32-4,
		Overflow_CTR0	: 33-32,
		Overflow_CTR1	: 34-33,
		Overflow_CTR2	: 35-34,
		ReservedBits2	: 61-35,
		Overflow_UNC	: 62-61,
		Overflow_Buf	: 63-62,
		Ovf_CondChg	: 64-63;
} GLOBAL_PERF_STATUS;

typedef	struct
{
	unsigned long long
		Clear_Ovf_PMC0	:  1-0,
		Clear_Ovf_PMC1	:  2-1,
		ReservedBits1	: 32-2,
		Clear_Ovf_CTR0 	: 33-32,
		Clear_Ovf_CTR1	: 34-33,
		Clear_Ovf_CTR2	: 35-34,
		ReservedBits2	: 61-35,
		Clear_Ovf_UNC	: 62-61,
		Clear_Ovf_Buf	: 63-62,
		Clear_CondChg	: 64-63;
} GLOBAL_PERF_OVF_CTRL;

#define	LOW_TEMP_VALUE		35
#define MED_TEMP_VALUE		45
#define	HIGH_TEMP_VALUE		65

typedef struct
{
	unsigned long long
		HighTempEnable	:  1-0,
		LowTempEnable	:  2-1,
		PROCHOTEnable	:  3-2,
		FORCEPREnable	:  4-3,
		OverheatEnable	:  5-4,
		ReservedBits1	:  8-5,
		Threshold1	: 15-8,
		Threshold1Enable: 16-15,
		Threshold2      : 23-16,
		Threshold2Enable: 24-23,
		PowerLimitEnable: 25-24,
		ReservedBits2	: 64-25;
} THERM_INTERRUPT;

typedef struct
{
	unsigned long long
		Status		:  1-0,
		StatusLog	:  2-1,
		PROCHOT		:  3-2,
		PROCHOTLog	:  4-3,
		CriticalTemp	:  5-4,
		CriticalTempLog	:  6-5,
		Threshold1	:  7-6,
		Threshold1Log	:  8-7,
		Threshold2	:  9-8,
		Threshold2Log	: 10-9,
		PowerLimit	: 11-10,
		PowerLimitLog	: 12-11,
		ReservedBits1	: 16-12,
		DTS		: 23-16,
		ReservedBits2	: 27-23,
		Resolution	: 31-27,
		ReadingValid	: 32-31,
		ReservedBits3	: 64-32;
} THERM_STATUS;

typedef struct
{
	unsigned long long
		ReservedBits1	: 16-0,
		Target		: 24-16,
		ReservedBits2	: 64-24;
} TJMAX;

#define	ARCHITECTURES 24
//	[GenuineIntel]
#define	GenuineIntel			{ExtFamily:0x0, Family:0x0, ExtModel:0x0, Model:0x0}
//	[Core]
#define	Core_Duo			{ExtFamily:0x0, Family:0x6, ExtModel:0x0, Model:0xE}
#define	Core_2Duo			{ExtFamily:0x0, Family:0x6, ExtModel:0x0, Model:0xF}
#define	Core_Kentsfield			{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0x5}
#define	Core_Yorkfield			{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0x7}
//	[Atom]
#define	Atom_Bonnell			{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xC}
#define	Atom_Silvermont			{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0x6}
#define	Atom_27				{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0x7}
#define	Atom_35				{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0x5}
#define	Atom_Saltwell			{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0x6}
//	[Nehalem]
#define	Nehalem_Bloomfield		{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xA}
#define	Nehalem_Lynnfield		{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xE}
#define	Nehalem_0F			{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xF}
#define	Nehalem_NehalemEP		{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xE}
//	[Westmere]
#define	Westmere_Arrandale		{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0x5}
#define	Westmere_Gulftown		{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xC}
#define	Westmere_WestmereEP		{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xF}
//	[Sandy Bridge]
#define	SandyBridge_1G			{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xD}
#define	SandyBridge_Bromolow		{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xA}
//	[Ivy Bridge]
#define	IvyBridge			{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xA}
#define	IvyBridgeEP			{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xE}
//	[Haswell]
#define	Haswell_3C			{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xC}
#define	Haswell_45			{ExtFamily:0x0, Family:0x6, ExtModel:0x4, Model:0x5}
#define	Haswell_46			{ExtFamily:0x0, Family:0x6, ExtModel:0x4, Model:0x6}

typedef	const struct
{
	struct	SIGNATURE	Signature;
		unsigned int	MaxOfCores;
		double		(*ClockSpeed)();
	const	char		*Architecture;
		void		*(*uCycle)(void *);
		bool		(*Init_MSR)(void *);
} ARCH;

enum	{SRC_TSC, SRC_BIOS, SRC_SPEC, SRC_ROM, SRC_USER, SRC_COUNT};

typedef	struct {
			signed int		FD;
			GLOBAL_PERF_COUNTER	GlobalPerfCounter;
			FIXED_PERF_COUNTER	FixedPerfCounter;
			struct {
				struct
				{
				unsigned long long
					UCC,
					URC;
				}		C0[2];
				unsigned long long
						C3[2],
						C6[2],
						TSC[2];
			} Cycles;
			struct {
				struct
				{
				unsigned long long
					UCC,
					URC;
				}		C0;
				unsigned long long
						C3,
						C6,
						TSC;
			} Delta;
			struct {
				double		Turbo,
						C0,
						C3,
						C6;
			} State;
			double			RelativeRatio,
						RelativeFreq;
			TJMAX			TjMax;
			THERM_INTERRUPT		ThermIntr;
			THERM_STATUS		ThermStat;
} CPU_STRUCT;

#define	IDLE_BASE_USEC	50000
#define	IDLE_COEF_DEF	20
#define	IDLE_COEF_MAX	80
#define	IDLE_COEF_MIN	2

#define	LEVEL_INVALID	0
#define	LEVEL_THREAD	1
#define	LEVEL_CORE	2

typedef	struct
{
	unsigned
			APIC_ID	 : 32-0,
			Core_ID	 : 32-0,
			Thread_ID: 32-0;
	CPU_STRUCT	*CPU;
} TOPOLOGY;

typedef struct
{
		signed int			ArchID;
		ARCH				Arch[ARCHITECTURES];
		FEATURES			Features;
		TOPOLOGY			*Topology;
		PERF_STATUS			PerfStatus;
		MISC_PROC_FEATURES		MiscFeatures;
		PLATFORM			Platform;
		TURBO				Turbo;
		off_t				BClockROMaddr;
		double				ClockSpeed;
		unsigned int			CPU,
						OnLine;
		char				Boost[2+2+2+1];

		struct {
			double			Turbo,
						C0,
						C3,
						C6;
		} Avg;
		unsigned int			Top,
						Hot,
						Cold,
						PerCore,
						ClockSrc;
		useconds_t			IdleTime;
} PROCESSOR;


enum {
	BACKGROUND_MAIN,
	FOREGROUND_MAIN,
	BACKGROUND_CORES,
	FOREGROUND_CORES,
	BACKGROUND_CSTATES,
	FOREGROUND_CSTATES,
	BACKGROUND_TEMPS,
	FOREGROUND_TEMPS,
	BACKGROUND_SYSINFO,
	FOREGROUND_SYSINFO,
	BACKGROUND_DUMP,
	FOREGROUND_DUMP,
	COLOR_AXES,
	COLOR_LABEL,
	COLOR_PRINT,
	COLOR_PROMPT,
	COLOR_CURSOR,
	COLOR_DYNAMIC,
	COLOR_GRAPH1,
	COLOR_GRAPH2,
	COLOR_GRAPH3,
	COLOR_INIT_VALUE,
	COLOR_LOW_VALUE,
	COLOR_MED_VALUE,
	COLOR_HIGH_VALUE,
	COLOR_PULSE,
	COLOR_FOCUS,
	COLOR_MDI_SEP,
	COLOR_COUNT
};

#define	_BACKGROUND_GLOBAL	0x11114c
#define	_FOREGROUND_GLOBAL	0x8fcefa
#define	_BACKGROUND_SPLASH	0x000000
#define	_FOREGROUND_SPLASH	0x8fcefa

#define	_BACKGROUND_MAIN	0x11114c
#define	_FOREGROUND_MAIN	0x8fcefa
#define	_BACKGROUND_CORES	0x191970
#define	_FOREGROUND_CORES	0x8fcefa
#define	_BACKGROUND_CSTATES	0x191970
#define	_FOREGROUND_CSTATES	0x8fcefa
#define	_BACKGROUND_TEMPS	0x191970
#define	_FOREGROUND_TEMPS	0x8fcefa
#define	_BACKGROUND_SYSINFO	0x191970
#define	_FOREGROUND_SYSINFO	0x8fcefa
#define	_BACKGROUND_DUMP	0x191970
#define	_FOREGROUND_DUMP	0x8fcefa

#define	_COLOR_AXES		0x404040
#define	_COLOR_LABEL		0xc0c0c0
#define	_COLOR_PRINT		0xf0f0f0
#define	_COLOR_PROMPT		0xffff00
#define	_COLOR_CURSOR		0xfd0000
#define	_COLOR_DYNAMIC		0xdddddd
#define	_COLOR_GRAPH1		0x6666f0
#define	_COLOR_GRAPH2		0x6666b0
#define	_COLOR_GRAPH3		0x666690
#define	_COLOR_INIT_VALUE	0x6666b0
#define	_COLOR_LOW_VALUE	0x00aa66
#define	_COLOR_MED_VALUE	0xe49400
#define	_COLOR_HIGH_VALUE	0xfd0000
#define	_COLOR_PULSE		0xf0f000
#define	_COLOR_FOCUS		0xffffff
#define	_COLOR_MDI_SEP		0x737373

typedef	struct
{
	unsigned int	RGB;
	char		*xrmClass,
			*xrmKey;
} COLORS;

enum	{MC_DEFAULT, MC_MOVE, MC_WAIT, MC_COUNT};

#define	ID_NULL		'\0'
#define	ID_MIN		'm'
#define	ID_SAVE		'.'
#define	ID_QUIT		'Q'
#define	ID_NORTH	'N'
#define	ID_SOUTH	'S'
#define	ID_EAST		'E'
#define	ID_WEST		'W'
#define	ID_PGUP		'U'
#define	ID_PGDW		'D'
#define	ID_PGHOME	'['
#define	ID_PGEND	']'
#define	ID_CTRLHOME	'{'
#define	ID_CTRLEND	'}'
#define	ID_PAUSE	'I'
#define	ID_RESET	'Z'
#define	ID_FREQ		'H'
#define	ID_CYCLE	'Y'
#define	ID_RATIO	'R'
#define	ID_STATE	'P'
#define	ID_TSC		't'
#define	ID_BIOS		'b'
#define	ID_SPEC		'a'
#define	ID_ROM		'r'
#define	ID_USER		'u'
#define	ID_INCLOOP	'+'
#define	ID_DECLOOP	'-'
#define	ID_WALLBOARD	'B'

#define	RSC_PAUSE	"Pause"
#define	RSC_RESET	"Reset"
#define	RSC_FREQ	"Freq."
#define	RSC_CYCLE	"Cycle"
#define	RSC_STATE	"States"
#define	RSC_RATIO	"Ratio"
#define	RSC_TSC		"TSC"
#define	RSC_BIOS	"BIOS"
#define	RSC_SPEC	"SPEC"
#define	RSC_ROM		"ROM"
#define	RSC_INCLOOP	"<<"
#define	RSC_DECLOOP	">>"
#define	RSC_WALLBOARD	"Brand"

#define	ICON_LABELS	{{Label:'M'}, {Label:'C'}, {Label:'S'}, {Label:'T'}, {Label:'I'}, {Label:'D'}}

typedef	enum	{DECORATION, SCROLLING, TEXT, ICON} WBTYPE;

typedef	union
{
	char		*Text;
	char		Label;
} RESOURCE;

struct WButton
{
	WBTYPE		Type;
	char		ID;
	int		Target;
	int		x,
			y;
	unsigned int	w,
			h;
	void		(*CallBack)();
	void		(*DrawFunc)();
	RESOURCE	Resource;
	struct	WButton	*Chain;
};

typedef	struct	WButton	WBUTTON;

enum	{HEAD, TAIL, CHAINS};

typedef struct
{
	Window		window;
	struct {
		Pixmap	B,
			F;
	} pixmap;
	GC		gc;
	int		x,
			y,
			width,
			height,
			border_width;
	struct
	{
	    XCharStruct	overall;
		int	dir,
			ascent,
			descent,
			charWidth,
			charHeight;
	} extents;
	unsigned long	background,
			foreground;
	WBUTTON		*wButton[CHAINS];
} XWINDOW;

typedef struct
{
	Window		window;
	GC		gc;
	Pixmap		bitmap;
	int		x, y,
			w, h;
} XSPLASH;

//			L-CTRL		L-ALT		R-CTRL		L-WIN		R-ALTGR
#define	AllModMask	(ControlMask	| Mod1Mask	| Mod3Mask	| Mod4Mask	| Mod5Mask)
#define	BASE_EVENTS	KeyPressMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask
#define	MOVE_EVENTS	ButtonReleaseMask | Button3MotionMask
#define	CLICK_EVENTS	ButtonPressMask

typedef struct
{
	bool		Locked;
	int		S,
			dx,
			dy;
} XTARGET;

#define	HDSIZE		".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"

typedef enum {MAIN, CORES, CSTATES, TEMPS, SYSINFO, DUMP, WIDGETS} LAYOUTS;

#define	FIRST_WIDGET	(MAIN + 1)
#define	LAST_WIDGET	(WIDGETS - 1)
#define	MESSAGE		MAIN

#define	Quarter_Char_Width(N)		(A->W[N].extents.charWidth >> 2)
#define	Half_Char_Width(N)		(A->W[N].extents.charWidth >> 1)
#define	One_Char_Width(N)		(A->W[N].extents.charWidth)
#define	One_Half_Char_Width(N)		(One_Char_Width(N) + Half_Char_Width(N))
#define	Twice_Char_Width(N)		(A->W[N].extents.charWidth << 1)
#define	Twice_Half_Char_Width(N)	(Twice_Char_Width(N) + Half_Char_Width(N))

#define	Quarter_Char_Height(N)		(A->W[N].extents.charHeight >> 2)
#define	Half_Char_Height(N)		(A->W[N].extents.charHeight >> 1)
#define	One_Char_Height(N)		(A->W[N].extents.charHeight)
#define	One_Half_Char_Height(N)		(One_Char_Height(N) + Half_Char_Height(N))
#define	Twice_Char_Height(N)		(A->W[N].extents.charHeight << 1)
#define	Twice_Half_Char_Height(N)	(Twice_Char_Height(N) + Half_Char_Height(N))

#define	Header_Height(N)		(One_Char_Height(N) + Quarter_Char_Height(N))
#define	Footer_Height(N)		(One_Char_Height(N) + Quarter_Char_Height(N))

#define	MAIN_TEXT_WIDTH		49
#define	MAIN_TEXT_HEIGHT	14

#define	MAIN_SECTION		"X-Freq "_MAJOR"."_MINOR"-"_NIGHTLY" CyrIng"

#define	CORES_TEXT_WIDTH	MAX(A->P.Turbo.MaxRatio_1C, 22)
#define	CORES_TEXT_HEIGHT	(A->P.CPU)

#define	CSTATES_TEXT_SPACING	3
#define	CSTATES_TEXT_WIDTH	( MAX(A->P.CPU, 8) * CSTATES_TEXT_SPACING )
#define	CSTATES_TEXT_HEIGHT	10

#define	TEMPS_TEXT_WIDTH	(A->P.Features.ThreadCount << 2)
#define	TEMPS_TEXT_HEIGHT	18

#define	SYSINFO_TEXT_WIDTH	80
#define	SYSINFO_TEXT_HEIGHT	20

#define	REG_ALIGN		24
// BIN64: 16 x 4 digits + '\0'
#define BIN64_STR		(16 * 4) + 1
// PRE_TEXT: ##' 'Addr[5]' 'Name&Padding[24]'['
#define PRE_TEXT		2 + 1 + 5 + 1 + REG_ALIGN + 1
// WIDTH: PRE_TEXT + BIN64 w/ 15 interspaces + ']' + ScrollButtons
#define	DUMP_TEXT_WIDTH		PRE_TEXT + BIN64_STR + 15 + 1 + 2
#define	DUMP_TEXT_HEIGHT	13

#define	MENU_FORMAT	"[F1]     Help             [F2]     Core\n"               \
			"[F3]     C-States         [F4]     Temps \n"             \
			"[F5]     System Info      [F6]     Dump\n"               \
			"                                 [Up]\n"                 \
			"  Page Scrolling          [Left]      [Right]\n"         \
			"                                [Down]\n"                \
			"[Pause]  Suspend/Resume\n"                               \
			"[PgDw]   Page Down        [PgUp]   Page Up\n"            \
			"[Home]   Line Begin       [End]    Line End\n"           \
			"KPad [+] Faster Loop      KPad [-] Slower Loop\n\n"      \
			"With the [Control] key, activate the followings :\n"     \
			"[Home]   Page Begin       [End]    Page End\n"           \
			"[L][l]   Refresh page     \n"                            \
			"[Y][y]   Cycles           [W][w]   Wallboard\n"          \
			"[H][h]   Frequency Hz     [P][p]   C-States %\n"         \
			"[R][r]   Ratio values     [Q][q]   Quit\n\n"             \
			"Commands :\n"                                            \
			"[Backspace] Remove the rightmost character\n"            \
			"[Erase] Suppress the full Command line\n"                \
			"[Enter] Submit the Command\n\n"                          \
			"Mouse buttons :\n"                                       \
			"[Left]  Activate Button   [Right] Grab & Move Widget\n"  \
			"[Wheel Down] Page Down    [Wheel Up] Page Up\n"

#define	CORE_NUM	"#%-2d"
#define	CORE_FREQ	"%4.0fMHz"
#define	CORE_CYCLES	"%016llu:%016llu"
#define	CORE_DELTA	"%04llu:%04llu %04llu %04llu / %04llu"
#define	CORE_RATIO	"%-3.1f"
#define	CSTATES_PERCENT	"%6.2f%% %6.2f%% %6.2f%% %6.2f%%"
#define	OVERCLOCK	"%s [%4.0f MHz]"
#define	TEMPERATURE	"%3d"

#define	PROC_FORMAT	"Processor  [%s]\n"									\
			"\n"											\
			"Base Clock [%5.2f MHz]                                 Source [%s]\n"			\
			"\n"											\
			" Family               Model             Stepping             Max# of\n"		\
			"  Code                 No.                 ID                Threads\n"		\
			"[%6X]            [%6X]            [%6d]            [%6d]\n"				\
			"\n"											\
			"Architecture [%s]\n"									\
			"\n"											\
			"Virtual Mode Extension                                        VME [%c]\n"		\
			"Debugging Extension                                            DE [%c]\n"		\
			"Page Size Extension                                           PSE [%c]\n"		\
			"Time Stamp Counter                                            TSC [%c]\n"		\
			"Model Specific Registers                                      MSR [%c]   [%s]\n"	\
			"Physical Address Extension                                    PAE [%c]\n"		\
			"Advanced Programmable Interrupt Controller                   APIC [%c]\n"		\
			"Memory Type Range Registers                                  MTRR [%c]\n"		\
			"Page Global Enable                                            PGE [%c]\n"		\
			"Machine-Check Architecture                                    MCA [%c]\n"		\
			"Page Attribute Table                                          PAT [%c]\n"		\
			"36-bit Page Size Extension                                  PSE36 [%c]\n"		\
			"Processor Serial Number                                       PSN [%c]\n"		\
			"Debug Store & Precise Event Based Sampling               DS, PEBS [%c]   [%s]\n"	\
			"Advanced Configuration & Power Interface                     ACPI [%c]\n"		\
			"Self-Snoop                                                     SS [%c]\n"		\
			"Hyper-Threading                                               HTT [%c]   [%s]\n"	\
			"Thermal Monitor                                TM1 [%c]        TM2 [%c]\n"		\
			"Pending Break Enable                                          PBE [%c]\n"		\
			"64-Bit Debug Store                                         DTES64 [%c]\n"		\
			"CPL Qualified Debug Store                                  DS-CPL [%c]\n"		\
			"Virtual Machine Extensions                                    VMX [%c]\n"		\
			"Safer Mode Extensions                                         SMX [%c]\n"		\
			"SpeedStep                                                    EIST [%c]   [%s]\n"	\
			"L1 Data Cache Context ID                                  CNXT-ID [%c]\n"		\
			"Fused Multiply Add                                            FMA [%c]\n"		\
			"xTPR Update Control                                          xTPR [%c]   [%s]\n"	\
			"Perfmon and Debug Capability                                 PDCM [%c]\n"		\
			"Process Context Identifiers                                  PCID [%c]\n"		\
			"Direct Cache Access                                           DCA [%c]\n"		\
			"Extended xAPIC Support                                     x2APIC [%c]\n"		\
			"Time Stamp Counter Deadline                          TSC-DEADLINE [%c]\n"		\
			"XSAVE/XSTOR States                                          XSAVE [%c]\n"		\
			"OS-Enabled Ext. State Management                          OSXSAVE [%c]\n"		\
			"Execution Disable Bit Support                              XD-Bit [%c]   [%s]\n"	\
			"1 GB Pages Support                                      1GB-PAGES [%c]\n"		\
			"Hardware Lock Elision                                         HLE [%c]\n"		\
			"Restricted Transactional Memory                               RTM [%c]\n"		\
			"Fast-Strings                                      REP MOVSB/STOSB [%c]   [%s]\n"	\
			"Automatic Thermal Control Circuit Enable                      TCC       [%s]\n"	\
			"Performance Monitoring Available                               PM       [%s]\n"	\
			"Branch Trace Storage Unavailable                              BTS       [%s]\n"	\
			"Limit CPUID Maxval                                    Limit-CPUID       [%s]\n"	\
			"\n"											\
			"Turbo Boost:       x %2d    x %2d    x %2d    x %2d             TURBO [%c]   [%s]\n"	\
			"\n"											\
			"Instruction set:\n"									\
			"FPU       [%c]           CX8 [%c]          SEP [%c]    CMOV [%c]        CLFSH [%c]\n"	\
			"MMX       [%c]          FXSR [%c]          SSE [%c]    SSE2 [%c]         SSE3 [%c]\n"	\
			"SSSE3     [%c]        SSE4.1 [%c]       SSE4.2 [%c]                 PCLMULDQ [%c]\n"	\
			"MONITOR   [%c][%s]      CX16 [%c]        MOVBE [%c]                   POPCNT [%c]\n"	\
			"AES       [%c]      AVX/AVX2 [%c/%c]       F16C [%c]                   RDRAND [%c]\n"	\
			"LAHF/SAHF [%c]       SYSCALL [%c]       RDTSCP [%c]    IA64 [%c]    BM1/BM2 [%c/%c]\n"	\
			"\n"

#define	TOPOLOGY_SECTION "Processor Topology:                                       %-3ux CPU Online\n"		\
			"CPU#    APIC    Core  Thread   State\n"
#define	TOPOLOGY_FORMAT	"%03u %8u%8u%8u   [%3s]\n"

#define	PERF_SECTION	"\n"											\
			"Performance:\n"									\
			"Monitoring Counters                                       %-3ux%3u bits\n"		\
			"Fixed Counters                                            %-3ux%3u bits\n"		\
			"\n"											\
			"CPU         Counter#0                Counter#1                Counter#2\n"		\
			" #      OS  User  AnyThread      OS  User  AnyThread      OS  User  AnyThread\n"
#define	PERF_FORMAT	"%03u      %1d     %1d     %1d            %1d     %1d     %1d            %1d     %1d     %1d\n"

#define	RAM_SECTION	"\n"											\
			"Memory Controler:\n"
#define	CHA_FORMAT	"Channel   tCL   tRCD  tRP   tRAS  tRRD  tRFC  tWR   tRTPr tWTPr tFAW  B2B\n"
#define	CAS_FORMAT	"   #%1i   |%4d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n"

#define	SYSINFO_SECTION	"System Information"
//                       ## 12345 123456789012345678901234[1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234]
#define	DUMP_SECTION	"   Addr.     Register               60   56   52   48   44   40   36   32   28   24   20   16   12    8    4    0"
#define	REG_HEXVAL	"%016llX"
#define	REG_FORMAT	"%02d %05X %s%%%zdc["

#define	TITLE_MDI_FMT		"X-Freq %.0fMHz %dC"
#define	TITLE_MAIN_FMT		"X-Freq %s.%s-%s"
#define	TITLE_CORES_FMT		"Core#%d @ %.0f MHz"
#define	TITLE_CSTATES_FMT	"C-States [%.2f%%] [%.2f%%]"
#define	TITLE_TEMPS_FMT		"Core#%d @ %dC"
#define	TITLE_SYSINFO_FMT	"Clock @ %5.2f MHz"

#define	SCROLLED_ROWS_PER_ONCE	1
#define	SCROLLED_ROWS_PER_PAGE	(MAIN_TEXT_HEIGHT >> 1)
#define	SCROLLED_COLS_PER_ONCE	1

#define	SetHScrolling(N, col)	A->L.Page[N].hScroll=col
#define	SetVScrolling(N, row)	A->L.Page[N].vScroll=row
#define	GetHScrolling(N)	A->L.Page[N].hScroll
#define	GetVScrolling(N)	A->L.Page[N].vScroll

#define	SetHViewport(N, col)	A->L.Page[N].Visible.cols=col
#define	SetVViewport(N, row)	A->L.Page[N].Visible.rows=row
#define	GetHViewport(N)		A->L.Page[N].Visible.cols
#define	GetVViewport(N)		A->L.Page[N].Visible.rows

#define	SetHListing(N, col)	A->L.Page[N].Listing.cols=col
#define	SetVListing(N, row)	A->L.Page[N].Listing.rows=row
#define	GetHListing(N)		A->L.Page[N].Listing.cols
#define	GetVListing(N)		A->L.Page[N].Listing.rows

#define	SetHFrame(N, col)	A->L.Page[N].FrameSize.cols=col
#define	SetVFrame(N, row)	A->L.Page[N].FrameSize.rows=row
#define	GetHFrame(N)		A->L.Page[N].FrameSize.cols
#define	GetVFrame(N)		A->L.Page[N].FrameSize.rows

#define	MAIN_FRAME_VIEW_HSHIFT		1
#define	MAIN_FRAME_VIEW_VSHIFT		4
#define	SYSINFO_FRAME_VIEW_HSHIFT	1
#define	SYSINFO_FRAME_VIEW_VSHIFT	3
#define	DUMP_FRAME_VIEW_HSHIFT		1
#define	DUMP_FRAME_VIEW_VSHIFT		1

typedef struct
{
	int	cols,
		rows;
} MaxText;

typedef struct
{
	struct	{
		int		H,
				V;
	} Margin, Start;
	struct	{
		bool		Pageable;
		MaxText		Visible,
				Listing,
				FrameSize;
		int		hScroll,
				vScroll;
		Pixmap		Pixmap;
		int		width,
				height;
		char		*Title;
	} Page[WIDGETS];
	struct {
		bool
				freqHertz,
				cycleValues,
				ratioValues,
				cStatePercent,
				alwaysOnTop,
				wallboard,
				flashCursor,
				hideSplash;
	} Play;
	struct {;
		int		Scroll,
				Length;
		char		*String;
	} WB;
	struct {
		XRectangle	*C0,
				*C3,
				*C6;
	} Usage;
	struct {
		char		*Name;
		unsigned int	Addr;
	} DumpTable[DUMP_TEXT_HEIGHT - 2];
	struct {;
		int		N;
		XSegment	*Segment;
	} Axes[WIDGETS];
	XPoint			TextCursor[3];
	struct {
		char		KeyBuffer[256];
		int		KeyLength;
	} Input;
	char			*Output;
	unsigned long		globalBackground,
				globalForeground;
	COLORS			Colors[COLOR_COUNT];
} LAYOUT;

#define	_IS_MDI_	(A->MDI != false)

// Misc drawing macro.
#define	fDraw(N, DoBuild, DoDraw) {		\
	if(DoBuild)  BuildLayout(A, N);		\
	MapLayout(A, N);			\
	if(DoDraw)   DrawLayout(A, N);		\
	FlushLayout(A, N);			\
}

#define	XDB_SETTINGS_FILE	".xfreq"
#define	XDB_CLASS_MAIN		"Main"
#define	XDB_CLASS_CORES		"Cores"
#define	XDB_CLASS_CSTATES	"CStates"
#define	XDB_CLASS_TEMPS		"Temps"
#define	XDB_CLASS_SYSINFO	"SysInfo"
#define	XDB_CLASS_DUMP		"Dump"
#define	XDB_KEY_FONT		"Font"
#define	XDB_KEY_LEFT		"Left"
#define	XDB_KEY_TOP		"Top"
#define	XDB_KEY_BACKGROUND	"Background"
#define	XDB_KEY_FOREGROUND	"Foreground"
#define	XDB_KEY_CLOCK_SOURCE	"ClockSource"
#define	XDB_KEY_ROM_ADDRESS	"ClockRomAddr"
#define	XDB_KEY_PLAY_FREQ	"PlayFrequency"
#define	XDB_KEY_PLAY_CYCLES	"PlayCycles"
#define	XDB_KEY_PLAY_RATIOS	"PlayRatios"
#define	XDB_KEY_PLAY_CSTATES	"PlayCStates"
#define	XDB_KEY_PLAY_WALLBOARD	"PlayBrand"

#define	OPTIONS_COUNT	18
typedef struct
{
	char		*argument;
	char		*format;
	void		*pointer;
	char		*manual;
	char		*xrmName;
} OPTIONS;

typedef struct
{
	PROCESSOR	P;
	struct IMCINFO	*M;
	Display		*display;
	Screen		*screen;
	pthread_t	TID_Draw;
	char		*fontName;
	XFontStruct	*xfont;
	char		xACL;
	Cursor		MouseCursor[MC_COUNT];
	XSPLASH		Splash;
	XWINDOW		W[WIDGETS];
	XTARGET		T;
	bool		MDI;
	LAYOUT		L;
	bool		LOOP,
			PAUSE[WIDGETS],
			MSR,
			BIOS;
	OPTIONS		Options[OPTIONS_COUNT];
} uARG;

typedef struct
{
	int		cpu;
	pthread_t	TID;
	uARG		*A;
} uAPIC;

typedef	struct
{
	char	*Instruction;
	void	(*Procedure)();
} COMMAND;

/* --- Splash bitmap --- */

#define splash_width 267
#define splash_height 50
static unsigned char splash_bits[] = {
   0xff, 0x3f, 0x00, 0x00, 0xf8, 0xff, 0x01, 0x00, 0x00, 0x00, 0xf0, 0xff,
   0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xfe, 0x3f,
   0x00, 0x00, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff,
   0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xc0, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0x01, 0x00, 0xfe, 0x7f, 0x00, 0x00,
   0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x3f, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe,
   0xff, 0xff, 0xff, 0xff, 0x03, 0x00, 0xfc, 0xff, 0x00, 0x00, 0xfe, 0x7f,
   0x00, 0x00, 0x00, 0x00, 0xfe, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x7f, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe, 0xff, 0xff,
   0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xf8, 0xff, 0x01, 0x00, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00,
   0xf0, 0xff, 0x03, 0x80, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0xe0, 0xff,
   0x07, 0xc0, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0xc0, 0xff, 0x07, 0xe0,
   0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
   0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0xc0, 0xff, 0x0f, 0xe0, 0xff, 0x03,
   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff,
   0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xff, 0x3f, 0xf8, 0xff, 0x01, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0,
   0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00,
   0x00, 0xfe, 0x3f, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07,
   0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00,
   0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xfe,
   0x7f, 0xfc, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00,
   0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00,
   0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xfc, 0xff, 0xfe,
   0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0,
   0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff,
   0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00,
   0x00, 0xe0, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xc0,
   0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xf0, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xc0, 0xff, 0xff,
   0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
   0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00,
   0x00, 0x00, 0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00,
   0xfe, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xf0, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0xff, 0xff,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff,
   0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xff, 0xff, 0x03, 0x00, 0x00, 0xff,
   0xff, 0x03, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0,
   0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe,
   0x07, 0x00, 0x00, 0xc0, 0xff, 0xff, 0x07, 0x00, 0x00, 0xff, 0xff, 0x03,
   0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0,
   0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00,
   0x00, 0xe0, 0xff, 0xff, 0x07, 0x00, 0x00, 0xff, 0xff, 0x03, 0xff, 0x07,
   0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00,
   0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xe0,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0xff, 0xff, 0x03, 0xff, 0x07, 0x00, 0x00,
   0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00,
   0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xf0, 0xff, 0xff,
   0x1f, 0x00, 0x00, 0xff, 0xff, 0x03, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0,
   0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff,
   0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0,
   0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe,
   0x07, 0x00, 0x00, 0xfc, 0x7f, 0xfe, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0,
   0x7f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00,
   0x00, 0xfe, 0x7f, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07,
   0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00,
   0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0xff,
   0x3f, 0xf8, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00,
   0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0x7f, 0x00, 0x00, 0x00,
   0x00, 0xff, 0x07, 0x00, 0x00, 0xfe, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xc0, 0xff, 0x0f, 0xe0, 0xff, 0x07, 0x00, 0x00,
   0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0,
   0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x07, 0xe0, 0xff, 0x07, 0xc0, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07,
   0xe0, 0xff, 0x03, 0xc0, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07,
   0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xf0, 0xff,
   0x03, 0x80, 0xff, 0x1f, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00,
   0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xf0, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0xfc, 0xff, 0x00, 0x00, 0xfe, 0x7f, 0x00, 0x00,
   0x00, 0x00, 0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0,
   0x7f, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xfe, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x07, 0xfe, 0x7f, 0x00, 0x00, 0xfc, 0xff, 0x00, 0x00, 0x00, 0x00,
   0xff, 0x07, 0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0xc0,
   0xff, 0xff, 0xff, 0xff, 0x0f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07,
   0xff, 0x3f, 0x00, 0x00, 0xf8, 0xff, 0x00, 0x00, 0x00, 0x00, 0xff, 0x07,
   0x00, 0x00, 0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0x80, 0xff, 0xff,
   0xff, 0xff, 0x0f, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07, 0xff, 0x1f,
   0x00, 0x00, 0xf0, 0xff, 0x01, 0x00, 0x00, 0x00, 0xff, 0x07, 0x00, 0x00,
   0x00, 0xf0, 0x7f, 0x00, 0x00, 0xe0, 0x7f, 0x00, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0x07 };

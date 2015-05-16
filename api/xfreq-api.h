/*
 * xfreq-api.h by CyrIng
 *
 * XFreq
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

#define _MAJOR   "2"
#define _MINOR   "1"
#define _NIGHTLY "52"
#define AutoDate _APPNAME" "_MAJOR"."_MINOR"-"_NIGHTLY" (C) CYRIL INGENIERIE "__DATE__"\n"

#if defined(Linux)
#define	SHM_FILENAME	"xfreq-shm"
#define	SMB_FILENAME	"xfreq-smb"
#else
#define	SHM_FILENAME	"/xfreq-shm"
#define	SMB_FILENAME	"/xfreq-smb"
#endif


enum	{SRC_TSC, SRC_BIOS, SRC_SPEC, SRC_ROM, SRC_TSC_AUX, SRC_COUNT};

typedef struct
{
	char		VendorID[16];
	struct
	{
		struct SIGNATURE
		{
			unsigned int
			Stepping	:  4-0,
			Model		:  8-4,
			Family		: 12-8,
			ProcType	: 14-12,
			Unused1		: 16-14,
			ExtModel	: 20-16,
			ExtFamily	: 28-20,
			Unused2		: 32-28;
		} AX;
		struct
		{
			unsigned int
			Brand_ID	:  8-0,
			CLFSH_Size	: 16-8,
			MaxThread	: 24-16,
			APIC_ID		: 32-24;
		} BX;
		struct
		{
			unsigned int
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
		} CX;
		struct
		{
			unsigned int
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
		} DX;
	} Std;
	unsigned int	ThreadCount;
	struct
	{
		struct
		{
			unsigned int
			SmallestSize	: 16-0,
			ReservedBits	: 32-16;
		} AX;
		struct
		{
			unsigned int
			LargestSize	: 16-0,
			ReservedBits	: 32-16;
		} BX;
		struct
		{
			unsigned int
			ExtSupported	:  1-0,
			BK_Int_MWAIT	:  2-1,
			ReservedBits	: 32-2;
		} CX;
		struct
		{
			unsigned int
			Num_C0_MWAIT	:  4-0,
			Num_C1_MWAIT	:  8-4,
			Num_C2_MWAIT	: 12-8,
			Num_C3_MWAIT	: 16-12,
			Num_C4_MWAIT	: 20-16,
			ReservedBits	: 32-20;
		} DX;
	} MONITOR_MWAIT_Leaf;
	struct
	{
		struct
		{
			unsigned int
			DTS	:  1-0,
			TurboIDA:  2-1,
			ARAT	:  3-2,
			Unused1	:  4-3,
			PLN	:  5-4,
			ECMD	:  6-5,
			PTM	:  7-6,
			Unused2	: 32-7;
		} AX;
		struct
		{
			unsigned int
			Threshld:  4-0,
			Unused1	: 32-4;
		} BX;
		struct
		{
			unsigned int
			HCF_Cap	:  1-0,
			ACNT_Cap:  2-1,
			Unused1	:  3-2,
			PEB_Cap	:  4-3,
			Unused2	: 32-4;
		} CX;
		struct
		{
			unsigned int
			Unused1	: 32-0;
		} DX;
	} Thermal_Power_Leaf;
	struct
	{
		struct
		{
			unsigned int
			Version	:  8-0,
			MonCtrs	: 16-8,
			MonWidth: 24-16,
			VectorSz: 32-24;
		} AX;
		struct
		{
			unsigned int
			CoreCycles	:  1-0,
			InstrRetired	:  2-1,
			RefCycles	:  3-2,
			LLC_Ref		:  4-3,
			LLC_Misses	:  5-4,
			BranchRetired	:  6-5,
			BranchMispred	:  7-6,
			ReservedBits	: 32-7;
		} BX;
		struct
		{
			unsigned int
			Unused1	: 32-0;
		} CX;
		struct
		{
			unsigned int
			FixCtrs	:  5-0,
			FixWidth: 13-5,
			Unused1	: 32-13;
		} DX;
	} Perf_Monitoring_Leaf;
	struct
	{
		struct
		{
			unsigned int
			MaxSubLeaf	: 32-0;
		} AX;
		struct
		{
			unsigned int
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
		} BX;
			unsigned int
		CX			: 32-0,
		DX			: 32-0;

	} ExtFeature;
	unsigned int	LargestExtFunc;
	struct
	{
		struct
		{
			unsigned int
			LAHFSAHF:  1-0,
			Unused1	: 32-1;
		} CX;
		struct
		{
			unsigned int
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
		} DX;
	} ExtFunc;
	unsigned int	InvariantTSC,
                        HTT_enabled;
	char		BrandString[48];
} FEATURES;

#define	IA32_TIME_STAMP_COUNTER		0x10
#define	IA32_PLATFORM_ID		0x17
#define	IA32_MPERF			0xe7
#define	IA32_APERF			0xe8
#define	IA32_PERF_STATUS		0x198
#define	IA32_PERF_CTL			0x199
#define	IA32_CLOCK_MODULATION		0x19a
#define	IA32_THERM_INTERRUPT		0x19b
#define IA32_THERM_STATUS		0x19c
#define	IA32_MISC_ENABLE		0x1a0
#define	IA32_ENERGY_PERF_BIAS		0x1b0
#define	IA32_PKG_THERM_STATUS		0x1b1
#define	IA32_PKG_THERM_INTERRUPT 	0x1b2
#define	IA32_MTRR_DEF_TYPE		0x2ff
#define	IA32_FIXED_CTR0			0x309
#define	IA32_FIXED_CTR1			0x30a
#define	IA32_FIXED_CTR2			0x30b
#define	IA32_FIXED_CTR_CTRL		0x38d
#define	IA32_PERF_GLOBAL_STATUS		0x38e
#define	IA32_PERF_GLOBAL_CTRL		0x38f
#define	IA32_PERF_GLOBAL_OVF_CTRL	0x390
#define	IA32_EFER			0xc0000080
#define	MSR_CORE_C3_RESIDENCY		0x3fc
#define	MSR_CORE_C6_RESIDENCY		0x3fd
#define	MSR_CORE_C7_RESIDENCY		0x3fe
#define	MSR_FSB_FREQ			0xcd
#define	MSR_PLATFORM_INFO		0xce
#define	MSR_PKG_CST_CONFIG_CTRL		0xe2
#define	MSR_TURBO_RATIO_LIMIT		0x1ad
#define	MSR_TEMPERATURE_TARGET		0x1a2
#define	MSR_POWER_CTL			0x1fc

typedef	struct
{
	unsigned long long int
		ReservedBits1	:  8-0,
		MaxBusRatio	: 13-8,
		ReservedBits2	: 50-13,
		PlatformId	: 53-50,
		ReservedBits3	: 64-53;
} PLATFORM_ID;

typedef	struct
{
	unsigned long long int
		Bus_Speed	:  3-0,
		ReservedBits	: 64-3;
} FSB_FREQ;

typedef	struct
{
	unsigned long long int
		CurrentRatio	: 16-0,
		ReservedBits1	: 31-16,
		XE		: 32-31,
		ReservedBits2	: 40-32,
		MaxBusRatio	: 45-40,
		ReservedBits3	: 46-45,
		NonInt_BusRatio	: 47-46,
		ReservedBits4	: 64-47;
} PERF_STATUS;

typedef	struct
{
	unsigned long long int
		EIST_Target	: 16-0,
		ReservedBits1	: 32-16,
		Turbo_IDA	: 33-32,
		ReservedBits2	: 64-33;
} PERF_CONTROL;

typedef struct
{
	unsigned long long int
		ReservedBits1	:  8-0,
		MaxNonTurboRatio: 16-8,
		ReservedBits2	: 28-16,
		Ratio_Limited	: 29-28,
		TDC_TDP_Limited	: 30-29,
		ReservedBits3	: 32-30,
		LowPowerMode	: 33-32,
		ConfigTDPlevels	: 35-33,
		ReservedBits4	: 40-35,
		MinimumRatio	: 48-40,
		MinOpeRatio	: 56-48,
		ReservedBits5	: 64-56;
} PLATFORM_INFO;

typedef struct
{
	unsigned long long int
		Pkg_CST_Limit	:  3-0,
		ReservedBits1	: 10-3,
		IO_MWAIT_Redir	: 11-10,
		ReservedBits2	: 15-11,
		CFG_Lock	: 16-15,
		ReservedBits3	: 24-16,
		Int_Filtering	: 25-24,	// Nehalem
		C3autoDemotion	: 26-25,
		C1autoDemotion	: 27-26,
		C3undemotion	: 28-27,	// Sandy Bridge
		C1undemotion	: 29-28,	// Sandy Bridge
		ReservedBits4	: 64-29;
} CSTATE_CONFIG;

typedef struct
{
	unsigned long long int
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
	unsigned long long int
		FastStrings	:  1-0,
		ReservedBits1	:  3-1,
		TCC		:  4-3,
		ReservedBits2	:  7-4,
		PerfMonitoring	:  8-7,
		ReservedBits3	: 11-8,
		BTS		: 12-11,
		PEBS		: 13-12,
		TM2_Enable	: 14-13,
		ReservedBits4	: 16-14,
		EIST		: 17-16,
		ReservedBits5	: 18-17,
		FSM		: 19-18,
		ReservedBits6	: 22-19,
		CPUID_MaxVal	: 23-22,
		xTPR		: 24-23,
		ReservedBits7	: 34-24,
		XD_Bit		: 35-34,
		ReservedBits8	: 37-35,
		DCU_Prefetcher	: 38-37,
		Turbo_IDA	: 39-38,
		IP_Prefetcher	: 40-39,
		ReservedBits9	: 64-40;
} MISC_PROC_FEATURES;

typedef struct
{
	unsigned long long int
		Type		:  8-0,
		ReservedBits1	: 10-8,
		FixeRange	: 11-10,
		Enable		: 12-11,
		ReservedBits2	: 64-12;
} MTRR_DEF_TYPE;

typedef struct
{
	unsigned long long int
		EN_PMC0		:  1-0,
		EN_PMC1		:  2-1,
		EN_PMC2		:  3-2,
		EN_PMC3		:  4-3,
		EN_PMCn		: 32-4,
		EN_FIXED_CTR0	: 33-32,
		EN_FIXED_CTR1	: 34-33,
		EN_FIXED_CTR2	: 35-34,
		ReservedBits2	: 64-35;
} GLOBAL_PERF_COUNTER;

typedef struct
{
	unsigned long long int
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
	unsigned long long int
		Overflow_PMC0	:  1-0,
		Overflow_PMC1	:  2-1,
		Overflow_PMC2	:  3-2,
		Overflow_PMC3	:  4-3,
		Overflow_PMCn	: 32-4,
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
	unsigned long long int
		Clear_Ovf_PMC0	:  1-0,
		Clear_Ovf_PMC1	:  2-1,
		Clear_Ovf_PMC2	:  3-2,
		Clear_Ovf_PMC3	:  4-3,
		Clear_Ovf_PMCn	: 32-2,
		Clear_Ovf_CTR0 	: 33-32,
		Clear_Ovf_CTR1	: 34-33,
		Clear_Ovf_CTR2	: 35-34,
		ReservedBits2	: 61-35,
		Clear_Ovf_UNC	: 62-61,
		Clear_Ovf_Buf	: 63-62,
		Clear_CondChg	: 64-63;
} GLOBAL_PERF_OVF_CTRL;

typedef	struct
{
	unsigned long long int
		SCE		:  1-0,
		ReservedBits1	:  8-1,
		LME		:  9-8,
		ReservedBits2	: 10-9,
		LMA		: 11-10,
		NXE		: 12-11,
		ReservedBits3	: 64-12;
} EXT_FEATURE_ENABLE;

typedef struct
{
	unsigned long long int
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
	unsigned long long int
		StatusBit	:  1-0,
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
	unsigned long long int
		ReservedBits1	: 16-0,
		Target		: 24-16,
		ReservedBits2	: 64-24;
} TJMAX;

typedef	struct
{
	unsigned long long int
		ReservedBits1	:  1-0,
		C1E		:  2-1,
		ReservedBits2	: 64-2;
} POWER_CONTROL;

#define	IDLE_BASE_USEC	50000
#define	IDLE_SCHED_DEF	19
#define	IDLE_COEF_DEF	20
#define	IDLE_COEF_MAX	80
#define	IDLE_COEF_MIN	2

typedef struct
{
	signed long int			ArchID;
	FEATURES			Features;
	PLATFORM_ID			PlatformId;
	PERF_STATUS			PerfStatus;
	PERF_CONTROL			PerfControl;
	MISC_PROC_FEATURES		MiscFeatures;
	MTRR_DEF_TYPE			MTRRdefType;
	EXT_FEATURE_ENABLE		ExtFeature;
	PLATFORM_INFO			PlatformInfo;
	CSTATE_CONFIG			CStateConfig;
	TURBO				Turbo;
	POWER_CONTROL			PowerControl;
	off_t				BClockROMaddr;
	double				ClockSpeed;
	unsigned int			CPU,
					OnLine;
	unsigned int			Boost[1+1+8];

	struct {
		double			Turbo,
					C0,
					C3,
					C6,
					C7,
					C1;
	} Avg;
	unsigned int			Top,
					Hot,
					Cold,
					PerCore,
					ClockSrc;
	useconds_t			IdleTime;
} PROCESSOR;

#define	ARCHITECTURE_LEN	32
typedef	struct
{
	struct	SIGNATURE	Signature;
		unsigned int	MaxOfCores;
		double		ClockSpeed;
		char		Architecture[ARCHITECTURE_LEN];
} ARCHITECTURE;

#define	DUMP_ARRAY_DIMENSION	16
#define	DUMP_REG_ALIGN		24
typedef	struct
{
	unsigned int		Core;
	char			Name[DUMP_REG_ALIGN];
	unsigned int		Addr;
	unsigned long long int	Value;
} DUMP_ARRAY;

#define	DUMP_LOADER							\
{									\
	{ 0, "IA32_PLATFORM_ID",        IA32_PLATFORM_ID,        0},	\
	{ 0, "MSR_PLATFORM_INFO",       MSR_PLATFORM_INFO,       0},	\
	{ 0, "IA32_MISC_ENABLE",        IA32_MISC_ENABLE,        0},	\
	{ 0, "IA32_PERF_STATUS",        IA32_PERF_STATUS,        0},	\
	{ 0, "IA32_THERM_STATUS",       IA32_THERM_STATUS,       0},	\
	{ 0, "MSR_TURBO_RATIO_LIMIT",   MSR_TURBO_RATIO_LIMIT,   0},	\
	{ 0, "MSR_TEMPERATURE_TARGET",  MSR_TEMPERATURE_TARGET,  0},	\
	{ 0, "IA32_FIXED_CTR1",         IA32_FIXED_CTR1,         0},	\
	{ 0, "IA32_FIXED_CTR2",         IA32_FIXED_CTR2,         0},	\
	{ 0, "MSR_CORE_C3_RESIDENCY",   MSR_CORE_C3_RESIDENCY,   0},	\
	{ 1, "IA32_FIXED_CTR1",         IA32_FIXED_CTR1,         0},	\
	{ 1, "IA32_FIXED_CTR2",         IA32_FIXED_CTR2,         0},	\
	{ 2, "IA32_FIXED_CTR1",         IA32_FIXED_CTR1,         0},	\
	{ 2, "IA32_FIXED_CTR2",         IA32_FIXED_CTR2,         0},	\
	{ 3, "IA32_FIXED_CTR1",         IA32_FIXED_CTR1,         0},	\
	{ 3, "IA32_FIXED_CTR2",         IA32_FIXED_CTR2,         0},	\
}

typedef struct
{
	Bool64		Monitor;
	DUMP_ARRAY	Array[DUMP_ARRAY_DIMENSION];
} DUMP_STRUCT;

#define	IMC_MAX	16
typedef	struct
{
	unsigned int ChannelCount;
	struct CHANNEL
	{
		struct
		{
			unsigned int
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
	}	Channel[IMC_MAX];
} IMC_INFO;

enum {	GenuineIntel,		\
	Core_Yonah,		\
	Core_Conroe,		\
	Core_Kentsfield,	\
	Core_Yorkfield,		\
	Core_Dunnington,	\
	Atom_Bonnell,		\
	Atom_Silvermont,	\
	Atom_Lincroft,		\
	Atom_Clovertrail,	\
	Atom_Saltwell,		\
	Silvermont_637,		\
	Silvermont_64D,		\
	Nehalem_Bloomfield,	\
	Nehalem_Lynnfield,	\
	Nehalem_MB,		\
	Nehalem_EX,		\
	Westmere,		\
	Westmere_EP,		\
	Westmere_EX,		\
	SandyBridge,		\
	SandyBridge_EP,		\
	IvyBridge,		\
	IvyBridge_EP,		\
	Haswell_DT,		\
	Haswell_MB,		\
	Haswell_ULT,		\
	Haswell_ULX,		\
	ARCHITECTURES
};

typedef	enum
{
		SORT_FIELD_NONE       = 0x0,
		SORT_FIELD_STATE      = 0x1,
		SORT_FIELD_COMM       = 0x2,
		SORT_FIELD_PID        = 0x3,
		SORT_FIELD_RUNTIME    = 0x4,
		SORT_FIELD_CTX_SWITCH = 0x5,
		SORT_FIELD_PRIORITY   = 0x6,
		SORT_FIELD_EXEC       = 0x7,
		SORT_FIELD_SUM_EXEC   = 0x8,
		SORT_FIELD_SUM_SLEEP  = 0x9,
		SORT_FIELD_NODE       = 0xa,
		SORT_FIELD_GROUP      = 0xb
} SORT_FIELDS;

#define	TASK_COMM_LEN		16
#define	TASK_PIPE_DEPTH		10
typedef	struct
{
	Bool32		Monitor;
	unsigned int	Attributes;
} SCHEDULE;

typedef	struct
{
	long long int	nsec_high,
			nsec_low;
} RUNTIME;

typedef	struct
{
	Bool32		Offline;
	unsigned
			APIC_ID	 : 32-0,
			Core_ID	 : 32-0,
			Thread_ID: 32-0;
} TOPOLOGY;

typedef	struct {
	TOPOLOGY		T;
	GLOBAL_PERF_COUNTER	GlobalPerfCounter;
	FIXED_PERF_COUNTER	FixedPerfCounter;
	struct {
		unsigned long long int
				INST[2];
		struct
		{
		unsigned long long int
				UCC,
				URC;
		}		C0[2];
		unsigned long long int
				C3[2],
				C6[2],
				C7[2],
				TSC[2],
				C1[2];
	} Cycles;
	struct {
		unsigned long long int
				INST;
		struct
		{
		unsigned long long int
				UCC,
				URC;
		}		C0;
		unsigned long long int
				C3,
				C6,
				C7,
				TSC,
				C1;
	} Delta;
	double			IPS,
				IPC,
				CPI;
	struct {
		double		Turbo,
				C0,
				C3,
				C6,
				C7,
				C1;
	} State;
	double			RelativeRatio,
				RelativeFreq;
	TJMAX			TjMax;
	THERM_INTERRUPT		ThermIntr;
	THERM_STATUS		ThermStat;

	struct TASK_STRUCT
	{
		char		state[8];
		char		comm[TASK_COMM_LEN];
		long int	pid;
		RUNTIME		vruntime;
		long long int	nvcsw;			// sum of [non]voluntary context switch counts
		long int	prio;
		RUNTIME		exec_vruntime;		// a duplicate of vruntime ?
		RUNTIME		sum_exec_runtime;
		RUNTIME		sum_sleep_runtime;
		long int	node;
		char		group_path[32];		// #define PATH_MAX 4096 # chars in a path name including nul
	} Task[TASK_PIPE_DEPTH];

	signed int		FD,
				Align64;
} CPU_STRUCT;

typedef	struct
{
	Bool64	MSR,
		RESET,
		SMBIOS,
		IMC,
		PROC;
} PRIVILEGE_LEVEL;

typedef struct
{
	atomic_ullong	IF;
	atomic_ullong	Rooms;
	atomic_ullong	Play;
	atomic_ullong	Data;
} SYNCHRONIZATION;

typedef	union
{
	unsigned long long int		Map64;
	struct {
		unsigned int		Addr;
		unsigned short int	Core;
		unsigned char		Arg;
		unsigned char		ID;
	} Map;
} XCHG_MAP;

typedef	struct
{
	char		AppName[TASK_COMM_LEN];
	SYNCHRONIZATION	Sync;
	PRIVILEGE_LEVEL	CPL;
	PROCESSOR	P;
	ARCHITECTURE	H;
	DUMP_STRUCT	D;
	IMC_INFO	M;
	SCHEDULE	S;
	SMBIOS_TREE	*B;
	CPU_STRUCT	C[];
} SHM_STRUCT;

#define	ID_NULL		0x00
#define	ID_DONE		0x80
#define	ID_QUIT		0x7f
#define	ID_SCHED	0x7e
#define	ID_RESET	0x7d
#define	ID_TSC		0x7c
#define	ID_BIOS		0x7b
#define	ID_SPEC		0x7a
#define	ID_ROM		0x79
#define	ID_TSC_AUX	0x78
#define	ID_REFRESH	0x77
#define	ID_INCLOOP	0x76
#define	ID_DECLOOP	0x75
#define	ID_DUMPMSR	0x74
#define	ID_READMSR	0x73
#define	ID_WRITEMSR	0x72
#define	ID_CTLFEATURE	0x71

#define	CTL_NOP		0b00000000
#define	CTL_ENABLE	0b00000001
#define	CTL_DISABLE	0b10000000
#define	CTL_TURBO	0b00000010
#define	CTL_EIST	0b00000100
#define	CTL_C1E		0b00001000
#define	CTL_C3A		0b00010000
#define	CTL_C1A		0b00100000
#define	CTL_TCC		0b01000000

#define	SIG_EMERGENCY_FMT	"\nShutdown(%02d)"
#define	TASK_PID_FMT		"%5ld"

extern unsigned int ROL32(unsigned int r32, unsigned short int m16);
extern unsigned int ROR32(unsigned int r32, unsigned short int m16);
/*	extern unsigned long long int BT64(unsigned long long int r64, unsigned long long int i64);	*/
extern void abstimespec(useconds_t usec, struct timespec *tsec);
extern int addtimespec(struct timespec *asec, const struct timespec *tsec);
extern void Sync_Init(SYNCHRONIZATION *sync);
extern void Sync_Destroy(SYNCHRONIZATION *sync);
extern unsigned int Sync_Open(SYNCHRONIZATION *sync);
extern void Sync_Close(unsigned int room, SYNCHRONIZATION *sync);
extern long int Sync_Wait(unsigned int room, SYNCHRONIZATION *sync, useconds_t idleTime);
extern void Sync_Signal(unsigned int room, SYNCHRONIZATION *sync);

extern char *Smb_Find_String(struct STRUCTINFO *smb, int ID);

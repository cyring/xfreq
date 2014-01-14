/*
 * XFreq.c #0.12 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

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
			DS	: 22-21,
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
	} Ext;
	char		BrandString[48+1];
} FEATURES;

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

#define	MSR_PLATFORM_INFO		0xce
#define	IA32_PERF_STATUS		0x198
#define IA32_THERM_STATUS		0x19c
#define MSR_TEMPERATURE_TARGET		0x1a2
#define	MSR_TURBO_RATIO_LIMIT		0x1ad
#define	MSR_PERF_FIXED_CTR1		0x30a
#define	MSR_PERF_FIXED_CTR2		0x30b
#define	MSR_PERF_FIXED_CTR_CTRL		0x38d
#define	MSR_PERF_GLOBAL_CTRL		0x38f

#define	SMBIOS_PROCINFO_STRUCTURE	4
#define	SMBIOS_PROCINFO_INSTANCE	0
#define	SMBIOS_PROCINFO_EXTCLK		0x12
#define	SMBIOS_PROCINFO_THREADS		0x25

typedef struct
{
	unsigned  long long
		ReservedBits1	:  8-0,
		MaxNonTurboRatio: 16-8,
		ReservedBits2	: 28-16,
		Ratio_Limited	: 29-28,
		TDC_TDP_Limited	: 30-29,
		ReservedBits3	: 40-30,
		MinimumRatio	: 48-40,
		ReservedBits4	: 64-48;
} PLATFORM;

typedef struct {
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

typedef struct {
	unsigned long long
		EN_PMC0		:  1-0,
		EN_PMC1		:  2-1,
		ReservedBits1	: 32-2,
		EN_FIXED_CTR0	: 33-32,
		EN_FIXED_CTR1	: 34-33,
		EN_FIXED_CTR2	: 35-34,
		ReservedBits2	: 64-35;
} GLOBAL_PERF_COUNTER;

typedef struct {
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

typedef struct {
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
} THERM;

typedef struct {
	unsigned long long
		ReservedBits1	: 16-0,
		Target		: 24-16,
		ReservedBits2	: 64-24;
} TJMAX;

#define	ARCHITECTURES 5
//	[Nehalem]
#define	Nehalem_CPUID_SIGNATURE		{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xA}
#define	Nehalem_BASE_CLOCK		133
//	[Sandy Bridge]
#define	Sandy_1G_CPUID_SIGNATURE	{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xD}
#define	Sandy_2G_CPUID_SIGNATURE	{ExtFamily:0x0, Family:0x6, ExtModel:0x2, Model:0xA}
#define	Sandy_BASE_CLOCK		100
//	[Ivy Bridge]
#define	Ivy_CPUID_SIGNATURE		{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xA}
#define	Ivy_BASE_CLOCK			100
//	[Haswell]
#define	Haswell_CPUID_SIGNATURE		{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xC}
#define	Haswell_BASE_CLOCK		100

const struct {
	struct	SIGNATURE	Signature;
		unsigned int	ClockSpeed;;
} DEFAULT[ARCHITECTURES]={
				{ Nehalem_CPUID_SIGNATURE,  Nehalem_BASE_CLOCK },
				{ Sandy_1G_CPUID_SIGNATURE, Sandy_BASE_CLOCK   },
				{ Sandy_2G_CPUID_SIGNATURE, Sandy_BASE_CLOCK   },
				{ Ivy_CPUID_SIGNATURE,      Ivy_BASE_CLOCK     },
				{ Haswell_CPUID_SIGNATURE,  Haswell_BASE_CLOCK },
			};

typedef struct {
		FEATURES			Features;
		PLATFORM			Platform;
		TURBO				Turbo;
		unsigned int			Top;
		signed int			ArchID;
		unsigned int			ClockSpeed;
		short int			ThreadCount;
		struct THREADS {
			signed int		FD;
			unsigned int		OperatingRatio,
						OperatingFreq;
			GLOBAL_PERF_COUNTER	GlobalPerfCounter;
			FIXED_PERF_COUNTER	FixedPerfCounter;
			unsigned long long	UnhaltedCoreCycles[2],
						UnhaltedRefCycles[2];
			unsigned int		UnhaltedFreq;
			TJMAX	TjMax;
			THERM	Therm;
		} *Core;
		useconds_t			IdleTime;
} PROCESSOR;

typedef struct {
	Display		*display;
	Window		window;
	Screen		*screen;
	struct {
		Pixmap	B,
			F;
	} pixmap;
	GC		gc;
	int		x,
			y;
	int		width,
			height;
	struct	{
		int	width,
			height;
	} margin;
	bool		alwaysOnTop,
			activity,
			pulse;
	struct	{
		char	fname[256];
	    XFontStruct	*font;
	    XCharStruct	overall;
		int	dir,
			ascent,
			descent,
			charWidth,
			charHeight;
	} extents;
	unsigned long	background,
			foreground;
	XSizeHints	*hints;
	XWindowAttributes attribs;
} XWINDOW;

typedef struct {
	Window		window;
	char		name[32];
	Window		child;
} DESKTOP;

#define	HDSIZE	".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"
#define	APP_TITLE	"#%d @ %dMHz - %dC"

typedef enum {MENU, CORE, PROC, RAM, BIOS, _COP_} PAGES;

#define	MENU_TITLE	"X-Freq"
#define	MENU_FORMAT	"[F1]     Help             [ESC]    Quit\n"        \
			"[F2]     Core             [F3]     Processor\n"   \
			"[F3]     RAM              [F4]     BIOS\n"        \
			"[PgDw]   Previous page    [PgUp]   Next page\n"   \
			"[Pause]  Suspend          [Return] Redraw\n"      \
			"[Home]   Keep on top      [End]    Keep below\n"  \
			"[P][p]   Activity pulse   [C][c]   Center page\n" \
			"                                 [Up]\n"          \
			"  Scrolling page          [Left]      [Right]\n"  \
			"                                [Down]\n"
#define	CORE_FREQ	" #%-2d%5d MHz "
#define	EXTCLK		"Clock[%3d MHz]"

#define	PROC_TITLE	"Processor"
#define	PROC_FORMAT	"[%s]\n\n"                                             \
			" Family       Model     Stepping     Max# of\n"       \
			"  Code         No.         ID        Threads\n"       \
			"[%6X]    [%6X]    [%6d]    [%6d]\n\n"                 \
			"Virtual Mode Extension                 VME [%c]\n" \
			"Debugging Extension                     DE [%c]\n" \
			"Page Size Extension                    PSE [%c]\n" \
			"Time Stamp Counter                     TSC [%c]\n" \
			"Model Specific Registers               MSR [%c]\n" \
			"Physical Address Extension             PAE [%c]\n" \
			"Advanced Programmable Interrupt Ctrl. APIC [%c]\n" \
			"Memory Type Range Registers           MTRR [%c]\n" \
			"Page Global Enable                     PGE [%c]\n" \
			"Machine-Check Architecture             MCA [%c]\n" \
			"Page Attribute Table                   PAT [%c]\n" \
			"36-bit Page Size Extension           PSE36 [%c]\n" \
			"Processor Serial Number                PSN [%c]\n" \
			"Debug Store                             DS [%c]\n" \
			"Advanced Configuration & Power Intrf. ACPI [%c]\n" \
			"Self-Snoop                              SS [%c]\n" \
			"Hyper-Threading                        HTT [%c]\n" \
			"Thermal Monitor               TM1 [%c]  TM2 [%c]\n" \
			"Pending Break Enable                   PBE [%c]\n" \
			"64-Bit Debug Store                  DTES64 [%c]\n" \
			"CPL Qualified Debug Store           DS-CPL [%c]\n" \
			"Virtual Machine Extensions             VMX [%c]\n" \
			"Safer Mode Extensions                  SMX [%c]\n" \
			"SpeedStep                             EIST [%c]\n" \
			"L1 Data Cache Context ID           CNXT-ID [%c]\n" \
			"Fused Multiply Add                     FMA [%c]\n" \
			"xTPR Update Control                   xTPR [%c]\n" \
			"Perfmon and Debug Capability          PDCM [%c]\n" \
			"Process Context Identifiers           PCID [%c]\n" \
			"Direct Cache Access                    DCA [%c]\n" \
			"Extended xAPIC Support              x2APIC [%c]\n" \
			"Time Stamp Counter Deadline   TSC-DEADLINE [%c]\n" \
			"XSAVE/XSTOR States                   XSAVE [%c]\n" \
			"OS-Enabled Ext. State Management   OSXSAVE [%c]\n" \
			"Execution Disable Bit Support       XD-Bit [%c]\n" \
			"1 GB Pages Support               1GB-PAGES [%c]\n" \
			"\nInstruction set:\n"                              \
			"FPU[%c]    CX8[%c]   SEP[%c]   CMOV[%c]   CLFSH[%c]\n" \
			"MMX[%c]   FXSR[%c]   SSE[%c]   SSE2[%c]    SSE3[%c]\n" \
			"SSSE3[%c]    SSE4.1[%c]   SSE4.2[%c]  PCLMULDQ[%c]\n" \
			"MONITOR[%c]    CX16[%c]    MOVBE[%c]    POPCNT[%c]\n" \
			"AES[%c]         AVX[%c]     F16C[%c]    RDRAND[%c]\n" \
			"LAHF/SAHF[%c]   SYSCALL[%c]   RDTSCP[%c]  IA64[%c]\n"


#define	RAM_TITLE	"RAM"
#define	CHA_FORMAT	"Channel   tCL   tRCD  tRP   tRAS  tRRD  tRFC  tWR   tRTPr tWTPr tFAW  B2B\n"
#define	CAS_FORMAT	"   #%1i   |%4d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n"

#define	BIOS_TITLE	"BIOS"
#define	BIOS_FORMAT	"Base Clock [%3d MHz]\n"

typedef struct {
	int	cols,
		rows;
} XMAXPRINT;

typedef struct {
	int		currentPage;
	struct	{
		bool	pageable;
		char	*title;
		XMAXPRINT max;
		int	hScroll,
			vScroll;
	} Page[_COP_];
	char		string[sizeof(HDSIZE)];
	char		bclock[sizeof(EXTCLK)];
	char		bump[2+2+2+1];
	XRectangle	*usage;
	XSegment	*axes;
} LAYOUT;

typedef struct {
	bool		LOOP,
			PAUSE;
	PROCESSOR	P;
	struct IMCINFO	*M;
	XWINDOW		W;
	DESKTOP		D;
	LAYOUT		L;
} uARG;

typedef struct {
	char *argument;
	char *format;
	void *pointer;
	char *manual;
} OPTION;

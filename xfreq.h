/*
 * XFreq.c #0.08 by CyrIng
 *
 * Copyright (C) 2013 CYRIL INGENIERIE
 * Licenses: GPL2
 */

typedef struct
{
	struct
	{
		struct
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


#define	MSR_PLATFORM_INFO		0xce
#define	IA32_PERF_STATUS		0x198
#define IA32_THERM_STATUS		0x19c
#define MSR_TEMPERATURE_TARGET		0x1a2
#define	MSR_TURBO_RATIO_LIMIT		0x1ad

#define	SMBIOS_PROCINFO_STRUCTURE	4
#define	SMBIOS_PROCINFO_INSTANCE	0
#define	SMBIOS_PROCINFO_EXTCLK		0x12

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
		ReservedBits1	: 64-32;
} TURBO;

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
} TEMP;

typedef struct {
		FEATURES Features;
		PLATFORM Platform;
		TURBO	Turbo;
		struct	CORE {
			int	FD,
				Ratio,
				Freq,
				Temp;
		}	*Core;
		int	Top;
		int	ClockSpeed;
}	PROCESSOR;

typedef struct {
	Display		*display;
	Window		window;
	Screen		*screen;
	GC		gc;
	int		x,
			y;
	int		width,
			height;
	struct	{
		int	width,
			height;
	} margin;
	struct	{
		char	fname[256];
	    XFontStruct	*font;
	    XCharStruct	overall;
		int	dir,
			ascent,
			descent;
	} extents;
	unsigned long	background,
			foreground;
} XWINDOW;

#define	TITLE	"#%d@%dMHz"
#define	HDSIZE	".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"
#define	FREQ	" #%-2d%5d MHz "
#define	BCLOCK	"Clock[%3d MHz]"

typedef struct {
	char		string[sizeof(HDSIZE)];
	char		bclock[sizeof(BCLOCK)];
	char		ratios[2+2+2+1];
	XRectangle	*usage;
	XSegment	*axes;
} LAYOUT;

typedef enum {false=0, true=1} bool;

typedef struct {
	bool		LOOP;
	PROCESSOR	P;
	XWINDOW		W;
	LAYOUT		L;
} uARG;

typedef	struct {
	char *argument;
	char *format;
	void *pointer;
	char *manual;
} OPTION;

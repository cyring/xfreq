/*
 * XFreq.h #0.17 SR5 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

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
		struct {
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
		struct	{
			unsigned
			Threshld:  4-0,
			Unused1	: 32-4;
		} EBX;
		struct	{
			unsigned
			HCF_Cap	:  1-0,
			ACNT_Cap:  2-1,
			Unused1	:  3-2,
			PEB_Cap	:  4-3,
			Unused2	: 32-4;
		} ECX;
		struct	{
			unsigned
			Unused1	: 32-0;
		} EDX;
	} Thermal_Power_Leaf;
	struct
	{
		struct	{
			unsigned
			Version	:  8-0,
			MonCtrs	: 16-8,
			MonWidth: 24-16,
			VectorSz: 32-24;
		} EAX;
		struct	{
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
		struct	{
			unsigned
			Unused1	: 32-0;
		} ECX;
		struct	{
			unsigned
			FixCtrs	:  5-0,
			FixWidth: 13-5,
			Unused1	: 32-13;
		} EDX;
	} Perf_Monitoring_Leaf;
	struct
	{
		struct {
			unsigned
			MaxSubLeaf	: 32-0;
		} EAX;
		struct {
			unsigned
			FSGSBASE	:  1-0,
			Unused1		:  7-1,
			SMEP		:  8-7,
			Unused2		:  9-8,
			FastStrings	: 10-9,
			INVPCID		: 11-10,
			Unused3		: 32-11;
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
} FEATURES;

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

#define	IA32_TIME_STAMP_COUNTER		0x10
#define	IA32_PERF_STATUS		0x198
#define	IA32_CLOCK_MODULATION		0x19a
#define	IA32_THERM_INTERRUPT		0x19b
#define IA32_THERM_STATUS		0x19c
#define	IA32_MISC_ENABLE		0x1a0
#define	IA32_ENERGY_PERF_BIAS		0x1b0
#define	IA32_FIXED_CTR1			0x30a
#define	IA32_FIXED_CTR2			0x30b
#define	IA32_FIXED_CTR_CTRL		0x38d
#define	IA32_PERF_GLOBAL_CTRL		0x38f
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


//	Read, Write a Model Specific Register.
#define	Read_MSR(FD, offset, msr)  pread(FD, msr, sizeof(*msr), offset)
#define	Write_MSR(FD, offset, msr) pwrite(FD, msr, sizeof(*msr), offset)

typedef	struct {
	unsigned long long
		Ratio		: 16-0,
		ReservedBits	: 64-16;
} PERF_STATUS;

typedef struct {
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

typedef	struct {
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

#define	LOW_TEMP_VALUE		35
#define MED_TEMP_VALUE		45
#define	HIGH_TEMP_VALUE		65

typedef struct {
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
} THERM_STATUS;

typedef struct {
	unsigned long long
		ReservedBits1	: 16-0,
		Target		: 24-16,
		ReservedBits2	: 64-24;
} TJMAX;

#define	ARCHITECTURES 12
//	[Nehalem]
#define	Nehalem_Bloomfield		{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xA}
#define	Nehalem_Lynnfield		{ExtFamily:0x0, Family:0x6, ExtModel:0x1, Model:0xE}
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
//	[Haswell]
#define	Haswell_3C			{ExtFamily:0x0, Family:0x6, ExtModel:0x3, Model:0xC}
#define	Haswell_45			{ExtFamily:0x0, Family:0x6, ExtModel:0x4, Model:0x5}
#define	Haswell_46			{ExtFamily:0x0, Family:0x6, ExtModel:0x4, Model:0x6}

const struct {
	struct	SIGNATURE	Signature;
	const	unsigned int	MaxOfCores;
	const	double		ClockSpeed;
	const	char		*Architecture;
} ARCH[ARCHITECTURES]={
				{ Nehalem_Bloomfield,    4, 133.66, "Nehalem" },
				{ Nehalem_Lynnfield,     4, 133.66, "Nehalem" },
				{ Nehalem_NehalemEP,     4, 133.66, "Nehalem" },
				{ Westmere_Arrandale,    4, 133.66, "Westmere" },
				{ Westmere_Gulftown,     6, 133.66, "Westmere" },
				{ Westmere_WestmereEP,   6, 133.66, "Westmere" },
				{ SandyBridge_1G,        6, 100.00, "SandyBridge" },
				{ SandyBridge_Bromolow,  4, 100.00, "SandyBridge" },
				{ IvyBridge,             6 ,100.00, "IvyBridge" },
				{ Haswell_3C,            4, 100.00, "Haswell" },
				{ Haswell_45,            4, 100.00, "Haswell" },
				{ Haswell_46,            4, 100.00, "Haswell" },
			};

typedef struct {
		signed int			ArchID;
		FEATURES			Features;
		MISC_PROC_FEATURES		MiscFeatures;
		PLATFORM			Platform;
		TURBO				Turbo;
		double				ClockSpeed;
		unsigned int			CPU;
		char				Bump[2+2+2+1];
		struct THREADS {
			signed int		FD;
//			PERF_STATUS		Operating;
			GLOBAL_PERF_COUNTER	GlobalPerfCounter;
			FIXED_PERF_COUNTER	FixedPerfCounter;
			struct {
				struct {
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
				struct {
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
		} *Core;
		struct {
			double			Turbo,
						C0,
						C3,
						C6;
		} Avg;
		unsigned int			Top,
						Hot,
						PerCore;
		useconds_t			IdleTime;
} PROCESSOR;

#define	GLOBAL_BACKGROUND	0x333333
#define	GLOBAL_FOREGROUND	0x8fcefa
#define	MAIN_BACKGROUND		0x333333
#define	MAIN_FOREGROUND		0x8fcefa
#define	CORES_BACKGROUND	0x191970
#define	CORES_FOREGROUND	0x8fcefa
#define	CSTATES_BACKGROUND	0x191970
#define	CSTATES_FOREGROUND	0x8fcefa
#define	TEMPS_BACKGROUND	0x191970
#define	TEMPS_FOREGROUND	0x8fcefa
#define	SYSINFO_BACKGROUND	0x191970
#define	SYSINFO_FOREGROUND	0x8fcefa
#define	DUMP_BACKGROUND		0x191970
#define	DUMP_FOREGROUND		0x8fcefa

#define	AXES_COLOR		0x404040
#define	LABEL_COLOR		0xc0c0c0
#define	PRINT_COLOR		0xf0f0f0
#define	PROMPT_COLOR		0xffff00
#define	CURSOR_COLOR		0xfd0000
#define	DYNAMIC_COLOR		0xdddddd
#define	GRAPH1_COLOR		0x6666f0
#define	GRAPH2_COLOR		0x6666b0
#define	GRAPH3_COLOR		0x666690
#define	INIT_VALUE_COLOR	0x6666b0
#define	LOW_VALUE_COLOR		0x00aa66
#define	MED_VALUE_COLOR		0xe49400
#define	HIGH_VALUE_COLOR	0xfd0000
#define	PULSE_COLOR		0xf0f000
#define	FOCUS_COLOR		0xffffff
#define	MDI_SEP_COLOR		0x737373

#define	ID_NULL		'\0'
#define	ID_MIN		'm'
#define	ID_NORTH	'N'
#define	ID_SOUTH	'S'
#define	ID_EAST		'E'
#define	ID_WEST		'W'
#define	ID_PGUP		'U'
#define	ID_PGDW		'D'
#define	ID_PAUSE	'I'
#define	ID_FREQ		'H'
#define	ID_CYCLE	'Y'
#define	ID_RATIO	'R'
#define	ID_STATE	'P'

#define	RSC_PAUSE	"Pause"
#define	RSC_FREQ	"Freq."
#define	RSC_CYCLE	"Cycle"
#define	RSC_STATE	"State"
#define	RSC_RATIO	"Ratio"

typedef	enum	{DECORATION, SCROLLING, TEXT, ICON} WBTYPE;

typedef	union	{
	char		*Text;
	char		Label;
	Pixmap		*Bitmap;
} RESOURCE;

struct WButton {
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

typedef struct {
	Window		window;
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

//			L-CTRL		L-ALT		R-CTRL		L-WIN		R-ALTGR
#define	AllModMask	(ControlMask	| Mod1Mask	| Mod3Mask	| Mod4Mask	| Mod5Mask)
#define	BASE_EVENTS	KeyPressMask | ExposureMask | VisibilityChangeMask | StructureNotifyMask | FocusChangeMask
#define	MOVE_EVENTS	ButtonReleaseMask | Button3MotionMask
#define	CLICK_EVENTS	ButtonPressMask

typedef struct {
	int		S,
			dx,
			dy;
} XTARGET;

#define	HDSIZE		".1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0.1.2.3.4.5.6.7.8.9.0"

typedef enum {MAIN, CORES, CSTATES, TEMPS, SYSINFO, DUMP, WIDGETS} LAYOUTS;

#define	FIRST_WIDGET	(MAIN + 1)
#define	LAST_WIDGET	(WIDGETS - 1)

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

#define	MAIN_TEXT_WIDTH		48
#define	MAIN_TEXT_HEIGHT	14

#define	CORES_TEXT_WIDTH	(A->P.Turbo.MaxRatio_1C)
#define	CORES_TEXT_HEIGHT	(A->P.CPU)

#define	TEMPS_TEXT_HEIGHT	18
#define	TEMPS_TEXT_WIDTH	(A->P.Features.ThreadCount << 2)

#define	CSTATES_TEXT_SPACING	3
#define	CSTATES_TEXT_WIDTH	( MAX(A->P.CPU, 6) * CSTATES_TEXT_SPACING )
#define	CSTATES_TEXT_HEIGHT	10

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
#define	DUMP_TABLE_ROWS		11

#define	MENU_FORMAT	"[F1]     Help             [F2]     Core\n"               \
			"[F3]     C-States         [F4]     Temps \n"             \
			"[F5]     System Info      [F6]     Dump\n"               \
			"                                 [Up]\n"                 \
			"  Page Scrolling          [Left]      [Right]\n"         \
			"                                [Down]\n"                \
			"[Pause]  Suspend/Resume\n"                               \
			"[PgDw]   Page Down        [PgUp]   Page Up\n"            \
			"[Home]   Keep on top      [End]    Keep below\n"         \
			"KPad [+] Faster           KPad [-] Slower\n\n"           \
			"With the [Control] key, activate the followings :\n"     \
			"[L][l]   Refresh          [C][c]   Center page\n"        \
			"[A][a]   Activity pulse   [W][w]   Wallboard\n"          \
			"[H][h]   Frequency Hz     [P][p]   Counters %\n"         \
			"[R][r]   Ratio values     [Q][q]   Quit\n\n"             \
			"Commands :\n"                                            \
			"[Backspace] Remove the rightmost character\n"            \
			"[Erase] Suppress the full Command line\n"                \
			"[Enter] Submit the Command\n\n"                          \
			"Mouse buttons :\n"                                       \
			"[Left]   Click any Button [Right] Grab & Move Widget\n"  \
			"[WheelUp] Page Scroll Up  [WheelDw] Page Scroll Down\n"

#define	CORE_NUM	"#%-2d"
#define	CORE_FREQ	"%4.0fMHz"
#define	CORE_CYCLES	"%016llu:%016llu"
#define	CORE_DELTA	"%04llu:%04llu %04llu %04llu / %04llu"
#define	CORE_RATIO	"%-3.1f"
#define	CSTATES_PERCENT	"%6.2f%% %6.2f%% %6.2f%% %6.2f%%"
#define	OVERCLOCK	"%s [%4.0f MHz]"
#define	TEMPERATURE	"%3d"

#define	PROC_FORMAT	"Processor [%s]  Architecture [%s]\n"                                       \
			"Base Clock [%5.2f MHz]\n\n"                                                  \
			" Family               Model             Stepping             Max# of\n"    \
			"  Code                 No.                 ID                Threads\n"    \
			"[%6X]            [%6X]            [%6d]            [%6d]\n\n"              \
			"Virtual Mode Extension                                         VME [%c]\n" \
			"Debugging Extension                                             DE [%c]\n" \
			"Page Size Extension                                            PSE [%c]\n" \
			"Time Stamp Counter                                             TSC [%c]\n" \
			"Model Specific Registers                                       MSR [%c]\n" \
			"Physical Address Extension                                     PAE [%c]\n" \
			"Advanced Programmable Interrupt Controller                    APIC [%c]\n" \
			"Memory Type Range Registers                                   MTRR [%c]\n" \
			"Page Global Enable                                             PGE [%c]\n" \
			"Machine-Check Architecture                                     MCA [%c]\n" \
			"Page Attribute Table                                           PAT [%c]\n" \
			"36-bit Page Size Extension                                   PSE36 [%c]\n" \
			"Processor Serial Number                                        PSN [%c]\n" \
			"Debug Store & Precise Event Based Sampling                DS, PEBS [%c]   [%s]\n" \
			"Advanced Configuration & Power Interface                      ACPI [%c]\n" \
			"Self-Snoop                                                      SS [%c]\n" \
			"Hyper-Threading                                                HTT [%c]\n" \
			"Thermal Monitor                                 TM1 [%c]        TM2 [%c]\n" \
			"Pending Break Enable                                           PBE [%c]\n" \
			"64-Bit Debug Store                                          DTES64 [%c]\n" \
			"CPL Qualified Debug Store                                   DS-CPL [%c]\n" \
			"Virtual Machine Extensions                                     VMX [%c]\n" \
			"Safer Mode Extensions                                          SMX [%c]\n" \
			"SpeedStep                                                     EIST [%c]   [%s]\n" \
			"L1 Data Cache Context ID                                   CNXT-ID [%c]\n" \
			"Fused Multiply Add                                             FMA [%c]\n" \
			"xTPR Update Control                                           xTPR [%c]   [%s]\n" \
			"Perfmon and Debug Capability                                  PDCM [%c]\n" \
			"Process Context Identifiers                                   PCID [%c]\n" \
			"Direct Cache Access                                            DCA [%c]\n" \
			"Extended xAPIC Support                                      x2APIC [%c]\n" \
			"Time Stamp Counter Deadline                           TSC-DEADLINE [%c]\n" \
			"XSAVE/XSTOR States                                           XSAVE [%c]\n" \
			"OS-Enabled Ext. State Management                           OSXSAVE [%c]\n" \
			"Execution Disable Bit Support                               XD-Bit [%c]   [%s]\n" \
			"1 GB Pages Support                                       1GB-PAGES [%c]\n" \
			"Fast-Strings                                       REP MOVSB/STOSB [%c]   [%s]\n" \
			"Automatic Thermal Control Circuit Enable                       TCC       [%s]\n" \
			"Performance Monitoring Available                                PM       [%s]\n" \
			"Branch Trace Storage Unavailable                               BTS       [%s]\n" \
			"Limit CPUID Maxval                                     Limit-CPUID       [%s]\n" \
			"Turbo Mode                                                   TURBO [%c]   [%s]\n" \
			"\nInstruction set:\n"                              \
			"FPU[%c]           CX8[%c]          SEP[%c]          CMOV[%c]      CLFSH[%c]\n" \
			"MMX[%c]          FXSR[%c]          SSE[%c]          SSE2[%c]       SSE3[%c]\n" \
			"SSSE3[%c]           SSE4.1[%c]          SSE4.2[%c]            PCLMULDQ[%c]\n" \
			"MONITOR[%c] [%s]      CX16[%c]           MOVBE[%c]              POPCNT[%c]\n" \
			"AES[%c]                AVX[%c]            F16C[%c]              RDRAND[%c]\n" \
			"LAHF/SAHF[%c]      SYSCALL[%c]          RDTSCP[%c]                IA64[%c]\n"

#define	RAM_SECTION	"\nRAM\n"
#define	CHA_FORMAT	"Channel   tCL   tRCD  tRP   tRAS  tRRD  tRFC  tWR   tRTPr tWTPr tFAW  B2B\n"
#define	CAS_FORMAT	"   #%1i   |%4d%6d%6d%6d%6d%6d%6d%6d%6d%6d%6d\n"

#define	SYSINFO_SECTION	"System Information"
//                       ## 12345 123456789012345678901234[1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234 1234]
#define	DUMP_SECTION	"   Addr.     Register               60   56   52   48   44   40   36   32   28   24   20   16   12    8    4    0"
#define	REG_HEXVAL	"%016llX"
#define	REG_FORMAT	"%02d %05X %s%%%zdc["

#define	TITLE_MAIN_FMT		"X-Freq %s.%s-%s"
#define	TITLE_CORES_FMT		"Core#%d @ %4.0fMHz"
#define	TITLE_CSTATES_FMT	"C-States [%.2f%%] [%.2f%%]"
#define	TITLE_TEMPS_FMT		"Core#%d @ %dC"
#define	TITLE_SYSINFO_FMT	"Clock @ %5.2f MHz"

typedef struct {
	int	cols,
		rows;
} MaxText;

typedef struct {
	struct	{
		int		H,
				V;
	} Margin;
	struct	{
		bool		Pageable;
		char		*Title;
		MaxText 	Text;
		int		hScroll,
				vScroll;
	} Page[WIDGETS];
	struct {
		bool
				freqHertz,
				cycleValues,
				ratioValues,
				cStatePercent,
				alwaysOnTop,
				wallboard,
				flashCursor;
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
	} DumpTable[DUMP_TABLE_ROWS];
	struct {;
		int		N;
		XSegment	*Segment;
	} Axes[WIDGETS];
	XPoint			Cursor[3];
	struct {
		char		KeyBuffer[256];
		int		KeyLength;
	} Input;
	char			*Output;
	unsigned long		globalBackground,
				globalForeground;
} LAYOUT;

#define	_IS_MDI_	(A->MDI != false)

// Fast drawing macro.
#define	fDraw(N, DoCenter, DoBuild, DoDraw) {	\
	if(DoCenter) CenterLayout(A, N);	\
	if(DoBuild)  BuildLayout(A, N);	\
	MapLayout(A, N);			\
	if(DoDraw)   DrawLayout(A, N);		\
	FlushLayout(A, N);			\
}

typedef struct {
	PROCESSOR	P;
	struct IMCINFO	*M;
	Display		*display;
	Screen		*screen;
	char		*fontName;
	XFontStruct	*xfont;
	XWINDOW		W[WIDGETS];
	XTARGET		T;
	bool		MDI;
	LAYOUT		L;
	bool		LOOP,
			PAUSE[WIDGETS];
	pthread_t	TID_Draw;
} uARG;

typedef struct {
	char *argument;
	char *format;
	void *pointer;
	char *manual;
} OPTION;

typedef	struct {
	char	*Instruction;
	void	(*Procedure)();
} COMMAND;

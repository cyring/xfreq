/*
 * xfreq-intel.h by CyrIng
 *
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#define	LEVEL_INVALID	0
#define	LEVEL_THREAD	1
#define	LEVEL_CORE	2

typedef	struct {
		union {
			struct
			{
				unsigned int
				SHRbits	:  5-0,
				Unused1	: 32-5;
			};
			unsigned int Register;
		} AX;
		union {
			struct
			{
				unsigned int
				Threads	: 16-0,
				Unused1	: 32-16;
			};
			unsigned int Register;
		} BX;
		union {
			struct
			{
				unsigned int
				Level	:  8-0,
				Type	: 16-8,
				Unused1 : 32-16;
			};
			unsigned int Register;
		} CX;
		union {
			struct
			{
				unsigned int
				 x2APIC_ID: 32-0;
			};
			unsigned int Register;
		} DX;
} CPUID_TOPOLOGY;

// Memory address of the Base Clock in the ROM of the ASUS Rampage II GENE.
#define	BCLK_ROM_ADDR	0xf08d9 + 0x12

// Read data from the PCI bus.
#define PCI_CONFIG_ADDRESS(bus, dev, fn, reg) \
	(0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

//	Read, Write a Model Specific Register.

#if defined(Linux)
#define	PAGE_SIZE	(sysconf(_SC_PAGESIZE))
#define	CPU_BP		"/dev/cpu/0/msr"
#define	CPU_AP		"/dev/cpu/999/msr"
#define	CPU_DEV		"/dev/cpu/%d/msr"

#define	Read_MSR(FD, msr, data)  pread(FD, data, sizeof(*data), msr)
#define	Write_MSR(FD, msr, data) pwrite(FD, data, sizeof(*data), msr)
#else // (FreeBSD) & Posix fix.
#define	CPU_BP		"/dev/cpuctl0"
#define	CPU_AP		"/dev/cpuctl999"
#define	CPU_DEV		"/dev/cpuctl%d"

int	Read_MSR(int FD, int msr, unsigned long long int *data)	\
{							\
	cpuctl_msr_args_t args;				\
	args.msr=msr;					\
	int rc=ioctl(FD, CPUCTL_RDMSR, &args);		\
	*(data)=args.data;				\
	return(rc);					\
}
int	Write_MSR(int FD, int msr, unsigned long long int *data) \
{							\
	cpuctl_msr_args_t args;				\
	args.msr=msr;					\
	args.data=*(data);				\
	int rc=ioctl(FD, CPUCTL_WRMSR, &args);		\
	return(rc);					\
}

#define	cpu_set_t cpuset_t
#define	pthread_setname_np pthread_set_name_np
#endif

//	[GenuineIntel]
#define	_GenuineIntel			{.ExtFamily=0x0, .Family=0x0, .ExtModel=0x0, .Model=0x0}
//	[Core]		06_0EH (32 bits)
#define	_Core_Yonah			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x0, .Model=0xE}
//	[Core2]		06_0FH, 06_15H, 06_17H, 06_1D
#define	_Core_Conroe			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x0, .Model=0xF}
#define	_Core_Kentsfield		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0x5}
#define	_Core_Yorkfield			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0x7}
#define	_Core_Dunnington		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0xD}
//	[Atom]		06_1CH, 06_26H, 06_27H (32 bits), 06_35H (32 bits), 06_36H
#define	_Atom_Bonnell			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0xC}
#define	_Atom_Silvermont		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0x6}
#define	_Atom_Lincroft			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0x7}
#define	_Atom_Clovertrail		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0x5}
#define	_Atom_Saltwell			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0x6}
//	[Silvermont]	06_37H, 06_4DH
#define	_Silvermont_637			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0x7}
#define	_Silvermont_64D			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x4, .Model=0xD}
//	[Nehalem]	06_1AH, 06_1EH, 06_1FH, 06_2EH
#define	_Nehalem_Bloomfield		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0xA}
#define	_Nehalem_Lynnfield		{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0xE}
#define	_Nehalem_MB			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x1, .Model=0xF}
#define	_Nehalem_EX			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0xE}
//	[Westmere]	06_25H, 06_2CH, 06_2FH
#define	_Westmere			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0x5}
#define	_Westmere_EP			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0xC}
#define	_Westmere_EX			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0xF}
//	[Sandy Bridge]	06_2AH, 06_2DH
#define	_SandyBridge			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0xA}
#define	_SandyBridge_EP			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x2, .Model=0xD}
//	[Ivy Bridge]	06_3AH, 06_3EH
#define	_IvyBridge			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0xA}
#define	_IvyBridge_EP			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0xE}
//	[Haswell]	06_3CH, 06_3FH, 06_45H, 06_46H
#define	_Haswell_DT			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0xC}
#define	_Haswell_MB			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x3, .Model=0xF}
#define	_Haswell_ULT			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x4, .Model=0x5}
#define	_Haswell_ULX			{.ExtFamily=0x0, .Family=0x6, .ExtModel=0x4, .Model=0x6}

typedef	struct
{
	struct	SIGNATURE	Signature;
		unsigned int	MaxOfCores;
		double		(*ClockSpeed)();
		char		*Architecture;
		void		*(*uCycle)(void *uA, int cpu, int T);
		Bool32		(*Init_MSR)(void *);
		Bool32		(*Close_MSR)(void *);
} ARCH;


/*
Borrow from:

/include/linux/sched.h:
struct task_struct {
	int prio, static_prio, normal_prio;
	struct sched_entity se;
	struct task_group *sched_task_group;
	pid_t pid;
	unsigned long nvcsw, nivcsw;			// context switch counts
	char comm[TASK_COMM_LEN];			// executable name excluding path
};
static inline pid_t task_pid_nr(struct task_struct *tsk)
static inline int task_node(const struct task_struct *p)

/kernel/sched/sched.h:
static inline struct task_group *task_group(struct task_struct *p)
{
	return p->sched_task_group;
}

/kernel/sched/debug.c:
print_task(struct seq_file *m, struct rq *rq, struct task_struct *p)
{
	if (rq->curr == p)
		SEQ_printf(m, "R");
	else
		SEQ_printf(m, " ");

	SEQ_printf(m, "%15s %5d %9Ld.%06ld %9Ld %5d ",
		p->comm, task_pid_nr(p),
		SPLIT_NS(p->se.vruntime),
		(long long)(p->nvcsw + p->nivcsw),
		p->prio);
//...
	SEQ_printf(m, "%9Ld.%06ld %9Ld.%06ld %9Ld.%06ld",
		SPLIT_NS(p->se.vruntime),
		SPLIT_NS(p->se.sum_exec_runtime),
		SPLIT_NS(p->se.statistics.sum_sleep_runtime));
//...
	SEQ_printf(m, " %d", task_node(p));
//...
	SEQ_printf(m, " %s", task_group_path(task_group(p)));
//...
	SEQ_printf(m, "\n");
}

/include/linux/limits.h:

/include/linux/types.h:
typedef __kernel_pid_t pid_t;

/uapi/asm-generic/int-ll64.h:

/uapi/asm-generic/posix_types.h:
typedef int __kernel_pid_t;
*/

#define	SCHED_CPU_SECTION	"cpu#%d"
#define	SCHED_PID_FIELD		"curr->pid"
#define	SCHED_PID_FORMAT	"  .%-30s: %%ld\n"
#define	SCHED_TASK_SECTION	"runnable tasks:"
#define	TASK_SECTION		"Task Scheduling"
#define	TASK_STATE_FMT		"%1s"
#define	TASK_COMM_FMT		"%15s"
#define	TASK_TIME_FMT		"%9Ld.%06Ld"
#define	TASK_CTXSWITCH_FMT	"%9Ld"
#define	TASK_PRIORITY_FMT	"%5ld"
#define	TASK_NODE_FMT		"%ld"
#define	TASK_GROUP_FMT		"%s"

#define	TASK_STRUCT_FORMAT	TASK_STATE_FMT""TASK_COMM_FMT" "TASK_PID_FMT" "TASK_TIME_FMT" "TASK_CTXSWITCH_FMT" "TASK_PRIORITY_FMT" "TASK_TIME_FMT" "TASK_TIME_FMT" "TASK_TIME_FMT" "TASK_NODE_FMT" "TASK_GROUP_FMT

#define	OPTIONS_COUNT	7
typedef struct
{
	char		*argument;
	char		*format;
	void		*pointer;
	char		*manual;
} OPTIONS;

#define	OPTIONS_LIST												\
{														\
	{"-c", "%d",   NULL,	"Pick up an architecture # (Int)\n"						\
				"\t\t  refer to the '-A' option"                                       },	\
	{"-S", "%u",   NULL,	"Clock source (Int)\n"								\
				"\t\t  argument is one of the [0]TSC [1]BIOS [2]SPEC [3]ROM [4]USER"   },	\
	{"-M", "%x",   NULL, 	"ROM address of the Base Clock (Hex)\n"						\
				"\t\t  argument is the BCLK memory address to read from"               },	\
	{"-s", "%u",   NULL,	"Idle time multiplier (Int)\n"							\
				"\t\t  argument is a coefficient multiplied by 50000 usec"             },	\
	{"-d", "%hhu", NULL,	"Registers dump enablement (Bool)"			               },	\
	{"-t", "%x",   NULL,	"Task scheduling monitoring sorted by 0x{R}0{N} (Hex)\n"			\
				"\t\t  where {R} bit:8 is set to reverse sorting\n"				\
				"\t\t  and {N} is one '/proc/sched_debug' field# from [0x1] to [0xb]"  },	\
	{"-z", "%hhu", NULL,	"Reset the MSR counters (Bool)"			                       },	\
}

typedef struct
{
	struct
	{
		int	Shm,
			SmBIOS;
	} FD;

	SHM_STRUCT	*SHM;

	SMBIOS_TREE	*SmBIOS, SmbTmpStorage;

	struct
	{
		size_t	Shm,
			SmBIOS;
	} Size;

	FEATURES	Features;
	ARCH		Arch[ARCHITECTURES];
const	DUMP_STRUCT	Loader;

	Bool64		LOOP;
	OPTIONS		Options[OPTIONS_COUNT];

	sigset_t	Signal;
	pthread_t	TID_SigHandler,
			TID_Read,
			TID_Dump,
			TID_Schedule;
	struct SAVEAREA
	{
		GLOBAL_PERF_COUNTER	GlobalPerfCounter;
		FIXED_PERF_COUNTER	FixedPerfCounter;
	} *SaveArea;
} uARG;

typedef struct
{
	long int	cpu;
	pthread_t	TID;
	uARG		*A;
} uAPIC;

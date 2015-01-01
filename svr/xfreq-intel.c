/*
 * xfreq-intel.c by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#if defined(Linux)
#define _GNU_SOURCE
#include <sched.h>
#include <sys/io.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdatomic.h>

#if defined(FreeBSD)
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <dev/io/iodev.h>
#include <sys/cpuctl.h>
#include <sys/cpuset.h>
#include <pthread_np.h>
#endif

#define	_APPNAME "XFreq-Intel"
#include "xfreq-api.h"
#include "xfreq-intel.h"

static  char    Version[] = AutoDate;


//	Initialize MSR based on Architecture. Open one MSR handle per Core.
bool	Init_MSR_GenuineIntel(void *uArg)
{
	uARG *A=(uARG *) uArg;

	ssize_t	retval=0;
	int	tmpFD=open(CPU_BP, O_RDONLY);
	bool	rc=true;
	// Read the minimum, maximum & the turbo ratios from Core number 0
	if(tmpFD != -1)
	{
		rc=((retval=Read_MSR(tmpFD, IA32_MISC_ENABLE,  (MISC_PROC_FEATURES *) &A->SHM->P.MiscFeatures)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_MTRR_DEF_TYPE,(MTRR_DEF_TYPE *) &A->SHM->P.MTRRdefType)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_PLATFORM_ID,  (PLATFORM_ID *) &A->SHM->P.PlatformId)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_PERF_STATUS,  (PERF_STATUS *) &A->SHM->P.PerfStatus)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_EFER,         (EXT_FEATURE *) &A->SHM->P.ExtFeature)) != -1);
		// MSR_PLATFORM_INFO may be available in this Intel Architecture ?
		if((rc=((retval=Read_MSR(tmpFD, MSR_PLATFORM_INFO, (PLATFORM_INFO *) &A->SHM->P.PlatformInfo)) != -1)) == true)
		{	//  Then, we get the Min & Max non Turbo Ratios which might inverted.
			A->SHM->P.Boost[0]=MIN(A->SHM->P.PlatformInfo.MinimumRatio, A->SHM->P.PlatformInfo.MaxNonTurboRatio);
			A->SHM->P.Boost[1]=MAX(A->SHM->P.PlatformInfo.MinimumRatio, A->SHM->P.PlatformInfo.MaxNonTurboRatio);
		}
		else	// Otherwise, set the Min & Max Ratios to the first non zero value found in the following MSR order:
			//	IA32_PLATFORM_ID[MaxBusRatio] , IA32_PERF_STATUS[MaxBusRatio] , IA32_PERF_STATUS[CurrentRatio]
			A->SHM->P.Boost[0]=A->SHM->P.Boost[1]=(A->SHM->P.PlatformId.MaxBusRatio) ? A->SHM->P.PlatformId.MaxBusRatio:(A->SHM->P.PerfStatus.MaxBusRatio) ? A->SHM->P.PerfStatus.MaxBusRatio:A->SHM->P.PerfStatus.CurrentRatio;
		// MSR_TURBO_RATIO_LIMIT may also be available ?
		if((rc=((retval=Read_MSR(tmpFD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->SHM->P.Turbo)) != -1)) == true)
		{
			A->SHM->P.Boost[2]=A->SHM->P.Turbo.MaxRatio_8C;
			A->SHM->P.Boost[3]=A->SHM->P.Turbo.MaxRatio_7C;
			A->SHM->P.Boost[4]=A->SHM->P.Turbo.MaxRatio_6C;
			A->SHM->P.Boost[5]=A->SHM->P.Turbo.MaxRatio_5C;
			A->SHM->P.Boost[6]=A->SHM->P.Turbo.MaxRatio_4C;
			A->SHM->P.Boost[7]=A->SHM->P.Turbo.MaxRatio_3C;
			A->SHM->P.Boost[8]=A->SHM->P.Turbo.MaxRatio_2C;
			A->SHM->P.Boost[9]=A->SHM->P.Turbo.MaxRatio_1C;
		}
		else	// In any case, last Ratio is equal to the maximum non Turbo Ratio.
			A->SHM->P.Boost[9]=A->SHM->P.Boost[1];

		close(tmpFD);
	}
	else rc=false;

	char	pathname[]=CPU_AP;
	int	cpu=0;
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
		if(A->SHM->C[cpu].T.Offline != true)
		{
			sprintf(pathname, CPU_DEV, cpu);
			if( (rc=((A->SHM->C[cpu].FD=open(pathname, O_RDWR)) != -1)) )
			{
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_THERM_INTERRUPT, (THERM_INTERRUPT *) &A->SHM->C[cpu].ThermIntr)) != -1);
			}
			A->SHM->C[cpu].TjMax.Target=100;
		}
	return(rc);
}

bool	Init_MSR_Core(void *uArg)
{
	uARG *A=(uARG *) uArg;

	ssize_t	retval=0;
	int	tmpFD=open(CPU_BP, O_RDONLY);
	bool	rc=true;
	// Read the minimum, maximum & the turbo ratios from Core number 0
	if(tmpFD != -1)
	{
		rc=((retval=Read_MSR(tmpFD, IA32_MISC_ENABLE,  (MISC_PROC_FEATURES *) &A->SHM->P.MiscFeatures)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_MTRR_DEF_TYPE,(MTRR_DEF_TYPE *) &A->SHM->P.MTRRdefType)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_PLATFORM_ID,  (PLATFORM_ID *) &A->SHM->P.PlatformId)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_PERF_STATUS,  (PERF_STATUS *) &A->SHM->P.PerfStatus)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_EFER,         (EXT_FEATURE *) &A->SHM->P.ExtFeature)) != -1);
		// MSR_PLATFORM_INFO may be available with Core2 ?
		if((rc=((retval=Read_MSR(tmpFD, MSR_PLATFORM_INFO, (PLATFORM_INFO *) &A->SHM->P.PlatformInfo)) != -1)) == true)
		{
			A->SHM->P.Boost[0]=MIN(A->SHM->P.PlatformInfo.MinimumRatio, A->SHM->P.PlatformInfo.MaxNonTurboRatio);
			A->SHM->P.Boost[1]=MAX(A->SHM->P.PlatformInfo.MinimumRatio, A->SHM->P.PlatformInfo.MaxNonTurboRatio);
		}
		else
			A->SHM->P.Boost[0]=A->SHM->P.Boost[1]=(A->SHM->P.PlatformId.MaxBusRatio) ? A->SHM->P.PlatformId.MaxBusRatio:(A->SHM->P.PerfStatus.MaxBusRatio) ? A->SHM->P.PerfStatus.MaxBusRatio:A->SHM->P.PerfStatus.CurrentRatio;
		// MSR_TURBO_RATIO_LIMIT may also be available with Core2 ?
		if((rc=((retval=Read_MSR(tmpFD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->SHM->P.Turbo)) != -1)) == true)
		{
			A->SHM->P.Boost[2]=A->SHM->P.Turbo.MaxRatio_8C;
			A->SHM->P.Boost[3]=A->SHM->P.Turbo.MaxRatio_7C;
			A->SHM->P.Boost[4]=A->SHM->P.Turbo.MaxRatio_6C;
			A->SHM->P.Boost[5]=A->SHM->P.Turbo.MaxRatio_5C;
			A->SHM->P.Boost[6]=A->SHM->P.Turbo.MaxRatio_4C;
			A->SHM->P.Boost[7]=A->SHM->P.Turbo.MaxRatio_3C;
			A->SHM->P.Boost[8]=A->SHM->P.Turbo.MaxRatio_2C;
			A->SHM->P.Boost[9]=A->SHM->P.Turbo.MaxRatio_1C;
		}
		else
			A->SHM->P.Boost[9]=A->SHM->P.Boost[1];

		close(tmpFD);
	}
	else rc=false;

	char	pathname[]=CPU_AP, warning[64];
	int	cpu=0;
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
		if(A->SHM->C[cpu].T.Offline != true)
		{
			sprintf(pathname, CPU_DEV, cpu);
			if( (rc=((A->SHM->C[cpu].FD=open(pathname, O_RDWR)) != -1)) )
			{
				// Enable the Performance Counters 1 and 2 :
				// - Set the global counter bits
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->SHM->C[cpu].GlobalPerfCounter)) != -1);
				if(A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR1 != 0)
				{
					sprintf(warning, "Warning: CPU#%02d: Fixed Counter #1 is already activated", cpu);
					perror(warning);
				}
				if(A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR2 != 0)
				{
					sprintf(warning, "Warning: CPU#%02d: Fixed Counter #2 is already activated", cpu);
					perror(warning);
				}
				A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR1=1;
				A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR2=1;
				rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->SHM->C[cpu].GlobalPerfCounter)) != -1);

				// - Set the fixed counter bits
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->SHM->C[cpu].FixedPerfCounter)) != -1);
				A->SHM->C[cpu].FixedPerfCounter.EN1_OS=1;
				A->SHM->C[cpu].FixedPerfCounter.EN2_OS=1;
				A->SHM->C[cpu].FixedPerfCounter.EN1_Usr=1;
				A->SHM->C[cpu].FixedPerfCounter.EN2_Usr=1;
				if(A->SHM->P.PerCore)
				{
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1=1;
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2=1;
				}
				else {
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1=0;
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2=0;
				}
				rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->SHM->C[cpu].FixedPerfCounter)) != -1);

				// Check & fixe Counter Overflow.
				GLOBAL_PERF_STATUS Overflow={0};
				GLOBAL_PERF_OVF_CTRL OvfControl={0};
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_STATUS, (GLOBAL_PERF_STATUS *) &Overflow)) != -1);
				if(Overflow.Overflow_CTR1)
				{
					sprintf(warning, "Remark CPU#%02d: UCC Counter #1 is overflowed", cpu);
					perror(warning);
					OvfControl.Clear_Ovf_CTR1=1;
				}
				if(Overflow.Overflow_CTR2)
				{
					sprintf(warning, "Remark CPU#%02d: URC Counter #1 is overflowed", cpu);
					perror(warning);
					OvfControl.Clear_Ovf_CTR2=1;
				}
				if(Overflow.Overflow_CTR1|Overflow.Overflow_CTR2)
				{
					sprintf(warning, "Remark CPU#%02d: Resetting Counters", cpu);
					perror(warning);
					rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_OVF_CTRL, (GLOBAL_PERF_OVF_CTRL *) &OvfControl)) != -1);
				}

				// Retreive the Thermal Junction Max. Fallback to 100°C if not available.
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->SHM->C[cpu].TjMax)) != -1);
				if(A->SHM->C[cpu].TjMax.Target == 0)
				{
					perror("Warning: Thermal Junction Max unavailable");
					A->SHM->C[cpu].TjMax.Target=100;
				}
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_THERM_INTERRUPT, (THERM_INTERRUPT *) &A->SHM->C[cpu].ThermIntr)) != -1);
			}
			else	// Fallback to an arbitrary & commom value.
			{
				sprintf(warning, "Remark CPU#%02d: Thermal Junction Max defaults to 100C", cpu);
				perror(warning);
				A->SHM->C[cpu].TjMax.Target=100;
			}
		}
	return(rc);
}

bool	Init_MSR_Nehalem(void *uArg)
{
	uARG *A=(uARG *) uArg;

	ssize_t	retval=0;
	int	tmpFD=open(CPU_BP, O_RDONLY);
	bool	rc=true;
	// Read the minimum, maximum & the turbo ratios from Core number 0
	if(tmpFD != -1)
	{
		rc=((retval=Read_MSR(tmpFD, IA32_MISC_ENABLE, (MISC_PROC_FEATURES *) &A->SHM->P.MiscFeatures)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_MTRR_DEF_TYPE,(MTRR_DEF_TYPE *) &A->SHM->P.MTRRdefType)) != -1);
		rc=((retval=Read_MSR(tmpFD, MSR_PLATFORM_INFO, (PLATFORM_INFO *) &A->SHM->P.PlatformInfo)) != -1);
		rc=((retval=Read_MSR(tmpFD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->SHM->P.Turbo)) != -1);
		rc=((retval=Read_MSR(tmpFD, IA32_EFER,         (EXT_FEATURE *) &A->SHM->P.ExtFeature)) != -1);
		close(tmpFD);

		A->SHM->P.Boost[0]=A->SHM->P.PlatformInfo.MinimumRatio;
		A->SHM->P.Boost[1]=A->SHM->P.PlatformInfo.MaxNonTurboRatio;
		A->SHM->P.Boost[2]=A->SHM->P.Turbo.MaxRatio_8C;
		A->SHM->P.Boost[3]=A->SHM->P.Turbo.MaxRatio_7C;
		A->SHM->P.Boost[4]=A->SHM->P.Turbo.MaxRatio_6C;
		A->SHM->P.Boost[5]=A->SHM->P.Turbo.MaxRatio_5C;
		A->SHM->P.Boost[6]=A->SHM->P.Turbo.MaxRatio_4C;
		A->SHM->P.Boost[7]=A->SHM->P.Turbo.MaxRatio_3C;
		A->SHM->P.Boost[8]=A->SHM->P.Turbo.MaxRatio_2C;
		A->SHM->P.Boost[9]=A->SHM->P.Turbo.MaxRatio_1C;
	}
	else rc=false;

	char	pathname[]=CPU_AP, warning[64];
	int	cpu=0;
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
		if(A->SHM->C[cpu].T.Offline != true)
		{
			sprintf(pathname, CPU_DEV, cpu);
			if( (rc=((A->SHM->C[cpu].FD=open(pathname, O_RDWR)) != -1)) )
			{
				// Enable the Performance Counters 1 and 2 :
				// - Set the global counter bits
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->SHM->C[cpu].GlobalPerfCounter)) != -1);
				if(A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR1 != 0)
				{
					sprintf(warning, "Warning: CPU#%02d: Fixed Counter #1 is already activated", cpu);
					perror(warning);
				}
				if(A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR2 != 0)
				{
					sprintf(warning, "Warning: CPU#%02d: Fixed Counter #2 is already activated", cpu);
					perror(warning);
				}
				A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR1=1;
				A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR2=1;
				rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->SHM->C[cpu].GlobalPerfCounter)) != -1);

				// - Set the fixed counter bits
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->SHM->C[cpu].FixedPerfCounter)) != -1);
				A->SHM->C[cpu].FixedPerfCounter.EN1_OS=1;
				A->SHM->C[cpu].FixedPerfCounter.EN2_OS=1;
				A->SHM->C[cpu].FixedPerfCounter.EN1_Usr=1;
				A->SHM->C[cpu].FixedPerfCounter.EN2_Usr=1;
				if(A->SHM->P.PerCore)
				{
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1=1;
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2=1;
				}
				else {
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1=0;
					A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2=0;
				}
				rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->SHM->C[cpu].FixedPerfCounter)) != -1);

				// Check & fixe Counter Overflow.
				GLOBAL_PERF_STATUS Overflow={0};
				GLOBAL_PERF_OVF_CTRL OvfControl={0};
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_STATUS, (GLOBAL_PERF_STATUS *) &Overflow)) != -1);
				if(Overflow.Overflow_CTR1)
				{
					sprintf(warning, "Remark CPU#%02d: UCC Counter #1 is overflowed", cpu);
					perror(warning);
					OvfControl.Clear_Ovf_CTR1=1;
				}
				if(Overflow.Overflow_CTR2)
				{
					sprintf(warning, "Remark CPU#%02d: URC Counter #1 is overflowed", cpu);
					perror(warning);
					OvfControl.Clear_Ovf_CTR2=1;
				}
				if(Overflow.Overflow_CTR1|Overflow.Overflow_CTR2)
				{
					sprintf(warning, "Remark CPU#%02d: Resetting Counters", cpu);
					perror(warning);
					rc=((retval=Write_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_OVF_CTRL, (GLOBAL_PERF_OVF_CTRL *) &OvfControl)) != -1);
				}

				// Retreive the Thermal Junction Max. Fallback to 100°C if not available.
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->SHM->C[cpu].TjMax)) != -1);
				if(A->SHM->C[cpu].TjMax.Target == 0)
				{
					perror("Warning: Thermal Junction Max unavailable");
					A->SHM->C[cpu].TjMax.Target=100;
				}
				rc=((retval=Read_MSR(A->SHM->C[cpu].FD, IA32_THERM_INTERRUPT, (THERM_INTERRUPT *) &A->SHM->C[cpu].ThermIntr)) != -1);
			}
			else	// Fallback to an arbitrary & commom value.
			{
				sprintf(warning, "Remark CPU#%02d: Thermal Junction Max defaults to 100C", cpu);
				perror(warning);
				A->SHM->C[cpu].TjMax.Target=100;
			}
		}
	return(rc);
}

// Close all MSR handles.
void	Close_MSR(uARG *A)
{
	int	cpu=0;
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
		if(A->SHM->C[cpu].T.Offline != true)
		{
			// Reset the fixed counters.
			A->SHM->C[cpu].FixedPerfCounter.EN1_Usr=0;
			A->SHM->C[cpu].FixedPerfCounter.EN2_Usr=0;
			A->SHM->C[cpu].FixedPerfCounter.EN1_OS=0;
			A->SHM->C[cpu].FixedPerfCounter.EN2_OS=0;
			A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN1=0;
			A->SHM->C[cpu].FixedPerfCounter.AnyThread_EN2=0;
			Write_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR_CTRL, &A->SHM->C[cpu].FixedPerfCounter);
			// Reset the global counters.
			A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR1=0;
			A->SHM->C[cpu].GlobalPerfCounter.EN_FIXED_CTR2=0;
			Write_MSR(A->SHM->C[cpu].FD, IA32_PERF_GLOBAL_CTRL, &A->SHM->C[cpu].GlobalPerfCounter);
			// Release the MSR handle associated to the Core.
			if(A->SHM->C[cpu].FD != -1)
				close(A->SHM->C[cpu].FD);
		}
}

// Read the Time Stamp Counter.
static __inline__ unsigned long long int RDTSC(void)
{
	unsigned Hi, Lo;

	__asm__ volatile
	(
		"rdtsc;"
		:"=a" (Lo),
		 "=d" (Hi)
	);
	return ((unsigned long long int) Lo) | (((unsigned long long int) Hi) << 32);
}


// [Genuine Intel]
double	ClockSpeed_GenuineIntel()
{
	return(100.00f);
};

// [Core]
double	ClockSpeed_Core()
{
	int FD=0;
	if( (FD=open(CPU_BP, O_RDONLY)) != -1) {
		FSB_FREQ FSB={0};
		Read_MSR(FD, MSR_FSB_FREQ, (unsigned long long int *) &FSB);
		close(FD);

		switch(FSB.Bus_Speed)
		{
			case 0b101:
				return(100.00f);
				break;
			case 0b001:
				return(133.33f);
				break;
			case 0b011:
				return(166.67f);
				break;
			default:
				return(100.00f);
				break;
		}
	}
	else
		return(100.00f);
};

// [Core2]
double	ClockSpeed_Core2()
{
	int FD=0;
	if( (FD=open(CPU_BP, O_RDONLY)) != -1) {
		FSB_FREQ FSB={0};
		Read_MSR(FD, MSR_FSB_FREQ, (unsigned long long int *) &FSB);
		close(FD);

		switch(FSB.Bus_Speed)
		{
			case 0b101:
				return(100.00f);
				break;
			case 0b001:
				return(133.33f);
				break;
			case 0b011:
				return(166.67f);
				break;
			case 0b010:
				return(200.00f);
				break;
			case 0b000:
				return(266.67f);
				break;
			case 0b100:
				return(333.33f);
				break;
			case 0b110:
				return(400.00f);
				break;
			default:
				return(100.00f);
				break;
		}
	}
	else
		return(100.00f);
};

// [Atom]
double	ClockSpeed_Atom()
{
	int FD=0;
	if( (FD=open(CPU_BP, O_RDONLY)) != -1) {
		FSB_FREQ FSB={0};
		Read_MSR(FD, MSR_FSB_FREQ, (unsigned long long int *) &FSB);
		close(FD);

		switch(FSB.Bus_Speed)
		{
			case 0b111:
				return(83.00f);
				break;
			case 0b101:
				return(100.00f);
				break;
			case 0b001:
				return(133.33f);
				break;
			case 0b011:
				return(166.67f);
				break;
			default:
				return(83.00f);
				break;
		}
	}
	else
		return(83.00f);
};

// [Silvermont]
double	ClockSpeed_Silvermont()
{
	int FD=0;
	if( (FD=open(CPU_BP, O_RDONLY)) != -1) {
		FSB_FREQ FSB={0};
		Read_MSR(FD, MSR_FSB_FREQ, (unsigned long long int *) &FSB);
		close(FD);

		switch(FSB.Bus_Speed)
		{
			case 0b100:
				return(80.0f);
				break;
			case 0b000:
				return(83.3f);
				break;
			case 0b001:
				return(100.0f);
				break;
			case 0b010:
				return(133.33f);
				break;
			case 0b011:
				return(116.7f);
				break;
			default:
				return(83.3f);
				break;
		}
	}
	else
		return(83.3f);
};

// [GenuineIntel]
void	*uCycle_GenuineIntel(void *uA, int cpu, int T)
{
	uARG *A=(uARG *) uA;
	// Unhalted Core & the Reference Cycles.
	Read_MSR(A->SHM->C[cpu].FD, IA32_APERF, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].UCC);
	Read_MSR(A->SHM->C[cpu].FD, IA32_MPERF, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].URC);
	// TSC.
	Read_MSR(A->SHM->C[cpu].FD, IA32_TIME_STAMP_COUNTER, (unsigned long long int *) &A->SHM->C[cpu].Cycles.TSC[T]);
	// C-States.
	// ToDo

	return(NULL);
}

// [Core]
void	*uCycle_Core(void *uA, int cpu, int T)
{
	uARG *A=(uARG *) uA;
	// Unhalted Core & the Reference Cycles.
	Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR1, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].UCC);
	Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR2, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].URC);
	// TSC.
	Read_MSR(A->SHM->C[cpu].FD, IA32_TIME_STAMP_COUNTER, (unsigned long long int *) &A->SHM->C[cpu].Cycles.TSC[T]);
	// C-States.
	// ToDo

	return(NULL);
}

// [Nehalem]
double	ClockSpeed_Nehalem_Bloomfield()
{
	return(133.33f);
};
#define	ClockSpeed_Nehalem_Lynnfield	ClockSpeed_Nehalem_Bloomfield
#define	ClockSpeed_Nehalem_MB		ClockSpeed_Nehalem_Bloomfield
#define	ClockSpeed_Nehalem_EX		ClockSpeed_Nehalem_Bloomfield

void	*uCycle_Nehalem(void *uA, int cpu, int T)
{
	uARG *A=(uARG *) uA;
	// Unhalted Core & Reference Cycles.
	Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR1, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].UCC);
	Read_MSR(A->SHM->C[cpu].FD, IA32_FIXED_CTR2, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C0[T].URC);
	// TSC in relation to the Logical Core.
	Read_MSR(A->SHM->C[cpu].FD, IA32_TIME_STAMP_COUNTER, (unsigned long long int *) &A->SHM->C[cpu].Cycles.TSC[T]);
	// C-States.
	Read_MSR(A->SHM->C[cpu].FD, MSR_CORE_C3_RESIDENCY, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C3[T]);
	Read_MSR(A->SHM->C[cpu].FD, MSR_CORE_C6_RESIDENCY, (unsigned long long int *) &A->SHM->C[cpu].Cycles.C6[T]);

	return(NULL);
}

// [Westmere]
double	ClockSpeed_Westmere()
{
	return(133.33f);
};
#define	ClockSpeed_Westmere_EP	ClockSpeed_Westmere
#define	ClockSpeed_Westmere_EX	ClockSpeed_Westmere

// [SandyBridge]
double	ClockSpeed_SandyBridge_EP()
{
	return(100.00f);
};
#define	ClockSpeed_SandyBridge	ClockSpeed_SandyBridge_EP

// [IvyBridge]
double	ClockSpeed_IvyBridge()
{
	return(100.00f);
};
#define	ClockSpeed_IvyBridge_EP	ClockSpeed_IvyBridge

// [Haswell]
double	ClockSpeed_Haswell_DT()
{
	return(100.00f);
};
#define	ClockSpeed_Haswell_MB	ClockSpeed_Haswell_DT
#define	ClockSpeed_Haswell_ULT	ClockSpeed_Haswell_DT
#define	ClockSpeed_Haswell_ULX	ClockSpeed_Haswell_DT

// Function to read data from the SMBIOS.
int	Read_SMBIOS(int structure, int instance, off_t offset, void *buf, size_t nbyte)
{
	ssize_t	retval=0;
	char	pathname[]="/sys/firmware/dmi/entries/999-99/raw";
	int	tmpFD=0, rc=-1;

	sprintf(pathname, "/sys/firmware/dmi/entries/%d-%d/raw", structure, instance);
	if( (tmpFD=open(pathname, O_RDONLY)) != -1 )
	{
		retval=pread(tmpFD, buf, nbyte, offset);
		close(tmpFD);
		rc=(retval != nbyte) ? -1 : 0;
	}
	return(rc);
}

// Estimate the Bus Clock Frequency from the TSC.
double	Compute_ExtClock(int Coef)
{
	unsigned long long int TSC[2];
	TSC[0]=RDTSC();
	usleep(IDLE_BASE_USEC * Coef);
	TSC[1]=RDTSC();
	return((double) (TSC[1] - TSC[0]) / (IDLE_BASE_USEC * Coef));
}

// Read the Bus Clock Frequency from the BIOS.
int	Get_ExternalClock()
{
	int	BCLK=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_EXTCLK, &BCLK, 1) != -1)
		return(BCLK);
	else
		return(0);
}

// Read the Base Clock in ROM memory.
int	Read_ROM_BCLK(off_t addr)
{
	int	fd=-1;
	ssize_t	br=1;
	char	buf[2]={0};
	int	bclk=0;

	if( (fd=open("/dev/mem", O_RDONLY)) != -1 )
	{
		if( (lseek(fd, addr, SEEK_SET ) != -1) && ((br=read(fd, buf, 2)) != 1) )
			bclk=((unsigned char) (buf[0])) + ((unsigned char) (buf[1] << 8));
		close(fd);
	}
	return(bclk);
}

// Read the number of logical Cores activated in the BIOS.
int	Get_ThreadCount()
{
	short int ThreadCount=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_THREADS, &ThreadCount, 1) != -1)
		return(ThreadCount);
	else
		return(0);
}

// Read the number of physicial Cores activated in the BIOS.
int	Get_CoreCount()
{
	short int CoreCount=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_CORES, &CoreCount, 1) != -1)
		return(CoreCount);
	else
		return(0);
}

// Retreive the Processor features through a call to the CPUID instruction.
void	Read_Features(FEATURES *features)
{
	int BX=0, DX=0, CX=0;
	__asm__ volatile
	(
		";xorq	%%rax, %%rax    \n\t"
		"cpuid"
		: "=b"	(BX),
		  "=d"	(DX),
		  "=c"	(CX)
                : "a" (0x0)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	features->VendorID[0]=BX; features->VendorID[1]=(BX >> 8); features->VendorID[2]= (BX >> 16); features->VendorID[3]= (BX >> 24);
	features->VendorID[4]=DX; features->VendorID[5]=(DX >> 8); features->VendorID[6]= (DX >> 16); features->VendorID[7]= (DX >> 24);
	features->VendorID[8]=CX; features->VendorID[9]=(CX >> 8); features->VendorID[10]=(CX >> 16); features->VendorID[11]=(CX >> 24);

	__asm__ volatile
	(
		";movq	$0x1, %%rax     \n\t"
		"cpuid"
		: "=a"	(features->Std.AX),
		  "=b"	(features->Std.BX),
		  "=c"	(features->Std.CX),
		  "=d"	(features->Std.DX)
                : "a" (0x1)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	__asm__ volatile
	(
		";movq	$0x4, %%rax     \n\t"
		"xorq	%%rcx, %%rcx    \n\t"
		"cpuid                  \n\t"
		"shr	$26, %%rax      \n\t"
		"and	$0x3f, %%rax    \n\t"
		"add	$1, %%rax"
		: "=a"	(features->ThreadCount)
                : "a" (0x4)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	__asm__ volatile
	(
		";movq	$0x6, %%rax     \n\t"
		"cpuid;"
		: "=a"	(features->Thermal_Power_Leaf.AX),
		  "=b"	(features->Thermal_Power_Leaf.BX),
		  "=c"	(features->Thermal_Power_Leaf.CX),
		  "=d"	(features->Thermal_Power_Leaf.DX)
                : "a" (0x6)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	__asm__ volatile
	(
		";movq	$0x7, %%rax     \n\t"
		"xorq	%%rbx, %%rbx    \n\t"
		"xorq	%%rcx, %%rcx    \n\t"
		"xorq	%%rdx, %%rdx    \n\t"
		"cpuid"
		: "=a"	(features->ExtFeature.AX),
		  "=b"	(features->ExtFeature.BX),
		  "=c"	(features->ExtFeature.CX),
		  "=d"	(features->ExtFeature.DX)
                : "a" (0x7)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	__asm__ volatile
	(
		";movq	$0xa, %%rax     \n\t"
		"cpuid"
		: "=a"	(features->Perf_Monitoring_Leaf.AX),
		  "=b"	(features->Perf_Monitoring_Leaf.BX),
		  "=c"	(features->Perf_Monitoring_Leaf.CX),
		  "=d"	(features->Perf_Monitoring_Leaf.DX)
                : "a" (0xa)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	__asm__ volatile
	(
		";movq	$0x80000000, %%rax      \n\t"
		"cpuid"
		: "=a"	(features->LargestExtFunc)
                : "a" (0x80000000)
#if defined(FreeBSD)
		: "ecx", "ebx"
#endif
	);
	if(features->LargestExtFunc >= 0x80000004 && features->LargestExtFunc <= 0x80000008)
	{
		__asm__ volatile
		(
			";movq	$0x80000007, %%rax      \n\t"
			"cpuid                          \n\t"
			"and	$0x100, %%rdx           \n\t"
			"shr	$8, %%rdx"
			: "=d"	(features->InvariantTSC)
			: "a" (0x80000007)
#if defined(FreeBSD)
			: "ecx", "ebx"
#endif
		);
		__asm__ volatile
		(
			";movq	$0x80000001, %%rax      \n\t"
			"cpuid"
			: "=c"	(features->ExtFunc.CX),
			  "=d"	(features->ExtFunc.DX)
			: "a" (0x80000001)
#if defined(FreeBSD)
			: "ecx", "ebx"
#endif
		);
		struct
		{
			struct
			{
				unsigned char Chr[4];
			} AX, BX, CX, DX;
		} Brand;
		char tmpString[48+1]/*={0x20}*/;
		int ix=0, jx=0, px=0;
		for(ix=0; ix<3; ix++)
		{
			__asm__ volatile
			(
				"cpuid"
				: "=a"	(Brand.AX),
				  "=b"	(Brand.BX),
				  "=c"	(Brand.CX),
				  "=d"	(Brand.DX)
				: "a"	(0x80000002 + ix)
#if defined(FreeBSD)
				: "ecx", "ebx"
#endif
			);
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.AX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.BX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.CX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.DX.Chr[jx];
		}
		for(ix=jx=0; jx < px; jx++)
			if(!(tmpString[jx] == 0x20 && tmpString[jx+1] == 0x20))
				features->BrandString[ix++]=tmpString[jx];
	}
}

// Enumerate the Processor's Cores and Threads topology.
static void *uReadAPIC(void *uApic)
{
	uAPIC	*pApic=(uAPIC *) uApic;
	uARG	*A=(uARG *) pApic->A;
	unsigned int cpu=pApic->cpu;

	int	InputLevel=0, NoMoreLevels=0,
		SMT_Mask_Width=0, SMT_Select_Mask=0,
		CorePlus_Mask_Width=0, CoreOnly_Select_Mask=0;

	CPUID_TOPOLOGY ExtTopology;
	memset(&ExtTopology, 0, sizeof(CPUID_TOPOLOGY));

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	if(pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) == 0)
	{
		do
		{
			__asm__ volatile
			(
				";movq	$0xb, %%rax     \n\t"
				"cpuid"
				: "=a"	(ExtTopology.AX),
				  "=b"	(ExtTopology.BX),
				  "=c"	(ExtTopology.CX),
				  "=d"	(ExtTopology.DX)
				:  "a"	(0xb),
				   "c"	(InputLevel)
#if defined(FreeBSD)
				: "ecx", "ebx"
#endif
			);
			// Exit from the loop if the BX register equals 0; or if the requested level exceeds the level of a Core.
			if(!ExtTopology.BX.Register || (InputLevel > LEVEL_CORE))
				NoMoreLevels=1;
			else
			{
				switch(ExtTopology.CX.Type)
					{
					case LEVEL_THREAD:
						{
						SMT_Mask_Width = ExtTopology.AX.SHRbits;
						SMT_Select_Mask= ~((-1) << SMT_Mask_Width );
						A->SHM->C[cpu].T.Thread_ID=ExtTopology.DX.x2APIC_ID & SMT_Select_Mask;

						if((A->SHM->C[cpu].T.Thread_ID > 0) && (A->SHM->P.Features.HTT_enabled == false))
							A->SHM->P.Features.HTT_enabled=true;
						}
						break;
					case LEVEL_CORE:
						{
						CorePlus_Mask_Width = ExtTopology.AX.SHRbits;
						CoreOnly_Select_Mask = (~((-1) << CorePlus_Mask_Width ) ) ^ SMT_Select_Mask;
						A->SHM->C[cpu].T.Core_ID = (ExtTopology.DX.x2APIC_ID & CoreOnly_Select_Mask) >> SMT_Mask_Width;
						}
						break;
					}
			}
			InputLevel++;
		}
		while(!NoMoreLevels);

		A->SHM->C[cpu].T.APIC_ID=ExtTopology.DX.x2APIC_ID;
		A->SHM->C[cpu].T.Offline=false;
	}
	else
	{
		A->SHM->C[cpu].T.APIC_ID=-1;
		A->SHM->C[cpu].T.Offline=true;
	}
	return(NULL);
}

unsigned int Create_Topology(uARG *A)
{
	unsigned int cpu=0, CountEnabledCPU=0;
	// Allocate a temporary structure to keep a link with each CPU during the uApic() function thread.
	uAPIC *uApic=calloc(A->SHM->P.CPU, sizeof(uAPIC));

	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
	{
		uApic[cpu].cpu=cpu;
		uApic[cpu].A=A;
		pthread_create(&uApic[cpu].TID, NULL, uReadAPIC, &uApic[cpu]);
	}
	for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
	{
		pthread_join(uApic[cpu].TID, NULL);

		if(A->SHM->C[cpu].T.Offline != true)
			CountEnabledCPU++;
	}
	free(uApic);

	return(CountEnabledCPU);
}

// Retreive the Integrated Memory Controler settings: the number of channels & their associated RAM timings.
#if defined(FreeBSD)
unsigned int inw(int fd, unsigned int addr)
{
	struct	iodev_pio_req in={.access=IODEV_PIO_READ, .port=addr, .width=2, .val=0};
	int rc=ioctl(fd, IODEV_PIO, &in);
	return((rc != -1) ? in.val : 0);
}
unsigned int inl(int fd, unsigned int addr)
{
	struct	iodev_pio_req in={.access=IODEV_PIO_READ, .port=addr, .width=4, .val=0};
	int rc=ioctl(fd, IODEV_PIO, &in);
	return((rc != -1) ? in.val : 0);
}
void outl(int fd, unsigned int addr, unsigned int val)
{
	struct	iodev_pio_req out={.access=IODEV_PIO_READ, .port=addr, .width=4, .val=val};
	int rc=ioctl(fd, IODEV_PIO, &out);
}

bool	IMC_Read_Info(uARG *A)
{
	int fd=open("/dev/io", O_RDWR);
	if(fd != -1)
	{
		unsigned int bus=0xff, dev=0x4, func=0;
		outl(fd, PCI_CONFIG_ADDRESS(bus, 3, 0, 0x48), 0xcf8);
		int code=(inw(fd, 0xcfc + (0x48 & 2)) >> 8) & 0x7;
		A->SHM->M.ChannelCount=(code == 7 ? 3 : code == 4 ? 1 : code == 2 ? 1 : code == 1 ? 1 : 2);

		unsigned int cha=0;
		for(cha=0; cha < MIN(A->SHM->M.ChannelCount, IMC_MAX); cha++)
		{
			unsigned int MRs=0, RANK_TIMING_B=0, BANK_TIMING=0, REFRESH_TIMING=0;

			outl(fd, PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x70), 0xcf8);
			MRs=inl(fd, 0xcfc);
			outl(fd, PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x84), 0xcf8);
			RANK_TIMING_B=inl(fd, 0xcfc);
			outl(fd, PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x88), 0xcf8);
			BANK_TIMING=inl(fd, 0xcfc);
			outl(fd, PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x8c), 0xcf8);
			REFRESH_TIMING=inl(fd, 0xcfc);

			A->SHM->M.Channel[cha].Timing.tCL  =((MRs >> 4) & 0x7) != 0 ? ((MRs >> 4) & 0x7) + 4 : 0;
			A->SHM->M.Channel[cha].Timing.tRCD =(BANK_TIMING & 0x1E00) >> 9;
			A->SHM->M.Channel[cha].Timing.tRP  =(BANK_TIMING & 0xF);
			A->SHM->M.Channel[cha].Timing.tRAS =(BANK_TIMING & 0x1F0) >> 4;
			A->SHM->M.Channel[cha].Timing.tRRD =(RANK_TIMING_B & 0x1c0) >> 6;
			A->SHM->M.Channel[cha].Timing.tRFC =(REFRESH_TIMING & 0x1ff);
			A->SHM->M.Channel[cha].Timing.tWR  =((MRs >> 9) & 0x7) != 0 ? ((MRs >> 9) & 0x7) + 4 : 0;
			A->SHM->M.Channel[cha].Timing.tRTPr=(BANK_TIMING & 0x1E000) >> 13;
			A->SHM->M.Channel[cha].Timing.tWTPr=(BANK_TIMING & 0x3E0000) >> 17;
			A->SHM->M.Channel[cha].Timing.tFAW =(RANK_TIMING_B & 0x3f);
			A->SHM->M.Channel[cha].Timing.B2B  =(RANK_TIMING_B & 0x1f0000) >> 16;
		}
		close(fd);
		return(true);
	}
	else
		return(false);
}

#else	//Linux

bool	IMC_Read_Info(uARG *A)
{
	if(!iopl(3))
	{
		unsigned int bus=0xff, dev=0x4, func=0;
		outl(PCI_CONFIG_ADDRESS(bus, 3, 0, 0x48), 0xcf8);
		int code=(inw(0xcfc + (0x48 & 2)) >> 8) & 0x7;
		A->SHM->M.ChannelCount=(code == 7 ? 3 : code == 4 ? 1 : code == 2 ? 1 : code == 1 ? 1 : 2);

		unsigned int cha=0;
		for(cha=0; cha < MIN(A->SHM->M.ChannelCount, IMC_MAX); cha++)
		{
			unsigned int MRs=0, RANK_TIMING_B=0, BANK_TIMING=0, REFRESH_TIMING=0;

			outl(PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x70), 0xcf8);
			MRs=inl(0xcfc);
			outl(PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x84), 0xcf8);
			RANK_TIMING_B=inl(0xcfc);
			outl(PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x88), 0xcf8);
			BANK_TIMING=inl(0xcfc);
			outl(PCI_CONFIG_ADDRESS(0xff, (dev + cha), func, 0x8c), 0xcf8);
			REFRESH_TIMING=inl(0xcfc);

			A->SHM->M.Channel[cha].Timing.tCL  =((MRs >> 4) & 0x7) != 0 ? ((MRs >> 4) & 0x7) + 4 : 0;
			A->SHM->M.Channel[cha].Timing.tRCD =(BANK_TIMING & 0x1E00) >> 9;
			A->SHM->M.Channel[cha].Timing.tRP  =(BANK_TIMING & 0xF);
			A->SHM->M.Channel[cha].Timing.tRAS =(BANK_TIMING & 0x1F0) >> 4;
			A->SHM->M.Channel[cha].Timing.tRRD =(RANK_TIMING_B & 0x1c0) >> 6;
			A->SHM->M.Channel[cha].Timing.tRFC =(REFRESH_TIMING & 0x1ff);
			A->SHM->M.Channel[cha].Timing.tWR  =((MRs >> 9) & 0x7) != 0 ? ((MRs >> 9) & 0x7) + 4 : 0;
			A->SHM->M.Channel[cha].Timing.tRTPr=(BANK_TIMING & 0x1E000) >> 13;
			A->SHM->M.Channel[cha].Timing.tWTPr=(BANK_TIMING & 0x3E0000) >> 17;
			A->SHM->M.Channel[cha].Timing.tFAW =(RANK_TIMING_B & 0x3f);
			A->SHM->M.Channel[cha].Timing.B2B  =(RANK_TIMING_B & 0x1f0000) >> 16;
		}
		iopl(0);
		return(true);
	}
	else
		return(false);
}
#endif

// Switch between the available Base Clock(FSB) sources.
void	SelectBaseClock(uARG *A)
{
	switch(A->SHM->P.ClockSrc)
	{
		default:
		case SRC_TSC:	// Base Clock = TSC divided by the maximum ratio (without Turbo).
			if(A->SHM->CPL.MSR && A->SHM->P.Features.InvariantTSC) {
				A->SHM->P.ClockSpeed=Compute_ExtClock(IDLE_COEF_DEF) / A->SHM->P.Boost[1];
				A->SHM->P.ClockSrc=SRC_TSC;
				break;
			}
		case SRC_BIOS:	// Read the Bus Clock Frequency in the SMBIOS (DMI).
			if((A->SHM->CPL.BIOS=(A->SHM->P.ClockSpeed=Get_ExternalClock()) != 0)) {
				A->SHM->P.ClockSrc=SRC_BIOS;
				break;
			}
		case SRC_SPEC:	// Set the Base Clock from the architecture array if CPU ID was found.
			if(A->SHM->P.ArchID != -1) {
				A->SHM->P.ClockSpeed=A->Arch[A->SHM->P.ArchID].ClockSpeed();
				A->SHM->P.ClockSrc=SRC_SPEC;
				break;
			}
		case SRC_ROM:	// Read the Bus Clock Frequency at a specified memory address.
			if((A->SHM->P.ClockSpeed=Read_ROM_BCLK(A->SHM->P.BClockROMaddr)) !=0 ) {
				A->SHM->P.ClockSrc=SRC_ROM;
				break;
			}
		case SRC_USER: {	// Set the Base Clock from the first row in the architecture array.
			A->SHM->P.ClockSpeed=A->Arch[0].ClockSpeed();
			A->SHM->P.ClockSrc=SRC_USER;
		}
		break;
	}
}

// Monitor the kernel tasks scheduling.
bool IsGreaterRuntime(RUNTIME *rt1, RUNTIME *rt2)
{
	return(	( (rt1->nsec_high > rt2->nsec_high) || ((rt1->nsec_high == rt2->nsec_high) && ((rt1->nsec_low > rt2->nsec_low))) ) ? true : false);
}

static void *uSchedule(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Schedule, "xfreq-svr-task");

	FILE	*fSD=NULL;

	if((A->SHM->CPL.PROC=((fSD=fopen("/proc/sched_debug", "r")) != NULL)) == true)
	{
		bool Reverse=(A->SHM->S.Attributes & 0x100) >> 8;
		short int SortField=A->SHM->S.Attributes & 0xf;

		struct TASK_STRUCT oTask={0};
		const struct TASK_STRUCT rTask=
		{
			.state=0x7f,
			.comm={0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x0},
			.pid=(unsigned short) -1,
			.vruntime={(unsigned) -1, (unsigned) -1},
			.nvcsw=(unsigned) -1,
			.prio=(unsigned short) -1,
			.exec_vruntime={(unsigned) -1, (unsigned) -1},
			.sum_exec_runtime={(unsigned) -1, (unsigned) -1},
			.sum_sleep_runtime={(unsigned) -1, (unsigned) -1},
			.node=(unsigned short) -1,
			.group_path={0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x0}
		},
		*pTask=(Reverse) ? &rTask : &oTask;

		unsigned int cpu=0, depth=0;
		for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
			for(depth=0; depth < TASK_PIPE_DEPTH; depth++)
				memcpy(&A->SHM->C[cpu].Task[depth], pTask, sizeof(struct TASK_STRUCT));

		char buffer[1024], schedPidFmt[48];
		sprintf(schedPidFmt, SCHED_PID_FORMAT, SCHED_PID_FIELD);

		while(fgets(buffer, sizeof(buffer), fSD) != NULL)
		{
			if((sscanf(buffer, SCHED_CPU_SECTION, &cpu) > 0) && (cpu >= 0) && (cpu < A->SHM->P.CPU))
			{
				while(fgets(buffer, sizeof(buffer), fSD) != NULL)
				{
					if((buffer[0] == '\n') || (sscanf(buffer, schedPidFmt, &oTask.pid) > 0))
					{
						A->SHM->C[cpu].Task[0].pid=oTask.pid;

						while(fgets(buffer, sizeof(buffer), fSD) != NULL)
						{
							if(!strncmp(buffer, SCHED_TASK_SECTION, 15))
							{
								fgets(buffer, sizeof(buffer), fSD);	// skip until "\n"
								fgets(buffer, sizeof(buffer), fSD);	// skip until "\n"

								while(fgets(buffer, sizeof(buffer), fSD) != NULL)
								{
									if((buffer[0] != '\n') && !feof(fSD))
									{
										sscanf(	buffer, TASK_STRUCT_FORMAT,
											&oTask.state,
											oTask.comm,
											&oTask.pid,
											&oTask.vruntime.nsec_high,
											&oTask.vruntime.nsec_low,
											&oTask.nvcsw,
											&oTask.prio,
											&oTask.exec_vruntime.nsec_high,
											&oTask.exec_vruntime.nsec_low,
											&oTask.sum_exec_runtime.nsec_high,
											&oTask.sum_exec_runtime.nsec_low,
											&oTask.sum_sleep_runtime.nsec_high,
											&oTask.sum_sleep_runtime.nsec_low,
											&oTask.node,
											oTask.group_path);

										if(A->SHM->C[cpu].Task[0].pid == oTask.pid)
											memcpy(&A->SHM->C[cpu].Task[0], &oTask, sizeof(struct TASK_STRUCT));
										else	// Insertion Sort.
											for(depth=1; depth < TASK_PIPE_DEPTH; depth++)
											{
												bool isFlag=false;

												switch(SortField)
												{
												case SORT_FIELD_NONE:
													isFlag=true;
												break;
												case SORT_FIELD_STATE:
													isFlag=(oTask.state > A->SHM->C[cpu].Task[depth].state);
												break;
												case SORT_FIELD_COMM:
													isFlag=(strcasecmp(oTask.comm, A->SHM->C[cpu].Task[depth].comm) > 0);
												break;
												case SORT_FIELD_PID:
													isFlag=(oTask.pid > A->SHM->C[cpu].Task[depth].pid);
												break;
												case SORT_FIELD_RUNTIME:
													isFlag=IsGreaterRuntime(&oTask.vruntime, &A->SHM->C[cpu].Task[depth].vruntime);
												break;
												case SORT_FIELD_CTX_SWITCH:
													isFlag=(oTask.nvcsw > A->SHM->C[cpu].Task[depth].nvcsw);
												break;
												case SORT_FIELD_PRIORITY:
													isFlag=(oTask.prio > A->SHM->C[cpu].Task[depth].prio);
												break;
												case SORT_FIELD_EXEC:
													isFlag=IsGreaterRuntime(&oTask.exec_vruntime, &A->SHM->C[cpu].Task[depth].exec_vruntime);
												break;
												case SORT_FIELD_SUM_EXEC:
													isFlag=IsGreaterRuntime(&oTask.sum_exec_runtime, &A->SHM->C[cpu].Task[depth].sum_exec_runtime);
												break;
												case SORT_FIELD_SUM_SLEEP:
													isFlag=IsGreaterRuntime(&oTask.sum_sleep_runtime, &A->SHM->C[cpu].Task[depth].sum_sleep_runtime);
												break;
												case SORT_FIELD_NODE:
													isFlag=(oTask.node > A->SHM->C[cpu].Task[depth].node);
												break;
												case SORT_FIELD_GROUP:
													isFlag=(strcasecmp(oTask.group_path, A->SHM->C[cpu].Task[depth].group_path) > 0);
												break;
												}
												if(isFlag ^ Reverse)
												{
													size_t shift=(TASK_PIPE_DEPTH - depth - 1) * sizeof(struct TASK_STRUCT);
													if(shift != 0)
														memmove(&A->SHM->C[cpu].Task[depth + 1], &A->SHM->C[cpu].Task[depth], shift);

													memcpy(&A->SHM->C[cpu].Task[depth], &oTask, sizeof(struct TASK_STRUCT));

													break;
												}
											}
									}
									else
										break;
								}
								break;
							}
						}
						break;
					}
				}
			}
		}
		fclose(fSD);
	}
	else
		perror("Warning: cannot open file '/proc/sched_debug'");

	return(NULL);
}

static void *uDump(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Dump, "xfreq-svr-dump");

	int i=0;
	for(i=0; i < DUMP_ARRAY_DIMENSION; i++)
		if(A->SHM->D.Array[i].Addr)
			Read_MSR(A->SHM->C[0].FD, A->SHM->D.Array[i].Addr, (unsigned long long int *) &A->SHM->D.Array[i].Value);

	return(NULL);
}

static void *uCycle(void *uApic)
{
	uAPIC	*pApic=(uAPIC *) uApic;
	uARG	*A=(uARG *) pApic->A;
	unsigned int cpu=pApic->cpu;

	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	pthread_setaffinity_np(pApic->TID, sizeof(cpu_set_t), &cpuset);

	char comm[TASK_COMM_LEN];
	sprintf(comm, "xfreq-cycle-%03d", cpu);
	pthread_setname_np(pApic->TID, comm);

	A->Arch[A->SHM->P.ArchID].uCycle(A, cpu, 1);
	// Compute the absolute Delta of Unhalted (Core & Ref) C0 Cycles = Current[1] - Previous[0].
	A->SHM->C[cpu].Delta.C0.UCC=	(A->SHM->C[cpu].Cycles.C0[0].UCC > A->SHM->C[cpu].Cycles.C0[1].UCC) ?
					 A->SHM->C[cpu].Cycles.C0[0].UCC - A->SHM->C[cpu].Cycles.C0[1].UCC
					:A->SHM->C[cpu].Cycles.C0[1].UCC - A->SHM->C[cpu].Cycles.C0[0].UCC;

	A->SHM->C[cpu].Delta.C0.URC=	A->SHM->C[cpu].Cycles.C0[1].URC - A->SHM->C[cpu].Cycles.C0[0].URC;

	A->SHM->C[cpu].Delta.C3=	A->SHM->C[cpu].Cycles.C3[1] - A->SHM->C[cpu].Cycles.C3[0];

	A->SHM->C[cpu].Delta.C6=	A->SHM->C[cpu].Cycles.C6[1] - A->SHM->C[cpu].Cycles.C6[0];

	A->SHM->C[cpu].Delta.TSC=	A->SHM->C[cpu].Cycles.TSC[1] - A->SHM->C[cpu].Cycles.TSC[0];

	// Compute Turbo State per Cycles Delta. (Protect against a division by zero)
	A->SHM->C[cpu].State.Turbo=(double) (A->SHM->C[cpu].Delta.C0.URC != 0) ?
					(double) (A->SHM->C[cpu].Delta.C0.UCC) / (double) A->SHM->C[cpu].Delta.C0.URC
						: 0.0f;
	// Compute C-States.
	A->SHM->C[cpu].State.C0=(double) (A->SHM->C[cpu].Delta.C0.URC) / (double) (A->SHM->C[cpu].Delta.TSC);
	A->SHM->C[cpu].State.C3=(double) (A->SHM->C[cpu].Delta.C3)  / (double) (A->SHM->C[cpu].Delta.TSC);
	A->SHM->C[cpu].State.C6=(double) (A->SHM->C[cpu].Delta.C6)  / (double) (A->SHM->C[cpu].Delta.TSC);

	A->SHM->C[cpu].RelativeRatio=A->SHM->C[cpu].State.Turbo * A->SHM->C[cpu].State.C0 * (double) A->SHM->P.Boost[1];

	// Relative Frequency = Relative Ratio x Bus Clock Frequency
	A->SHM->C[cpu].RelativeFreq=A->SHM->C[cpu].RelativeRatio * A->SHM->P.ClockSpeed;

	// Save TSC.
	A->SHM->C[cpu].Cycles.TSC[0]=A->SHM->C[cpu].Cycles.TSC[1];
	// Save the Unhalted Core & Reference Cycles for next iteration.
	A->SHM->C[cpu].Cycles.C0[0].UCC=A->SHM->C[cpu].Cycles.C0[1].UCC;
	A->SHM->C[cpu].Cycles.C0[0].URC =A->SHM->C[cpu].Cycles.C0[1].URC;
	// Save also the C-State Reference Cycles.
	A->SHM->C[cpu].Cycles.C3[0]=A->SHM->C[cpu].Cycles.C3[1];
	A->SHM->C[cpu].Cycles.C6[0]=A->SHM->C[cpu].Cycles.C6[1];

	// Update the Digital Thermal Sensor.
	if( (Read_MSR(A->SHM->C[cpu].FD, IA32_THERM_STATUS, (THERM_STATUS *) &A->SHM->C[cpu].ThermStat)) == -1)
		A->SHM->C[cpu].ThermStat.DTS=0;

	return(NULL);
}

// Implementation of the callback functions.
void	Play(uARG *A, char ID)
{
	switch(ID)
	{
		case ID_DONE:
				A->SHM->PlayID=ID_NULL;
			break;
		case ID_QUIT:
				A->LOOP=false;
			break;
		case ID_INCLOOP:
			if(A->SHM->P.IdleTime < IDLE_COEF_MAX)
			{
				A->SHM->P.IdleTime++;
				SelectBaseClock(A);
			}
			break;
		case ID_DECLOOP:
			if(A->SHM->P.IdleTime > IDLE_COEF_MIN)
			{
				A->SHM->P.IdleTime--;
				SelectBaseClock(A);
			}
			break;
		case ID_SCHED:
			if(A->SHM->CPL.PROC)
			{
				if(A->SHM->S.Monitor == false)
				{
					if((A->SHM->S.Attributes & 0xf) == SORT_FIELD_NONE)
						A->SHM->S.Attributes=SORT_FIELD_RUNTIME;

					A->SHM->S.Monitor=true;
				}
				else
					A->SHM->S.Monitor=false;
			}
			break;
		case ID_RESET:
				A->SHM->P.Cold=0;
			break;
		case ID_TSC:
			{
				A->SHM->P.ClockSrc=SRC_TSC;
				SelectBaseClock(A);
			}
			break;
		case ID_BIOS:
			{
				A->SHM->P.ClockSrc=SRC_BIOS;
				SelectBaseClock(A);
			}
			break;
		case ID_SPEC:
			{
				A->SHM->P.ClockSrc=SRC_SPEC;
				SelectBaseClock(A);
			}
			break;
		case ID_ROM:
			{
				A->SHM->P.ClockSrc=SRC_ROM;
				SelectBaseClock(A);
			}
			break;
	}
}

char	*FQN_Settings(const char *fName)
{
	char *Home=getenv("HOME");
	if(Home != NULL)
	{
		char	*lHome=strdup(Home),
			*rHome=strdup(Home),
			*dName=dirname(lHome),
			*bName=basename(rHome),
			*FQN=malloc(4096);

		if(!strcmp(dName, "/"))
			sprintf(FQN, "%s%s/%s", dName, bName, fName);
		else
			sprintf(FQN, "%s/%s/%s", dName, bName, fName);

		free(lHome);
		free(rHome);

		return(FQN);
	}
	return(NULL);
}

// Parse the options and the arguments.
bool	ScanOptions(uARG *A, int argc, char *argv[])
{
	int i=0, j=0;
	bool noerr=true;

	//  Parse the command line options which may override settings.
	if( (argc - ((argc >> 1) << 1)) )
	{
		for(j=1; j < argc; j+=2)
		{
			for(i=0; i < OPTIONS_COUNT; i++)
				if(!strcmp(argv[j], A->Options[i].argument))
				{
					sscanf(argv[j+1], A->Options[i].format, A->Options[i].pointer);
					break;
				}
			if(i == OPTIONS_COUNT)
			{
				noerr=false;
				break;
			}
		}
	}
	else
		noerr=false;

	if(noerr == false)
	{
		char	*program=strdup(argv[0]),
			*progName=basename(program);

		if(!strcmp(argv[1], "-h"))
		{
			printf(	_APPNAME" ""usage:\n\t%s [-option argument] .. [-option argument]\n\nwhere options include:\n", progName);

			for(i=0; i < OPTIONS_COUNT; i++)
				printf("\t%s\t%s\n", A->Options[i].argument, A->Options[i].manual);

			printf(	"\t-A\tPrint out the built-in architectures\n" \
				"\t-v\tPrint version information\n" \
				"\t-h\tPrint out this message\n" \
				"\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n" \
				"\nReport bugs to xfreq[at]cyring.fr\n");
		}
		else if(!strcmp(argv[1], "-A"))
		{
			printf(	"\n" \
				"     Architecture              Family         CPU        FSB Clock\n" \
				"  #                            _Model        [max]        [ MHz ]\n");
			for(i=0; i < ARCHITECTURES; i++)
				printf(	"%3d  %-25s %1X%1X_%1X%1X          %3d         %6.2f\n",
					i, A->Arch[i].Architecture,
					A->Arch[i].Signature.ExtFamily, A->Arch[i].Signature.Family,
					A->Arch[i].Signature.ExtModel, A->Arch[i].Signature.Model,
					A->Arch[i].MaxOfCores, A->Arch[i].ClockSpeed());
		}
		else if(!strcmp(argv[1], "-v"))
			printf(Version);
		else
		{
			if(j > 0)
				printf("%s: unrecognized option '%s'\nTry '%s -h' for more information.\n", progName, argv[j], progName);
			else
				printf("%s: malformed options.\nTry '%s -h' for more information.\n", progName, progName);
		}
		free(program);
	}
	return(noerr);
}

static void *uEmergency(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_SigHandler, "xfreq-svr-kill");

	int caught=0;
	while(A->LOOP && !sigwait(&A->Signal, &caught))
		switch(caught)
		{
			case SIGINT:
			case SIGQUIT:
			case SIGUSR1:
			case SIGTERM:
			{
				char str[sizeof(SIG_EMERGENCY_FMT)];
				sprintf(str, SIG_EMERGENCY_FMT, caught);
				perror(str);
				Play(A, ID_QUIT);
			}
				break;
/*			case SIGCONT:
			{
			}
				break;
			case SIGUSR2:
//			case SIGTSTP:
			{
			}
				break;	*/
		}
	return(NULL);
}

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[])
{
	pthread_setname_np(pthread_self(), "xfreq-svr-main");

	uARG	A=
	{
		.FD=0, .SHM=NULL,

		.Arch=
		{
			{ _GenuineIntel,         2,  ClockSpeed_GenuineIntel,         calloc(12 + 1, 1),           uCycle_GenuineIntel, Init_MSR_GenuineIntel },
			{ _Core_Yonah,           2,  ClockSpeed_Core,                 "Core/Yonah",                uCycle_GenuineIntel, Init_MSR_GenuineIntel },
			{ _Core_Conroe,          2,  ClockSpeed_Core2,                "Core2/Conroe",              uCycle_Core,         Init_MSR_Core         },
			{ _Core_Kentsfield,      4,  ClockSpeed_Core2,                "Core2/Kentsfield",          uCycle_Core,         Init_MSR_Core         },
			{ _Core_Yorkfield,       4,  ClockSpeed_Core2,                "Core2/Yorkfield",           uCycle_Core,         Init_MSR_Core         },
			{ _Core_Dunnington,      6,  ClockSpeed_Core2,                "Xeon/Dunnington",           uCycle_Core,         Init_MSR_Core         },
			{ _Atom_Bonnell,         2,  ClockSpeed_Atom,                 "Atom/Bonnell",              uCycle_Core,         Init_MSR_Core         },
			{ _Atom_Silvermont,      8,  ClockSpeed_Atom,                 "Atom/Silvermont",           uCycle_Core,         Init_MSR_Core         },
			{ _Atom_Lincroft,        1,  ClockSpeed_Atom,                 "Atom/Lincroft",             uCycle_Core,         Init_MSR_Core         },
			{ _Atom_Clovertrail,     2,  ClockSpeed_Atom,                 "Atom/Clovertrail",          uCycle_Core,         Init_MSR_Core         },
			{ _Atom_Saltwell,        2,  ClockSpeed_Atom,                 "Atom/Saltwell",             uCycle_Core,         Init_MSR_Core         },
			{ _Silvermont_637,       4,  ClockSpeed_Silvermont,           "Silvermont",                uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Silvermont_64D,       4,  ClockSpeed_Silvermont,           "Silvermont",                uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Nehalem_Bloomfield,   4,  ClockSpeed_Nehalem_Bloomfield,   "Nehalem/Bloomfield",        uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Nehalem_Lynnfield,    4,  ClockSpeed_Nehalem_Lynnfield,    "Nehalem/Lynnfield",         uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Nehalem_MB,           2,  ClockSpeed_Nehalem_MB,           "Nehalem/Mobile",            uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Nehalem_EX,           8,  ClockSpeed_Nehalem_EX,           "Nehalem/eXtreme.EP",        uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Westmere,             2,  ClockSpeed_Westmere,             "Westmere",                  uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Westmere_EP,          6,  ClockSpeed_Westmere_EP,          "Westmere/EP",               uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Westmere_EX,         10,  ClockSpeed_Westmere_EX,          "Westmere/eXtreme",          uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _SandyBridge,          4,  ClockSpeed_SandyBridge,          "SandyBridge",               uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _SandyBridge_EP,       6,  ClockSpeed_SandyBridge_EP,       "SandyBridge/eXtreme.EP",    uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _IvyBridge,            4,  ClockSpeed_IvyBridge,            "IvyBridge",                 uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _IvyBridge_EP,         6,  ClockSpeed_IvyBridge_EP,         "IvyBridge/EP",              uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Haswell_DT,           4,  ClockSpeed_Haswell_DT,           "Haswell/Desktop",           uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Haswell_MB,           4,  ClockSpeed_Haswell_MB,           "Haswell/Mobile",            uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Haswell_ULT,          2,  ClockSpeed_Haswell_ULT,          "Haswell/Ultra Low TDP",     uCycle_Nehalem,      Init_MSR_Nehalem      },
			{ _Haswell_ULX,          2,  ClockSpeed_Haswell_ULX,          "Haswell/Ultra Low eXtreme", uCycle_Nehalem,      Init_MSR_Nehalem      },
		},
		.Loader={.Monitor=true, .Array=REGISTERS_LIST},
		.LOOP=true,
		.Options=OPTIONS_LIST,
		.TID_SigHandler=0,
		.TID_Read=0,
		.TID_Schedule=0,
	};
	FEATURES Features;
	size_t	ShmSize=0;
	uid_t	UID=geteuid();
	bool	ROOT=(UID == 0),	// Check root access.
		fEmergencyThread=false;
	int	rc=0;

	if(ROOT == true)
	{	// Read the CPU Features.
		strcpy(A.Arch[0].Architecture, "Genuine");
		memset(&Features, 0, sizeof(FEATURES));
		Read_Features(&Features);

		if(Features.Std.BX.MaxThread > 0)
		{
			SHM_STRUCT tSHM;

			__asm__ volatile
			(
				"push	%%rdx		;"
				"xorq	%%rdx, %%rdx	;"
				"mulq	%%rcx		;"
				"addq	%%rbx, %%rax	;"
				"pop	%%rcx		;"
				"divq	%%rcx		;"
				"cmpq	$0x0, %%rdx	;"
				"jz	AlignToPage	;"
				"inc	%%rax		;"
				"AlignToPage:		;"
				"mulq	%%rcx		;"
				: "=a"	(ShmSize)
				: "a"	(sizeof(tSHM.C[0])),
				  "b"	(sizeof(tSHM)),
				  "c"	(Features.Std.BX.MaxThread),
				  "d"	(PAGE_SIZE)
			);
		}
		umask(!S_IRUSR|!S_IWUSR|!S_IRGRP|!S_IWGRP|!S_IROTH|!S_IWOTH);
		if(((A.FD=shm_open(SHM_FILENAME, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) != -1)
		&& (ftruncate(A.FD, ShmSize) != -1)
		&& ((A.SHM=mmap(0, ShmSize, PROT_READ|PROT_WRITE, MAP_SHARED, A.FD, 0)) != MAP_FAILED))
		{
			// Store the Server application signature.
			strncpy(A.SHM->AppName, _APPNAME, TASK_COMM_LEN - 1);

			A.SHM->CPL.MSR=false;
			A.SHM->CPL.BIOS=false,
			A.SHM->CPL.IMC=false,
			A.SHM->CPL.PROC=true;

			// Fill the shared memory with default values.
			memset(&A.SHM->P, 0, sizeof(PROCESSOR));
			A.SHM->P.ArchID=ARCHITECTURES;
			A.SHM->P.PlatformInfo.MinimumRatio=7;
			A.SHM->P.PlatformInfo.MaxNonTurboRatio=30;
			A.SHM->P.BClockROMaddr=BCLK_ROM_ADDR;
			A.SHM->P.ClockSrc=SRC_TSC;
			A.SHM->P.IdleTime=IDLE_COEF_DEF;

			memset(&A.SHM->H, 0, sizeof(ARCHITECTURE));

			memcpy(&A.SHM->D, &A.Loader, sizeof(DUMP_STRUCT));

			memcpy(&A.SHM->P.Features, &Features, sizeof(FEATURES));
			strcpy(A.Arch[0].Architecture, A.SHM->P.Features.VendorID);

			memset(&A.SHM->S, 0, sizeof(SCHEDULE));
			A.SHM->S.Monitor=false;
			A.SHM->S.Attributes=SORT_FIELD_NONE;

			memset(&A.SHM->M, 0, sizeof(IMC_INFO));

			A.Options[0].pointer=&A.SHM->P.ArchID;
			A.Options[1].pointer=&A.SHM->P.ClockSrc;
			A.Options[2].pointer=&A.SHM->P.BClockROMaddr;
			A.Options[3].pointer=&A.SHM->P.IdleTime;
			A.Options[4].pointer=&A.SHM->D.Monitor;
			A.Options[5].pointer=&A.SHM->S.Attributes;

			if(ScanOptions(&A, argc, argv))
			{
				Sync_Init(&A.SHM->Sync);

				sigemptyset(&A.Signal);
				sigaddset(&A.Signal, SIGINT);	// [CTRL] + [C]
				sigaddset(&A.Signal, SIGQUIT);
				sigaddset(&A.Signal, SIGUSR1);
//				sigaddset(&A.Signal, SIGUSR2);
				sigaddset(&A.Signal, SIGTERM);
//				sigaddset(&A.Signal, SIGCONT);
//				sigaddset(&A.Signal, SIGTSTP);	// [CTRL] + [Z]

				if(!pthread_sigmask(SIG_BLOCK, &A.Signal, NULL)
				&& !pthread_create(&A.TID_SigHandler, NULL, uEmergency, &A))
					fEmergencyThread=true;
				else
					perror("Remark: cannot start the signal handler");

				// Find or force the Architecture specifications.
				if((A.SHM->P.ArchID < 0) || (A.SHM->P.ArchID >= ARCHITECTURES))
				{
					if(A.SHM->P.ArchID < 0) A.SHM->P.ArchID=ARCHITECTURES;
					do {
						A.SHM->P.ArchID--;
						if(!(A.Arch[A.SHM->P.ArchID].Signature.ExtFamily ^ A.SHM->P.Features.Std.AX.ExtFamily)
						&& !(A.Arch[A.SHM->P.ArchID].Signature.Family ^ A.SHM->P.Features.Std.AX.Family)
						&& !(A.Arch[A.SHM->P.ArchID].Signature.ExtModel ^ A.SHM->P.Features.Std.AX.ExtModel)
						&& !(A.Arch[A.SHM->P.ArchID].Signature.Model ^ A.SHM->P.Features.Std.AX.Model))
							break;
					} while(A.SHM->P.ArchID >=0);
				}

				if(!(A.SHM->P.CPU=A.SHM->P.Features.ThreadCount))
				{
					perror("Warning: cannot read the maximum number of Cores from CPUID");

					if(A.SHM->P.Features.Std.DX.HTT)
					{
						if(!(A.SHM->CPL.BIOS=(A.SHM->P.CPU=Get_ThreadCount()) != 0))
							perror("Warning: cannot read the maximum number of Threads from BIOS DMI\nCheck if 'dmi' kernel module is loaded");
					}
					else
					{
						if(!(A.SHM->CPL.BIOS=(A.SHM->P.CPU=Get_CoreCount()) != 0))
							perror("Warning: cannot read the maximum number of Cores BIOS DMI\nCheck if 'dmi' kernel module is loaded");
					}
				}
				if(!A.SHM->P.CPU)
				{		// Fallback to architecture specifications.
					char remark[128]={0};
					strcat(remark, "Remark: apply a maximum number of Cores from the ");

					if(A.SHM->P.ArchID != -1)
					{
						A.SHM->P.CPU=A.Arch[A.SHM->P.ArchID].MaxOfCores;
						strcat(remark, A.Arch[A.SHM->P.ArchID].Architecture);
					}
					else
					{
						A.SHM->P.CPU=A.Arch[0].MaxOfCores;
						strcat(remark, A.Arch[0].Architecture);
					}
					strcat(remark, "specifications");
					perror(remark);
				}

				A.SHM->P.OnLine=Create_Topology(&A);

				if(A.SHM->P.Features.HTT_enabled)
					A.SHM->P.PerCore=false;
				else
					A.SHM->P.PerCore=true;

				// Open once the MSR gate.
				if(!(A.SHM->CPL.MSR=A.Arch[A.SHM->P.ArchID].Init_MSR(&A)))
					perror("Warning: cannot read the MSR registers\nCheck if the 'msr' kernel module is loaded");

				// Read the Integrated Memory Controler information.
				if((A.SHM->CPL.IMC=IMC_Read_Info(&A)) == false)
					perror("Warning: cannot read the IMC controler");

				SelectBaseClock(&A);

				if(A.SHM->P.ArchID != -1)
				{
					unsigned int cpu=0;

					memcpy(&A.SHM->H.Signature, &A.Arch[A.SHM->P.ArchID].Signature, sizeof(struct SIGNATURE));
					A.SHM->H.MaxOfCores=A.Arch[A.SHM->P.ArchID].MaxOfCores;
					A.SHM->H.ClockSpeed=A.Arch[A.SHM->P.ArchID].ClockSpeed();
					strncpy(A.SHM->H.Architecture, A.Arch[A.SHM->P.ArchID].Architecture, 32);

					if((A.SHM->S.Attributes & 0xf) != SORT_FIELD_NONE)
						Play(&A, ID_SCHED);

					printf(	"Processor [%s]\t%d x CPU\n" \
						"Signature [%1X%1X_%1X%1X] Architecture [%s]\n" \
						"\n%s",
						A.SHM->P.Features.BrandString, A.SHM->P.CPU,
						A.SHM->H.Signature.ExtFamily,
						A.SHM->H.Signature.Family,
						A.SHM->H.Signature.ExtModel,
						A.SHM->H.Signature.Model,
						A.SHM->H.Architecture,
						Version);
					fflush(stdout);

					uAPIC *uApic=calloc(A.SHM->P.CPU, sizeof(uAPIC));

					// Initialize C-States.
					for(cpu=0; cpu < A.SHM->P.CPU; cpu++)
						if(A.SHM->C[cpu].T.Offline != true)
							A.Arch[A.SHM->P.ArchID].uCycle(&A, cpu, 0);

					while(A.LOOP)
					{
						bool fJoinSchedThread=false;
						if(A.SHM->S.Monitor && A.SHM->CPL.PROC)
							fJoinSchedThread=A.SHM->S.Monitor=(pthread_create(&A.TID_Schedule, NULL, uSchedule, &A) == 0);
						bool fJoinDumpThread=false;
						if(A.SHM->D.Monitor)
							fJoinDumpThread=(pthread_create(&A.TID_Dump, NULL, uDump, &A) == 0);

						// Reset C-States average and max Temperature.
						A.SHM->P.Avg.Turbo=A.SHM->P.Avg.C0=A.SHM->P.Avg.C3=A.SHM->P.Avg.C6=0;
						unsigned int maxFreq=0, maxTemp=A.SHM->C[0].TjMax.Target;

						for(cpu=0; cpu < A.SHM->P.CPU; cpu++)
							if(A.SHM->C[cpu].T.Offline != true)
							{
								uApic[cpu].cpu=cpu;
								uApic[cpu].A=&A;
								pthread_create(&uApic[cpu].TID, NULL, uCycle, &uApic[cpu]);
							}
						for(cpu=0; cpu < A.SHM->P.CPU; cpu++)
							if(A.SHM->C[cpu].T.Offline != true)
							{
								pthread_join(uApic[cpu].TID, NULL);

								// Sum the C-States before the average.
								A.SHM->P.Avg.Turbo+=A.SHM->C[cpu].State.Turbo;
								A.SHM->P.Avg.C0+=A.SHM->C[cpu].State.C0;
								A.SHM->P.Avg.C3+=A.SHM->C[cpu].State.C3;
								A.SHM->P.Avg.C6+=A.SHM->C[cpu].State.C6;

								// Index the Top CPU speed.
								if(maxFreq < A.SHM->C[cpu].RelativeFreq)
								{
									maxFreq=A.SHM->C[cpu].RelativeFreq;
									A.SHM->P.Top=cpu;
								}

								// Index which Core is hot.
								if(A.SHM->C[cpu].ThermStat.DTS < maxTemp)
								{
									maxTemp=A.SHM->C[cpu].ThermStat.DTS;
									A.SHM->P.Hot=cpu;
								}

								// Store the coldest temperature.
								if(A.SHM->C[cpu].ThermStat.DTS > A.SHM->P.Cold)
									A.SHM->P.Cold=A.SHM->C[cpu].ThermStat.DTS;
							}
						// Average the C-States.
						A.SHM->P.Avg.Turbo/=A.SHM->P.CPU;
						A.SHM->P.Avg.C0/=A.SHM->P.CPU;
						A.SHM->P.Avg.C3/=A.SHM->P.CPU;
						A.SHM->P.Avg.C6/=A.SHM->P.CPU;

						if(fJoinDumpThread == true)
							A.SHM->D.Monitor=(pthread_join(A.TID_Dump, NULL) == 0);
						if(fJoinSchedThread == true)
							A.SHM->S.Monitor=(pthread_join(A.TID_Schedule, NULL) == 0);

						Sync_Signal(1, &A.SHM->Sync);

						// Settle down N x 50000 microseconds as specified by the command argument.
						long int idleRemaining;
						if((idleRemaining=Sync_Wait(0, &A.SHM->Sync, A.SHM->P.IdleTime)))
						{
							if(A.SHM->PlayID != ID_NULL)
							{
								Play(&A, A.SHM->PlayID);

								A.SHM->PlayID=ID_DONE;

								if(A.SHM->P.IdleTime < IDLE_COEF_MIN)	A.SHM->P.IdleTime=IDLE_COEF_MIN;
								if(A.SHM->P.IdleTime > IDLE_COEF_MAX)	A.SHM->P.IdleTime=IDLE_COEF_MAX;
							}
							usleep(IDLE_BASE_USEC*idleRemaining);
						}
					}
					// Shutting down.
					free(uApic);
				}
				else
				{
					perror(NULL);
					rc=2;
				}
				// Release the ressources.
				Close_MSR(&A);

				if(fEmergencyThread == true)
				{
					pthread_kill(A.TID_SigHandler, SIGUSR1);
					pthread_join(A.TID_SigHandler, NULL);
				}
				Sync_Destroy(&A.SHM->Sync);
			}
			else
				rc=1;

			if(munmap(A.SHM, ShmSize) == -1)
				perror("Error: unmapping the shared memory");
			if(shm_unlink(SHM_FILENAME) == -1)
				perror("Error: unlinking the shared memory");
		}
		else
		{
			perror("Error: creating the shared memory");
			rc=2;
		}
	}
	else
	{
		perror("Error: missing root privileges");
		rc=2;
	}

	free(A.Arch[0].Architecture);

	return(rc);
}

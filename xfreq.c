/*
 * XFreq.c #0.22 SR0 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define _GNU_SOURCE
#include <sched.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "xfreq.h"

static  char    Version[] = AutoDate;

// The drawing thread.
static pthread_mutex_t	uDraw_mutex;
static void *uDraw(void *uArg);

unsigned long long
	DumpRegister(signed int FD, unsigned long long msr, char *pHexStr, char *pBinStr)
{
	unsigned long long value=0;
	Read_MSR(FD, msr, &value);

	char hexStr[16+1]={0}, binStr[64+1]={0};
	// Convert value as an ASCII UPPERCASE Hexadecimal.
	sprintf(hexStr, REG_HEXVAL, value);
	// Convert value as an ASCII Binary.
	const char *BIN[0x10]=	{
			"0000",
			"0001",
			"0010",
			"0011",

			"0100",
			"0101",
			"0110",
			"0111",

			"1000",
			"1001",
			"1010",
			"1011",

			"1100",
			"1101",
			"1110",
			"1111",
			};
	unsigned int H=0, nibble=0;
	for(H=0; H < strlen(hexStr); H++) {
		nibble=(int) hexStr[H];
		nibble=(nibble > (int) '9') ? 10 + nibble - (int) 'A' : nibble - (int) '0';
		binStr[(H << 2)+0]=BIN[nibble][0];
		binStr[(H << 2)+1]=BIN[nibble][1];
		binStr[(H << 2)+2]=BIN[nibble][2];
		binStr[(H << 2)+3]=BIN[nibble][3];
	}

	// Terminate string and copy back to caller.
	binStr[(H << 2)]='\0';
	if(pHexStr != NULL)
		strcpy(pHexStr, hexStr);
	if(pBinStr != NULL)
		strcpy(pBinStr, binStr);
	return(value);
}

void	ClearMsg(uARG *A) {
	if(A->L.Output != NULL) {
		free(A->L.Output);
		A->L.Output=NULL;

		SetHListing(MESSAGE, 0);
		SetVListing(MESSAGE, 0);
		SetHScrolling(MESSAGE, 0);
		SetVScrolling(MESSAGE, 0);

		XSetForeground(A->display, A->W[MESSAGE].gc, A->W[MESSAGE].background);
		XFillRectangle(A->display, A->L.Page[MESSAGE].Pixmap, A->W[MESSAGE].gc,
				0,
				0,
				GetHFrame(MESSAGE) * One_Char_Width(MESSAGE),
				GetVFrame(MESSAGE) * One_Char_Height(MESSAGE));
	}
}

void	Output(uARG *A, const char *message)
{
	if(A->L.Output != NULL && GetVListing(MESSAGE) > GetVFrame(MESSAGE))
		ClearMsg(A);

	const size_t requestBlock=strlen(message) + 1;
	if(A->L.Output == NULL)
		A->L.Output=calloc(1, requestBlock);
	else {
		const size_t allocBlock=strlen(A->L.Output) + 1;
		A->L.Output=realloc(A->L.Output, allocBlock + requestBlock);
	}
	strcat(A->L.Output, message);
}

//	Open one MSR handle per Processor Core.
int	Open_MSR(uARG *A)
{
	ssize_t	retval=0;
	int	tmpFD=open("/dev/cpu/0/msr", O_RDONLY);
	int	rc=0;
	// Read the minimum, maximum & the turbo ratios from Core number 0
	if(tmpFD != -1) {
		rc=((retval=Read_MSR(tmpFD, IA32_MISC_ENABLE, (MISC_PROC_FEATURES *) &A->P.MiscFeatures)) != -1);
		rc=((retval=Read_MSR(tmpFD, MSR_PLATFORM_INFO, (PLATFORM *) &A->P.Platform)) != -1);
		rc=((retval=Read_MSR(tmpFD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->P.Turbo)) != -1);
		close(tmpFD);
	}
	else	// Fallback to arbitrary values for the sake the axes drawing.
	{
		A->P.Platform.MinimumRatio=10;
		A->P.Platform.MaxNonTurboRatio=20;
		A->P.Turbo.MaxRatio_1C=60;
		A->P.Turbo.MaxRatio_2C=55;
		A->P.Turbo.MaxRatio_3C=50;
		A->P.Turbo.MaxRatio_4C=45;
		A->P.Turbo.MaxRatio_5C=40;
		A->P.Turbo.MaxRatio_6C=35;
		A->P.Turbo.MaxRatio_7C=30;
		A->P.Turbo.MaxRatio_8C=25;
	}
	char	pathname[]="/dev/cpu/999/msr", warning[64];
	int	cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++)
	  if(A->P.Topology[cpu].CPU != NULL) {
		sprintf(pathname, "/dev/cpu/%d/msr", cpu);
		if( (rc=((A->P.Topology[cpu].CPU->FD=open(pathname, O_RDWR)) != -1)) ) {
			// Enable the Performance Counters 1 and 2 :
			// - Set the global counter bits
			rc=((retval=Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->P.Topology[cpu].CPU->GlobalPerfCounter)) != -1);
			if(A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR1 != 0) {
				sprintf(warning, "Warning: CPU#%02d: Fixed Counter #1 is already activated.\n", cpu);
				Output(A, warning);
			}
			if(A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR2 != 0) {
				sprintf(warning, "Warning: CPU#%02d: Fixed Counter #2 is already activated.\n", cpu);
				Output(A, warning);
			}
			A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR1=1;
			A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR2=1;
			rc=((retval=Write_MSR(A->P.Topology[cpu].CPU->FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->P.Topology[cpu].CPU->GlobalPerfCounter)) != -1);

			// - Set the fixed counter bits
			rc=((retval=Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->P.Topology[cpu].CPU->FixedPerfCounter)) != -1);
			A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_OS=1;
			A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_OS=1;
			A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_Usr=1;
			A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_Usr=1;
			if(A->P.PerCore) {
				A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN1=1;
				A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN2=1;
			}
			else {
				A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN1=0;
				A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN2=0;
			}
			rc=((retval=Write_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->P.Topology[cpu].CPU->FixedPerfCounter)) != -1);

			// Check & fixe Counter Overflow.
			GLOBAL_PERF_STATUS Overflow={0};
			GLOBAL_PERF_OVF_CTRL OvfControl={0};
			rc=((retval=Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_PERF_GLOBAL_STATUS, (GLOBAL_PERF_STATUS *) &Overflow)) != -1);
			if(Overflow.Overflow_CTR1) {
				sprintf(warning, "Remark CPU#%02d: UCC Counter #1 is overflowed.\n", cpu);
				Output(A, warning);
				OvfControl.Clear_Ovf_CTR1=1;
			}
			if(Overflow.Overflow_CTR2) {
				sprintf(warning, "Remark CPU#%02d: URC Counter #1 is overflowed.\n", cpu);
				Output(A, warning);
				OvfControl.Clear_Ovf_CTR2=1;
			}
			if(Overflow.Overflow_CTR1|Overflow.Overflow_CTR2) {
				sprintf(warning, "Remark CPU#%02d: Resetting Counters.\n", cpu);
				Output(A, warning);
				rc=((retval=Write_MSR(A->P.Topology[cpu].CPU->FD, IA32_PERF_GLOBAL_OVF_CTRL, (GLOBAL_PERF_OVF_CTRL *) &OvfControl)) != -1);
			}

			// Retreive the Thermal Junction Max. Fallback to 100°C if not available.
			rc=((retval=Read_MSR(A->P.Topology[cpu].CPU->FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->P.Topology[cpu].CPU->TjMax)) != -1);
			if(A->P.Topology[cpu].CPU->TjMax.Target == 0) {
				Output(A, "Warning: Thermal Junction Max unavailable.\n");
				A->P.Topology[cpu].CPU->TjMax.Target=100;
			}
			rc=((retval=Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_THERM_INTERRUPT, (THERM_INTERRUPT *) &A->P.Topology[cpu].CPU->ThermIntr)) != -1);
		}
		else	// Fallback to an arbitrary & commom value.
		{
			sprintf(warning, "Remark CPU#%02d: Thermal Junction Max defaults to 100C.\n", cpu);
			Output(A, warning);
			A->P.Topology[cpu].CPU->TjMax.Target=100;
		}
	}
	return(rc);
}

// Close all MSR handles.
void	Close_MSR(uARG *A)
{
	int	cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++)
	  if(A->P.Topology[cpu].CPU != NULL) {
		// Reset the fixed counters.
		A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_Usr=0;
		A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_Usr=0;
		A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_OS=0;
		A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_OS=0;
		A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN1=0;
		A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN2=0;
		Write_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR_CTRL, &A->P.Topology[cpu].CPU->FixedPerfCounter);
		// Reset the global counters.
		A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR1=0;
		A->P.Topology[cpu].CPU->GlobalPerfCounter.EN_FIXED_CTR2=0;
		Write_MSR(A->P.Topology[cpu].CPU->FD, IA32_PERF_GLOBAL_CTRL, &A->P.Topology[cpu].CPU->GlobalPerfCounter);
		// Release the MSR handle associated to the Core.
		if(A->P.Topology[cpu].CPU->FD != -1)
			close(A->P.Topology[cpu].CPU->FD);
	}
}

// Read the Time Stamp Counter.
static __inline__ unsigned long long RDTSC(void)
{
	unsigned Hi, Lo;

	__asm__ volatile
	(
		"rdtsc;"
		:"=a" (Lo),
		 "=d" (Hi)
	);
	return ((unsigned long long) Lo) | (((unsigned long long) Hi) << 32);
}

// The function thread which updates the Core values.
static void *uCycle(void *uArg)
{
	uARG *A=(uARG *) uArg;

	unsigned int cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++)
	  if(A->P.Topology[cpu].CPU != NULL) {
		// Initial read of the Unhalted Core & Reference Cycles.
		Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR1, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C0[0].UCC);
		Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR2, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C0[0].URC);
		// Initial read of the TSC in relation to the Logical Core.
		Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_TIME_STAMP_COUNTER, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.TSC[0]);
		// Initial read of other C-States.
		Read_MSR(A->P.Topology[cpu].CPU->FD, MSR_CORE_C3_RESIDENCY, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C3[0]);
		Read_MSR(A->P.Topology[cpu].CPU->FD, MSR_CORE_C6_RESIDENCY, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C6[0]);
	}

	while(A->LOOP) {
		// Settle down N x 50000 microseconds as specified by the command argument.
		usleep(IDLE_BASE_USEC * A->P.IdleTime);

/* CRITICAL_IN  */
		for(cpu=0; cpu < A->P.CPU; cpu++)
		  if(A->P.Topology[cpu].CPU != NULL) {
			// Update the Unhalted Core & the Reference Cycles.
			Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR1, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C0[1].UCC);
			Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_FIXED_CTR2, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C0[1].URC);
			// Update TSC.
			Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_TIME_STAMP_COUNTER, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.TSC[1]);
			// Update C-States.
			Read_MSR(A->P.Topology[cpu].CPU->FD, MSR_CORE_C3_RESIDENCY, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C3[1]);
			Read_MSR(A->P.Topology[cpu].CPU->FD, MSR_CORE_C6_RESIDENCY, (unsigned long long *) &A->P.Topology[cpu].CPU->Cycles.C6[1]);
		}
/* CRITICAL_OUT */

		// Reset C-States average.
		A->P.Avg.Turbo=A->P.Avg.C0=A->P.Avg.C3=A->P.Avg.C6=0;

		unsigned int maxFreq=0, maxTemp=A->P.Topology[0].CPU->TjMax.Target;
		for(cpu=0; cpu < A->P.CPU; cpu++)
		  if(A->P.Topology[cpu].CPU != NULL) {
			// Compute the absolute Delta of Unhalted (Core & Ref) C0 Cycles = Current[1] - Previous[0].
			A->P.Topology[cpu].CPU->Delta.C0.UCC=	(A->P.Topology[cpu].CPU->Cycles.C0[0].UCC > A->P.Topology[cpu].CPU->Cycles.C0[1].UCC) ?
							 A->P.Topology[cpu].CPU->Cycles.C0[0].UCC - A->P.Topology[cpu].CPU->Cycles.C0[1].UCC
							:A->P.Topology[cpu].CPU->Cycles.C0[1].UCC - A->P.Topology[cpu].CPU->Cycles.C0[0].UCC;

			A->P.Topology[cpu].CPU->Delta.C0.URC=	A->P.Topology[cpu].CPU->Cycles.C0[1].URC - A->P.Topology[cpu].CPU->Cycles.C0[0].URC;

			A->P.Topology[cpu].CPU->Delta.C3=	A->P.Topology[cpu].CPU->Cycles.C3[1] - A->P.Topology[cpu].CPU->Cycles.C3[0];

			A->P.Topology[cpu].CPU->Delta.C6=	A->P.Topology[cpu].CPU->Cycles.C6[1] - A->P.Topology[cpu].CPU->Cycles.C6[0];

			A->P.Topology[cpu].CPU->Delta.TSC=	A->P.Topology[cpu].CPU->Cycles.TSC[1] - A->P.Topology[cpu].CPU->Cycles.TSC[0];

			// Compute Turbo State per Cycles Delta. (Protect against a division by zero)
			A->P.Topology[cpu].CPU->State.Turbo=(double) (A->P.Topology[cpu].CPU->Delta.C0.URC != 0) ?
						  (double) (A->P.Topology[cpu].CPU->Delta.C0.UCC) / (double) A->P.Topology[cpu].CPU->Delta.C0.URC
						 : 0.0f;
			// Compute C-States.
			A->P.Topology[cpu].CPU->State.C0=(double) (A->P.Topology[cpu].CPU->Delta.C0.URC) / (double) (A->P.Topology[cpu].CPU->Delta.TSC);
			A->P.Topology[cpu].CPU->State.C3=(double) (A->P.Topology[cpu].CPU->Delta.C3)  / (double) (A->P.Topology[cpu].CPU->Delta.TSC);
			A->P.Topology[cpu].CPU->State.C6=(double) (A->P.Topology[cpu].CPU->Delta.C6)  / (double) (A->P.Topology[cpu].CPU->Delta.TSC);

			A->P.Topology[cpu].CPU->RelativeRatio=A->P.Topology[cpu].CPU->State.Turbo * A->P.Topology[cpu].CPU->State.C0 * (double) A->P.Platform.MaxNonTurboRatio;

			// Relative Frequency = Relative Ratio x Bus Clock Frequency
			A->P.Topology[cpu].CPU->RelativeFreq=A->P.Topology[cpu].CPU->RelativeRatio * A->P.ClockSpeed;

			// Save TSC.
			A->P.Topology[cpu].CPU->Cycles.TSC[0]=A->P.Topology[cpu].CPU->Cycles.TSC[1];
			// Save the Unhalted Core & Reference Cycles for next iteration.
			A->P.Topology[cpu].CPU->Cycles.C0[0].UCC=A->P.Topology[cpu].CPU->Cycles.C0[1].UCC;
			A->P.Topology[cpu].CPU->Cycles.C0[0].URC =A->P.Topology[cpu].CPU->Cycles.C0[1].URC;
			// Save also the C-State Reference Cycles.
			A->P.Topology[cpu].CPU->Cycles.C3[0]=A->P.Topology[cpu].CPU->Cycles.C3[1];
			A->P.Topology[cpu].CPU->Cycles.C6[0]=A->P.Topology[cpu].CPU->Cycles.C6[1];

			// Sum the C-States before the average.
			A->P.Avg.Turbo+=A->P.Topology[cpu].CPU->State.Turbo;
			A->P.Avg.C0+=A->P.Topology[cpu].CPU->State.C0;
			A->P.Avg.C3+=A->P.Topology[cpu].CPU->State.C3;
			A->P.Avg.C6+=A->P.Topology[cpu].CPU->State.C6;

			// Index the Top CPU speed.
			if(maxFreq < A->P.Topology[cpu].CPU->RelativeFreq) {
				maxFreq=A->P.Topology[cpu].CPU->RelativeFreq;
				A->P.Top=cpu;
			}

			// Update the Digital Thermal Sensor.
			if( (Read_MSR(A->P.Topology[cpu].CPU->FD, IA32_THERM_STATUS, (THERM_STATUS *) &A->P.Topology[cpu].CPU->ThermStat)) == -1)
				A->P.Topology[cpu].CPU->ThermStat.DTS=0;

			// Index which Core is hot.
			if(A->P.Topology[cpu].CPU->ThermStat.DTS < maxTemp) {
				maxTemp=A->P.Topology[cpu].CPU->ThermStat.DTS;
				A->P.Hot=cpu;
			}
			// Store the coldest temperature.
			if(A->P.Topology[cpu].CPU->ThermStat.DTS > A->P.Cold)
				A->P.Cold=A->P.Topology[cpu].CPU->ThermStat.DTS;
		}
		// Average the C-States.
		A->P.Avg.Turbo/=A->P.CPU;
		A->P.Avg.C0/=A->P.CPU;
		A->P.Avg.C3/=A->P.CPU;
		A->P.Avg.C6/=A->P.CPU;

		// Fire the drawing thread.
		if(pthread_mutex_trylock(&uDraw_mutex) == 0)
			pthread_create(&A->TID_Draw, NULL, uDraw, A);
	}
	// Is drawing still processing ?
	if(pthread_mutex_trylock(&uDraw_mutex) == EBUSY)
		pthread_join(A->TID_Draw, NULL);

	return(NULL);
}

// Read any data from the SMBIOS.
int	Read_SMBIOS(int structure, int instance, off_t offset, void *buf, size_t nbyte)
{
	ssize_t	retval=0;
	char	pathname[]="/sys/firmware/dmi/entries/999-99/raw";
	int	tmpFD=0, rc=-1;

	sprintf(pathname, "/sys/firmware/dmi/entries/%d-%d/raw", structure, instance);
	if( (tmpFD=open(pathname, O_RDONLY)) != -1 ) {
		retval=pread(tmpFD, buf, nbyte, offset);
		close(tmpFD);
		rc=(retval != nbyte) ? -1 : 0;
	}
	return(rc);
}

// Estimate the Bus Clock Frequency from the TSC.
double	Compute_ExtClock(int ratio)
{
	unsigned long long TSC[2];
	TSC[0]=RDTSC();
	usleep(IDLE_BASE_USEC * ratio);
	TSC[1]=RDTSC();
	return((double) (TSC[1] - TSC[0]) / (IDLE_BASE_USEC * ratio));
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

// Call the CPUID instruction.
void	Read_Features(FEATURES *features)
{
	__asm__ volatile
	(
		"movq	$0x1, %%rax;"
		"cpuid;"
		: "=a"	(features->Std.EAX),
		  "=b"	(features->Std.EBX),
		  "=c"	(features->Std.ECX),
		  "=d"	(features->Std.EDX)
	);
	__asm__ volatile
	(
		"movq	$0x4, %%rax;"
		"xorq	%%rcx, %%rcx;"
		"cpuid;"
		"shr	$26, %%rax;"
		"and	$0x3f, %%rax;"
		"add	$1, %%rax;"
		: "=a"	(features->ThreadCount)
	);
	__asm__ volatile
	(
		"movq	$0x6, %%rax;"
		"cpuid;"
		: "=a"	(features->Thermal_Power_Leaf.EAX),
		  "=b"	(features->Thermal_Power_Leaf.EBX),
		  "=c"	(features->Thermal_Power_Leaf.ECX),
		  "=d"	(features->Thermal_Power_Leaf.EDX)
	);
	__asm__ volatile
	(
		"movq	$0x7, %%rax;"
		"xorq	%%rbx, %%rbx;"
		"xorq	%%rcx, %%rcx;"
		"xorq	%%rdx, %%rdx;"
		"cpuid;"
		: "=a"	(features->ExtFeature.EAX),
		  "=b"	(features->ExtFeature.EBX),
		  "=c"	(features->ExtFeature.ECX),
		  "=d"	(features->ExtFeature.EDX)
	);
	__asm__ volatile
	(
		"movq	$0xa, %%rax;"
		"cpuid;"
		: "=a"	(features->Perf_Monitoring_Leaf.EAX),
		  "=b"	(features->Perf_Monitoring_Leaf.EBX),
		  "=c"	(features->Perf_Monitoring_Leaf.ECX),
		  "=d"	(features->Perf_Monitoring_Leaf.EDX)
	);
	__asm__ volatile
	(
		"movq	$0x80000000, %%rax;"
		"cpuid;"
		: "=a"	(features->LargestExtFunc)
	);
	if(features->LargestExtFunc >= 0x80000004 && features->LargestExtFunc <= 0x80000008)
	{
		__asm__ volatile
		(
			"movq	$0x80000001, %%rax;"
			"cpuid;"
			: "=c"	(features->ExtFunc.ECX),
			  "=d"	(features->ExtFunc.EDX)
		);
		struct
		{
			struct
			{
			unsigned char Chr[4];
			} EAX, EBX, ECX, EDX;
		} Brand;
		char tmpString[48+1]={0x20};
		int ix=0, jx=0, px=0;
		for(ix=0; ix<3; ix++)
		{
			__asm__ volatile
			(
				"cpuid;"
				: "=a"	(Brand.EAX),
				  "=b"	(Brand.EBX),
				  "=c"	(Brand.ECX),
				  "=d"	(Brand.EDX)
				: "a"	(0x80000002 + ix)
			);
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.EAX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.EBX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.ECX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					tmpString[px]=Brand.EDX.Chr[jx];
		}
		for(ix=jx=0; jx < px; jx++)
			if(!(tmpString[jx] == 0x20 && tmpString[jx+1] == 0x20))
				features->BrandString[ix++]=tmpString[jx];
	}
}

static void *uReadAPIC(void *uApic)
{
	uAPIC	*pApic=(uAPIC *) uApic;
	uARG	*A=(uARG *) pApic->A;
	int	cpu=pApic->cpu;

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
		do	{
			__asm__ volatile
				(
				"movq	$0xb, %%rax;"
				"cpuid;"
				: "=a"	(ExtTopology.EAX),
				  "=b"	(ExtTopology.EBX),
				  "=c"	(ExtTopology.ECX),
				  "=d"	(ExtTopology.EDX)
				: "c"	(InputLevel)
				);
			if(!ExtTopology.EAX.Register && !ExtTopology.EBX.Register)
				NoMoreLevels=1;
			else
				{
				switch(ExtTopology.ECX.Type)
					{
					case LEVEL_THREAD:
						{
						SMT_Mask_Width = ExtTopology.EAX.SHRbits;
						SMT_Select_Mask= ~((-1) << SMT_Mask_Width );
						A->P.Topology[cpu].Thread_ID=ExtTopology.EDX.x2APIC_ID & SMT_Select_Mask;

						if((A->P.Topology[cpu].Thread_ID > 0) && (A->P.Features.HTT_enabled == false))
							A->P.Features.HTT_enabled=true;
						}
						break;
					case LEVEL_CORE:
						{
						CorePlus_Mask_Width = ExtTopology.EAX.SHRbits;
						CoreOnly_Select_Mask = (~((-1) << CorePlus_Mask_Width ) ) ^ SMT_Select_Mask;
						A->P.Topology[cpu].Core_ID = (ExtTopology.EDX.x2APIC_ID & CoreOnly_Select_Mask) >> SMT_Mask_Width;
						}
						break;
					}
				}
			InputLevel++;
			}
		while(!NoMoreLevels);

		A->P.Topology[cpu].APIC_ID=ExtTopology.EDX.x2APIC_ID;

		if(A->P.Topology[cpu].CPU == NULL)
			A->P.Topology[cpu].CPU=calloc(1, sizeof(CPU_STRUCT));
		}
	else
		A->P.Topology[cpu].APIC_ID=-1;

	return(NULL);
}

unsigned int Create_Topology(uARG *A)
{
	int cpu=0, CountEnabledCPU=0;

	A->P.Topology=calloc(A->P.CPU, sizeof(TOPOLOGY));

	uAPIC *uApic=calloc(A->P.CPU, sizeof(uAPIC));

	for(cpu=0; cpu < A->P.CPU; cpu++) {
		uApic[cpu].cpu=cpu;
		uApic[cpu].A=A;
		pthread_create(&uApic[cpu].TID, NULL, uReadAPIC, &uApic[cpu]);
	}
	for(cpu=0; cpu < A->P.CPU; cpu++)
		{
		pthread_join(uApic[cpu].TID, NULL);

		if(A->P.Topology[cpu].CPU != NULL)
			CountEnabledCPU++;
		}
	free(uApic);

	return(CountEnabledCPU);
}

void	Delete_Topology(uARG *A)
{
	int	cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++)
		if(A->P.Topology[cpu].CPU != NULL)
			free(A->P.Topology[cpu].CPU);
	free(A->P.Topology);
}

// Retreive the Integrated Memory Controler settings: the number of channels & their associated RAM timings.
struct IMCINFO *IMC_Read_Info()
{
	struct	IMCINFO *imc=calloc(1, sizeof(struct IMCINFO));

	if(!iopl(3))
	{
		unsigned bus=0xff, dev=0x4, func=0;
		outl(PCI_CONFIG_ADDRESS(bus, 3, 0, 0x48), 0xcf8);
		int code=(inw(0xcfc + (0x48 & 2)) >> 8) & 0x7;
		imc->ChannelCount=(code == 7 ? 3 : code == 4 ? 1 : code == 2 ? 1 : code == 1 ? 1 : 2);
		imc->Channel=calloc(imc->ChannelCount, sizeof(struct CHANNEL));

		unsigned cha=0;
		for(cha=0; cha < imc->ChannelCount; cha++)
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

			imc->Channel[cha].Timing.tCL  =((MRs >> 4) & 0x7) != 0 ? ((MRs >> 4) & 0x7) + 4 : 0;
			imc->Channel[cha].Timing.tRCD =(BANK_TIMING & 0x1E00) >> 9;
			imc->Channel[cha].Timing.tRP  =(BANK_TIMING & 0xF);
			imc->Channel[cha].Timing.tRAS =(BANK_TIMING & 0x1F0) >> 4;
			imc->Channel[cha].Timing.tRRD =(RANK_TIMING_B & 0x1c0) >> 6;
			imc->Channel[cha].Timing.tRFC =(REFRESH_TIMING & 0x1ff);
			imc->Channel[cha].Timing.tWR  =((MRs >> 9) & 0x7) != 0 ? ((MRs >> 9) & 0x7) + 4 : 0;
			imc->Channel[cha].Timing.tRTPr=(BANK_TIMING & 0x1E000) >> 13;
			imc->Channel[cha].Timing.tWTPr=(BANK_TIMING & 0x3E0000) >> 17;
			imc->Channel[cha].Timing.tFAW =(RANK_TIMING_B & 0x3f);
			imc->Channel[cha].Timing.B2B  =(RANK_TIMING_B & 0x1f0000) >> 16;
		}
		iopl(0);
	}
	return(imc);
}
// Release the IMC structure pointers.
void IMC_Free_Info(struct IMCINFO *imc)
{
	if(imc!=NULL)
	{
		if(imc->Channel!=NULL)
			free(imc->Channel);
		free(imc);
	}
}

void	SelectBaseClock(uARG *A)
{
	switch(A->P.ClockSrc) {
		default:
		case SRC_TSC:	// Base Clock = TSC divided by the maximum ratio (without Turbo).
			if(A->MSR) {
				A->P.ClockSpeed=Compute_ExtClock(IDLE_RATIO_DEF) / A->P.Platform.MaxNonTurboRatio;
				A->P.ClockSrc=SRC_TSC;
				break;
			}
			else
				Output(A, "Remark: MSR unavailable. Setting Clock source from BIOS.\n");
		case SRC_BIOS:	// Read the Bus Clock Frequency in the SMBIOS (DMI).
			if((A->BIOS=(A->P.ClockSpeed=Get_ExternalClock()) != 0)) {
				A->P.ClockSrc=SRC_BIOS;
				break;
			}
			else
				Output(A, "Remark: BIOS unavailable. Setting Clock source from the Architecture specifications.\n");
		case SRC_SPEC:	// Set the Base Clock from the architecture array if CPU ID is found.
			if(A->P.ArchID != -1) {
				A->P.ClockSpeed=ARCH[A->P.ArchID].ClockSpeed;
				A->P.ClockSrc=SRC_SPEC;
				break;
			}
			else {
				char defaultClock[128]={0};
				sprintf(defaultClock, "Remark: Architecture not found. Setting Clock source from %s @ %.2f MHz.\n",
					ARCH[0].Architecture, ARCH[0].ClockSpeed);
				Output(A, defaultClock);
			}
		case SRC_ROM:	// Read the Bus Clock Frequency at a specified memory address.
			if((A->P.ClockSpeed=Read_ROM_BCLK(A->P.BClockROMaddr)) !=0 ) {
				A->P.ClockSrc=SRC_ROM;
				break;
			}
		case SRC_USER:{	// Set the Base Clock from the first row in the architecture array.
			A->P.ClockSpeed=ARCH[0].ClockSpeed;
			A->P.ClockSrc=SRC_USER;
		}
		break;
	}
}

// Drawing Button functions.
void	DrawDecorationButton(uARG *A, WBUTTON *wButton)
{
	switch(wButton->ID) {
		case ID_QUIT: {
			XFillRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x + (wButton->w >> 1) - 1, wButton->y, 3, (wButton->h >> 1));
			XDrawArc(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x,
					wButton->y,
					wButton->w,
					wButton->h,
					34 << 8, 66 << 8);
		}
			break;
		case ID_MIN: {
			int inner=(MIN(wButton->w, wButton->h) >> 1) + 2;
			XDrawRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x, wButton->y, wButton->w, wButton->h );
			XFillRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc,
					wButton->x + 2, wButton->y + 2, wButton->w - inner, wButton->h - inner);
		}
			break;
	}
}

void	DrawScrollingButton(uARG *A, WBUTTON *wButton)
{
	switch(wButton->ID) {
		case ID_NORTH: {
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int valign=wButton->h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + wButton->h - inner;
			Arrow[1].x= +halign;		Arrow[1].y= -valign;
			Arrow[2].x= +halign;		Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_SOUTH: {
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int valign=wButton->h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= +halign;		Arrow[1].y= +valign;
			Arrow[2].x= +halign;		Arrow[2].y= -valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_EAST: {
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int halign=wButton->w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=wButton->x + inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= +halign;		Arrow[1].y= +valign;
			Arrow[2].x= -halign;		Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
		case ID_WEST: {
			unsigned int inner=1;
			XPoint Arrow[3];
			unsigned int halign=wButton->w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=wButton->x + wButton->w - inner;	Arrow[0].y=wButton->y + inner;
			Arrow[1].x= -halign;				Arrow[1].y= +valign;
			Arrow[2].x= +halign;				Arrow[2].y= +valign;
			XFillPolygon(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, Arrow, 3, Nonconvex, CoordModePrevious);
		}
			break;
	}
}

void	DrawTextButton(uARG *A, WBUTTON *wButton)
{
	if(wButton->Resource.Text != NULL) {
		size_t length=strlen(wButton->Resource.Text);
		unsigned int inner=(wButton->w - (length * One_Char_Width(wButton->Target))) >> 1;

		XDrawRectangle(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, wButton->x, wButton->y, wButton->w, wButton->h);
		XDrawString(A->display, A->W[wButton->Target].pixmap.B, A->W[wButton->Target].gc, wButton->x+inner, wButton->y+(A->W[wButton->Target].extents.ascent), wButton->Resource.Text, length);
	}
}

void	DrawIconButton(uARG *A, WBUTTON *wButton)
{
	unsigned int inner=wButton->w >> 2;
	char str[2]={wButton->Resource.Label, '\0'};

	XDrawRectangle(A->display, A->W[MAIN].pixmap.B, A->W[MAIN].gc, wButton->x, wButton->y, wButton->w, wButton->h);
	XDrawString(A->display, A->W[MAIN].pixmap.B, A->W[MAIN].gc, wButton->x+inner, wButton->y+(A->W[MAIN].extents.ascent), str, 1);
}

// CallBack functions definition.
void	CallBackQuit(uARG *A, WBUTTON *wButton) ;
void	CallBackButton(uARG *A, WBUTTON *wButton) ;
void	CallBackMinimizeWidget(uARG *A, WBUTTON *wButton) ;
void	CallBackRestoreWidget(uARG *A, WBUTTON *wButton) ;

// Create a button with callback, drawing & collision functions.
void	CreateButton(uARG *A, WBTYPE Type, char ID, int Target, int x, int y, unsigned int w, unsigned h, void *CallBack, RESOURCE *Resource)
{
	WBUTTON *NewButton=malloc(sizeof(WBUTTON));

	NewButton->Type=Type;
	NewButton->ID=ID;

	NewButton->x=x;
	NewButton->y=y;
	NewButton->w=w;
	NewButton->h=h;

	NewButton->CallBack=CallBack;
	int G=NewButton->Target=Target;

	switch(NewButton->Type)
	{
		case DECORATION: {
			NewButton->Resource.Bitmap=NULL;
			NewButton->DrawFunc=DrawDecorationButton;
		}
			break;
		case SCROLLING: {
			NewButton->Resource.Bitmap=NULL;
			NewButton->DrawFunc=DrawScrollingButton;
		}
			break;
		case TEXT: {
			if(Resource->Text != NULL) {
				NewButton->Resource.Text=malloc(strlen(Resource->Text) + 1);
				strcpy(NewButton->Resource.Text, Resource->Text);
				NewButton->DrawFunc=DrawTextButton;
			}
			else {
				NewButton->Type=ICON;
				NewButton->Resource.Label=0x20;
				NewButton->DrawFunc=DrawIconButton;
			}
		}
			break;
		case ICON: {
			G=MAIN;
			NewButton->Resource.Label=Resource->Label;
			NewButton->DrawFunc=DrawIconButton;
		}
			break;
	}

	NewButton->Chain=NULL;
	if(A->W[G].wButton[HEAD] == NULL)
		A->W[G].wButton[HEAD]=NewButton;
	else
		A->W[G].wButton[TAIL]->Chain=NewButton;
	A->W[G].wButton[TAIL]=NewButton;
}

// Destroy a button attached to a Widget.
void	DestroyButton(uARG *A, int G, char ID)
{
	WBUTTON *wButton=A->W[G].wButton[HEAD], *cButton=A->W[G].wButton[HEAD];
	while(cButton != NULL)
		if(cButton->ID == ID) {
			if(cButton == A->W[G].wButton[TAIL])
				A->W[G].wButton[TAIL]=wButton;
			wButton->Chain=cButton->Chain;
			switch(cButton->Type) {
				case TEXT:
					if(cButton->Resource.Text != NULL)
						free(cButton->Resource.Text);
					break;
				case DECORATION:
				case SCROLLING:
				case ICON:
					break;
			}
			free(cButton);
			break;
		}
		else {
			wButton=cButton;
			cButton=cButton->Chain;
		}
}

void	DestroyAllButtons(uARG *A, int G)
{
	WBUTTON *wButton=A->W[G].wButton[HEAD], *cButton=NULL;
	while(wButton != NULL) {
		cButton=wButton->Chain;
		switch(wButton->Type) {
			case TEXT:
				if(wButton->Resource.Text != NULL)
					free(wButton->Resource.Text);
			case DECORATION:
			case SCROLLING:
			case ICON:
				break;
		}
		free(wButton);
		wButton=cButton;
	}
}

void	WidgetButtonPress(uARG *A, int G, XEvent *E)
{
	int x=0, y=0;
	switch(E->type) {
		case ButtonPress: {
			x=E->xbutton.x;
			y=E->xbutton.y;
		}
			break;
		case MotionNotify: {
			x=E->xmotion.x;
			y=E->xmotion.y;
		}
			break;
	}
	int T=G, xOffset=0, yOffset=0;
	if(_IS_MDI_) {
		for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
			if( E->xbutton.subwindow == A->W[T].window) {
				xOffset=A->W[T].x;
				yOffset=A->W[T].y;
				break;
			}
	}
	// Search which button is pressed.
	WBUTTON *wButton=NULL;
	for(wButton=A->W[T].wButton[HEAD]; wButton != NULL; wButton=wButton->Chain)
		if((x > wButton->x + xOffset)
		&& (y > wButton->y + yOffset)
		&& (x < wButton->x + xOffset + wButton->w)
		&& (y < wButton->y + yOffset + wButton->h)) {
			// Execute its callback.
			wButton->CallBack(A, wButton);
			break;
		}
}

// Draw the TextCursor shape,
void	DrawCursor(uARG *A, int G, XPoint *Origin)
{
	A->L.TextCursor[0].x=Origin->x;
	A->L.TextCursor[0].y=Origin->y;
	XFillPolygon(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.TextCursor, 3, Nonconvex, CoordModePrevious);
}

// All-in-One function to print a string filled with some New Line terminated texts.
MaxText XPrint(Display *display, Drawable drawable, GC gc, int x, int y, char *NewLineStr, int spacing)
{
	char *pStartLine=NewLineStr, *pNewLine=NULL;
	MaxText  Text={0,0};
	while((pNewLine=strchr(pStartLine,'\n')) != NULL) {
		int cols=pNewLine - pStartLine;
		XDrawString(	display, drawable, gc,
				x,
				y + (spacing * Text.rows),
				pStartLine, cols);
		Text.cols=MAX(cols, Text.cols);
		Text.rows++ ;
		pStartLine=pNewLine+1;
	}
	return(Text);
}

// Scale the MDI window = MAIN Widget.
void	ScaleMDI(uARG *A)
{
	int G=FIRST_WIDGET, RightMost=LAST_WIDGET, BottomMost=LAST_WIDGET;
	while(G <= LAST_WIDGET) {
		if((A->W[G].x + A->W[G].width)  > (A->W[RightMost].x + A->W[RightMost].width))
			RightMost=G;
		if((A->W[G].y + A->W[G].height) > (A->W[BottomMost].y + A->W[BottomMost].height))
			BottomMost=G;
		G++ ;
	}
	A->W[MAIN].width=A->W[RightMost].x + A->W[RightMost].width + A->L.Margin.H;
	A->W[MAIN].height=A->W[BottomMost].y + A->W[BottomMost].height + A->L.Margin.V + Footer_Height(G);
	// Adjust the Header & Footer axes with the new width.
	int i=0;
	for(i=0; i < A->L.Axes[MAIN].N; i++)
		A->L.Axes[MAIN].Segment[i].x2=A->W[MAIN].width;
	// Adjust scrolling width.
	SetHViewport(MAIN, (A->W[MAIN].width / One_Char_Width(MAIN)) - 3);
	SetVFrame(MAIN, GetVViewport(MAIN) << MAIN_FRAME_VIEW_VSHIFT);
}

// ReSize a Widget window & inform WM.
void	ReSizeWidget(uARG *A, int G)
{
	XSizeHints *hints=NULL;
	if((hints=XAllocSizeHints()) != NULL) {
		hints->x=A->W[G].x;
		hints->y=A->W[G].y;
		hints->min_width= hints->max_width= A->W[G].width;
		hints->min_height=hints->max_height=A->W[G].height;
		hints->flags=USPosition|USSize|PMinSize|PMaxSize;
		XSetWMNormalHints(A->display, A->W[G].window, hints);
		XFree(hints);
	}
	XWindowAttributes xwa={0};
	XGetWindowAttributes(A->display, A->W[G].window, &xwa);
	// Did the Window Manager really resize the Window ?
	if((xwa.width != A->W[G].width) || (xwa.height != A->W[G].height))
		XResizeWindow(A->display, A->W[G].window, A->W[G].width, A->W[G].height);
}

// Move a Widget Window (when the MDI mode is enabled).
void	MoveWidget(uARG *A, XEvent *E)
{
	switch(E->type) {
		case ButtonPress:
			if(!A->T.Locked) {
				// Which Widget is the target ?
				if(_IS_MDI_ && E->xbutton.subwindow) {
					for(A->T.S=LAST_WIDGET; A->T.S >= FIRST_WIDGET; A->T.S--)
						if( E->xbutton.subwindow == A->W[A->T.S].window) {
							A->T.Locked=true;
							break;
						}
				} else {
					for(A->T.S=LAST_WIDGET; A->T.S >= MAIN; A->T.S--)
						if( E->xbutton.window == A->W[A->T.S].window) {
							A->T.Locked=true;
							break;
						}
				}
				if(A->T.Locked) {
					// A Widget is locked then store delta between the mouse position and its window origin.
					if(_IS_MDI_ && (A->T.S != MAIN)) {
						A->T.dx=E->xbutton.x - A->W[A->T.S].x;
						A->T.dy=E->xbutton.y - A->W[A->T.S].y;
					} else {
						A->T.dx=E->xbutton.x_root - A->W[A->T.S].x;
						A->T.dy=E->xbutton.y_root - A->W[A->T.S].y;
					}
					XRaiseWindow(A->display, A->W[A->T.S].window);
					XDefineCursor(A->display, A->W[A->T.S].window, A->MouseCursor[MC_MOVE]);
				}
			}
			break;
		case ButtonRelease:
			if(A->T.Locked) {
				XDefineCursor(A->display, A->W[A->T.S].window, A->MouseCursor[MC_DEFAULT]);
				A->T.S=0;
				A->T.Locked=false;
			}
			break;
		case MotionNotify:
			if(A->T.Locked) {
				// Move the Widget until the button is released.
				int	tx=0,
					ty=0,
					tw=0,
					th=0;
				if(_IS_MDI_ && (A->T.S != MAIN)) {
					tx=E->xmotion.x - A->T.dx,
					ty=E->xmotion.y - A->T.dy,
					tw=A->W[MAIN].width  - (A->W[A->T.S].width + (A->W[A->T.S].border_width << 1)),
					th=A->W[MAIN].height - (A->W[A->T.S].height + (A->W[A->T.S].border_width << 1));
				}
				else {
					tx=E->xmotion.x_root - A->T.dx,
					ty=E->xmotion.y_root - A->T.dy,
					tw=WidthOfScreen(A->screen)  - (A->W[A->T.S].width + (A->W[A->T.S].border_width << 1)),
					th=HeightOfScreen(A->screen) - (A->W[A->T.S].height + (A->W[A->T.S].border_width << 1));
				}
				// Stick the widget position to the Screen borders.
				if(tx > tw)	tx=tw;
				if(tx < 0)	tx=0;
				if(ty > th)	ty=th;
				if(ty < 0)	ty=0;
				XMoveWindow(A->display, A->W[A->T.S].window, tx, ty);
			}
			break;
	}
}

void	IconifyWidget(uARG *A, int G, XEvent *E)
{
	switch(E->type) {
		case UnmapNotify: {
			// When a Widget is iconified, minimized or unmapped, create an associated button in the MAIN window.
			RESOURCE Resource[WIDGETS]=ICON_LABELS;

			int vSpacing=Header_Height(MAIN) + (G * One_Half_Char_Height(MAIN));

			CreateButton(	A, ICON, (ID_NULL + G), G,
					A->W[MAIN].width - (Twice_Char_Width(MAIN) - Quarter_Char_Width(MAIN)),
					vSpacing,
					One_Half_Char_Width(MAIN),
					One_Char_Height(MAIN),
					CallBackRestoreWidget,
					&Resource[G]);
		}
			break;
		case MapNotify: {
			// Remove the button when the Widget is restored on screen.
			DestroyButton(A, MAIN, (ID_NULL + G));
		}
			break;
	}
}

void	MinimizeWidget(uARG *A, int G)
{
	if(_IS_MDI_) {
		if(G != MAIN)
			XUnmapWindow(A->display, A->W[G].window);
	}
	else
		XIconifyWindow(A->display, A->W[G].window, DefaultScreen(A->display));
}

void	RestoreWidget(uARG *A, int G)
{
	XMapWindow(A->display, A->W[G].window);
}

void	SetWidgetName(uARG *A, int G, char *name)
{
	XStoreName(A->display, A->W[G].window, name);
	XSetIconName(A->display, A->W[G].window, name);
}

static void *uSplash(void *uArg)
{
	uARG *A=(uARG *) uArg;
	XEvent	E={0};
	while(A->LOOP) {
		XNextEvent(A->display, &E);
		switch(E.type) {
			case ClientMessage:
				if(E.xclient.send_event)
					A->LOOP=false;
				break;
			case Expose:
				if(!E.xexpose.count)
					{
					XSetForeground(A->display, A->Splash.gc, A->L.globalForeground);
					XCopyPlane(	A->display,
							A->Splash.bitmap,
							A->Splash.window,
							A->Splash.gc,
							0,
							0,
							splash_width,
							splash_height,
							(A->Splash.w - splash_width) >> 1,
							(A->Splash.h - splash_height) >> 1,
							1);
					}
				break;
		}
	}
	return(NULL);
}

void	StartSplash(uARG *A)
{
	A->Splash.x=(WidthOfScreen(A->screen) >> 1)  - (A->Splash.w >> 1);
	A->Splash.y=(HeightOfScreen(A->screen) >> 1) - (A->Splash.h >> 1);
	if((A->Splash.window=XCreateSimpleWindow(A->display,
						DefaultRootWindow(A->display),
						A->Splash.x,
						A->Splash.y,
						A->Splash.w,
						A->Splash.h,
						0,
						0x0,
						0)) != 0
	&& (A->Splash.gc=XCreateGC(A->display, A->Splash.window, 0, NULL)) != 0)
		{
		A->Splash.bitmap=XCreateBitmapFromData(A->display, A->Splash.window, (const char *) splash_bits, splash_width, splash_height);

		XSizeHints *hints=NULL;
		if((hints=XAllocSizeHints()) != NULL)
			{
			hints->x=A->Splash.x;
			hints->y=A->Splash.y;
			hints->min_width= hints->max_width= A->Splash.w;
			hints->min_height=hints->max_height=A->Splash.h;
			hints->flags=USPosition|USSize|PMinSize|PMaxSize;
			XSetWMNormalHints(A->display, A->Splash.window, hints);
			XFree(hints);
			}
		else
			XMoveWindow(A->display, A->Splash.window, A->Splash.x, A->Splash.y);

		struct {
			unsigned long	flags,
					functions,
					decorations;
			long		inputMode;
			unsigned long	status;
			} data={flags:2, functions:0, decorations:0, inputMode:0, status:0};

		Atom property=XInternAtom(A->display, "_MOTIF_WM_HINTS", true);
		XChangeProperty(A->display, A->Splash.window, property, property, 32, PropModeReplace, (unsigned char *) &data, 5);

		XSelectInput(A->display, A->Splash.window, ExposureMask);
		XMapWindow(A->display, A->Splash.window);

		pthread_create(&A->TID_Draw, NULL, uSplash, A);
		}
}

void	StopSplash(uARG *A)
{
	XClientMessageEvent E={type:ClientMessage, window:A->Splash.window, format:32};
	XSendEvent(A->display, A->Splash.window, 0, 0, (XEvent *)&E);
	XFlush(A->display);

	pthread_join(A->TID_Draw, NULL);

	XUnmapWindow(A->display, A->Splash.window);
	XFreePixmap(A->display, A->Splash.bitmap);
	XFreeGC(A->display, A->Splash.gc);
	XDestroyWindow(A->display, A->Splash.window);

	A->TID_Draw=0;
	A->LOOP=true;
}

// Release the X-Window resources.
void	CloseDisplay(uARG *A)
{
	int MC=MC_COUNT;
	do {
		MC-- ;
		if(A->MouseCursor[MC])
			XFreeCursor(A->display, A->MouseCursor[MC]);
	} while(MC);

	if(A->xfont->fid)
		XUnloadFont(A->display, A->xfont->fid);
	free(A->fontName);

	XCloseDisplay(A->display);
}

// Initialize a new X-Window display.
int	OpenDisplay(uARG *A)
{
	int noerr=true;
	if((A->display=XOpenDisplay(NULL)) && (A->screen=DefaultScreenOfDisplay(A->display)) )
	{
		switch(A->xACL) {
			case 'Y':
			case 'y':
				XEnableAccessControl(A->display);
				break;
			case 'N':
			case 'n':
				XDisableAccessControl(A->display);
				break;
		}

		// Try to load the requested font.
		if(strlen(A->fontName) == 0)
			strcpy(A->fontName, "Fixed");

		if((A->xfont=XLoadQueryFont(A->display, A->fontName)) == NULL)
			noerr=false;
		else {
			// Adjust margins using the font size.
			A->L.Margin.H=(A->xfont->max_bounds.rbearing - A->xfont->min_bounds.lbearing) << 1;
			A->L.Margin.V=(A->xfont->ascent + A->xfont->descent) + ((A->xfont->ascent + A->xfont->descent) >> 2);
		}
		if(noerr) {
			A->MouseCursor[MC_DEFAULT]=XCreateFontCursor(A->display, XC_left_ptr);
			A->MouseCursor[MC_MOVE]=XCreateFontCursor(A->display, XC_fleur);
			A->MouseCursor[MC_WAIT]=XCreateFontCursor(A->display, XC_watch);
		}
	}
	else	noerr=false;
	return(noerr);
}

// Release the Widget resources.
void	CloseWidgets(uARG *A)
{
	if(A->L.Output)
		free(A->L.Output);

	int G=0;
	for(G=LAST_WIDGET; G >= MAIN ; G--)
		{
		XFreePixmap(A->display, A->W[G].pixmap.B);
		XFreePixmap(A->display, A->W[G].pixmap.F);
		if(A->L.Page[G].Pageable)
			XFreePixmap(A->display, A->L.Page[G].Pixmap);
		XFreeGC(A->display, A->W[G].gc);
		XDestroyWindow(A->display, A->W[G].window);
		free(A->L.Axes[G].Segment);
		DestroyAllButtons(A, G);
		}
	free(A->L.WB.String);
	free(A->L.Usage.C0);
	free(A->L.Usage.C3);
	free(A->L.Usage.C6);
}

// Create the Widgets.
int	OpenWidgets(uARG *A)
{
	int noerr=true;
	char str[sizeof(HDSIZE)];

	// Allocate memory for chart elements.
	A->L.Usage.C0=malloc(A->P.CPU * sizeof(XRectangle));
	A->L.Usage.C3=malloc(A->P.CPU * sizeof(XRectangle));
	A->L.Usage.C6=malloc(A->P.CPU * sizeof(XRectangle));

	XSetWindowAttributes swa={
	/* Pixmap: background, None, or ParentRelative	*/	background_pixmap:None,
	/* unsigned long: background pixel		*/	background_pixel:A->L.globalBackground,
	/* Pixmap: border of the window or CopyFromParent */	border_pixmap:CopyFromParent,
	/* unsigned long: border pixel value */			border_pixel:A->L.globalForeground,
	/* int: one of bit gravity values */			bit_gravity:0,
	/* int: one of the window gravity values */		win_gravity:0,
	/* int: NotUseful, WhenMapped, Always */		backing_store:DoesBackingStore(DefaultScreenOfDisplay(A->display)),
	/* unsigned long: planes to be preserved if possible */	backing_planes:AllPlanes,
	/* unsigned long: value to use in restoring planes */	backing_pixel:0,
	/* Bool: should bits under be saved? (popups) */	save_under:DoesSaveUnders(DefaultScreenOfDisplay(A->display)),
	/* long: set of events that should be saved */		event_mask:EventMaskOfScreen(DefaultScreenOfDisplay(A->display)),
	/* long: set of events that should not propagate */	do_not_propagate_mask:0,
	/* Bool: boolean value for override_redirect */		override_redirect:False,
	/* Colormap: color map to be associated with window */	colormap:DefaultColormap(A->display, DefaultScreen(A->display)),
	/* Cursor: Cursor to be displayed (or None) */		cursor:A->MouseCursor[MC_DEFAULT]};

	int G=0;
	for(G=MAIN; noerr && (G < WIDGETS); G++)
		{
			// Dispose Widgets from each other : [Right & Bottom + width & height] Or -[1,-1] = X,Y origin + Margins.
			int U=A->W[G].x;
			int V=A->W[G].y;
			if(_IS_MDI_) {
				if(G != MAIN) {
					switch(U) {
						case -1:
							A->W[G].x=A->L.Margin.H;
							break;
						case MAIN:
							A->W[G].x=A->L.Margin.H + A->W[U].width;
							break;
						default:
							A->W[G].x=A->L.Margin.H + A->W[U].x + A->W[U].width;
							break;
					}
					switch(V) {
						case -1:
							A->W[G].y=A->L.Margin.V;
							break;
						case MAIN:
							A->W[G].y=A->L.Margin.V + A->W[V].height;
							break;
						default:
							A->W[G].y=A->L.Margin.V + A->W[V].y + A->W[V].height;
							break;
					}
				}
				else {
					A->W[G].x=A->L.Start.H;
					A->W[G].y=A->L.Start.V;
				}
			}
			else {
				if(U == -1)
					A->W[G].x=A->L.Start.H;
				else
					A->W[G].x=A->L.Margin.H + A->W[U].x + A->W[U].width;
				if(V == -1)
					A->W[G].y=A->L.Start.V;
				else
					A->W[G].y=A->L.Margin.V + A->W[V].y + A->W[V].height;
			}

			// Override default colors if values supplied in the command line.
			if(A->L.globalBackground != GLOBAL_BACKGROUND)
				A->W[G].background=A->L.globalBackground;
			if(A->L.globalForeground != GLOBAL_FOREGROUND)
				A->W[G].foreground=A->L.globalForeground;

			// Create the Widgets.
			if((A->W[G].window=XCreateWindow(A->display,
							_IS_MDI_ && (G != MAIN) ?
							A->W[MAIN].window
							: DefaultRootWindow(A->display),
							A->W[G].x, A->W[G].y, A->W[G].width, A->W[G].height, A->W[G].border_width,
							CopyFromParent, InputOutput, CopyFromParent, CWBorderPixel|CWOverrideRedirect|CWCursor, &swa)) )
				{
				sprintf(str, TITLE_MAIN_FMT, _MAJOR, _MINOR, _NIGHTLY);
				SetWidgetName(A, G, str);

				if((A->W[G].gc=XCreateGC(A->display, A->W[G].window, 0, NULL)))
					{
					XSetFont(A->display, A->W[G].gc, A->xfont->fid);

					switch(G) {
						// Compute Widgets scaling.
						case MAIN: {
							SetHViewport(G, MAIN_TEXT_WIDTH  - 3);
							SetVViewport(G, MAIN_TEXT_HEIGHT - 2);
							SetHFrame(G, GetHViewport(G) << MAIN_FRAME_VIEW_HSHIFT);
							SetVFrame(G, GetVViewport(G) << MAIN_FRAME_VIEW_VSHIFT);

							XTextExtents(	A->xfont, HDSIZE, MAIN_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										+ A->W[G].extents.descent;

							A->W[G].width	= A->W[G].extents.overall.width;
							A->W[G].height	= One_Char_Height(G) * MAIN_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
							A->W[G].height+=Footer_Height(G);

							// First run if MAIN is not defined as MDI then create buttons.
							if(!_IS_MDI_) {
								unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

								CreateButton(	A, DECORATION, ID_QUIT, G,
										A->W[G].width - (One_Char_Width(G) * 5) - 2,
										2,
										square - 2,
										square - 2,
										CallBackQuit,
										NULL);

								CreateButton(	A, DECORATION, ID_MIN, G,
										A->W[G].width - Twice_Char_Width(G) - 2,
										2,
										square - 2,
										square - 2,
										CallBackMinimizeWidget,
										NULL);

								CreateButton(	A, SCROLLING, ID_NORTH, G,
										A->W[G].width - square,
										Header_Height(G) + 2,
										square,
										square,
										CallBackButton,
										NULL);

								CreateButton(	A, SCROLLING, ID_SOUTH, G,
										A->W[G].width - square,
										A->W[G].height - (Footer_Height(G) + square + 2),
										square,
										square,
										CallBackButton,
										NULL);

								CreateButton(	A, SCROLLING, ID_EAST, G,
										A->W[G].width
										- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
										A->W[G].height - (square + 2),
										square,
										square,
										CallBackButton,
										NULL);

								CreateButton(	A, SCROLLING, ID_WEST, G,
										A->W[G].width
										- (MAX( 3*One_Char_Height(G)+Quarter_Char_Height(G),
											3 * One_Char_Width(G)+Quarter_Char_Width(G)) + 2),
										A->W[G].height - (square + 2),
										square,
										square,
										CallBackButton,
										NULL);
							}
						}
							break;
						case CORES: {
							SetHViewport(G, CORES_TEXT_WIDTH);
							SetVViewport(G, CORES_TEXT_HEIGHT);
							SetHFrame(G, GetHViewport(G));
							SetVFrame(G, GetVViewport(G));

							XTextExtents(	A->xfont, HDSIZE, CORES_TEXT_WIDTH << 1,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										 + A->W[G].extents.descent;

							A->W[G].width	= A->W[G].extents.overall.width
									+ (One_Char_Width(G) << 2)
									+ Half_Char_Width(G);
							A->W[G].height	= Quarter_Char_Height(G)
									+ One_Char_Height(G) * (CORES_TEXT_HEIGHT + 2);

							// Prepare the chart axes.
							A->L.Axes[G].N=CORES_TEXT_WIDTH + 2;
							// A->L.Axes[G].N=CORES_TEXT_WIDTH + 1; /* Without a footer (plus 1 to malloc size) */
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int	i=0,
								j=A->W[G].extents.overall.width / CORES_TEXT_WIDTH;
							for(i=0; i <= CORES_TEXT_WIDTH; i++) {
								A->L.Axes[G].Segment[i].x1=(j * i) + (One_Char_Width(G) * 3);
								A->L.Axes[G].Segment[i].y1=3 + One_Char_Height(G);
								A->L.Axes[G].Segment[i].x2=A->L.Axes[G].Segment[i].x1;
								A->L.Axes[G].Segment[i].y2=((CORES_TEXT_HEIGHT + 1) * One_Char_Height(G)) - 3;
							}
							// With a footer.
							A->L.Axes[G].Segment[i].x1=0;
							A->L.Axes[G].Segment[i].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[i].x2=A->W[G].width;
							A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							A->W[G].height+=Footer_Height(G);

							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL);

							struct {
								char		ID;
								RESOURCE	RSC;
							} Loader[]={	{ID:ID_PAUSE, RSC:{Text:RSC_PAUSE}},
									{ID:ID_FREQ , RSC:{Text:RSC_FREQ} },
									{ID:ID_CYCLE, RSC:{Text:RSC_CYCLE}},
									{ID:ID_RATIO, RSC:{Text:RSC_RATIO}},
									{ID:ID_NULL , RSC:{Text:NULL}}
								};
							int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
							for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
								CreateButton(	A, TEXT, Loader[i].ID, G,
											spacing,
											A->W[G].height - Footer_Height(G) + 2,
											One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
											One_Char_Height(G),
											CallBackButton,
											&Loader[i].RSC);
						}
							break;
						case CSTATES: {
							SetHViewport(G, CSTATES_TEXT_WIDTH);
							SetVViewport(G, CSTATES_TEXT_HEIGHT);
							SetHFrame(G, GetHViewport(G));
							SetVFrame(G, GetVViewport(G));

							XTextExtents(	A->xfont, HDSIZE, CSTATES_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth	= A->xfont->max_bounds.rbearing
											- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight	= A->W[G].extents.ascent
											+ A->W[G].extents.descent;

							A->W[G].width	= Quarter_Char_Width(G)
									+ Twice_Half_Char_Width(G)
									+ (CSTATES_TEXT_WIDTH * One_Half_Char_Width(G));
							A->W[G].height	= Twice_Half_Char_Height(G)
									+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT);

							// Prepare the chart axes.
							A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 2;
							// A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 1; /* Without a footer */
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int i=0;
							for(i=0; i < CSTATES_TEXT_HEIGHT; i++) {
								A->L.Axes[G].Segment[i].x1=0;
								A->L.Axes[G].Segment[i].y1=(i + 1) * One_Char_Height(G);
								A->L.Axes[G].Segment[i].x2=A->W[G].width;
								A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							}
							// Percent.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1=A->W[G].width
												- One_Char_Width(G) - Quarter_Char_Width(G);
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y1=One_Char_Height(G);
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x2=A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y2=One_Char_Height(G)
												+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT);
							// With a footer.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x1=0;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x2=A->W[G].width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y2=A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1;
							A->W[G].height+=Footer_Height(G);

							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL);

							struct {
								char		ID;
								RESOURCE	RSC;
							} Loader[]={	{ID:ID_PAUSE, RSC:{Text:RSC_PAUSE}},
									{ID:ID_STATE, RSC:{Text:RSC_STATE}},
									{ID:ID_NULL , RSC:{Text:NULL}}
								};
							int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
							for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
								CreateButton(	A, TEXT, Loader[i].ID, G,
										spacing,
										A->W[G].height - Footer_Height(G) + 2,
										One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
										One_Char_Height(G),
										CallBackButton,
										&Loader[i].RSC);
						}
							break;
						case TEMPS: {
							SetHViewport(G, TEMPS_TEXT_WIDTH);
							SetVViewport(G, TEMPS_TEXT_HEIGHT);
							SetHFrame(G, GetHViewport(G));
							SetVFrame(G, GetVViewport(G));

							XTextExtents(	A->xfont, HDSIZE, TEMPS_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth	= A->xfont->max_bounds.rbearing
											- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight	= A->W[G].extents.ascent
											+ A->W[G].extents.descent;

							A->W[G].width	= A->W[G].extents.overall.width
									+ (One_Char_Width(G) * 6);
							A->W[G].height	=  Quarter_Char_Height(G)
									+ One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1);

							// Prepare the chart axes.
							A->L.Axes[G].N=TEMPS_TEXT_WIDTH + 2;
							// A->L.Axes[G].N=TEMPS_TEXT_WIDTH + 1; /* Without a footer (plus 1 to malloc size) */
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int i=0;
							for(i=0; i < TEMPS_TEXT_WIDTH; i++) {
								A->L.Axes[G].Segment[i].x1=Twice_Char_Width(G) + (i * One_Char_Width(G));
								A->L.Axes[G].Segment[i].y1=(TEMPS_TEXT_HEIGHT + 1) * One_Char_Height(G);
								A->L.Axes[G].Segment[i].x2=A->L.Axes[G].Segment[i].x1 + One_Char_Width(G);
								A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							}
							// Padding.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 3].x2=A->L.Axes[G].Segment[A->L.Axes[G].N - 3].x1
													+ Twice_Half_Char_Width(G);
							// Temps.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1=Twice_Char_Width(G);
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y1=One_Char_Height(G);
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x2=A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y2=One_Char_Height(G)
												+ (One_Char_Height(G) * TEMPS_TEXT_HEIGHT);
							// With a footer.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x1=0;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x2=A->W[G].width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y2=A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1;
							A->W[G].height+=Footer_Height(G);

							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL);

							struct {
								char		ID;
								RESOURCE	RSC;
							} Loader[]={	{ID:ID_PAUSE, RSC:{Text:RSC_PAUSE}},
									{ID:ID_NULL , RSC:{Text:NULL}}
								};
							int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
							for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
								CreateButton(	A, TEXT, Loader[i].ID, G,
											spacing,
											A->W[G].height - Footer_Height(G) + 2,
											One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
											One_Char_Height(G),
											CallBackButton,
											&Loader[i].RSC);
						}
							break;
						case SYSINFO: {
							SetHViewport(G, SYSINFO_TEXT_WIDTH  - 3);
							SetVViewport(G, SYSINFO_TEXT_HEIGHT - 2);
							SetHFrame(G, GetHViewport(G) << SYSINFO_FRAME_VIEW_HSHIFT);
							SetVFrame(G, GetVViewport(G) << SYSINFO_FRAME_VIEW_VSHIFT);

							XTextExtents(	A->xfont, HDSIZE, SYSINFO_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										+ A->W[G].extents.descent;

							A->W[G].width	= A->W[G].extents.overall.width;
							A->W[G].height	= One_Char_Height(G) * SYSINFO_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
							A->W[G].height+=Footer_Height(G);

							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL);

							CreateButton(	A, SCROLLING, ID_NORTH, G,
									A->W[G].width - square,
									Header_Height(G) + 2,
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_SOUTH, G,
									A->W[G].width - square,
									A->W[G].height - (Footer_Height(G) + square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_EAST, G,
									A->W[G].width
									- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_WEST, G,
									2,
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							struct {
								char		ID;
								RESOURCE	RSC;
							} Loader[]={	{ID:ID_PAUSE,     RSC:{Text:RSC_PAUSE}},
									{ID:ID_WALLBOARD, RSC:{Text:RSC_WALLBOARD}},
									{ID:ID_INCLOOP,   RSC:{Text:RSC_INCLOOP}},
									{ID:ID_TSC,       RSC:{Text:RSC_TSC}},
									{ID:ID_BIOS,      RSC:{Text:RSC_BIOS}},
									{ID:ID_SPEC,      RSC:{Text:RSC_SPEC}},
									{ID:ID_ROM,       RSC:{Text:RSC_ROM}},
									{ID:ID_DECLOOP,   RSC:{Text:RSC_DECLOOP}},
									{ID:ID_NULL ,     RSC:{Text:NULL}}
								};
							int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
							int i=0;
							for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
								CreateButton(	A, TEXT, Loader[i].ID, G,
											spacing,
											A->W[G].height - Footer_Height(G) + 2,
											One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
											One_Char_Height(G),
											CallBackButton,
											&Loader[i].RSC);

							// Prepare a Wallboard string with the Processor information.
							int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;
							sprintf(str, OVERCLOCK, A->P.Features.BrandString, A->P.Platform.MaxNonTurboRatio * A->P.ClockSpeed);
							A->L.WB.Length=strlen(str) + (padding << 1);
							A->L.WB.String=calloc(A->L.WB.Length, 1);
							memset(A->L.WB.String, 0x20, A->L.WB.Length);
							A->L.WB.Scroll=padding;
							A->L.WB.Length=strlen(str) + A->L.WB.Scroll;
						}
							break;
						case DUMP: {
							SetHViewport(G, DUMP_TEXT_WIDTH  - 3);
							SetVViewport(G, DUMP_TEXT_HEIGHT - 2);
							SetHFrame(G, GetHViewport(G) << DUMP_FRAME_VIEW_HSHIFT);
							SetVFrame(G, GetVViewport(G) << DUMP_FRAME_VIEW_VSHIFT);

							XTextExtents(	A->xfont, HDSIZE, DUMP_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										+ A->W[G].extents.descent;

							A->W[G].width	= A->W[G].extents.overall.width;
							A->W[G].height	= One_Char_Height(G) * DUMP_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height(G) + 1;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height - 1;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
							A->W[G].height+=Footer_Height(G);

							unsigned int square=MAX(One_Char_Height(G), One_Char_Width(G));

							CreateButton(	A, DECORATION, ID_MIN, G,
									A->W[G].width - Twice_Char_Width(G) - 2,
									2,
									square - 2,
									square - 2,
									CallBackMinimizeWidget,
									NULL);

							CreateButton(	A, SCROLLING, ID_NORTH, G,
									A->W[G].width - square,
									Header_Height(G) + 2,
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_SOUTH, G,
									A->W[G].width - square,
									A->W[G].height - (Footer_Height(G) + square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_EAST, G,
									A->W[G].width
									- (MAX(Twice_Char_Height(G),Twice_Char_Width(G)) + 2),
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							CreateButton(	A, SCROLLING, ID_WEST, G,
									2,
									A->W[G].height - (square + 2),
									square,
									square,
									CallBackButton,
									NULL);

							struct {
								char		ID;
								RESOURCE	RSC;
							} Loader[]={	{ID:ID_PAUSE, RSC:{Text:RSC_PAUSE}},
									{ID:ID_NULL , RSC:{Text:NULL}}
								};
							int spacing=MAX(One_Char_Height(G), One_Char_Width(G)) + 2 + 2;
							int i=0;
							for(i=0; Loader[i].ID != ID_NULL; i++, spacing+=One_Char_Width(G) * (2 + strlen(Loader[i-1].RSC.Text)))
								CreateButton(	A, TEXT, Loader[i].ID, G,
										spacing,
										A->W[G].height - Footer_Height(G) + 2,
										One_Char_Width(G) * (1 + strlen(Loader[i].RSC.Text)),
										One_Char_Height(G),
										CallBackButton,
										&Loader[i].RSC);
						}
							break;
					}
					ReSizeWidget(A, G);
				}
				else	noerr=false;
			}
			else	noerr=false;
		}
		if(noerr && _IS_MDI_) {
			ScaleMDI(A);
			ReSizeWidget(A, MAIN);

			unsigned int square=MAX(A->W[MAIN].extents.charHeight, A->W[MAIN].extents.charWidth);

			CreateButton(	A, DECORATION, ID_QUIT, MAIN,
					A->W[MAIN].width - (A->W[MAIN].extents.charWidth * 5) - 2,
					2,
					square - 2,
					square - 2,
					CallBackQuit,
					NULL);

			CreateButton(	A, DECORATION, ID_MIN, MAIN,
					A->W[MAIN].width - (A->W[MAIN].extents.charWidth << 1) - 2,
					2,
					square - 2,
					square - 2,
					CallBackMinimizeWidget,
					NULL);

			CreateButton(	A, SCROLLING, ID_NORTH, MAIN,
					A->W[MAIN].width - square,
					Header_Height(MAIN) + 2,
					square,
					square,
					CallBackButton,
					NULL);

			CreateButton(	A, SCROLLING, ID_SOUTH, MAIN,
					A->W[MAIN].width - square,
					A->L.Axes[MAIN].Segment[1].y2 - (square + 2),
					square,
					square,
					CallBackButton,
					NULL);

			CreateButton(	A, SCROLLING, ID_EAST, MAIN,
					A->W[MAIN].width
					- (MAX((A->W[MAIN].extents.charHeight << 1), (A->W[MAIN].extents.charWidth << 1)) + 2),
					A->L.Axes[MAIN].Segment[1].y2 + 2,
					square,
					square,
					CallBackButton,
					NULL);

			CreateButton(	A, SCROLLING, ID_WEST, MAIN,
					A->W[MAIN].width
					- (MAX(	3 * A->W[MAIN].extents.charHeight+(A->W[MAIN].extents.charHeight >> 2),
						3 * A->W[MAIN].extents.charWidth+(A->W[MAIN].extents.charWidth >> 2))
					+ 2),
					A->L.Axes[MAIN].Segment[1].y2 + 2,
					square,
					square,
					CallBackButton,
					NULL);
		}
		for(G=MAIN; noerr && (G < WIDGETS); G++)
			if((A->W[G].pixmap.B=XCreatePixmap(	A->display, A->W[G].window,
								A->W[G].width, A->W[G].height,
								DefaultDepthOfScreen(A->screen) ))
			&& (A->W[G].pixmap.F=XCreatePixmap(	A->display, A->W[G].window,
								A->W[G].width, A->W[G].height,
								DefaultDepthOfScreen(A->screen) )))
			{
				if(A->L.Page[G].Pageable) {
					A->L.Page[G].width	= (GetHViewport(G) * One_Char_Width(G))
								+ One_Char_Width(G);
					A->L.Page[G].height	= (GetVViewport(G) * One_Char_Height(G))
								+ Half_Char_Height(G) + Quarter_Char_Height(G)
								- 3;	// above the footer line.

					A->L.Page[G].Pixmap=XCreatePixmap(	A->display, A->W[G].window,
										GetHFrame(G) * One_Char_Width(G),
										GetVFrame(G) * One_Char_Height(G),
										DefaultDepthOfScreen(A->screen) );
					// Preset the scrolling area.
					XSetForeground(A->display, A->W[G].gc, A->W[G].background);
					XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							0,
							0,
							GetHFrame(G) * One_Char_Width(G),
							GetVFrame(G) * One_Char_Height(G));
				}
				long EventProfile=BASE_EVENTS;
				if(_IS_MDI_) {
					if(G == MAIN)
						EventProfile=EventProfile | CLICK_EVENTS | MOVE_EVENTS;
				}
				else	EventProfile=EventProfile | CLICK_EVENTS | MOVE_EVENTS;

				XSelectInput(A->display, A->W[G].window, EventProfile);
			}
			else	noerr=false;

	// Store some Ratios into a string for future chart drawing.
	sprintf(A->P.Boost, "%02d%02d%02d",	A->P.Platform.MinimumRatio,
						A->P.Platform.MaxNonTurboRatio,
						A->P.Turbo.MaxRatio_1C);
	return(noerr);
}

// Draw the layout background.
void	BuildLayout(uARG *A, int G)
{
	XSetBackground(A->display, A->W[G].gc, A->W[G].background);
	// Clear entirely the background.
	XSetForeground(A->display, A->W[G].gc, A->W[G].background);
	XFillRectangle(A->display, A->W[G].pixmap.B, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height);

	// Draw the axes.
	XSetForeground(A->display, A->W[G].gc, AXES_COLOR);
	XDrawSegments(A->display, A->W[G].pixmap.B, A->W[G].gc, A->L.Axes[G].Segment, A->L.Axes[G].N);

	// Draw the buttons.
	XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
	struct WButton *wButton=NULL;
	for(wButton=A->W[G].wButton[HEAD]; wButton != NULL; wButton=wButton->Chain)
		wButton->DrawFunc(A, wButton);

	switch(G) {
		case MAIN:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );

			// If the MDI mode is enabled then draw a separator line.
			if(_IS_MDI_) {
				XSetForeground(A->display, A->W[G].gc, MDI_SEP_COLOR);
				XDrawLine(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->L.Axes[G].Segment[1].x1,
					A->L.Axes[G].Segment[1].y2 + Footer_Height(G) + 1,
					A->L.Axes[G].Segment[1].x2,
					A->L.Axes[G].Segment[1].y2 + Footer_Height(G) + 1);
			}
			if(A->L.Output) {
				if(A->L.Page[G].Pageable) {
					// Clear the scrolling area.
					XSetForeground(A->display, A->W[G].gc, A->W[G].background);
					XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							0,
							0,
							GetHFrame(G) * One_Char_Width(G),
							GetVFrame(G) * One_Char_Height(G));
					// Draw the Output log in a scrolling area.
					XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);

					MaxText Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
								Half_Char_Width(G),
								One_Char_Height(G),
								A->L.Output,
								One_Char_Height(G));

					SetHListing(G, MAX(Listing.cols, GetHScrolling(G)));
					SetVListing(G, MAX(Listing.rows, GetVScrolling(G)));

					if(GetVListing(G) >= GetVViewport(G))
						SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					SetHScrolling(G, 0);
				}
				else {
					// Draw the Output log in a fixed height area.
					XRectangle R[]=	{ {
							x:0,
							y:Header_Height(G),
							width:A->W[G].width,
							height:(One_Char_Height(G) * MAIN_TEXT_HEIGHT) - Footer_Height(G),
							} };
 					XSetClipRectangles(A->display, A->W[G].gc, 0, 0, R, 1, Unsorted);

 					XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
					A->L.Page[G].Listing=XPrint(A->display, A->W[G].pixmap.B, A->W[G].gc,
								One_Char_Width(G),
								One_Char_Height(G),
								A->L.Output,
								One_Char_Height(G));

					XSetClipMask(A->display, A->W[G].gc, None);
				}
			}
		}
			break;
		case CORES:
		{
			char str[sizeof(CORE_NUM)];

			// Draw the Core identifiers, the header, and the footer on the chart.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Half_Char_Width(G),
						( One_Char_Height(G) * (cpu + 1 + 1) ),
						str, strlen(str) );
			}
			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G),
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					"Ratio", 5);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((5 + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					"5", 1);
			XSetForeground(A->display, A->W[G].gc, LOW_VALUE_COLOR);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->P.Platform.MinimumRatio + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Boost[0], 2);
			XSetForeground(A->display, A->W[G].gc, MED_VALUE_COLOR);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->P.Platform.MaxNonTurboRatio + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Boost[2], 2);
			XSetForeground(A->display, A->W[G].gc, HIGH_VALUE_COLOR);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width(G) * ((A->P.Turbo.MaxRatio_1C + 1) * 2),
					One_Char_Height(G) * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Boost[4], 2);
		}
			break;
		case CSTATES:
		{
			char str[sizeof(CORE_NUM)];

			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - One_Half_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT),
					"%", 1 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (10 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"90", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (30 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"70", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (50 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"50", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (70 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"30", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width(G) - Quarter_Char_Width(G),
					One_Char_Height(G)
					+ (90 * (One_Char_Height(G) * CSTATES_TEXT_HEIGHT)) / 100,
					"10", 2 );
			XDrawString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G) + (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT + 1)),
					"~", 1 );

			// Draw the Core identifiers.
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Twice_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G)),
						One_Char_Height(G),
						str, strlen(str) );
				if(A->P.Topology[cpu].CPU == NULL) {
					XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
					XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
							One_Half_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G)),
							One_Char_Height(G)
							+ (One_Char_Height(G) * CSTATES_TEXT_HEIGHT),
							"OFF", 3 );
				}
			}
		}
			break;
		case TEMPS:
		{
			char str[sizeof(CORE_NUM)];

			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawImageString(A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G),
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width(G),
					One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
					"Temps", 5);
			// Draw the Core identifiers.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						(One_Char_Width(G) * 5)
						+ Quarter_Char_Width(G)
						+ (cpu << 1) * Twice_Char_Width(G),
						One_Char_Height(G),
						str, strlen(str) );
			}
		}
			break;
		case SYSINFO:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );

			char str[256]={0};

			int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;
			sprintf(str, OVERCLOCK, A->P.Features.BrandString, A->P.Platform.MaxNonTurboRatio * A->P.ClockSpeed);
			memcpy(&A->L.WB.String[padding], str, strlen(str));

			sprintf(str, "Loop(%d usecs)", (IDLE_BASE_USEC * A->P.IdleTime) );
			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - (24 * One_Char_Width(G)),
					A->W[G].height - Quarter_Char_Height(G),
					str, strlen(str) );

			if(A->L.Page[G].Pageable) {
				char 		*items=calloc(8192, 1);
				const char	powered[2]={'N', 'Y'},
						*enabled[2]={"OFF", "ON"},
						*ClockSrcStr[SRC_COUNT]={"TSC", "BIOS", "SPEC", "ROM", "USER"};

				sprintf(items, PROC_FORMAT,
					A->P.Features.BrandString,
					A->P.ClockSpeed,					ClockSrcStr[A->P.ClockSrc],
					A->P.Features.Std.EAX.ExtFamily + A->P.Features.Std.EAX.Family,
					(A->P.Features.Std.EAX.ExtModel << 4) + A->P.Features.Std.EAX.Model,
					A->P.Features.Std.EAX.Stepping,
					A->P.Features.ThreadCount,
					ARCH[A->P.ArchID].Architecture,
					powered[A->P.Features.Std.EDX.VME],
					powered[A->P.Features.Std.EDX.DE],
					powered[A->P.Features.Std.EDX.PSE],
					powered[A->P.Features.Std.EDX.TSC],
					powered[A->P.Features.Std.EDX.MSR],			enabled[A->MSR],
					powered[A->P.Features.Std.EDX.PAE],
					powered[A->P.Features.Std.EDX.APIC],
					powered[A->P.Features.Std.EDX.MTRR],
					powered[A->P.Features.Std.EDX.PGE],
					powered[A->P.Features.Std.EDX.MCA],
					powered[A->P.Features.Std.EDX.PAT],
					powered[A->P.Features.Std.EDX.PSE36],
					powered[A->P.Features.Std.EDX.PSN],
					powered[A->P.Features.Std.EDX.DS_PEBS],			enabled[!A->P.MiscFeatures.PEBS],
					powered[A->P.Features.Std.EDX.ACPI],
					powered[A->P.Features.Std.EDX.SS],
					powered[A->P.Features.Std.EDX.HTT],			enabled[A->P.Features.HTT_enabled],
					powered[A->P.Features.Std.EDX.TM1],
					powered[A->P.Features.Std.ECX.TM2],
					powered[A->P.Features.Std.EDX.PBE],
					powered[A->P.Features.Std.ECX.DTES64],
					powered[A->P.Features.Std.ECX.DS_CPL],
					powered[A->P.Features.Std.ECX.VMX],
					powered[A->P.Features.Std.ECX.SMX],
					powered[A->P.Features.Std.ECX.EIST],			enabled[A->P.MiscFeatures.EIST],
					powered[A->P.Features.Std.ECX.CNXT_ID],
					powered[A->P.Features.Std.ECX.FMA],
					powered[A->P.Features.Std.ECX.xTPR],			enabled[!A->P.MiscFeatures.xTPR],
					powered[A->P.Features.Std.ECX.PDCM],
					powered[A->P.Features.Std.ECX.PCID],
					powered[A->P.Features.Std.ECX.DCA],
					powered[A->P.Features.Std.ECX.x2APIC],
					powered[A->P.Features.Std.ECX.TSCDEAD],
					powered[A->P.Features.Std.ECX.XSAVE],
					powered[A->P.Features.Std.ECX.OSXSAVE],
					powered[A->P.Features.ExtFunc.EDX.XD_Bit],		enabled[!A->P.MiscFeatures.XD_Bit],
					powered[A->P.Features.ExtFunc.EDX.PG_1GB],
					powered[A->P.Features.ExtFeature.EBX.HLE],
					powered[A->P.Features.ExtFeature.EBX.RTM],
					powered[A->P.Features.ExtFeature.EBX.FastStrings],	enabled[A->P.MiscFeatures.FastStrings],
												enabled[A->P.MiscFeatures.TCC],
												enabled[A->P.MiscFeatures.PerfMonitoring],
												enabled[!A->P.MiscFeatures.BTS],
												enabled[A->P.MiscFeatures.CPUID_MaxVal],

					A->P.Turbo.MaxRatio_4C, A->P.Turbo.MaxRatio_3C, A->P.Turbo.MaxRatio_2C, A->P.Turbo.MaxRatio_1C,
					powered[A->P.Features.Thermal_Power_Leaf.EAX.Turbo],	enabled[!A->P.MiscFeatures.Turbo],

					powered[A->P.Features.Std.EDX.FPU],
					powered[A->P.Features.Std.EDX.CX8],
					powered[A->P.Features.Std.EDX.SEP],
					powered[A->P.Features.Std.EDX.CMOV],
					powered[A->P.Features.Std.EDX.CLFSH],
					powered[A->P.Features.Std.EDX.MMX],
					powered[A->P.Features.Std.EDX.FXSR],
					powered[A->P.Features.Std.EDX.SSE],
					powered[A->P.Features.Std.EDX.SSE2],
					powered[A->P.Features.Std.ECX.SSE3],
					powered[A->P.Features.Std.ECX.SSSE3],
					powered[A->P.Features.Std.ECX.SSE41],
					powered[A->P.Features.Std.ECX.SSE42],
					powered[A->P.Features.Std.ECX.PCLMULDQ],
					powered[A->P.Features.Std.ECX.MONITOR],			enabled[A->P.MiscFeatures.FSM],
					powered[A->P.Features.Std.ECX.CX16],
					powered[A->P.Features.Std.ECX.MOVBE],
					powered[A->P.Features.Std.ECX.POPCNT],
					powered[A->P.Features.Std.ECX.AES],
					powered[A->P.Features.Std.ECX.AVX], powered[A->P.Features.ExtFeature.EBX.AVX2],
					powered[A->P.Features.Std.ECX.F16C],
					powered[A->P.Features.Std.ECX.RDRAND],
					powered[A->P.Features.ExtFunc.ECX.LAHFSAHF],
					powered[A->P.Features.ExtFunc.EDX.SYSCALL],
					powered[A->P.Features.ExtFunc.EDX.RDTSCP],
					powered[A->P.Features.ExtFunc.EDX.IA64],
					powered[A->P.Features.ExtFeature.EBX.BMI1], powered[A->P.Features.ExtFeature.EBX.BMI2]);

				sprintf(str, TOPOLOGY_SECTION, A->P.OnLine);
				strcat(items, str);
				int cpu=0;
				for(cpu=0; cpu < A->P.CPU; cpu++) {
					if(A->P.Topology[cpu].CPU != NULL)
						sprintf(str, TOPOLOGY_FORMAT,
							cpu,
							A->P.Topology[cpu].APIC_ID,
							A->P.Topology[cpu].Core_ID,
							A->P.Topology[cpu].Thread_ID,
							enabled[1]);
					else
						sprintf(str, "     #%02u       -       -       -   [%3s]\n",
							cpu,
							enabled[0]);
					strcat(items, str);
				}

				sprintf(str, PERF_SECTION,
						A->P.Features.Perf_Monitoring_Leaf.EAX.MonCtrs, A->P.Features.Perf_Monitoring_Leaf.EAX.MonWidth,
						A->P.Features.Perf_Monitoring_Leaf.EDX.FixCtrs, A->P.Features.Perf_Monitoring_Leaf.EDX.FixWidth);
				strcat(items, str);
				for(cpu=0; cpu < A->P.CPU; cpu++) {
					if(A->P.Topology[cpu].CPU != NULL)
						sprintf(str, PERF_FORMAT,
							cpu,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN0_OS,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN0_Usr,
							A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN0,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_OS,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN1_Usr,
							A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN1,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_OS,
							A->P.Topology[cpu].CPU->FixedPerfCounter.EN2_Usr,
							A->P.Topology[cpu].CPU->FixedPerfCounter.AnyThread_EN2);
					else
						sprintf(str, " - \n");
					strcat(items, str);
				}

				strcat(items, RAM_SECTION);
				strcat(items, CHA_FORMAT);

				if(A->M != NULL) {
					unsigned cha=0;
					for(cha=0; cha < A->M->ChannelCount; cha++) {
						sprintf(str, CAS_FORMAT,
							cha,
							A->M->Channel[cha].Timing.tCL,
							A->M->Channel[cha].Timing.tRCD,
							A->M->Channel[cha].Timing.tRP,
							A->M->Channel[cha].Timing.tRAS,
							A->M->Channel[cha].Timing.tRRD,
							A->M->Channel[cha].Timing.tRFC,
							A->M->Channel[cha].Timing.tWR,
							A->M->Channel[cha].Timing.tRTPr,
							A->M->Channel[cha].Timing.tWTPr,
							A->M->Channel[cha].Timing.tFAW,
							A->M->Channel[cha].Timing.B2B);
						strcat(items, str);
					}
				}
				else
					strcat(items, "Unknown IMC\n");

				// Clear the scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->W[G].background);
				XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
						0,
						0,
						GetHFrame(G) * One_Char_Width(G),
						GetVFrame(G) * One_Char_Height(G));
				// Draw the system information.
				XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
				A->L.Page[G].Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							Half_Char_Width(G),
							One_Char_Height(G),
							items,
							One_Char_Height(G));
				free(items);
			}
			else {
				sprintf(str, OVERCLOCK, A->P.Features.BrandString, A->P.Platform.MaxNonTurboRatio * A->P.ClockSpeed);
				XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
				XDrawString(A->display, A->W[G].pixmap.B, A->W[G].gc,
						Half_Char_Width(G),
						Header_Height(G) + One_Char_Height(G),
						str,
						strlen(str));
			}
		}
			break;
		case DUMP:
		{
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width(G),
					One_Char_Height(G),
					A->L.Page[G].Title,
					strlen(A->L.Page[G].Title) );
		}
			break;
	}
}

// Fusion in one map the background and the foreground layouts.
void	MapLayout(uARG *A, int G)
{
	XCopyArea(A->display, A->W[G].pixmap.B, A->W[G].pixmap.F, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
}

// Send the map to the display.
void	FlushLayout(uARG *A, int G)
{
	if(A->L.Page[G].Pageable)
		XCopyArea(A->display, A->L.Page[G].Pixmap, A->W[G].pixmap.F, A->W[G].gc,
			One_Char_Width(G) *  A->L.Page[G].hScroll,
			One_Char_Height(G) * A->L.Page[G].vScroll,
			A->L.Page[G].width,
			A->L.Page[G].height,
			0,
			Header_Height(G) + 2 );	// bellow the header line.

	XCopyArea(A->display,A->W[G].pixmap.F, A->W[G].window, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
	XFlush(A->display);
}

// Draw the layout foreground.
void	DrawLayout(uARG *A, int G)
{
	switch(G) {
		case MAIN:
		{
			int edline=_IS_MDI_ ? A->L.Axes[G].Segment[1].y2 + Footer_Height(G) : A->W[G].height;
			// Draw the buffer if it is not empty.
			if(A->L.Input.KeyLength > 0)
			{
				XSetForeground(A->display, A->W[G].gc, PROMPT_COLOR);
				XDrawImageString(A->display, A->W[G].pixmap.F, A->W[G].gc,
						Quarter_Char_Width(G),
						edline - Quarter_Char_Height(G),
						A->L.Input.KeyBuffer, A->L.Input.KeyLength);
			}
			// Flash the TextCursor.
			A->L.Play.flashCursor=!A->L.Play.flashCursor;
			XSetForeground(A->display, A->W[G].gc, A->L.Play.flashCursor ? CURSOR_COLOR : A->W[G].background);

			XPoint Origin=	{
					x:(A->L.Input.KeyLength * One_Char_Width(G)) + Quarter_Char_Width(G),
					y:edline - (Quarter_Char_Height(G) >> 1)
					};
			DrawCursor(A, G, &Origin);

		}
			break;
		case CORES:
		{
			char str[16];
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++)
			  if(A->P.Topology[cpu].CPU != NULL) {
				// Select a color based ratio.
				unsigned long BarBg, BarFg;
				if(A->P.Topology[cpu].CPU->RelativeRatio <= A->P.Platform.MinimumRatio)
					{ BarBg=INIT_VALUE_COLOR; BarFg=DYNAMIC_COLOR; }
				if(A->P.Topology[cpu].CPU->RelativeRatio >  A->P.Platform.MinimumRatio)
					{ BarBg=LOW_VALUE_COLOR; BarFg=CORES_BACKGROUND; }
				if(A->P.Topology[cpu].CPU->RelativeRatio >= A->P.Platform.MaxNonTurboRatio)
					{ BarBg=MED_VALUE_COLOR; BarFg=CORES_BACKGROUND; }
				if(A->P.Topology[cpu].CPU->RelativeRatio >= A->P.Turbo.MaxRatio_4C)
					{ BarBg=HIGH_VALUE_COLOR; BarFg=CORES_BACKGROUND; }

				// Draw the Core frequency.
				XSetForeground(A->display, A->W[G].gc, BarBg);
				XFillRectangle(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						One_Char_Width(G) * 3,
						3 + ( One_Char_Height(G) * (cpu + 1) ),
						(A->W[G].extents.overall.width * A->P.Topology[cpu].CPU->RelativeRatio) / CORES_TEXT_WIDTH,
						One_Char_Height(G) - 3);

				// For each Core, display its frequency, C-STATE & ratio.
				if(A->L.Play.freqHertz && (A->P.Topology[cpu].CPU->RelativeRatio >= 5.0f) ) {
					XSetForeground(A->display, A->W[G].gc, BarFg);
					sprintf(str, CORE_FREQ, A->P.Topology[cpu].CPU->RelativeFreq);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width(G) * 5,
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );
				}

				XSetForeground(A->display, A->W[G].gc, DYNAMIC_COLOR);
				if(A->L.Play.cycleValues) {
					sprintf(str, CORE_DELTA,
							A->P.Topology[cpu].CPU->Delta.C0.UCC / (IDLE_BASE_USEC * A->P.IdleTime),
							A->P.Topology[cpu].CPU->Delta.C0.URC / (IDLE_BASE_USEC * A->P.IdleTime),
							A->P.Topology[cpu].CPU->Delta.C3 / (IDLE_BASE_USEC * A->P.IdleTime),
							A->P.Topology[cpu].CPU->Delta.C6 / (IDLE_BASE_USEC * A->P.IdleTime),
							A->P.Topology[cpu].CPU->Delta.TSC / (IDLE_BASE_USEC * A->P.IdleTime));
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width(G) * 13,
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );
				}
				else if(!A->L.Play.ratioValues) {
					sprintf(str, CORE_CYCLES,
							A->P.Topology[cpu].CPU->Cycles.C0[1].UCC,
							A->P.Topology[cpu].CPU->Cycles.C0[1].URC);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width(G) * 13,
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );
				}

				if(A->L.Play.ratioValues) {
					sprintf(str, CORE_RATIO, A->P.Topology[cpu].CPU->RelativeRatio);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							Twice_Char_Width(G) * A->P.Turbo.MaxRatio_1C,
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );
				}
			} else {
					XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							Twice_Char_Width(G) * A->P.Turbo.MaxRatio_1C,
							One_Char_Height(G) * (cpu + 1 + 1),
							"OFF", 3 );
			}
		}
			break;
		case CSTATES:
		{
			char str[32];
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++)
			  if(A->P.Topology[cpu].CPU != NULL) {
				// Prepare the C0 chart.
				A->L.Usage.C0[cpu].x=Half_Char_Width(G) + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width(G));
				A->L.Usage.C0[cpu].y=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Topology[cpu].CPU->State.C0);
				A->L.Usage.C0[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C0[cpu].height=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Topology[cpu].CPU->State.C0;
				// Prepare the C3 chart.
				A->L.Usage.C3[cpu].x=Half_Char_Width(G) + A->L.Usage.C0[cpu].x + A->L.Usage.C0[cpu].width;
				A->L.Usage.C3[cpu].y=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Topology[cpu].CPU->State.C3);
				A->L.Usage.C3[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C3[cpu].height=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Topology[cpu].CPU->State.C3;
				// Prepare the C6 chart.
				A->L.Usage.C6[cpu].x=Half_Char_Width(G) + A->L.Usage.C3[cpu].x + A->L.Usage.C3[cpu].width;
				A->L.Usage.C6[cpu].y=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Topology[cpu].CPU->State.C6);
				A->L.Usage.C6[cpu].width=(One_Char_Width(G) * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C6[cpu].height=One_Char_Height(G)
							+ (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Topology[cpu].CPU->State.C6;
			}			// Display the C-State averages.
			sprintf(str, CSTATES_PERCENT,	100.f * A->P.Avg.Turbo,
							100.f * A->P.Avg.C0,
							100.f * A->P.Avg.C3,
							100.f * A->P.Avg.C6);
			XDrawString(A->display, A->W[G].pixmap.F, A->W[G].gc,
						Twice_Char_Width(G),
						One_Char_Height(G) + (One_Char_Height(G) * (CSTATES_TEXT_HEIGHT + 1)),
						str, strlen(str) );

			// Draw C0, C3 & C6 states.
			XSetForeground(A->display, A->W[G].gc, GRAPH1_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C0, A->P.CPU);
			XSetForeground(A->display, A->W[G].gc, GRAPH2_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C3, A->P.CPU);
			XSetForeground(A->display, A->W[G].gc, GRAPH3_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C6, A->P.CPU);

			if(A->L.Play.cStatePercent)
				for(cpu=0; cpu < A->P.CPU; cpu++)
				  if(A->P.Topology[cpu].CPU != NULL) {
					XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
					sprintf(str, CORE_NUM, cpu);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							0,
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );

					XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
					sprintf(str, CSTATES_PERCENT,	100.f * A->P.Topology[cpu].CPU->State.Turbo,
									100.f * A->P.Topology[cpu].CPU->State.C0,
									100.f * A->P.Topology[cpu].CPU->State.C3,
									100.f * A->P.Topology[cpu].CPU->State.C6);

					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							Twice_Half_Char_Width(G),
							One_Char_Height(G) * (cpu + 1 + 1),
							str, strlen(str) );
				}
		}
			break;
		case TEMPS:
		{
			char str[16];
			// Update & draw the temperature histogram.
			int i=0;
			XSegment *U=&A->L.Axes[G].Segment[i], *V=&A->L.Axes[G].Segment[i+1];
			for(i=0; i < (TEMPS_TEXT_WIDTH - 1); i++, U=&A->L.Axes[G].Segment[i], V=&A->L.Axes[G].Segment[i+1]) {
				U->x1=V->x1 - One_Char_Width(G);
				U->x2=V->x2 - One_Char_Width(G);
				U->y1=V->y1;
				U->y2=V->y2;
			}
			V=&A->L.Axes[G].Segment[i - 1];
			U->x1=(TEMPS_TEXT_WIDTH + 2) * One_Char_Width(G);
			U->y1=V->y2;
			U->x2=U->x1 + One_Char_Width(G);
/*
			U->y2=(( (TEMPS_TEXT_HEIGHT * A->P.Topology[A->P.Hot].CPU->ThermStat.DTS)
				/ A->P.Topology[A->P.Hot].CPU->TjMax.Target) + 2) * One_Char_Height(G);
*/
			U->y2=(A->W[G].height * A->P.Topology[A->P.Hot].CPU->ThermStat.DTS)
				/ A->P.Topology[A->P.Hot].CPU->TjMax.Target;

			XSetForeground(A->display, A->W[G].gc, GRAPH1_COLOR);
			XDrawSegments(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Axes[G].Segment, A->L.Axes[G].N - 2);

			// Display the Core temperature.
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++)
			  if(A->P.Topology[cpu].CPU != NULL) {
				XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
				sprintf(str, TEMPERATURE, A->P.Topology[cpu].CPU->TjMax.Target - A->P.Topology[cpu].CPU->ThermStat.DTS);
				XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						(One_Char_Width(G) * 5) + Quarter_Char_Width(G) + (cpu << 1) * Twice_Char_Width(G),
						One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
						str, strlen(str));
			} else {
				XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
				XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						(One_Char_Width(G) * 5) + Quarter_Char_Width(G) + (cpu << 1) * Twice_Char_Width(G),
						One_Char_Height(G) * (TEMPS_TEXT_HEIGHT + 1 + 1),
						"OFF", 3);
			}
			// Show Temperature Thresholds
/*
			XSetForeground(A->display, A->W[G].gc, GRAPH2_COLOR);
			int Threshold[2]={
					(( (TEMPS_TEXT_HEIGHT * A->P.Topology[A->P.Hot].CPU->ThermIntr.Threshold1)
					/ A->P.Topology[A->P.Hot].CPU->TjMax.Target) + 2) * One_Char_Height(G),
					(( (TEMPS_TEXT_HEIGHT * A->P.Topology[A->P.Hot].CPU->ThermIntr.Threshold2)
					/ A->P.Topology[A->P.Hot].CPU->TjMax.Target) + 2) * One_Char_Height(G)
					};
			XDrawLine(	A->display, A->W[G].pixmap.F, A->W[G].gc,
					Twice_Char_Width(G),
					Threshold[0],
					U->x2,
					Threshold[1]);
			XSetForeground(A->display, A->W[G].gc, DYNAMIC_COLOR);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					One_Half_Char_Width(G) << 2,
					Threshold[0],
					"T#1", 3);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					TEMPS_TEXT_WIDTH * One_Char_Width(G),
					Threshold[1],
					"T#2", 3);
*/
			// Display the hottest temperature between all Cores, and the coldest since start up.
			int	HotValue=A->P.Topology[A->P.Hot].CPU->TjMax.Target - A->P.Topology[A->P.Hot].CPU->ThermStat.DTS;

			if(HotValue <= LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, INIT_VALUE_COLOR);
			if(HotValue > LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, LOW_VALUE_COLOR);
			if(HotValue > MED_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, MED_VALUE_COLOR);
			if(HotValue >= HIGH_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, HIGH_VALUE_COLOR);
			sprintf(str, TEMPERATURE, HotValue);
			XDrawImageString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					U->x2,
					U->y2,
					str, 3);

			int	ColdValue=A->P.Topology[A->P.Hot].CPU->TjMax.Target - A->P.Cold,
				yCold=(( (TEMPS_TEXT_HEIGHT * A->P.Cold)
					/ A->P.Topology[A->P.Hot].CPU->TjMax.Target) + 2) * One_Char_Height(G);

			if(ColdValue <= LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, INIT_VALUE_COLOR);
			if(ColdValue > LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, LOW_VALUE_COLOR);
			if(ColdValue > MED_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, MED_VALUE_COLOR);
			if(ColdValue >= HIGH_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, HIGH_VALUE_COLOR);
			sprintf(str, TEMPERATURE, ColdValue);
			XDrawImageString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					0,
					yCold,
					str, 3);
		}
			break;
		case SYSINFO:
			// Display the Wallboard.
			if(A->L.Play.wallboard) {
				// Scroll the Wallboard.
				if(A->L.WB.Scroll < A->L.WB.Length)
					A->L.WB.Scroll++;
				else
					A->L.WB.Scroll=0;
				int padding=SYSINFO_TEXT_WIDTH - strlen(A->L.Page[G].Title) - 6;

				XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
				XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						One_Char_Width(G) * (strlen(A->L.Page[G].Title) + 2),
						One_Char_Height(G),
						&A->L.WB.String[A->L.WB.Scroll], padding);
			}
			break;
		case DUMP:
			// Dump a bunch of Registers with their Address, Name & Value.
		{
			// Do we dump more than the Widget height ?
			int Rows=0, row=0;
			if(A->L.Page[G].Pageable)
				Rows=GetVViewport(G);
			else
				Rows=DUMP_TEXT_HEIGHT - 2;

			char *items=calloc(Rows, DUMP_TEXT_WIDTH);
			for(row=0; row < Rows; row++)
			{
				char binStr[BIN64_STR]={0};
				DumpRegister(A->P.Topology[0].CPU->FD, A->L.DumpTable[row].Addr, NULL, binStr);

				char mask[PRE_TEXT]={0}, str[PRE_TEXT]={0};
				sprintf(mask, REG_FORMAT, row,
							A->L.DumpTable[row].Addr,
							A->L.DumpTable[row].Name,
							REG_ALIGN - strlen(A->L.DumpTable[row].Name));
				sprintf(str, mask, 0x20);
				strcat(items, str);

				int H=0;
				for(H=0; H < 15; H++) {
					strncat(items, &binStr[H << 2], 4);
					strcat(items, " ");
				};
				strncat(items, &binStr[H << 2], 4);
				strcat(items, "]\n");
			}
			if(A->L.Page[G].Pageable) {
				// Clear the scrolling area.
				XSetForeground(A->display, A->W[G].gc, A->W[G].background);
				XFillRectangle(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
						0,
						0,
						GetHFrame(G) * One_Char_Width(G),
						GetVFrame(G) * One_Char_Height(G));
				// Draw the Registers in a scrolling area.
				XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
				A->L.Page[G].Listing=XPrint(A->display, A->L.Page[G].Pixmap, A->W[G].gc,
							One_Char_Width(G),
							One_Char_Height(G),
							items,
							One_Char_Height(G));
			} else {
				// Draw the Registers in a fixed height area.
				XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
				A->L.Page[G].Listing=XPrint(A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width(G),
							Header_Height(G) + One_Char_Height(G),
							items,
							One_Char_Height(G));
			}
			free(items);
		}
			break;
	}
}

// Update the Widget name with the Top Core frequency, temperature, c-states.
void	UpdateWidgetName(uARG *A, int G)
{
	char str[32];
	switch(G) {
		case MAIN:
			if(_IS_MDI_) {
				sprintf(str, TITLE_MDI_FMT,
						A->P.Topology[A->P.Top].CPU->RelativeFreq,
						A->P.Topology[A->P.Hot].CPU->TjMax.Target - A->P.Topology[A->P.Hot].CPU->ThermStat.DTS);
				SetWidgetName(A, G, str);
			}
			break;
		case CORES: {
				sprintf(str, TITLE_CORES_FMT, A->P.Top, A->P.Topology[A->P.Top].CPU->RelativeFreq);
				SetWidgetName(A, G, str);
		}
			break;
		case CSTATES: {
				sprintf(str, TITLE_CSTATES_FMT, 100 * A->P.Avg.C0, 100 * (A->P.Avg.C3 + A->P.Avg.C6));
				SetWidgetName(A, G, str);
		}
			break;
		case TEMPS: {
				sprintf(str, TITLE_TEMPS_FMT,
						A->P.Top, A->P.Topology[A->P.Hot].CPU->TjMax.Target - A->P.Topology[A->P.Hot].CPU->ThermStat.DTS);
				SetWidgetName(A, G, str);
		}
			break;
		case SYSINFO: {
				sprintf(str, TITLE_SYSINFO_FMT, A->P.ClockSpeed);
				SetWidgetName(A, G, str);
		}
			break;
	}
}

// The far drawing procedure which paints the foreground.
static void *uDraw(void *uArg)
{
	uARG *A=(uARG *) uArg;
	int G=0;
	for(G=MAIN; G < WIDGETS; G++) {
		if(!A->PAUSE[G]) {
			MapLayout(A, G);
			DrawLayout(A, G);
			FlushLayout(A, G);
		}
		UpdateWidgetName(A, G);
	}
	// Drawing is done.
	pthread_mutex_unlock(&uDraw_mutex);
	return(NULL);
}

// Implementation of CallBack functions.
void	Play(uARG *A, int G, char ID)
{
	switch(ID) {
		case ID_NORTH:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetVScrolling(G) > 0) {
					SetVScrolling(G, GetVScrolling(G) - SCROLLED_ROWS_PER_ONCE);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_SOUTH:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G))) {
					SetVScrolling(G, GetVScrolling(G) + SCROLLED_ROWS_PER_ONCE);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_EAST:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G))) {
					SetHScrolling(G, GetHScrolling(G) + SCROLLED_COLS_PER_ONCE);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_WEST:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) > 0) {
					SetHScrolling(G, GetHScrolling(G) - SCROLLED_COLS_PER_ONCE);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGUP:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetVScrolling(G) > SCROLLED_ROWS_PER_PAGE) {
					SetVScrolling(G, GetVScrolling(G) - SCROLLED_ROWS_PER_PAGE);
					CallFlush=true;
				}
				else if(GetVScrolling(G) > 0) {
					SetVScrolling(G, 0);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGDW:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G) - SCROLLED_ROWS_PER_PAGE)) {
					SetVScrolling(G, GetVScrolling(G) + SCROLLED_ROWS_PER_PAGE);
					CallFlush=true;
				}
				else if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G))) {
					SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGHOME:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) > 0) {
					SetHScrolling(G, 0);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PGEND:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G))) {
					SetHScrolling(G, GetHListing(G) - GetHViewport(G));
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_CTRLHOME:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) > 0) {
					SetHScrolling(G, 0);
					CallFlush=true;
				}
				if(GetVScrolling(G) > 0) {
					SetVScrolling(G, 0);
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_CTRLEND:
			if(A->L.Page[G].Pageable) {
				bool CallFlush=false;
				if(GetHScrolling(G) < (GetHListing(G) - GetHViewport(G))) {
					SetHScrolling(G, GetHListing(G) - GetHViewport(G));
					CallFlush=true;
				}
				if(GetVScrolling(G) < (GetVListing(G) - GetVViewport(G))) {
					SetVScrolling(G, GetVListing(G) - GetVViewport(G));
					CallFlush=true;
				}
				if(CallFlush)
					FlushLayout(A, G);
			}
			break;
		case ID_PAUSE: {
			A->PAUSE[G]=!A->PAUSE[G];
			XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT * A->PAUSE[G]]);
			}
			break;
		case ID_INCLOOP:
			if(A->P.IdleTime < IDLE_RATIO_MAX) {
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				A->P.IdleTime++;
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_DECLOOP:
			if(A->P.IdleTime > IDLE_RATIO_MIN) {
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				A->P.IdleTime--;
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_FREQ:
			A->L.Play.freqHertz=!A->L.Play.freqHertz;
			break;
		case ID_CYCLE:
			A->L.Play.cycleValues=!A->L.Play.cycleValues;
			break;
		case ID_RATIO:
			A->L.Play.ratioValues=!A->L.Play.ratioValues;
			break;
		case ID_STATE:
			A->L.Play.cStatePercent=!A->L.Play.cStatePercent;
			break;
		case ID_TSC:
			{
				A->P.ClockSrc=SRC_TSC;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_BIOS:
			{
				A->P.ClockSrc=SRC_BIOS;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_SPEC:
			{
				A->P.ClockSrc=SRC_SPEC;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_ROM:
			{
				A->P.ClockSrc=SRC_ROM;
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_WAIT]);
				pthread_mutex_lock(&uDraw_mutex);
				SelectBaseClock(A);
				pthread_mutex_unlock(&uDraw_mutex);
				fDraw(G, true, false);
				XDefineCursor(A->display, A->W[G].window, A->MouseCursor[MC_DEFAULT]);
			}
			break;
		case ID_WALLBOARD:
			A->L.Play.wallboard=!A->L.Play.wallboard;
			break;
	}
}

// Instructions set.
void	Proc_Release(uARG *A)
{
	Output(A, Version);
}

void	Proc_Quit(uARG *A)
{
	Output(A, "Shutting down ...\n");
	A->LOOP=false;
}

void	Proc_Menu(uARG *A)
{
	Output(A, MENU_FORMAT);
}

void	Proc_Help(uARG *A);

COMMAND	Command[]={
			{"help", Proc_Help},
			{"menu", Proc_Menu},
			{"quit", Proc_Quit},
			{"version", Proc_Release},
			{NULL, NULL},
		};

void	Proc_Help(uARG *A)
{
	char stringNL[16]={0};
	int C=0;
	for(C=0; Command[C].Instruction != NULL; C++) {
		sprintf(stringNL, "%s\n", Command[C].Instruction);
		Output(A, stringNL);
	}
}

// Process the commands language.
void	ExecCommand(uARG *A)
{
	int C=0;
	while(Command[C].Instruction != NULL) {
		if(!strcmp(A->L.Input.KeyBuffer, Command[C].Instruction))
			break;
		C++;
	}
	if(Command[C].Instruction != NULL)
		Command[C].Procedure(A);
	else
		Output(A, "SYNTAX ERROR\n");
}

void	CallBackButton(uARG *A, WBUTTON *wButton)
{
	// Flash the selected button.
	XSetForeground(A->display, A->W[wButton->Target].gc, FOCUS_COLOR);
	XDrawRectangle(A->display, A->W[wButton->Target].window, A->W[wButton->Target].gc, wButton->x, wButton->y, wButton->w, wButton->h);
	// Update associated values.
	Play(A, wButton->Target, wButton->ID);
}

void	CallBackQuit(uARG *A, WBUTTON *wButton)
{
	Proc_Quit(A);
}

void	CallBackMinimizeWidget(uARG *A, WBUTTON *wButton)
{
	MinimizeWidget(A, wButton->Target);
}

void	CallBackRestoreWidget(uARG *A, WBUTTON *wButton)
{
	RestoreWidget(A, wButton->Target);
}

// the main thread which manages the X-Window events loop.
static void *uLoop(uARG *A)
{
	XEvent	E={0};
	while(A->LOOP) {
		XNextEvent(A->display, &E);

		int G=0;
		for(G=MAIN; G < WIDGETS; G++)
			if(E.xany.window == A->W[G].window)
				break;

		switch(E.type) {
			case Expose: {
				if(!E.xexpose.count)
					FlushLayout(A, G);
			}
				break;
			case KeyPress:
				{
				KeySym	KeySymPressed;
				XComposeStatus ComposeStatus={0};
				char xkBuffer[256];
				int  xkLength;
				if((xkLength=XLookupString(&E.xkey,
							xkBuffer,
							256,
							&KeySymPressed,
							&ComposeStatus)) )
				if(!(E.xkey.state & AllModMask)
				&& (KeySymPressed >= XK_space)
				&& (KeySymPressed <= XK_asciitilde)
				&& ((A->L.Input.KeyLength + xkLength) < 255)) {
					memcpy(&A->L.Input.KeyBuffer[A->L.Input.KeyLength], xkBuffer, xkLength);
					A->L.Input.KeyLength+=xkLength;
					fDraw(MAIN, false, true);
					}
				switch(KeySymPressed) {
					case XK_Delete:
					case XK_KP_Delete:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyLength=0;
							fDraw(MAIN, false, true);
						}
						break;
					case XK_BackSpace:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyLength--;
							fDraw(MAIN, false, true);
						}
						break;
					case XK_Return:
					case XK_KP_Enter:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyBuffer[A->L.Input.KeyLength]='\0';
							ExecCommand(A);
							A->L.Input.KeyLength=0;
							fDraw(MAIN, true, false);
						}
						break;
					case XK_Escape:
					case XK_Pause:
						if(G != MAIN)
							Play(A, G, ID_PAUSE);
						break;
					case XK_h:
					case XK_H:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_FREQ);
						break;
					case XK_l:
					case XK_L:
						// Fire the drawing thread.
						if(E.xkey.state & ControlMask) {
							if(pthread_mutex_trylock(&uDraw_mutex) == 0)
								pthread_create(&A->TID_Draw, NULL, uDraw, A);
						}
						break;
					case XK_p:
					case XK_P:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_STATE);
						break;
					case XK_q:
					case XK_Q:
						if(E.xkey.state & ControlMask) {
							Proc_Quit(A);
							fDraw(MAIN, true, false);
						}
						break;
					case XK_r:
					case XK_R:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_RATIO);
						break;
					case XK_w:
					case XK_W:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_WALLBOARD);
						break;
					case XK_y:
					case XK_Y:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CYCLE);
						break;
					case XK_Up:
					case XK_KP_Up:
						Play(A, G, ID_NORTH);
						break;
					case XK_Down:
					case XK_KP_Down:
						Play(A, G, ID_SOUTH);
						break;
					case XK_Right:
					case XK_KP_Right:
						Play(A, G, ID_EAST);
						break;
					case XK_Left:
					case XK_KP_Left:
						Play(A, G, ID_WEST);
						break;
					case XK_Page_Up:
					case XK_KP_Page_Up:
						Play(A, G, ID_PGUP);
						break;
					case XK_Page_Down:
					case XK_KP_Page_Down:
						Play(A, G, ID_PGDW);
						break;
					case XK_Home:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CTRLHOME);
						else
							Play(A, G, ID_PGHOME);
						break;
					case XK_End:
						if(E.xkey.state & ControlMask)
							Play(A, G, ID_CTRLEND);
						else
							Play(A, G, ID_PGEND);
						break;
					case XK_KP_Add:
						Play(A, G, ID_DECLOOP);
						break;
					case XK_KP_Subtract:
						Play(A, G, ID_INCLOOP);
						break;
					case XK_F1: {
						Proc_Menu(A);
						fDraw(MAIN, true, false);
					}
						break;
					case XK_F2:
					case XK_F3:
					case XK_F4:
					case XK_F5:
					case XK_F6: {
						// Convert the function key number into a Widget index.
						int T=XLookupKeysym(&E.xkey, 0) - XK_F1;
						// Get the Map state.
						XWindowAttributes xwa={0};
						XGetWindowAttributes(A->display, A->W[T].window, &xwa);
						// Hide or unhide the Widget.
						switch(xwa.map_state) {
							case IsViewable:
								MinimizeWidget(A, T);
								break;
							case IsUnmapped:
								RestoreWidget(A, T);
								break;
						}
					}
						break;
				}
			}
				break;
			case ButtonPress:
				switch(E.xbutton.button) {
					case Button1:
						WidgetButtonPress(A, G, &E);
						break;
					case Button3:
						MoveWidget(A, &E);
						break;
					case Button4: {
						int T=G;
						if(_IS_MDI_) {
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						Play(A, T, ID_NORTH);
						break;
					}
					case Button5: {
						int T=G;
						if(_IS_MDI_) {
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						Play(A, T, ID_SOUTH);
						break;
					}
				}
				break;
			case ButtonRelease:
				switch(E.xbutton.button) {
					case Button3:
						MoveWidget(A, &E);
						break;
				}
				break;
			case MotionNotify:
				if(E.xmotion.state & Button3Mask)
					MoveWidget(A, &E);
				if(E.xmotion.state & Button1Mask)
					WidgetButtonPress(A, G, &E);
				break;
			case ConfigureNotify: {
				A->W[G].x=E.xconfigure.x;
				A->W[G].y=E.xconfigure.y;
				A->W[G].width=E.xconfigure.width;
				A->W[G].height=E.xconfigure.height;
				A->W[G].border_width=E.xconfigure.border_width;
			}
				break;
			case FocusIn:
				XSetWindowBorder(A->display, A->W[G].window, FOCUS_COLOR);
				break;
			case FocusOut:
				XSetWindowBorder(A->display, A->W[G].window, A->W[G].foreground);
				break;
			case DestroyNotify:
				A->LOOP=false;
				break;
			case UnmapNotify: {
					if(G != MAIN)
						A->PAUSE[G]=true;

					IconifyWidget(A, G, &E);
					fDraw(MAIN, true, false);
			}
				break;
			case MapNotify: {
					if(G != MAIN)
						A->PAUSE[G]=false;

					IconifyWidget(A, G, &E);
					fDraw(MAIN, true, false);
			}
				break;
			case VisibilityNotify:
				switch (E.xvisibility.state) {
					case VisibilityUnobscured:
						break;
					case VisibilityPartiallyObscured:
					case VisibilityFullyObscured:
						if(A->L.Play.alwaysOnTop)
							XRaiseWindow(A->display, A->W[G].window);
						break;
				}
				break;
		}
	}
	return(NULL);
}

// Parse the command line arguments.
int	Args(uARG *A, int argc, char *argv[])
{
	OPTION	options[] = {
				{"-D", "%d", &A->MDI,                  "Enable MDI Window (0/1)"                            },
				{"-F", "%s", A->fontName,              "Font name"                                          },
				{"-a", "%c", &A->xACL,                 "Enable or disable X ACL (Y/N)"                      },
				{"-x", "%d", &A->L.Start.H,            "Left position (Pixel value)"                        },
				{"-y", "%d", &A->L.Start.V,            "Top position (Pixel value)"                         },
				{"-b", "%x", &A->L.globalBackground,   "Background color (Hex. value)"                      },
				{"-f", "%x", &A->L.globalForeground,   "Foreground color (Hex. value)"                      },
/*				{"-c", "%ud",&A->P.PerCore,            "Monitor per Thread/Core (0/1)"                      },*/
				{"-S", "%ud",&A->P.ClockSrc,           "Clock source TSC(0) BIOS(1) SPEC(2) ROM(3) USER(4)" },
				{"-M", "%x", &A->P.BClockROMaddr,      "ROM memory address of the Base Clock (Hex. value)"  },
				{"-s", "%ud",&A->P.IdleTime,           "Idle time ratio where N x 50000 (usec)"             },
				{"-h", "%ud",&A->L.Play.freqHertz,     "CPU frequency (0/1)"                                },
				{"-l", "%ud",&A->L.Play.cycleValues,   "Cycle Values (0/1)"                                 },
				{"-r", "%ud",&A->L.Play.ratioValues,   "Ratio Values (0/1)"                                 },
				{"-p", "%ud",&A->L.Play.cStatePercent, "C-STATE percentage (0/1)"                           },
				{"-t", "%ud",&A->L.Play.alwaysOnTop,   "Always On Top (0/1)"                                },
				{"-w", "%ud",&A->L.Play.wallboard,     "Scroll wallboard (0/1)"                             },
		};
	const int s=sizeof(options)/sizeof(OPTION);
	int i=0, j=0, noerr=true;

	if( (argc - ((argc >> 1) << 1)) ) {
		for(j=1; j < argc; j+=2) {
			for(i=0; i < s; i++)
				if(!strcmp(argv[j], options[i].argument)) {
					sscanf(argv[j+1], options[i].format, options[i].pointer);
					break;
				}
			if(i == s) {
				noerr=false;
				break;
			}
		}
	}
	else
		noerr=false;

	if(noerr == false) {
		printf("Usage: %s [OPTION...]\n\n", argv[0]);
		for(i=0; i < s; i++)
			printf("\t%s\t%s\n", options[i].argument, options[i].manual);
		printf("\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n");
		printf("\nReport bugs to webmaster@cyring.fr\n");
	}
	return(noerr);
}

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[])
{
	uARG	A= {
			display:NULL,
			screen:NULL,
			fontName:malloc(sizeof(char)*256),
			xfont:NULL,
			MouseCursor:{0},
			P: {
				ArchID:-1,
				Features: {
					Std:{{0}},
					ThreadCount:0,
					Thermal_Power_Leaf:{{0}},
					Perf_Monitoring_Leaf:{{0}},
					ExtFeature:{{0}},
					LargestExtFunc:0,
					ExtFunc:{{0}},
					BrandString:{0},
					HTT_enabled:false,
				},
				Topology:NULL,
				MiscFeatures:{0},
				Platform:{0},
				Turbo:{0},
				BClockROMaddr:BCLK_ROM_ADDR,
				ClockSpeed:0,
				CPU:0,
				Boost:{0},
				Top:0,
				Hot:0,
				Cold:0,
				PerCore:false,
				ClockSrc:SRC_TSC,
				IdleTime:IDLE_RATIO_DEF,
			},
			Splash: {window:0, gc:0, x:0, y:0, w:splash_width + (splash_width >> 2), h:splash_height << 1},
			W: {
				// MAIN
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:-1,
				y:-1,
				width:300,
				height:150,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:MAIN_BACKGROUND,
				foreground:MAIN_FOREGROUND,
				wButton:{NULL},
				},
				// CORES
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:-1,
				y:MAIN,
				width:300,
				height:150,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:CORES_BACKGROUND,
				foreground:CORES_FOREGROUND,
				wButton:{NULL},
				},
				// CSTATES
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:CORES,
				y:MAIN,
				width:300,
				height:175,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:CSTATES_BACKGROUND,
				foreground:CSTATES_FOREGROUND,
				wButton:{NULL},
				},
				// TEMPS
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:-1,
				y:CSTATES,
				width:225,
				height:150,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:TEMPS_BACKGROUND,
				foreground:TEMPS_FOREGROUND,
				wButton:{NULL},
				},
				// SYSINFO
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:TEMPS,
				y:CSTATES,
				width:500,
				height:300,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:SYSINFO_BACKGROUND,
				foreground:SYSINFO_FOREGROUND,
				wButton:{NULL},
				},
				// DUMP
				{
				window:0,
				pixmap: {B:0, F:0},
				gc:0,
				x:-1,
				y:TEMPS,
				width:675,
				height:200,
				border_width:1,
				extents: {
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:DUMP_BACKGROUND,
				foreground:DUMP_FOREGROUND,
				wButton:{NULL},
				},
			},
			T: {
				Locked:false,
				S:0,
				dx:0,
				dy:0,
			},
			L: {
				globalBackground:GLOBAL_BACKGROUND,
				globalForeground:GLOBAL_FOREGROUND,
				// Margins must be initialized with a zero size.
				Margin: {
					H:0,
					V:0,
				},
				Start: {
					H:0,
					V:0,
				},
				Page: {
					// MAIN
					{
						Pageable:true,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:1},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:MAIN_SECTION,
					},
					// CORES
					{
						Pageable:false,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:0},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:NULL,
					},
					// CSTATES
					{
						Pageable:false,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:0},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:NULL,
					},
					// TEMPS
					{
						Pageable:false,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:0},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:NULL,
					},
					// SYSINFO
					{
						Pageable:true,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:1},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:SYSINFO_SECTION,
					},
					// DUMP
					{
						Pageable:true,
						Visible: {cols:0, rows:0},
						Listing: {cols:0, rows:1},
						FrameSize: {cols:1, rows:1},
						hScroll:0,
						vScroll:0,
						Pixmap:0,
						width:0,
						height:0,
						Title:DUMP_SECTION,
					},
				},
				Play: {
					freqHertz:true,
					cycleValues:false,
					ratioValues:true,
					cStatePercent:false,
					alwaysOnTop: false,
					wallboard:false,
					flashCursor:true,
				},
				WB: {
					Scroll:0,
					Length:0,
					String:NULL,
				},
				Usage:{C0:NULL, C3:NULL, C6:NULL},
				DumpTable: {
					{"IA32_PERF_STATUS", IA32_PERF_STATUS},
					{"IA32_CLOCK_MODULATION", IA32_CLOCK_MODULATION},
					{"IA32_PKG_THERM_INTERRUP", IA32_PKG_THERM_INTERRUPT},
					{"IA32_THERM_STATUS", IA32_THERM_STATUS},
					{"IA32_MISC_ENABLE", IA32_MISC_ENABLE},
					{"IA32_FIXED_CTR1", IA32_FIXED_CTR1},
					{"IA32_FIXED_CTR_CTRL", IA32_FIXED_CTR_CTRL},
					{"IA32_PERF_GLOBAL_CTRL", IA32_PERF_GLOBAL_CTRL},
					{"MSR_PLATFORM_INFO", MSR_PLATFORM_INFO},
					{"MSR_TURBO_RATIO_LIMIT", MSR_TURBO_RATIO_LIMIT},
					{"MSR_TEMPERATURE_TARGET", MSR_TEMPERATURE_TARGET},
				},
				Axes:{{0, NULL}},
				// Design the TextCursor
				TextCursor: {	{x:+0, y:+0},
						{x:+4, y:-4},
						{x:+4, y:+4} },
				Input:{KeyBuffer:"", KeyLength:0,},
				Output:NULL,
			},
			LOOP: true,
			PAUSE: {false},
			TID_Draw: 0,
		};
	int	rc=0;

	if(Args(&A, argc, argv))
	{
		// Initialize & run the Widget.
		if(XInitThreads() && OpenDisplay(&A))
		{
			StartSplash(&A);

			bool ROOT=false;

			// Check root access.
			if(!(ROOT=(geteuid() == 0)))
				Output(&A, "Warning: root permission is denied.\n");

			// Read the CPU Features.
			Read_Features(&A.P.Features);

			// Find the Processor Architecture.
			for(A.P.ArchID=ARCHITECTURES; A.P.ArchID >=0 ; A.P.ArchID--)
				if(!(ARCH[A.P.ArchID].Signature.ExtFamily ^ A.P.Features.Std.EAX.ExtFamily)
				&& !(ARCH[A.P.ArchID].Signature.Family ^ A.P.Features.Std.EAX.Family)
				&& !(ARCH[A.P.ArchID].Signature.ExtModel ^ A.P.Features.Std.EAX.ExtModel)
				&& !(ARCH[A.P.ArchID].Signature.Model ^ A.P.Features.Std.EAX.Model))
					break;

			if(!(A.P.CPU=A.P.Features.ThreadCount))
			{
				Output(&A, "Warning: can not read the maximum number of Cores from CPUID.\n");

				if(A.P.Features.Std.EDX.HTT)
				{
					if(!(A.BIOS=(A.P.CPU=Get_ThreadCount()) != 0))
						Output(&A, "Warning: can not read the maximum number of Threads from BIOS DMI\nCheck if 'dmi' kernel module is loaded.\n");
				}
				else
				{
					if(!(A.BIOS=(A.P.CPU=Get_CoreCount()) != 0))
						Output(&A, "Warning: can not read the maximum number of Cores BIOS DMI\nCheck if 'dmi' kernel module is loaded\n");
				}
			}
			if(!A.P.CPU)
			{		// Fallback to architecture specifications.
				Output(&A, "Remark: apply a maximum number of Cores from the ");

				if(A.P.ArchID != -1)
				{
					A.P.CPU=ARCH[A.P.ArchID].MaxOfCores;
					Output(&A, ARCH[A.P.ArchID].Architecture);
				}
				else
				{
					A.P.CPU=ARCH[0].MaxOfCores;
					Output(&A, ARCH[0].Architecture);
				}
				Output(&A, "specifications.\n");
			}

			// Allocate the Cores working structure.
			pthread_mutex_init(&uDraw_mutex, NULL);

			A.P.OnLine=Create_Topology(&A);

			if(A.P.Features.HTT_enabled)
				A.P.PerCore=false;
			else
				A.P.PerCore=true;

			// Open once the MSR gate.
			if(!(A.MSR=(Open_MSR(&A) != 0)))
				Output(&A, "Warning: can not read the MSR registers\nCheck if the 'msr' kernel module is loaded.\n");

			// Read the Integrated Memory Controler information.
			A.M=IMC_Read_Info();
			if(!A.M->ChannelCount)
				Output(&A, "Warning: can not read the IMC controler.\n");

			SelectBaseClock(&A);

			StopSplash(&A);

			if(OpenWidgets(&A))
			{
				Output(&A, "Welcome to X-Freq\n");

				int G=0;
				for(G=MAIN; G < WIDGETS; G++) {
					BuildLayout(&A, G);
					MapLayout(&A, G);
					XMapWindow(A.display, A.W[G].window);
				}
				Output(&A, "Enter help to list commands.\n");

				pthread_t TID_Cycle=0;
				if(!pthread_create(&TID_Cycle, NULL, uCycle, &A)) {
					uLoop(&A);
					pthread_join(TID_Cycle, NULL);
				}
				else rc=2;

				CloseWidgets(&A);
			}
			else rc=2;

			// Release the ressources.
			IMC_Free_Info(A.M);

			Close_MSR(&A);

			Delete_Topology(&A);

			pthread_mutex_destroy(&uDraw_mutex);

			CloseDisplay(&A);
		}
		else	rc=2;
	}
	else	rc=1;
	return(rc);
}

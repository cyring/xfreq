/*
 * XFreq.c #0.16 SR4 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define _MAJOR   "0"
#define _MINOR   "16"
#define _NIGHTLY "4"
#define AutoDate "X-Freq "_MAJOR"."_MINOR"-"_NIGHTLY" (C) CYRIL INGENIERIE "__DATE__
static  char    version[] = AutoDate;

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "xfreq.h"

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

void	Output(uARG *A, const char *message)
{
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
		A->P.Platform.MinimumRatio=5;
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
	char	pathname[]="/dev/cpu/999/msr";
	int	cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++) {
		sprintf(pathname, "/dev/cpu/%d/msr", cpu);
		if( (rc=((A->P.Core[cpu].FD=open(pathname, O_RDWR)) != -1)) ) {
			// Enable the Performance Counters 1 and 2 :
			// - Set the global counter bits
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->P.Core[cpu].GlobalPerfCounter)) != -1);
			A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR1=1;
			A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR2=1;
			rc=((retval=Write_MSR(A->P.Core[cpu].FD, IA32_PERF_GLOBAL_CTRL, (GLOBAL_PERF_COUNTER *) &A->P.Core[cpu].GlobalPerfCounter)) != -1);
			// - Set the fixed counter bits
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->P.Core[cpu].FixedPerfCounter)) != -1);
			A->P.Core[cpu].FixedPerfCounter.EN1_OS=1;
			A->P.Core[cpu].FixedPerfCounter.EN2_OS=1;
			A->P.Core[cpu].FixedPerfCounter.EN1_Usr=1;
			A->P.Core[cpu].FixedPerfCounter.EN2_Usr=1;
			if(A->P.PerCore) {
				A->P.Core[cpu].FixedPerfCounter.AnyThread_EN1=1;
				A->P.Core[cpu].FixedPerfCounter.AnyThread_EN2=1;
			}
			else {
				A->P.Core[cpu].FixedPerfCounter.AnyThread_EN1=0;
				A->P.Core[cpu].FixedPerfCounter.AnyThread_EN2=0;
			}
			rc=((retval=Write_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR_CTRL, (FIXED_PERF_COUNTER *) &A->P.Core[cpu].FixedPerfCounter)) != -1);
			// Retreive the Thermal Junction Max. Fallback to 100°C if not available.
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->P.Core[cpu].TjMax)) != -1);
			if(A->P.Core[cpu].TjMax.Target == 0)
				A->P.Core[cpu].TjMax.Target=100;
			Read_MSR(A->P.Core[cpu].FD, IA32_THERM_INTERRUPT, (THERM_INTERRUPT *) &A->P.Core[cpu].ThermIntr);
		}
		else	// Fallback to an arbitrary & commom value.
		{
			A->P.Core[cpu].TjMax.Target=100;
		}
	}
	return(rc);
}

// Close all MSR handles.
void	Close_MSR(uARG *A)
{
	int	cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++) {
		// Reset the fixed counters.
		A->P.Core[cpu].FixedPerfCounter.EN1_Usr=0;
		A->P.Core[cpu].FixedPerfCounter.EN2_Usr=0;
		A->P.Core[cpu].FixedPerfCounter.EN1_OS=0;
		A->P.Core[cpu].FixedPerfCounter.EN2_OS=0;
		A->P.Core[cpu].FixedPerfCounter.AnyThread_EN1=0;
		A->P.Core[cpu].FixedPerfCounter.AnyThread_EN2=0;
		Write_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR_CTRL, &A->P.Core[cpu].FixedPerfCounter);
		// Reset the global counters.
		A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR1=0;
		A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR2=0;
		Write_MSR(A->P.Core[cpu].FD, IA32_PERF_GLOBAL_CTRL, &A->P.Core[cpu].GlobalPerfCounter);
		// Release the MSR handle associated to the Core.
		if(A->P.Core[cpu].FD != -1)
			close(A->P.Core[cpu].FD);
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

// The Processor thread which updates the Core values.
static void *uCycle(void *uArg)
{
	uARG *A=(uARG *) uArg;

	// unsigned long tsc=RDTSC();
	unsigned int cpu=0;
	for(cpu=0; cpu < A->P.CPU; cpu++) {
		// Initial read of the Unhalted Core & Reference Cycles.
		Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR1, (unsigned long long *) &A->P.Core[cpu].RefCycles.C0[0].UCC);
		Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR2, (unsigned long long *) &A->P.Core[cpu].RefCycles.C0[0].URC );
		// Initial read of other C-States.
		Read_MSR(A->P.Core[cpu].FD, MSR_CORE_C3_RESIDENCY, (unsigned long long *) &A->P.Core[cpu].RefCycles.C3[0]);
		Read_MSR(A->P.Core[cpu].FD, MSR_CORE_C6_RESIDENCY, (unsigned long long *) &A->P.Core[cpu].RefCycles.C6[0]);
		// Initial read of the TSC in relation to the Logical Core.
		Read_MSR(A->P.Core[cpu].FD, IA32_TIME_STAMP_COUNTER, (unsigned long long *) &A->P.Core[cpu].RefCycles.TSC[0]);
		// A->P.Core[cpu].RefCycles.TSC[0]=tsc;
	}

	while(A->LOOP) {
		// Settle down some microseconds as specified by the command argument.
		usleep(A->P.IdleTime);

/* CRITICAL_IN  */
		// tsc=RDTSC();
		for(cpu=0; cpu < A->P.CPU; cpu++) {
			// Update the Base Operating Ratio.
			Read_MSR(A->P.Core[cpu].FD, IA32_PERF_STATUS, (PERF_STATUS *) &A->P.Core[cpu].Operating);
			// Update the Unhalted Core & the Reference Cycles.
			Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR1, (unsigned long long *) &A->P.Core[cpu].RefCycles.C0[1].UCC);
			Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR2, (unsigned long long *) &A->P.Core[cpu].RefCycles.C0[1].URC);
			// Update C-States.
			Read_MSR(A->P.Core[cpu].FD, MSR_CORE_C3_RESIDENCY, (unsigned long long *) &A->P.Core[cpu].RefCycles.C3[1]);
			Read_MSR(A->P.Core[cpu].FD, MSR_CORE_C6_RESIDENCY, (unsigned long long *) &A->P.Core[cpu].RefCycles.C6[1]);
			// Update TSC.
			Read_MSR(A->P.Core[cpu].FD, IA32_TIME_STAMP_COUNTER, (unsigned long long *) &A->P.Core[cpu].RefCycles.TSC[1]);
			// A->P.Core[cpu].RefCycles.TSC[1]=tsc;
		}
/* CRITICAL_OUT */

		// Reset C-States average.
		A->P.Avg.C0=A->P.Avg.C3=A->P.Avg.C6=0;

		unsigned int maxFreq=0, maxTemp=A->P.Core[0].TjMax.Target;
		for(cpu=0; cpu < A->P.CPU; cpu++) {
			// Compute the Operating Frequency.
			A->P.Core[cpu].OperatingFreq=A->P.Core[cpu].Operating.Ratio * A->P.ClockSpeed;

			// Compute the Delta of Unhalted (Core & Ref) C0 Cycles = Current[1] - Previous[0]
			A->P.Core[cpu].Delta.C0.UCC=A->P.Core[cpu].RefCycles.C0[1].UCC - A->P.Core[cpu].RefCycles.C0[0].UCC;
			A->P.Core[cpu].Delta.C0.URC=A->P.Core[cpu].RefCycles.C0[1].URC - A->P.Core[cpu].RefCycles.C0[0].URC;
			A->P.Core[cpu].Delta.C3=A->P.Core[cpu].RefCycles.C3[1] - A->P.Core[cpu].RefCycles.C3[0];
			A->P.Core[cpu].Delta.C6=A->P.Core[cpu].RefCycles.C6[1] - A->P.Core[cpu].RefCycles.C6[0];
			A->P.Core[cpu].Delta.TSC=A->P.Core[cpu].RefCycles.TSC[1] - A->P.Core[cpu].RefCycles.TSC[0];

			// Compute the Current Core Ratio per Cycles Delta. (Protect against a division by zero with the Operating value)
			A->P.Core[cpu].UnhaltedRatio=	(A->P.Core[cpu].Delta.C0.URC != 0) ?
							(A->P.Core[cpu].Operating.Ratio * A->P.Core[cpu].Delta.C0.UCC) / A->P.Core[cpu].Delta.C0.URC
							: A->P.Core[cpu].Operating.Ratio;

			// Dynamic Frequency = Unhalted Ratio x Bus Clock Frequency
			A->P.Core[cpu].UnhaltedFreq=A->P.Core[cpu].UnhaltedRatio * A->P.ClockSpeed;

			// Compute the Relative Core Ratio based on the Unhalted States divided by the TSC.
			A->P.Core[cpu].RelativeRatio=(A->P.Core[cpu].Operating.Ratio * A->P.Core[cpu].Delta.C0.UCC) / A->P.Core[cpu].Delta.TSC;

			// Relative Frequency = Relative Ratio x Bus Clock Frequency
			A->P.Core[cpu].RelativeFreq=A->P.Core[cpu].RelativeRatio * A->P.ClockSpeed;

			// Compute C-States.
			A->P.Core[cpu].State.C0=(double) (A->P.Core[cpu].Delta.C0.URC) / (double) (A->P.Core[cpu].Delta.TSC);
			A->P.Core[cpu].State.C3=(double) (A->P.Core[cpu].Delta.C3)  / (double) (A->P.Core[cpu].Delta.TSC);
			A->P.Core[cpu].State.C6=(double) (A->P.Core[cpu].Delta.C6)  / (double) (A->P.Core[cpu].Delta.TSC);

			// Save TSC.
			A->P.Core[cpu].RefCycles.TSC[0]=A->P.Core[cpu].RefCycles.TSC[1];
			// Save the Unhalted Core & Reference Cycles for next iteration.
			A->P.Core[cpu].RefCycles.C0[0].UCC=A->P.Core[cpu].RefCycles.C0[1].UCC;
			A->P.Core[cpu].RefCycles.C0[0].URC =A->P.Core[cpu].RefCycles.C0[1].URC;
			// Save also the C-State Reference Cycles.
			A->P.Core[cpu].RefCycles.C3[0]=A->P.Core[cpu].RefCycles.C3[1];
			A->P.Core[cpu].RefCycles.C6[0]=A->P.Core[cpu].RefCycles.C6[1];

			// Sum the C-States before the average.
			A->P.Avg.C0+=A->P.Core[cpu].State.C0;
			A->P.Avg.C3+=A->P.Core[cpu].State.C3;
			A->P.Avg.C6+=A->P.Core[cpu].State.C6;

			// Index the Top CPU speed.
			if(maxFreq < A->P.Core[cpu].UnhaltedFreq) {
				maxFreq=A->P.Core[cpu].UnhaltedFreq;
				A->P.Top=cpu;
			}

			// Update the Digital Thermal Sensor.
			if( (Read_MSR(A->P.Core[cpu].FD, IA32_THERM_STATUS, (THERM_STATUS *) &A->P.Core[cpu].ThermStat)) == -1)
				A->P.Core[cpu].ThermStat.DTS=0;

			// Index the Hotest Core.
			if(A->P.Core[cpu].ThermStat.DTS < maxTemp) {
				maxTemp=A->P.Core[cpu].ThermStat.DTS;
				A->P.Hot=cpu;
			}
		}
		// Average the C-States.
		A->P.Avg.C0/=A->P.CPU;
		A->P.Avg.C3/=A->P.CPU;
		A->P.Avg.C6/=A->P.CPU;

		// Fire the drawing thread.
		if(pthread_mutex_trylock(&uDraw_mutex) == 0)
			pthread_create(&A->TID_Draw, NULL, uDraw, A);
	}
	// Drawing is still processing ?
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

// Old fashion style to compute the processor frequency based on TSC.
unsigned long long int FallBack_Freq()
{
	struct timezone tz;
	struct timeval tvstart, tvstop;
	unsigned long long int cycles[2];
	unsigned int microseconds;

	memset(&tz, 0, sizeof(tz));

	gettimeofday(&tvstart, &tz);
	cycles[0] = RDTSC();
	gettimeofday(&tvstart, &tz);

	usleep(10000);

	cycles[1] = RDTSC();
	gettimeofday(&tvstop, &tz);
	microseconds = ( (tvstop.tv_sec - tvstart.tv_sec) * 10000) + (tvstop.tv_usec - tvstart.tv_usec);

	return( (cycles[1] - cycles[0]) / microseconds );
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
void	CPUID(FEATURES *features)
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
			: "=c"	(features->Ext.ECX),
			  "=d"	(features->Ext.EDX)
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

// Draw Scrolling button.
void	DrawScrollingButton(uARG *A, int G, int x, int y, int w, int h, char direction)
{
	unsigned int inner=1;
	XPoint Arrow[3];

	switch(direction) {
		case NORTH_DIR: {
			unsigned int valign=h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=x + inner;	Arrow[0].y=y + h - inner;
			Arrow[1].x= +halign;	Arrow[1].y= -valign;
			Arrow[2].x= +halign;	Arrow[2].y= +valign;
		}
			break;
		case SOUTH_DIR: {
			unsigned int valign=h - (inner << 1);
			unsigned int halign=valign >> 1;
			Arrow[0].x=x + inner;	Arrow[0].y=y + inner;
			Arrow[1].x= +halign;	Arrow[1].y= +valign;
			Arrow[2].x= +halign;	Arrow[2].y= -valign;
		}
			break;
		case EAST_DIR: {
			unsigned int halign=w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=x + inner;	Arrow[0].y=y + inner;
			Arrow[1].x= +halign;	Arrow[1].y= +valign;
			Arrow[2].x= -halign;	Arrow[2].y= +valign;
		}
			break;
		case WEST_DIR: {
			unsigned int halign=w - (inner << 1);
			unsigned int valign=halign >> 1;
			Arrow[0].x=x + w - inner;	Arrow[0].y=y + inner;
			Arrow[1].x= -halign;	Arrow[1].y= +valign;
			Arrow[2].x= +halign;	Arrow[2].y= +valign;
		}
			break;
	}
	XFillPolygon(A->display, A->W[G].pixmap.B, A->W[G].gc, Arrow, 3, Nonconvex, CoordModePrevious);
}

// CallBack functions definition.
void	ScrollingCallBack(uARG *A, int G, char direction) ;

// Create a button with callback, drawing & collision functions.
void	CreateButton(uARG *A, int G, int x, int y, unsigned int w, unsigned h, void *DrawFunc, void *CallBack, unsigned long parameter)
{
	struct WButton *NewButton=malloc(sizeof(struct WButton));

	NewButton->x=x;
	NewButton->y=y;
	NewButton->w=w;
	NewButton->h=h;
	NewButton->DrawFunc=DrawFunc;
	NewButton->CallBack=CallBack;
	NewButton->Parameter=parameter;

	NewButton->Chain=NULL;
	if(A->W[G].wButton[HEAD] == NULL)
		A->W[G].wButton[HEAD]=NewButton;
	else
		A->W[G].wButton[TAIL]->Chain=NewButton;
	A->W[G].wButton[TAIL]=NewButton;
}

// Destroy all buttons attached to a Widget.
void	DestroyAllButton(uARG *A, int G)
{
	struct WButton *wButton=A->W[G].wButton[HEAD], *cButton=NULL;
	while(wButton != NULL) {
		cButton=wButton->Chain;
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

	struct WButton *wButton=NULL;
	for(wButton=A->W[T].wButton[HEAD]; wButton != NULL; wButton=wButton->Chain)
		if((x > wButton->x + xOffset)
		&& (y > wButton->y + yOffset)
		&& (x < wButton->x + xOffset + wButton->w)
		&& (y < wButton->y + yOffset + wButton->h)) {
			wButton->CallBack(A, T, wButton->Parameter);
			break;
		}
}

// Draw the Cursor shape,
void	DrawCursor(uARG *A, int G, XPoint *Origin)
{
	A->L.Cursor[0].x=Origin->x;
	A->L.Cursor[0].y=Origin->y;
	XFillPolygon(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Cursor, 3, Nonconvex, CoordModePrevious);
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
		Text.cols=(cols > Text.cols) ? cols : Text.cols;
		Text.rows++ ;
		pStartLine=pNewLine+1;
	}
	return(Text);
}

// Scale the MDI window.
void	ScaleMDI(uARG *A)
{
	int G=0, RightMost=0, BottomMost=0;
	for(G=FIRST_WIDGET; G <= LAST_WIDGET; G++) {
		if((A->W[RightMost].x + A->W[RightMost].width) < (A->W[G].x + A->W[G].width))
			RightMost=G;
		if((A->W[BottomMost].y + A->W[BottomMost].height) < (A->W[G].y + A->W[G].height))
			BottomMost=G;
	}
	A->W[MAIN].width=A->W[RightMost].x + A->W[RightMost].width + A->L.Margin.H;
	A->W[MAIN].height=A->W[BottomMost].y + A->W[BottomMost].height + A->L.Margin.V + Footer_Height;
	// Adjust the Header & Footer axes with the new width.
	int i=0;
	for(i=0; i < A->L.Axes[MAIN].N; i++)
		A->L.Axes[MAIN].Segment[i].x2=A->W[MAIN].width;
}

// ReSize a Widget window & inform WM.
void	ReSizeWidget(uARG *A, int G)
{
	XSizeHints *hints=NULL;
	if((hints=XAllocSizeHints()) != NULL) {
		hints->min_width= hints->max_width= A->W[G].width;
		hints->min_height=hints->max_height=A->W[G].height;
		hints->flags=PMinSize|PMaxSize;
		XSetWMNormalHints(A->display, A->W[G].window, hints);
		XFree(hints);
	}
	XWindowAttributes xwa={0};
	XGetWindowAttributes(A->display, A->W[G].window, &xwa);
	if((xwa.width != A->W[G].width) || (xwa.height != A->W[G].height))
		XResizeWindow(A->display, A->W[G].window, A->W[G].width, A->W[G].height);
}

// Move a Widget Window (when the MDI mode is enabled).
void	MoveWidget(uARG *A, XEvent *E)
{
	switch(E->type) {
		case ButtonPress:
			if(!A->T.S) {
				for(A->T.S=LAST_WIDGET; A->T.S >= FIRST_WIDGET; A->T.S--)
					if( E->xbutton.subwindow == A->W[A->T.S].window)
						break;
				A->T.dx=E->xbutton.x - A->W[A->T.S].x;
				A->T.dy=E->xbutton.y - A->W[A->T.S].y;
			}
			break;
		case ButtonRelease:
			if(A->T.S) {
				A->W[A->T.S].x=E->xbutton.x - A->T.dx;
				A->W[A->T.S].y=E->xbutton.y - A->T.dy;
				A->T.S=0;
			}
			break;
		case MotionNotify:
			if(A->T.S)
				XMoveWindow(A->display, A->W[A->T.S].window, E->xmotion.x - A->T.dx, E->xmotion.y - A->T.dy);
			break;
	}
}

// Create the X-Window Widget.
int	OpenWidgets(uARG *A)
{
	int noerr=true;
	char str[sizeof(HDSIZE)];

	// Allocate memory for chart elements.
	A->L.Usage.C0=malloc(A->P.CPU * sizeof(XRectangle));
	A->L.Usage.C3=malloc(A->P.CPU * sizeof(XRectangle));
	A->L.Usage.C6=malloc(A->P.CPU * sizeof(XRectangle));

	if((A->display=XOpenDisplay(NULL)) && (A->screen=DefaultScreenOfDisplay(A->display)) )
		{
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
	/* Cursor: cursor to be displayed (or None) */		cursor:None};

		// Try to load the requested font.
		if(strlen(A->fontName) == 0)
			strcpy(A->fontName, "Fixed");

		if((A->xfont=XLoadQueryFont(A->display, A->fontName)) == NULL)
			noerr=false;

		int G=0;
		for(G=0; noerr && (G < WIDGETS); G++) {
			// Dispose Widgets from each other : [Right & Bottom + width & height] Or -[1,-1] = X,Y origin + Margins.
			int U=A->W[G].x;
			int V=A->W[G].y;
			A->W[G].x=(U == -1) ? A->W[MAIN].x + A->L.Margin.H : A->W[U].x + A->W[U].width + A->L.Margin.H;
			A->W[G].y=(V == -1) ? A->W[MAIN].y + A->L.Margin.V : A->W[V].y + A->W[V].height + A->L.Margin.V;
			// Override default colors if values supplied in the command line.
			if(A->L.globalBackground != GLOBAL_BACKGROUND)
				A->W[G].background=A->L.globalBackground;
			if(A->L.globalForeground != GLOBAL_FOREGROUND)
				A->W[G].foreground=A->L.globalForeground;
			// Define the Widgets.
			if((A->W[G].window=XCreateWindow(A->display,
							_IS_MDI_ && (G != MAIN) ?
							A->W[MAIN].window
							: DefaultRootWindow(A->display),
							A->W[G].x, A->W[G].y, A->W[G].width, A->W[G].height,
							_IS_MDI_ ? 1 : 0,
							CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &swa)) )
				{
				if((A->W[G].gc=XCreateGC(A->display, A->W[G].window, 0, NULL)))
					{
					XSetFont(A->display, A->W[G].gc, A->xfont->fid);

					switch(G) {
						// Compute Widgets scaling.
						case MAIN: {
							XTextExtents(	A->xfont, HDSIZE, MAIN_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
											+ A->W[G].extents.descent;
							A->W[G].width=	A->W[G].extents.overall.width;
							A->W[G].height=	Half_Char_Width + One_Char_Height * MAIN_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;

							// First run : adjust the global margins with the font size. Don't overlap axes.
							A->L.Margin.H=Twice_Char_Width;
							A->L.Margin.V=One_Char_Height + Quarter_Char_Height;
							A->W[G].height+=Footer_Height;

							// First run : if MAIN defined as the MDI then reset its position.
							if(_IS_MDI_) {
								A->W[G].x=0;
								A->W[G].y=0;
							}
							else {
								char direction[4]={NORTH_DIR, SOUTH_DIR, EAST_DIR, WEST_DIR};
								unsigned int square=MAX(One_Char_Height, One_Char_Width);

								CreateButton(A, G,	A->W[G].width - (square + 2),
											Header_Height + 2,
											square,
											square,
											DrawScrollingButton,
											ScrollingCallBack,
											direction[0]);
								CreateButton(A, G,	A->W[G].width - (square + 2),
											A->W[G].height - (Footer_Height + square + 2),
											square,
											square,
											DrawScrollingButton,
											ScrollingCallBack,
											direction[1]);
								CreateButton(A, G,	A->W[G].width
											- (MAX(Twice_Char_Height,Twice_Char_Width) + 2),
											A->W[G].height - (square + 2),
											square,
											square,
											DrawScrollingButton,
											ScrollingCallBack,
											direction[2]);
								CreateButton(A, G,	A->W[G].width
											- (MAX( 3*One_Char_Height+Quarter_Char_Height,
												3 * One_Char_Width+Quarter_Char_Width)
											+ 2),
											A->W[G].height - (square + 2),
											square,
											square,
											DrawScrollingButton,
											ScrollingCallBack,
											direction[3]);
							}
						}
							break;
						case CORES: {
							XTextExtents(	A->xfont, HDSIZE, CORES_TEXT_WIDTH << 1,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										 + A->W[G].extents.descent;
							A->W[G].width=	A->W[G].extents.overall.width
									+ (One_Char_Width << 2)
									+ Half_Char_Width;
							A->W[G].height=	Half_Char_Width + One_Char_Height * (CORES_TEXT_HEIGHT + 2);

							// Prepare the chart axes.
							// With footer: A->L.Axes[G].N=CORES_TEXT_WIDTH + 2;
							A->L.Axes[G].N=CORES_TEXT_WIDTH + 1;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int	i=0,
								j=A->W[G].extents.overall.width / CORES_TEXT_WIDTH;
							for(i=0; i <= CORES_TEXT_WIDTH; i++) {
								A->L.Axes[G].Segment[i].x1=(j * i) + (One_Char_Width * 3);
								A->L.Axes[G].Segment[i].y1=3 + One_Char_Height;
								A->L.Axes[G].Segment[i].x2=A->L.Axes[G].Segment[i].x1;
								A->L.Axes[G].Segment[i].y2=((CORES_TEXT_HEIGHT + 1) * One_Char_Height) - 3;
							}
							/* With footer.
							A->L.Axes[G].Segment[i].x1=0;
							A->L.Axes[G].Segment[i].y1=A->W[G].height;
							A->L.Axes[G].Segment[i].x2=A->W[G].width;
							A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							A->W[G].height+=Footer_Height; */
						}
							break;
						case CSTATES: {
							XTextExtents(	A->xfont, HDSIZE, CSTATES_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
											+ A->W[G].extents.descent;
							A->W[G].width=	Quarter_Char_Width
									+ Twice_Half_Char_Width
									+ (CSTATES_TEXT_WIDTH * One_Half_Char_Width);
							A->W[G].height=	Twice_Half_Char_Height + (One_Char_Height * CSTATES_TEXT_HEIGHT);

							// Prepare the chart axes.
							// With footer: A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 2;
							A->L.Axes[G].N=CSTATES_TEXT_HEIGHT + 1;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int i=0;
							for(i=0; i < CSTATES_TEXT_HEIGHT; i++) {
								A->L.Axes[G].Segment[i].x1=0;
								A->L.Axes[G].Segment[i].y1=(i + 1) * One_Char_Height;
								A->L.Axes[G].Segment[i].x2=A->W[G].width;
								A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							}
							// Percent.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1=A->W[G].width
												- One_Char_Width - Quarter_Char_Width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y1=One_Char_Height;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x2=A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y2=One_Char_Height
												+ (One_Char_Height * CSTATES_TEXT_HEIGHT);
							/* With footer.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x1=0;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1=A->W[G].height;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x2=A->W[G].width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y2=A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1;
							A->W[G].height+=Footer_Height; */
						}
							break;
						case TEMPS: {
							XTextExtents(	A->xfont, HDSIZE, TEMPS_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
											+ A->W[G].extents.descent;
							A->W[G].width= A->W[G].extents.overall.width + (One_Char_Width * 5);
							A->W[G].height=Half_Char_Width	+ One_Char_Height * (TEMPS_TEXT_HEIGHT + 1 + 1);

							// Prepare the chart axes.
							// With footer: A->L.Axes[G].N=TEMPS_TEXT_WIDTH + 2;
							A->L.Axes[G].N=TEMPS_TEXT_WIDTH + 1;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							int i=0;
							for(i=0; i < TEMPS_TEXT_WIDTH; i++) {
								A->L.Axes[G].Segment[i].x1=(i + 3) * One_Char_Width;
								A->L.Axes[G].Segment[i].y1=(TEMPS_TEXT_HEIGHT + 1) * One_Char_Height;
								A->L.Axes[G].Segment[i].x2=A->L.Axes[G].Segment[i].x1 + One_Char_Width;
								A->L.Axes[G].Segment[i].y2=A->L.Axes[G].Segment[i].y1;
							}
							// Temps.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x1=One_Half_Char_Width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y1=One_Char_Height;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].x2=One_Half_Char_Width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 2].y2=One_Char_Height
												+ (One_Char_Height * TEMPS_TEXT_HEIGHT);
							/* With footer.
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x1=0;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1=A->W[G].height;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].x2=A->W[G].width;
							A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y2=A->L.Axes[G].Segment[A->L.Axes[G].N - 1].y1;
							A->W[G].height+=Footer_Height; */
						}
							break;
						case SYSINFO: {
							XTextExtents(	A->xfont, HDSIZE, SYSINFO_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
											+ A->W[G].extents.descent;
							A->W[G].width=A->W[G].extents.overall.width;
							A->W[G].height=Half_Char_Width + One_Char_Height * SYSINFO_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
							A->W[G].height+=Footer_Height;

							char direction[4]={NORTH_DIR, SOUTH_DIR, EAST_DIR, WEST_DIR};
							unsigned int square=MAX(One_Char_Height, One_Char_Width);

							CreateButton(A, G,	A->W[G].width - (square + 2),
										Header_Height + 2,
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[0]);
							CreateButton(A, G,	A->W[G].width - (square + 2),
										A->W[G].height - (Footer_Height + square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[1]);
							CreateButton(A, G,	A->W[G].width
										- (MAX(Twice_Char_Height,Twice_Char_Width) + 2),
										A->W[G].height - (square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[2]);
							CreateButton(A, G,	2,
										A->W[G].height - (square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[3]);
						}
							break;
						case DUMP: {
							XTextExtents(	A->xfont, HDSIZE, DUMP_TEXT_WIDTH,
									&A->W[G].extents.dir, &A->W[G].extents.ascent,
									&A->W[G].extents.descent, &A->W[G].extents.overall);

							A->W[G].extents.charWidth=A->xfont->max_bounds.rbearing
										- A->xfont->min_bounds.lbearing;
							A->W[G].extents.charHeight=A->W[G].extents.ascent
										+ A->W[G].extents.descent;
							A->W[G].width= A->W[G].extents.overall.width + Half_Char_Width;
							A->W[G].height=Half_Char_Width + One_Char_Height * DUMP_TEXT_HEIGHT;

							// Prepare the chart axes.
							A->L.Axes[G].N=2;
							A->L.Axes[G].Segment=malloc(A->L.Axes[G].N * sizeof(XSegment));
							// Header.
							A->L.Axes[G].Segment[0].x1=0;
							A->L.Axes[G].Segment[0].y1=Header_Height;
							A->L.Axes[G].Segment[0].x2=A->W[G].width;
							A->L.Axes[G].Segment[0].y2=A->L.Axes[G].Segment[0].y1;
							// Footer.
							A->L.Axes[G].Segment[1].x1=A->L.Axes[G].Segment[0].x1;
							A->L.Axes[G].Segment[1].y1=A->W[G].height;
							A->L.Axes[G].Segment[1].x2=A->L.Axes[G].Segment[0].x2;
							A->L.Axes[G].Segment[1].y2=A->L.Axes[G].Segment[1].y1;
							A->W[G].height+=Footer_Height;

							char direction[4]={NORTH_DIR, SOUTH_DIR, EAST_DIR, WEST_DIR};
							unsigned int square=MAX(One_Char_Height, One_Char_Width);

							CreateButton(A, G,	A->W[G].width - (square + 2),
										Header_Height + 2,
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[0]);
							CreateButton(A, G,	A->W[G].width - (square + 2),
										A->W[G].height - (Footer_Height + square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[1]);
							CreateButton(A, G,	A->W[G].width
										- (MAX(Twice_Char_Height,Twice_Char_Width) + 2),
										A->W[G].height - (square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[2]);
							CreateButton(A, G,	2,
										A->W[G].height - (square + 2),
										square,
										square,
										DrawScrollingButton,
										ScrollingCallBack,
										direction[3]);
						}
							break;
					}
					ReSizeWidget(A, G);
					XSetWindowBorder(A->display, A->W[G].window, LABEL_COLOR);
				}
				else	noerr=false;
			}
			else	noerr=false;
		}
		if(noerr && _IS_MDI_) {
			ScaleMDI(A);
			ReSizeWidget(A, MAIN);

			char direction[4]={NORTH_DIR, SOUTH_DIR, EAST_DIR, WEST_DIR};
			unsigned int square=MAX(A->W[MAIN].extents.charHeight,A->W[MAIN].extents.charWidth);

			CreateButton(A, MAIN,	A->W[MAIN].width - (square + 2),
						Header_Height + 2,
						square,
						square,
						DrawScrollingButton,
						ScrollingCallBack,
						direction[0]);
			CreateButton(A, MAIN,	A->W[MAIN].width - (square + 2),
						A->L.Axes[MAIN].Segment[1].y2 - (square + 2),
						square,
						square,
						DrawScrollingButton,
						ScrollingCallBack,
						direction[1]);
			CreateButton(A, MAIN,	A->W[MAIN].width
						- (MAX((A->W[MAIN].extents.charHeight << 1),(A->W[MAIN].extents.charWidth << 1)) + 2),
						A->L.Axes[MAIN].Segment[1].y2 + 2,
						square,
						square,
						DrawScrollingButton,
						ScrollingCallBack,
						direction[2]);
			CreateButton(A, MAIN,	A->W[MAIN].width
						- (MAX(	3 * A->W[MAIN].extents.charHeight+(A->W[MAIN].extents.charHeight >> 2),
							3 * A->W[MAIN].extents.charWidth+(A->W[MAIN].extents.charWidth >> 2))
						+ 2),
						A->L.Axes[MAIN].Segment[1].y2 + 2,
						square,
						square,
						DrawScrollingButton,
						ScrollingCallBack,
						direction[3]);
		}
		for(G=0; noerr && (G < WIDGETS); G++)
			if((A->W[G].pixmap.B=XCreatePixmap(A->display, A->W[G].window,
								A->W[G].width, A->W[G].height,
								DefaultDepthOfScreen(A->screen)))
			&& (A->W[G].pixmap.F=XCreatePixmap(A->display, A->W[G].window,
								A->W[G].width, A->W[G].height,
								DefaultDepthOfScreen(A->screen))) )
			{
				long EventProfile=BASE_EVENTS;
				if(_IS_MDI_) {
					if(G == MAIN)
						EventProfile=EventProfile | CLICK_EVENTS | MOVE_EVENTS;
				}
				else	EventProfile=EventProfile | CLICK_EVENTS;

				XSelectInput(A->display, A->W[G].window, EventProfile);
			}
			else	noerr=false;
	}
	else	noerr=false;

	// Prepare a Wallboard string with the Processor information.
	sprintf(str, OVERCLOCK, A->P.Features.BrandString, A->P.Platform.MaxNonTurboRatio * A->P.ClockSpeed);
	A->L.WB.Length=strlen(str) + (A->P.Platform.MaxNonTurboRatio << 2);
	A->L.WB.String=calloc(A->L.WB.Length, 1);
	memset(A->L.WB.String, 0x20, A->L.WB.Length);
	memcpy(&A->L.WB.String[A->P.Platform.MaxNonTurboRatio << 1], str, strlen(str));
	A->L.WB.Scroll=(A->P.Platform.MaxNonTurboRatio << 1);
	A->L.WB.Length=strlen(str) + A->L.WB.Scroll;

	// Store some Ratios into a string for future chart drawing.
	sprintf(A->P.Bump, "%02d%02d%02d",	A->P.Platform.MinimumRatio,
						A->P.Platform.MaxNonTurboRatio,
						A->P.Turbo.MaxRatio_1C);
	return(noerr);
}

// Center the layout of the current page.
void	CenterLayout(uARG *A, int G) {
	A->L.Page[G].hScroll=1;
	A->L.Page[G].vScroll=1;
}

// Display & scroll content into the specified clip rectangle.
void	ScrollLayout(uARG *A, int G, char *items, int spacing, XRectangle *R)
{
	XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
	XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
			One_Char_Width,
			One_Char_Height,
			A->L.Page[G].title,
			strlen(A->L.Page[G].title) );

	XSetClipRectangles(A->display, A->W[G].gc,
			0,
			0,
			R, 1, Unsorted);

	XSetForeground(A->display, A->W[G].gc, PRINT_COLOR);
	A->L.Page[G].Text=XPrint(A->display, A->W[G].pixmap.B, A->W[G].gc,
				One_Char_Width * A->L.Page[G].hScroll,
				+ One_Half_Char_Height
				+ (One_Char_Height * A->L.Page[G].vScroll),
				items,
				spacing);
	XSetClipMask(A->display, A->W[G].gc, None);
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
		wButton->DrawFunc(A, G, wButton->x, wButton->y, wButton->w, wButton->h, wButton->Parameter);

	switch(G) {
		case MAIN:
		{
			if(_IS_MDI_) {
				XSetForeground(A->display, A->W[G].gc, MDI_SEP_COLOR);
				XDrawLine(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->L.Axes[G].Segment[1].x1,
					A->L.Axes[G].Segment[1].y2 + Footer_Height,
					A->L.Axes[G].Segment[1].x2,
					A->L.Axes[G].Segment[1].y2 + Footer_Height);

			}
			if(A->L.Output) {
				XRectangle R[]=	{ {
						x:0,
						y:Header_Height,
						width:A->W[G].width,
						height:(One_Char_Height * MAIN_TEXT_HEIGHT) - Footer_Height,
						} };
				ScrollLayout(A, G, A->L.Output, One_Char_Height+Quarter_Char_Height, R);
			}
		}
			break;
		case CORES: {
			char str[sizeof(CORE_NUM)];

			// Draw the Core identifiers, the header, and the footer on the chart.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Half_Char_Width,
						( One_Char_Height * (cpu + 1 + 1) ),
						str, strlen(str) );
			}
			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width,
					One_Char_Height,
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width,
					One_Char_Height * (CORES_TEXT_HEIGHT + 1 + 1),
					"Ratio", 5);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width * ((5 + 1) * 2),
					One_Char_Height * (CORES_TEXT_HEIGHT + 1 + 1),
					"5", 1);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width * ((A->P.Platform.MinimumRatio + 1) * 2),
					One_Char_Height * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Bump[0], 2);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width * ((A->P.Platform.MaxNonTurboRatio + 1) * 2),
					One_Char_Height * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Bump[2], 2);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					One_Char_Width * ((A->P.Turbo.MaxRatio_1C + 1) * 2),
					One_Char_Height * (CORES_TEXT_HEIGHT + 1 + 1),
					&A->P.Bump[4], 2);
		}
			break;
		case CSTATES:
		{
			char str[sizeof(CORE_NUM)];

			XSetForeground(A->display, A->W[G].gc, LABEL_COLOR);
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - One_Half_Char_Width - Quarter_Char_Width,
					One_Char_Height,
					"%", 1 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width - Quarter_Char_Width,
					One_Char_Height
					+ (10 * (One_Char_Height * CSTATES_TEXT_HEIGHT)) / 100,
					"90", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width - Quarter_Char_Width,
					One_Char_Height
					+ (30 * (One_Char_Height * CSTATES_TEXT_HEIGHT)) / 100,
					"70", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width - Quarter_Char_Width,
					One_Char_Height
					+ (50 * (One_Char_Height * CSTATES_TEXT_HEIGHT)) / 100,
					"50", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width - Quarter_Char_Width,
					One_Char_Height
					+ (70 * (One_Char_Height * CSTATES_TEXT_HEIGHT)) / 100,
					"30", 2 );
			XDrawImageString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					A->W[G].width - Twice_Char_Width - Quarter_Char_Width,
					One_Char_Height
					+ (90 * (One_Char_Height * CSTATES_TEXT_HEIGHT)) / 100,
					"10", 2 );
			XDrawString(A->display, A->W[G].pixmap.B, A->W[G].gc,
					One_Char_Width,
					One_Char_Height + (One_Char_Height * (CSTATES_TEXT_HEIGHT + 1)),
					"~", 1 );

			// Draw the Core identifiers.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						Twice_Char_Width + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width),
						One_Char_Height,
						str, strlen(str) );
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
					Quarter_Char_Width,
					One_Char_Height,
					"Core", 4);
			XDrawString(	A->display,
					A->W[G].pixmap.B,
					A->W[G].gc,
					Quarter_Char_Width,
					One_Char_Height * (TEMPS_TEXT_HEIGHT + 1 + 1),
					"Temps", 5);
			// Draw the Core identifiers.
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				sprintf(str, CORE_NUM, cpu);
				XDrawString(	A->display, A->W[G].pixmap.B, A->W[G].gc,
						(One_Char_Width * 5)
						+ Half_Char_Width
						+ (cpu << 1) * Twice_Char_Width,
						One_Char_Height,
						str, strlen(str) );
			}
		}
			break;
		case SYSINFO:
		{
			char items[8192]={0}, str[SYSINFO_TEXT_WIDTH]={0};

			const char	powered[2]={'N', 'Y'},
					*enabled[2]={"OFF", "ON"};
			sprintf(items, PROC_FORMAT,
					A->P.Features.BrandString,
					ARCH[A->P.ArchID].Architecture,
					A->P.Features.Std.EAX.ExtFamily + A->P.Features.Std.EAX.Family,
					(A->P.Features.Std.EAX.ExtModel << 4) + A->P.Features.Std.EAX.Model,
					A->P.Features.Std.EAX.Stepping,
					A->P.Features.ThreadCount,
					powered[A->P.Features.Std.EDX.VME],
					powered[A->P.Features.Std.EDX.DE],
					powered[A->P.Features.Std.EDX.PSE],
					powered[A->P.Features.Std.EDX.TSC],
					powered[A->P.Features.Std.EDX.MSR],
					powered[A->P.Features.Std.EDX.PAE],
					powered[A->P.Features.Std.EDX.APIC],
					powered[A->P.Features.Std.EDX.MTRR],
					powered[A->P.Features.Std.EDX.PGE],
					powered[A->P.Features.Std.EDX.MCA],
					powered[A->P.Features.Std.EDX.PAT],
					powered[A->P.Features.Std.EDX.PSE36],
					powered[A->P.Features.Std.EDX.PSN],
					powered[A->P.Features.Std.EDX.DS],
					powered[A->P.Features.Std.EDX.ACPI],
					powered[A->P.Features.Std.EDX.SS],
					powered[A->P.Features.Std.EDX.HTT],
					powered[A->P.Features.Std.EDX.TM1],
					powered[A->P.Features.Std.ECX.TM2],
					powered[A->P.Features.Std.EDX.PBE],
					powered[A->P.Features.Std.ECX.DTES64],
					powered[A->P.Features.Std.ECX.DS_CPL],
					powered[A->P.Features.Std.ECX.VMX],
					powered[A->P.Features.Std.ECX.SMX],
					powered[A->P.Features.Std.ECX.EIST],	enabled[A->P.MiscFeatures.EIST],
					powered[A->P.Features.Std.ECX.CNXT_ID],
					powered[A->P.Features.Std.ECX.FMA],
					powered[A->P.Features.Std.ECX.xTPR],	enabled[!A->P.MiscFeatures.xTPR],
					powered[A->P.Features.Std.ECX.PDCM],
					powered[A->P.Features.Std.ECX.PCID],
					powered[A->P.Features.Std.ECX.DCA],
					powered[A->P.Features.Std.ECX.x2APIC],
					powered[A->P.Features.Std.ECX.TSCDEAD],
					powered[A->P.Features.Std.ECX.XSAVE],
					powered[A->P.Features.Std.ECX.OSXSAVE],
					powered[A->P.Features.Ext.EDX.XD_Bit],	enabled[!A->P.MiscFeatures.XD_Bit],
					powered[A->P.Features.Ext.EDX.PG_1GB],
										enabled[A->P.MiscFeatures.FastStrings],
										enabled[A->P.MiscFeatures.TCC],
										enabled[A->P.MiscFeatures.PerfMonitoring],
										enabled[!A->P.MiscFeatures.BTS],
										enabled[!A->P.MiscFeatures.PEBS],
										enabled[A->P.MiscFeatures.CPUID_MaxVal],
										enabled[!A->P.MiscFeatures.Turbo],
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
					powered[A->P.Features.Std.ECX.MONITOR],	enabled[A->P.MiscFeatures.FSM],
					powered[A->P.Features.Std.ECX.CX16],
					powered[A->P.Features.Std.ECX.MOVBE],
					powered[A->P.Features.Std.ECX.POPCNT],
					powered[A->P.Features.Std.ECX.AES],
					powered[A->P.Features.Std.ECX.AVX],
					powered[A->P.Features.Std.ECX.F16C],
					powered[A->P.Features.Std.ECX.RDRAND],
					powered[A->P.Features.Ext.ECX.LAHFSAHF],
					powered[A->P.Features.Ext.EDX.SYSCALL],
					powered[A->P.Features.Ext.EDX.RDTSCP],
					powered[A->P.Features.Ext.EDX.IA64] );

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
				strcat(items, "Unknown\n");

			strcat(items, BIOS_SECTION);
			sprintf(str, BIOS_FORMAT, A->P.ClockSpeed);
			strcat(items, str);

			// Dispose & scroll all data strings stored in items.
			XRectangle R[]=	{ {
						x:0,
						y:Header_Height,
						width:A->W[G].width,
						height:A->W[G].height - Header_Height - Footer_Height,
					} };
			ScrollLayout(A, G, items, One_Char_Height, R);
		}
			break;
		case DUMP:
			#define	PrettyPrint(regName, regAddr) { \
				sprintf(mask, REG_FORMAT, regAddr, regName, REG_ALIGN - strlen(regName)); \
				sprintf(str, mask, 0x20); \
				strcat(items, str); \
				int H=0; \
				for(H=0; H < 15; H++) { \
					strncat(items, &binStr[H << 2], 4); \
					strcat(items, " "); \
				}; \
				strncat(items, &binStr[H << 2], 4); \
				strcat(items, "]\n"); \
			}
		// Dump a bunch of Registers with their Address, Name & Value.
		{
			char	items[DUMP_TEXT_WIDTH * DUMP_TEXT_HEIGHT]={0},
				binStr[BIN64_STR]={0},
				mask[PRE_TEXT]={0},
				str[PRE_TEXT]={0};

			DumpRegister(A->P.Core[4].FD, IA32_PERF_STATUS, NULL, binStr);
			PrettyPrint("IA32_PERF_STATUS", IA32_PERF_STATUS);

			DumpRegister(A->P.Core[0].FD, IA32_THERM_INTERRUPT, NULL, binStr);
			PrettyPrint("IA32_THERM_INTERRUPT", IA32_THERM_INTERRUPT);

			DumpRegister(A->P.Core[0].FD, IA32_THERM_STATUS, NULL, binStr);
			PrettyPrint("IA32_THERM_STATUS", IA32_THERM_STATUS);

			DumpRegister(A->P.Core[0].FD, IA32_MISC_ENABLE, NULL, binStr);
			PrettyPrint("IA32_MISC_ENABLE", IA32_MISC_ENABLE);

			DumpRegister(A->P.Core[0].FD, IA32_FIXED_CTR1, NULL, binStr);
			PrettyPrint("IA32_FIXED_CTR1", IA32_FIXED_CTR1);

			DumpRegister(A->P.Core[0].FD, IA32_FIXED_CTR2, NULL, binStr);
			PrettyPrint("IA32_FIXED_CTR2", IA32_FIXED_CTR2);

			DumpRegister(A->P.Core[0].FD, IA32_FIXED_CTR_CTRL, NULL, binStr);
			PrettyPrint("IA32_FIXED_CTR_CTRL", IA32_FIXED_CTR_CTRL);

			DumpRegister(A->P.Core[0].FD, IA32_PERF_GLOBAL_CTRL, NULL, binStr);
			PrettyPrint("IA32_PERF_GLOBAL_CTRL", IA32_PERF_GLOBAL_CTRL);

			DumpRegister(A->P.Core[0].FD, MSR_PLATFORM_INFO, NULL, binStr);
			PrettyPrint("MSR_PLATFORM_INFO", MSR_PLATFORM_INFO);

			DumpRegister(A->P.Core[0].FD, MSR_TURBO_RATIO_LIMIT, NULL, binStr);
			PrettyPrint("MSR_TURBO_RATIO_LIMIT", MSR_TURBO_RATIO_LIMIT);

			DumpRegister(A->P.Core[0].FD, MSR_TEMPERATURE_TARGET, NULL, binStr);
			PrettyPrint("MSR_TEMPERATURE_TARGET", MSR_TEMPERATURE_TARGET);

			// Dispose & scroll all data strings stored in items.
			XRectangle R[]=	{ {
						x:0,
						y:Header_Height,
						width:A->W[G].width,
						height:A->W[G].height - Header_Height - Footer_Height,
					} };
			ScrollLayout(A, G, items, One_Char_Height, R);
		}
			break;
	}
}

// Release the Widget ressources.
void	CloseWidgets(uARG *A)
{
	if(A->L.Output)
		free(A->L.Output);

	XUnloadFont(A->display, A->xfont->fid);

	int G=0;
	for(G=LAST_WIDGET; G >= 0 ; G--) {
		XFreePixmap(A->display, A->W[G].pixmap.B);
		XFreePixmap(A->display, A->W[G].pixmap.F);
		XFreeGC(A->display, A->W[G].gc);
		XDestroyWindow(A->display, A->W[G].window);
		free(A->L.Axes[G].Segment);
		DestroyAllButton(A, G);
	}
	XCloseDisplay(A->display);

	free(A->L.WB.String);
	free(A->L.Usage.C0);
	free(A->L.Usage.C3);
	free(A->L.Usage.C6);
	free(A->fontName);
}

// Fusion in one map the background and the foreground layouts.
void	MapLayout(uARG *A, int G)
{
	XCopyArea(A->display, A->W[G].pixmap.B, A->W[G].pixmap.F, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
}

// Send the map to the display.
void	FlushLayout(uARG *A, int G)
{
	XCopyArea(A->display,A->W[G].pixmap.F, A->W[G].window, A->W[G].gc, 0, 0, A->W[G].width, A->W[G].height, 0, 0);
	XFlush(A->display);
}

// An activity pulse blinks during the calculation (red) or when in pause (yellow).
void	DrawPulse(uARG *A, int G)
{
	XSetForeground(A->display, A->W[G].gc, (A->L.Play.flashPulse=!A->L.Play.flashPulse) ? PULSE_COLOR : A->W[G].background);
	XDrawArc(A->display, A->W[G].pixmap.F, A->W[G].gc,
		A->W[G].width - (Twice_Char_Width << 1),
		Header_Height >> 2,
		One_Char_Width,
		One_Char_Width,
		0, 360 << 8);
}

// Scroll the Wallboard.
void	DrawWB(uARG *A, int G)
{
	if(A->L.WB.Scroll < A->L.WB.Length)
		A->L.WB.Scroll++;
	else
		A->L.WB.Scroll=0;
	// Display the Wallboard.
	XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
	XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
			One_Char_Width * 6,
			One_Char_Height,
			&A->L.WB.String[A->L.WB.Scroll], (A->P.Platform.MaxNonTurboRatio << 1));
}

// Draw the layout foreground.
void	DrawLayout(uARG *A, int G)
{
	switch(G) {
		case MAIN: {
			int edline=_IS_MDI_ ? A->L.Axes[G].Segment[1].y2 + Footer_Height : A->W[G].height;
			// Draw the buffer if it is not empty.
			if(A->L.Input.KeyLength > 0)
			{
				XSetForeground(A->display, A->W[G].gc, PROMPT_COLOR);
				XDrawImageString(A->display, A->W[G].pixmap.F, A->W[G].gc,
						Quarter_Char_Width,
						edline - Quarter_Char_Height,
						A->L.Input.KeyBuffer, A->L.Input.KeyLength);
			}
			// Flash the cursor.
			A->L.Play.flashCursor=!A->L.Play.flashCursor;
			XSetForeground(A->display, A->W[G].gc, A->L.Play.flashCursor ? CURSOR_COLOR : A->W[G].background);

			XPoint Origin=	{
					x:(A->L.Input.KeyLength * One_Char_Width) - Quarter_Char_Width,
					y:edline - (Quarter_Char_Height >> 1)
					};
			DrawCursor(A, G, &Origin);

			if(A->L.Play.flashActivity)
				DrawPulse(A, G);
		}
			break;
		case CORES:
		{
			char str[16];
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++)
			{
				// Select a color based ratio.
				unsigned long BarBg, BarFg;
				if(A->P.Core[cpu].RelativeRatio <= A->P.Platform.MinimumRatio)
					{ BarBg=INIT_VALUE_COLOR; BarFg=DYNAMIC_COLOR; }
				if(A->P.Core[cpu].RelativeRatio >  A->P.Platform.MinimumRatio)
					{ BarBg=LOW_VALUE_COLOR; BarFg=CORES_BACKGROUND; }
				if(A->P.Core[cpu].RelativeRatio >= A->P.Platform.MaxNonTurboRatio)
					{ BarBg=MED_VALUE_COLOR; BarFg=CORES_BACKGROUND; }
				if(A->P.Core[cpu].RelativeRatio >= A->P.Turbo.MaxRatio_4C)
					{ BarBg=HIGH_VALUE_COLOR; BarFg=CORES_BACKGROUND; }

				// Draw the Core frequency.
				XSetForeground(A->display, A->W[G].gc, BarBg);
				XFillRectangle(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						One_Char_Width * 3,
						3 + ( One_Char_Height * (cpu + 1) ),
						(A->W[G].extents.overall.width * A->P.Core[cpu].RelativeRatio) / CORES_TEXT_WIDTH,
						One_Char_Height - 3);

				// For each Core, display its frequency, C-STATE & ratio.
				if(A->L.Play.freqHertz) {
					XSetForeground(A->display, A->W[G].gc, BarFg);
					sprintf(str, CORE_FREQ, A->P.Core[cpu].RelativeFreq, A->P.Core[cpu].UnhaltedFreq);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width * 5,
							One_Char_Height * (cpu + 1 + 1),
							str, strlen(str) );
				}
				if(A->L.Play.cState) {
					XSetForeground(A->display, A->W[G].gc, DYNAMIC_COLOR);
					sprintf(str, CORE_STATE,100 * A->P.Core[cpu].State.C0,
								100 * A->P.Core[cpu].State.C3,
								100 * A->P.Core[cpu].State.C6);
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							One_Char_Width * 18,
							One_Char_Height * (cpu + 1 + 1),
							str, strlen(str) );
				}
				if(A->L.Play.ratioValues) {
					XSetForeground(A->display, A->W[G].gc, DYNAMIC_COLOR);
					sprintf(str, CORE_RATIO, A->P.Core[cpu].RelativeRatio );
					XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
							Twice_Char_Width * A->P.Turbo.MaxRatio_1C,
							One_Char_Height * (cpu + 1 + 1),
							str, strlen(str) );
				}
			}
			if(A->L.Play.wallboard)
				DrawWB(A, G);
		}
			break;
		case CSTATES:
		{
			char str[32];
			XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				// Prepare the C0 chart.
				A->L.Usage.C0[cpu].x=Half_Char_Width + ((cpu * CSTATES_TEXT_SPACING) * One_Half_Char_Width);
				A->L.Usage.C0[cpu].y=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Core[cpu].State.C0);
				A->L.Usage.C0[cpu].width=(One_Char_Width * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C0[cpu].height=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Core[cpu].State.C0;
				// Prepare the C3 chart.
				A->L.Usage.C3[cpu].x=Half_Char_Width + A->L.Usage.C0[cpu].x + A->L.Usage.C0[cpu].width;
				A->L.Usage.C3[cpu].y=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Core[cpu].State.C3);
				A->L.Usage.C3[cpu].width=(One_Char_Width * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C3[cpu].height=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Core[cpu].State.C3;
				// Prepare the C6 chart.
				A->L.Usage.C6[cpu].x=Half_Char_Width + A->L.Usage.C3[cpu].x + A->L.Usage.C3[cpu].width;
				A->L.Usage.C6[cpu].y=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * (1 - A->P.Core[cpu].State.C6);
				A->L.Usage.C6[cpu].width=(One_Char_Width * CSTATES_TEXT_SPACING) / 3;
				A->L.Usage.C6[cpu].height=One_Char_Height
							+ (One_Char_Height * (CSTATES_TEXT_HEIGHT - 1)) * A->P.Core[cpu].State.C6;
			}
			// Display the C-State averages.
			sprintf(str, CORE_STATE,
						100 * A->P.Avg.C0,
						100 * A->P.Avg.C3,
						100 * A->P.Avg.C6);
			XDrawString(A->display, A->W[G].pixmap.F, A->W[G].gc,
						Twice_Char_Width,
						One_Char_Height + (One_Char_Height * (CSTATES_TEXT_HEIGHT + 1)),
						str, strlen(str) );

			// Draw C0, C3 & C6 states.
			XSetForeground(A->display, A->W[G].gc, GRAPH1_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C0, A->P.CPU);
			XSetForeground(A->display, A->W[G].gc, GRAPH2_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C3, A->P.CPU);
			XSetForeground(A->display, A->W[G].gc, GRAPH3_COLOR);
			XFillRectangles(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Usage.C6, A->P.CPU);
		}
			break;
		case TEMPS:
		{
			char str[16];
			// Update & draw the temperature histogram.
			int i=0;
			XSegment *U=&A->L.Axes[G].Segment[i], *V=&A->L.Axes[G].Segment[i+1];
			for(i=0; i < (TEMPS_TEXT_WIDTH - 1); i++, U=&A->L.Axes[G].Segment[i], V=&A->L.Axes[G].Segment[i+1]) {
				U->x1=V->x1 - One_Char_Width;
				U->x2=V->x2 - One_Char_Width;
				U->y1=V->y1;
				U->y2=V->y2;
			}
			V=&A->L.Axes[G].Segment[i - 1];
			U->x1=(TEMPS_TEXT_WIDTH + 2) * One_Char_Width;
			U->y1=V->y2;
			U->x2=U->x1 + One_Char_Width;
			U->y2=(( (TEMPS_TEXT_HEIGHT * A->P.Core[A->P.Hot].ThermStat.DTS)
				/ A->P.Core[A->P.Hot].TjMax.Target) + 2) * One_Char_Height;

			XSetForeground(A->display, A->W[G].gc, GRAPH1_COLOR);
			XDrawSegments(A->display, A->W[G].pixmap.F, A->W[G].gc, A->L.Axes[G].Segment, A->L.Axes[G].N - 2);

			// Display the Core temperature.
			int cpu=0;
			for(cpu=0; cpu < A->P.CPU; cpu++) {
				XSetForeground(A->display, A->W[G].gc, A->W[G].foreground);
				sprintf(str, "%3d", A->P.Core[cpu].TjMax.Target - A->P.Core[cpu].ThermStat.DTS);
				XDrawString(	A->display, A->W[G].pixmap.F, A->W[G].gc,
						(One_Char_Width * 5) + Half_Char_Width + (cpu << 1) * Twice_Char_Width,
						One_Char_Height * (TEMPS_TEXT_HEIGHT + 1 + 1),
						str, strlen(str));
			}
			// Show Temperature Thresholds
			XSetForeground(A->display, A->W[G].gc, GRAPH2_COLOR);
			int Threshold[2]={
					(( (TEMPS_TEXT_HEIGHT * A->P.Core[A->P.Hot].ThermIntr.Threshold1)
					/ A->P.Core[A->P.Hot].TjMax.Target) + 2) * One_Char_Height,
					(( (TEMPS_TEXT_HEIGHT * A->P.Core[A->P.Hot].ThermIntr.Threshold2)
					/ A->P.Core[A->P.Hot].TjMax.Target) + 2) * One_Char_Height
					};
			XDrawLine(	A->display, A->W[G].pixmap.F, A->W[G].gc,
					Twice_Char_Width,
					Threshold[0],
					U->x2,
					Threshold[1]);
			XSetForeground(A->display, A->W[G].gc, DYNAMIC_COLOR);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					One_Half_Char_Width << 2,
					Threshold[0],
					"T#1", 3);
			XDrawString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					TEMPS_TEXT_WIDTH * One_Char_Width,
					Threshold[1],
					"T#2", 3);

			// Display the hottest temperature between all Cores.
			int HotValue=A->P.Core[A->P.Hot].TjMax.Target - A->P.Core[A->P.Hot].ThermStat.DTS;

			if(HotValue <= LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, INIT_VALUE_COLOR);
			if(HotValue > LOW_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, LOW_VALUE_COLOR);
			if(HotValue > MED_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, MED_VALUE_COLOR);
			if(HotValue >= HIGH_TEMP_VALUE)
				XSetForeground(A->display, A->W[G].gc, HIGH_VALUE_COLOR);
			sprintf(str, "%3d", HotValue);
			XDrawImageString(A->display,
					A->W[G].pixmap.F,
					A->W[G].gc,
					Half_Char_Width,
					U->y2,
					str, 3);
		}
			break;
	}
}

// Update the Widget name with the Top Core frequency and its temperature.
void	UpdateTitle(uARG *A, int G)
{
	char str[32];
	switch(G) {
		case MAIN:
		case DUMP:
			sprintf(str, "X-Freq %s.%s-%s",
				_MAJOR, _MINOR, _NIGHTLY);
			break;
		case CORES:
			sprintf(str, "Core#%d @ %dMHz",
				A->P.Top, A->P.Core[A->P.Top].UnhaltedFreq);
			break;
		case CSTATES:
			sprintf(str, "C-States [%.2f%%] [%.2f%%]", 100 * A->P.Avg.C0, 100 * (A->P.Avg.C3 + A->P.Avg.C6));
			break;
		case TEMPS:
			sprintf(str, "Core#%d @ %dC",
				A->P.Top, A->P.Core[A->P.Hot].TjMax.Target - A->P.Core[A->P.Hot].ThermStat.DTS);
			break;
		case SYSINFO:
			sprintf(str, "Clock @ %dMHz",
				A->P.ClockSpeed);
			break;
	}
	XStoreName(A->display, A->W[G].window, str);
	XSetIconName(A->display, A->W[G].window, str);
}

// The far drawing procedure which paints the foreground.
static void *uDraw(void *uArg)
{
	uARG *A=(uARG *) uArg;
	int G=0;
	for(G=0; G < WIDGETS; G++) {
		if(!A->PAUSE[G]) {
			MapLayout(A, G);
			DrawLayout(A, G);
			UpdateTitle(A, G);
			FlushLayout(A, G);
		}
	}
	// Drawing is done.
	pthread_mutex_unlock(&uDraw_mutex);
	return(NULL);
}

// Implementation of CallBack functions.
void	ScrollingFunction(uARG *A, int G, char direction)
{
	switch(direction) {
		case NORTH_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].vScroll < A->L.Page[G].Text.rows) {
				A->L.Page[G].vScroll++ ;
				fDraw(G, false, true, false);
			}
			break;
		case SOUTH_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].vScroll > -A->L.Page[G].Text.rows) {
				A->L.Page[G].vScroll-- ;
				fDraw(G, false, true, false);
			}
			break;
		case EAST_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].hScroll > -A->L.Page[G].Text.cols) {
				A->L.Page[G].hScroll-- ;
				fDraw(G, false, true, false);
			}
			break;
		case WEST_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].hScroll < A->L.Page[G].Text.cols) {
				A->L.Page[G].hScroll++ ;
				fDraw(G, false, true, false);
			}
			break;
		case PGUP_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].vScroll < A->L.Page[G].Text.rows) {
				A->L.Page[G].vScroll+=10 ;
				fDraw(G, false, true, false);
			}
			break;
		case PGDW_DIR:
			if(A->L.Page[G].pageable
			&& A->L.Page[G].vScroll > -A->L.Page[G].Text.rows) {
				A->L.Page[G].vScroll-=10;
				fDraw(G, false, true, false);
			}
			break;
	}
}
void	ScrollingCallBack(uARG *A, int G, char direction)
{
	ScrollingFunction(A, G, direction);
	fDraw(G, false, true, false);
}

// Instructions set.
void	Proc_Release(uARG *A)
{
	#define Release _MAJOR"."_MINOR"-"_NIGHTLY"\n"
	Output(A, Release);
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

// the main thread which manages the X-Window events loop.
static void *uLoop(uARG *A)
{
	XEvent	E={0};
	while(A->LOOP) {
		XNextEvent(A->display, &E);

		int G=0;
		for(G=0; G < WIDGETS; G++)
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
					fDraw(MAIN, false, false, true);
					}
				switch(KeySymPressed) {
					case XK_Delete:
					case XK_KP_Delete:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyLength=0;
							fDraw(MAIN, false, false, true);
						}
						break;
					case XK_BackSpace:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyLength--;
							fDraw(MAIN, false, false, true);
						}
						break;
					case XK_Return:
					case XK_KP_Enter:
						if(A->L.Input.KeyLength > 0) {
							A->L.Input.KeyBuffer[A->L.Input.KeyLength]='\0';
							ExecCommand(A);
							A->L.Input.KeyLength=0;
							fDraw(MAIN, false, true, false);
						}
						break;
					case XK_Escape:
					case XK_Pause:
						for(G=FIRST_WIDGET; G <= LAST_WIDGET; G++)
							A->PAUSE[G]=!A->PAUSE[G];
						break;
					case XK_Home:
						if(A->L.Play.alwaysOnTop == false) {
							XRaiseWindow(A->display, A->W[G].window);
							A->L.Play.alwaysOnTop=true;
						}
						break;
					case XK_End:
						if(A->L.Play.alwaysOnTop == true) {
							XLowerWindow(A->display, A->W[G].window);
							A->L.Play.alwaysOnTop=false;
						}
						break;
					case XK_a:
					case XK_A:
						if(E.xkey.state & ControlMask)
							A->L.Play.flashActivity=!A->L.Play.flashActivity;
						break;
					case XK_c:
					case XK_C:
						if(E.xkey.state & ControlMask)
							if(A->L.Page[G].pageable) {
								fDraw(G, true, true, false);
							}
						break;
					case XK_h:
					case XK_H:
						if(E.xkey.state & ControlMask)
							A->L.Play.freqHertz=!A->L.Play.freqHertz;
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
							A->L.Play.cState=!A->L.Play.cState;
						break;
					case XK_q:
					case XK_Q:
						if(E.xkey.state & ControlMask) {
							Proc_Quit(A);
							fDraw(MAIN, true, true, false);
						}
						break;
					case XK_r:
					case XK_R:
						if(E.xkey.state & ControlMask)
							A->L.Play.ratioValues=!A->L.Play.ratioValues;
						break;
					case XK_w:
					case XK_W:
						if(E.xkey.state & ControlMask)
							A->L.Play.wallboard=!A->L.Play.wallboard;
						break;
					case XK_Up:
					case XK_KP_Up:
						ScrollingFunction(A, G, NORTH_DIR);
						break;
					case XK_Down:
					case XK_KP_Down:
						ScrollingFunction(A, G, SOUTH_DIR);
						break;
					case XK_Right:
					case XK_KP_Right:
						ScrollingFunction(A, G, EAST_DIR);
						break;
					case XK_Left:
					case XK_KP_Left:
						ScrollingFunction(A, G, WEST_DIR);
						break;
					case XK_Page_Up:
					case XK_KP_Page_Up:
						ScrollingFunction(A, G, PGUP_DIR);
						break;
					case XK_Page_Down:
					case XK_KP_Page_Down:
						ScrollingFunction(A, G, PGDW_DIR);
						break;
					case XK_KP_Add: {
						char str[32];
						if(A->P.IdleTime > 50000)
							A->P.IdleTime-=25000;
						sprintf(str, "[%d usecs]", A->P.IdleTime);
						XSetForeground(A->display, A->W[G].gc, PULSE_COLOR);
						XDrawImageString(A->display, A->W[G].window, A->W[G].gc,
								A->W[G].width - (15 * One_Char_Width),
								A->W[G].height - Quarter_Char_Height,
								str, strlen(str) );
					}
						break;
					case XK_KP_Subtract: {
						char str[32];
						A->P.IdleTime+=25000;
						sprintf(str, "[%d usecs]", A->P.IdleTime);
						XSetForeground(A->display, A->W[G].gc, PULSE_COLOR);
						XDrawImageString(A->display, A->W[G].window, A->W[G].gc,
								A->W[G].width - (15 * One_Char_Width),
								A->W[G].height - Quarter_Char_Height,
								str, strlen(str) );
					}
						break;
					case XK_F1:
						Proc_Menu(A);
						break;
					case XK_F2:
					case XK_F3:
					case XK_F4:
					case XK_F5:
					case XK_F6: {
						// Convert the function key number into a Widget index.
						G=XLookupKeysym(&E.xkey, 0) - XK_F1;
						// Get Map status.
						XWindowAttributes xwa={0};
						XGetWindowAttributes(A->display, A->W[G].window, &xwa);
						// Hide or unhide the Widget.
						if(xwa.map_state == IsUnmapped)
							XMapWindow(A->display, A->W[G].window);
						else if(_IS_MDI_) {
								if(G != MAIN)
									XUnmapWindow(A->display, A->W[G].window);
							}
							else
								XIconifyWindow(A->display, A->W[G].window, DefaultScreen(A->display));
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
						if(_IS_MDI_)
							MoveWidget(A, &E);
						else
							WidgetButtonPress(A, G, &E);
						break;
					case Button4: {
						int T=G;
						if(_IS_MDI_) {
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						ScrollingFunction(A, T, NORTH_DIR);
						break;
					}
					case Button5: {
						int T=G;
						if(_IS_MDI_) {
							for(T=LAST_WIDGET; T >= FIRST_WIDGET; T--)
								if( E.xbutton.subwindow == A->W[T].window)
									break;
						}
						ScrollingFunction(A, T, SOUTH_DIR);
						break;
					}
				}
				break;
			case ButtonRelease:
				switch(E.xbutton.button) {
					case Button3:
						if(_IS_MDI_)
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
			case FocusIn:
				XSetWindowBorder(A->display, A->W[G].window, FOCUS_COLOR);
				break;
			case FocusOut:
				XSetWindowBorder(A->display, A->W[G].window, A->W[G].foreground);
				break;
			case DestroyNotify:
				A->LOOP=false;
				break;
			case UnmapNotify:
				A->PAUSE[G]=true;
				break;
			case MapNotify:
				A->PAUSE[G]=false;
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
				{"-D", "%lx",&A->MDI,                  "Enable MDI Window (0/1)"      },
				{"-F", "%s", A->fontName,              "Font name"                    },
				{"-x", "%d", &A->L.Margin.H,           "Left position"                },
				{"-y", "%d", &A->L.Margin.V,           "Top position"                 },
				{"-b", "%x", &A->L.globalBackground,   "Background color"             },
				{"-f", "%x", &A->L.globalForeground,   "Foreground color"             },
				{"-c", "%ud",&A->P.PerCore,            "Monitor per Thread/Core (0/1)"},
				{"-s", "%ld",&A->P.IdleTime,           "Idle time (usec)"             },
				{"-a", "%ud",&A->L.Play.flashActivity, "Pulse activity (0/1)"         },
				{"-h", "%ud",&A->L.Play.freqHertz,     "CPU frequency (0/1)"          },
				{"-p", "%ud",&A->L.Play.cState,        "C-STATE percentage (0/1)"     },
				{"-r", "%ud",&A->L.Play.ratioValues,   "Ratio Values (0/1)"     },
				{"-t", "%ud",&A->L.Play.alwaysOnTop,   "Always On Top (0/1)"          },
				{"-w", "%ud",&A->L.Play.wallboard,     "Scroll wallboard (0/1)"       },
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
			P: {
				ArchID:-1,
				ClockSpeed:0,
				CPU:0,
				Bump:{0},
				Core:NULL,
				Top:0,
				Hot:0,
				PerCore:false,
				IdleTime:1000000,
			},
			W: {
				// MAIN
				{
				window:0,
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:-1,
				y:-1,
				width:300,
				height:150,
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
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:-1,
				y:MAIN,
				width:300,
				height:150,
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
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:CORES,
				y:MAIN,
				width:300,
				height:175,
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
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:-1,
				y:CSTATES,
				width:225,
				height:150,
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
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:TEMPS,
				y:CSTATES,
				width:500,
				height:300,
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
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:-1,
				y:SYSINFO,
				width:675,
				height:200,
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
				Page: {
					// MAIN
					{
						pageable:true,
						title:version,
						Text: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
					// CORES
					{
						pageable:false,
						title:NULL,
						Text: {
							cols:0,
							rows:0,
						},
						hScroll:0,
						vScroll:0,
					},
					// CSTATES
					{
						pageable:false,
						title:NULL,
						Text: {
							cols:0,
							rows:0,
						},
						hScroll:1,
						vScroll:1,
					},
					// TEMPS
					{
						pageable:false,
						title:NULL,
						Text: {
							cols:0,
							rows:0,
						},
						hScroll:1,
						vScroll:1,
					},
					// SYSINFO
					{
						pageable:true,
						title:SYSINFO_SECTION,
						Text: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
					// DUMP
					{
						pageable:true,
						title:DUMP_SECTION,
						Text: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
				},
				Play: {
					flashActivity:false,
					ratioValues:false,
					freqHertz:true,
					cState:false,
					alwaysOnTop: false,
					flashPulse:false,
					wallboard:false,
					flashCursor:true,
				},
				WB: {
					Scroll:0,
					Length:0,
					String:NULL,
				},
				Usage:{C0:NULL, C3:NULL, C6:NULL},
				Axes:{{0, NULL}},
				// Design the Cursor
				Cursor:{{x:+0, y:+0},
					{x:+4, y:-4},
					{x:+4, y:+4}},
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
		bool ROOT=false, MSR=false, BIOS=false;

		// Check root access.
		if(!(ROOT=(geteuid() == 0)))
			Output(&A, "Warning : root access is denied\n");

		// Read the CPU Features.
		CPUID(&A.P.Features);

		// Find the Processor Architecture.
		for(A.P.ArchID=ARCHITECTURES; A.P.ArchID >=0 ; A.P.ArchID--)
				if(!(ARCH[A.P.ArchID].Signature.ExtFamily ^ A.P.Features.Std.EAX.ExtFamily)
				&& !(ARCH[A.P.ArchID].Signature.Family ^ A.P.Features.Std.EAX.Family)
				&& !(ARCH[A.P.ArchID].Signature.ExtModel ^ A.P.Features.Std.EAX.ExtModel)
				&& !(ARCH[A.P.ArchID].Signature.Model ^ A.P.Features.Std.EAX.Model))
					break;

		if(!A.P.PerCore)
		{
			if(!(BIOS=(A.P.CPU=Get_ThreadCount()) != 0))
			{
				Output(&A, "Warning : can not read the BIOS DMI\nCheck if 'dmi' kernel module is loaded\n");

				// Fallback to the CPUID fixed count of threads if unavailable from BIOS.
				A.P.CPU=A.P.Features.ThreadCount;
				Output(&A, "Logical Core Count : falling back to the CPUID specifications\n");
			}
		}
		else
			if(!(BIOS=(A.P.CPU=Get_CoreCount()) != 0))
			{
				Output(&A, "Warning : can not read the BIOS DMI\nCheck if 'dmi' kernel module is loaded\n");

				if(A.P.ArchID != -1)
				{
					// Fallback to architecture specifications.
					A.P.CPU=ARCH[A.P.ArchID].MaxOfCores;
					Output(&A, "Physical Core Count : falling back to the Architecture specifications\n");
				}
				else
				{
					A.P.CPU=A.P.Features.ThreadCount;
					Output(&A, "Physical Core Count : falling back to the CPUID features\n");
				}
			}

		// Allocate the Cores working structure.
		pthread_mutex_init(&uDraw_mutex, NULL);

		A.P.Core=malloc(A.P.CPU * sizeof(struct THREADS));

		// Open once the MSR gate.
		if(!(MSR=(Open_MSR(&A) != 0)))
			Output(&A, "Warning : can not read the MSR registers\nCheck if 'msr' kernel module is loaded\n");

		// Read the bus clock frequency from the BIOS.
		if(BIOS)
			A.P.ClockSpeed=Get_ExternalClock();

		if(!BIOS || !A.P.ClockSpeed) {
			if(MSR)	// Fallback to an estimated clock frequency.
			{
				A.P.ClockSpeed=FallBack_Freq() / A.P.Platform.MaxNonTurboRatio;
				Output(&A, "Base Clock Frequency : falling back to an estimated measurement\n");
			}
			else {
				// Fallback at least to the specifications.
				if(A.P.ArchID != -1) {
					A.P.ClockSpeed=ARCH[A.P.ArchID].ClockSpeed;
					Output(&A, "Base Clock Frequency : falling back to the Architecture specifications\n");
				}
				else {
					A.P.ClockSpeed=ARCH[0].ClockSpeed;

					char strlog[16+50];
					sprintf(strlog, "Base Clock Frequency : falling back to the %s architecture specifications\n",
							ARCH[0].Architecture);
					Output(&A, strlog);
				}
			}
		}

		// Read the Integrated Memory Controler information.
		A.M=IMC_Read_Info();
		if(!A.M->ChannelCount)
			Output(&A, "Warning : can not read the IMC controler\n");

		// Initialize & run the Widget.
		if(XInitThreads() && OpenWidgets(&A))
		{
			Output(&A, "Welcome to X-Freq\n");

			int G=0;
			for(G=0; G < WIDGETS; G++) {
				BuildLayout(&A, G);
				MapLayout(&A, G);
				UpdateTitle(&A, G);
				XMapWindow(A.display, A.W[G].window);
			}
			Output(&A, "Enter help to list commands\n");
			CenterLayout(&A, MAIN);
			BuildLayout(&A, MAIN);

			pthread_t TID_Cycle=0;
			if(!pthread_create(&TID_Cycle, NULL, uCycle, &A)) {
				uLoop(&A);
				pthread_join(TID_Cycle, NULL);
			}
			else rc=2;

			CloseWidgets(&A);
		}
		else	rc=2;

		// Release the ressources.
		IMC_Free_Info(A.M);
		Close_MSR(&A);

		free(A.P.Core);
		pthread_mutex_destroy(&uDraw_mutex);
	}
	else	rc=1;
	return(rc);
}

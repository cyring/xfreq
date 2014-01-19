/*
 * XFreq.c #0.14 by CyrIng
 *
 * Copyright (C) 2013-2014 CYRIL INGENIERIE
 * Licenses: GPL2
 */

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
#include "xfreq.h"

//	Read, Write a Model Specific Register.
#define	Read_MSR(FD, offset, msr)  pread(FD, msr, sizeof(*msr), offset)
#define	Write_MSR(FD, offset, msr) pwrite(FD, msr, sizeof(*msr), offset)

// The drawing thread.
static void *uDraw(void *uArg);

//	Open one MSR handle per Processor Core.
int	Open_MSR(uARG *A) {
	ssize_t	retval=0;
	int	tmpFD=open("/dev/cpu/0/msr", O_RDONLY);
	int	rc=0;
	// Read the minimum, maximum & the turbo ratios from Core number 0
	if(tmpFD != -1) {
		rc=((retval=Read_MSR(tmpFD, MSR_PLATFORM_INFO, (PLATFORM *) &A->P.Platform)) != -1);
		rc=((retval=Read_MSR(tmpFD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->P.Turbo)) != -1);
		close(tmpFD);
	}
	char	pathname[]="/dev/cpu/999/msr";
	int	cpu=0;
	for(cpu=0; rc && (cpu < A->P.ThreadCount); cpu++) {
		sprintf(pathname, "/dev/cpu/%d/msr", cpu);
		if( (rc=((A->P.Core[cpu].FD=open(pathname, O_RDWR)) != -1)) )
			// Enable the Performance Counters 1 and 2 :
			// - Set the global counter bits
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, IA32_PERF_GLOBAL_CTRL, &A->P.Core[cpu].GlobalPerfCounter)) != -1);
			A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR1=1;
			A->P.Core[cpu].GlobalPerfCounter.EN_FIXED_CTR2=1;
			rc=((retval=Write_MSR(A->P.Core[cpu].FD, IA32_PERF_GLOBAL_CTRL, &A->P.Core[cpu].GlobalPerfCounter)) != -1);
			// - Set the fixed counter bits
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR_CTRL, &A->P.Core[cpu].FixedPerfCounter)) != -1);
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
			rc=((retval=Write_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR_CTRL, &A->P.Core[cpu].FixedPerfCounter)) != -1);
			// Retreive the Thermal Junction Max. Fallback to 100°C if not available.
			rc=((retval=Read_MSR(A->P.Core[cpu].FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->P.Core[cpu].TjMax)) != -1);
			if(A->P.Core[cpu].TjMax.Target == 0)
				A->P.Core[cpu].TjMax.Target=100;
	}
	return(rc);
}

// Close all MSR handles.
void	Close_MSR(uARG *A) {
	int	cpu=0;
	for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
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
static void *uCycle(void *uArg) {
	uARG *A=(uARG *) uArg;
	pthread_t TID_Draw=0;

	// Initial read of the TSC.
	A->P.TSC[0]=RDTSC();

	register unsigned int cpu=0;
	for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
		// Initial read of the Unhalted Core & Reference Cycles.
		Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR1, &A->P.Core[cpu].UnhaltedCoreCycles[0]);
		Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR2, &A->P.Core[cpu].UnhaltedRefCycles[0] );
	}
	while(A->LOOP) {
		// Settle down some microseconds as specified by the command argument.
		usleep(A->P.IdleTime);
		// Update the TSC.
		A->P.TSC[1]=RDTSC();
		register unsigned long long DeltaTSC=A->P.TSC[1] - A->P.TSC[0];
		// Save TSC.
		A->P.TSC[0]=A->P.TSC[1];

		unsigned int maxFreq=0;
		for(cpu=0, maxFreq=0; cpu < A->P.ThreadCount; cpu++) {
			// Update the Base Operating Ratio.
			Read_MSR(A->P.Core[cpu].FD, IA32_PERF_STATUS, (unsigned long long *) &A->P.Core[cpu].OperatingRatio);
			// Update the Unhalted Core & the Reference Cycles.
			Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR1, &A->P.Core[cpu].UnhaltedCoreCycles[1]);
			Read_MSR(A->P.Core[cpu].FD, IA32_FIXED_CTR2, &A->P.Core[cpu].UnhaltedRefCycles[1]);
			// Compute the Operating Frequency.
			A->P.Core[cpu].OperatingFreq=A->P.Core[cpu].OperatingRatio * A->P.ClockSpeed;
			// Compute the Delta of Unhalted (Core & Ref) Cycles = Current[1] - Previous[0]
			register unsigned long long	UnhaltedCoreCycles	= A->P.Core[cpu].UnhaltedCoreCycles[1]
										- A->P.Core[cpu].UnhaltedCoreCycles[0],
							UnhaltedRefCycles	= A->P.Core[cpu].UnhaltedRefCycles[1]
										- A->P.Core[cpu].UnhaltedRefCycles[0];
			// Compute C-State.
			A->P.Core[cpu].State.C0=(double) (UnhaltedRefCycles) / (double) (DeltaTSC);
			// Compute the Current Core Ratio per Cycles Delta. Set with the Operating value to protect against a division by zero.
			A->P.Core[cpu].UnhaltedRatio	= (UnhaltedRefCycles != 0) ?
							 (A->P.Core[cpu].OperatingRatio * UnhaltedCoreCycles) / UnhaltedRefCycles
							: A->P.Core[cpu].OperatingRatio;
			// Actual Frequency = Current Ratio x Bus Clock Frequency
			A->P.Core[cpu].UnhaltedFreq=A->P.Core[cpu].UnhaltedRatio * A->P.ClockSpeed;
			// Save the Unhalted Core & Reference Cycles for next iteration.
			A->P.Core[cpu].UnhaltedCoreCycles[0]=A->P.Core[cpu].UnhaltedCoreCycles[1];
			A->P.Core[cpu].UnhaltedRefCycles[0] =A->P.Core[cpu].UnhaltedRefCycles[1];
			// Index the Top CPU speed.
			if(maxFreq < A->P.Core[cpu].UnhaltedFreq) {
				maxFreq=A->P.Core[cpu].UnhaltedFreq;
				A->P.Top=cpu;
			}
			// Update the Digital Thermal Sensor.
			if( (Read_MSR(A->P.Core[cpu].FD, IA32_THERM_STATUS, (THERM *) &A->P.Core[cpu].Therm)) == -1)
				A->P.Core[cpu].Therm.DTS=0;
		}
		pthread_create(&TID_Draw, NULL, uDraw, A);
	}
	pthread_join(TID_Draw, NULL);
	return(NULL);
}

// Read any data from the SMBIOS.
int	Read_SMBIOS(int structure, int instance, off_t offset, void *buf, size_t nbyte) {
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
int	Get_ExternalClock() {
	int	BCLK=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_EXTCLK, &BCLK, 1) != -1)
		return(BCLK);
	else
		return(0);
}

// Read the number of logical Cores activated in the BIOS.
int	Get_ThreadCount() {
	short int ThreadCount=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_THREADS, &ThreadCount, 1) != -1)
		return(ThreadCount);
	else
		return(0);
}

// Read the number of physicial Cores activated in the BIOS.
int	Get_CoreCount() {
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

// Read data from the PCI bus.
#define PCI_CONFIG_ADDRESS(bus, dev, fn, reg) \
	(0x80000000 | (bus << 16) | (dev << 11) | (fn << 8) | (reg & ~3))

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

// All-in-One function to print a string filled with some New Line terminated texts.
XMAXPRINT XPrint(Display *display, Drawable drawable, GC gc, int x, int y, char *NewLineStr, int spacing) {
	char *pStartLine=NewLineStr, *pNewLine=NULL;
	XMAXPRINT  max={0,0};
	while((pNewLine=strchr(pStartLine,'\n')) != NULL) {
		int cols=pNewLine - pStartLine;
		XDrawString(	display, drawable, gc,
				x,
				y + (spacing * max.rows),
				pStartLine, cols);
		max.cols=(cols > max.cols) ? cols : max.cols;
		max.rows++ ;
		pStartLine=pNewLine+1;
	}
	return(max);
}

// Create the X-Window Widget.
int	OpenLayout(uARG *A) {
	int noerr=true;

	if((A->W.display=XOpenDisplay(NULL)) && (A->W.screen=DefaultScreenOfDisplay(A->W.display)) ) {
				XSetWindowAttributes swa={
	/* Pixmap: background, None, or ParentRelative	*/	background_pixmap:None,
	/* unsigned long: background pixel		*/	background_pixel:BlackPixel(A->W.display, DefaultScreen(A->W.display)),
	/* Pixmap: border of the window or CopyFromParent */	border_pixmap:CopyFromParent,
	/* unsigned long: border pixel value */			border_pixel:WhitePixel(A->W.display, DefaultScreen(A->W.display)),
	/* int: one of bit gravity values */			bit_gravity:0,
	/* int: one of the window gravity values */		win_gravity:0,
	/* int: NotUseful, WhenMapped, Always */		backing_store:DoesBackingStore(DefaultScreenOfDisplay(A->W.display)),
	/* unsigned long: planes to be preserved if possible */	backing_planes:AllPlanes,
	/* unsigned long: value to use in restoring planes */	backing_pixel:0,
	/* Bool: should bits under be saved? (popups) */	save_under:DoesSaveUnders(DefaultScreenOfDisplay(A->W.display)),
	/* long: set of events that should be saved */		event_mask:EventMaskOfScreen(DefaultScreenOfDisplay(A->W.display)),
	/* long: set of events that should not propagate */	do_not_propagate_mask:0,
	/* Bool: boolean value for override_redirect */		override_redirect:False,
	/* Colormap: color map to be associated with window */	colormap:DefaultColormap(A->W.display, DefaultScreen(A->W.display)),
	/* Cursor: cursor to be displayed (or None) */		cursor:None,
	};
		GC tmpGC=0;
		if((A->W.window=XCreateWindow(	A->W.display, DefaultRootWindow(A->W.display),
						A->W.x, A->W.y, A->W.width, A->W.height,
						0, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect, &swa)) )
		{
			// Try to load the requested font and compute scaling.
			if((tmpGC=XCreateGC(A->W.display, A->W.window, 0, NULL))) {
				if((A->W.extents.font = XLoadQueryFont(A->W.display, A->W.extents.fname)))
				{
					XSetFont(A->W.display, tmpGC, A->W.extents.font->fid);

					XTextExtents(	A->W.extents.font, HDSIZE, A->P.Turbo.MaxRatio_1C << 1,
							&A->W.extents.dir, &A->W.extents.ascent,
							&A->W.extents.descent, &A->W.extents.overall);

					A->W.extents.charWidth=A->W.extents.font->max_bounds.rbearing - A->W.extents.font->min_bounds.lbearing;
					A->W.extents.charHeight=A->W.extents.ascent + A->W.extents.descent;

					A->W.margin.width=(A->W.extents.charWidth << 2);
					A->W.margin.height=(A->W.extents.charHeight >> 2);

					A->W.width=(A->W.margin.width << 1) + A->W.extents.overall.width + (A->W.extents.charWidth << 1);
					A->W.height=(A->W.margin.height << 1) + A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1);

					XUnloadFont(A->W.display, A->W.extents.font->fid);
				}
				else	noerr=false;

				XFreeGC(A->W.display, tmpGC);
			}
			else	noerr=false;

			if(noerr
			&& (A->W.pixmap.B=XCreatePixmap(A->W.display, A->W.window, A->W.width, A->W.height, DefaultDepthOfScreen(A->W.screen)))
			&& (A->W.pixmap.F=XCreatePixmap(A->W.display, A->W.window, A->W.width, A->W.height, DefaultDepthOfScreen(A->W.screen)))
			&& (A->W.gc=XCreateGC(A->W.display, A->W.pixmap.F, 0, NULL)) )
			{
				A->W.extents.font = XLoadQueryFont(A->W.display, A->W.extents.fname);
				XSetFont(A->W.display, A->W.gc, A->W.extents.font->fid);

				// Allocate memory for chart elements.
				A->L.usage=malloc(A->P.ThreadCount * sizeof(XRectangle));
				A->L.axes=malloc(A->P.Turbo.MaxRatio_1C * sizeof(XSegment));

				// Seek root or DE window.
				if(A->D.window != (XID)-1) {
					if(A->D.window == 0x0)
						A->D.window=DefaultRootWindow(A->W.display);
					else {
						char *tmpDesktop=NULL;
						if(XFetchName(A->W.display, A->D.window, &tmpDesktop) == 0)
							A->D.window=(XID)-1;
						else {
							strcpy(A->D.name, tmpDesktop);
							XFree(tmpDesktop);
						}
					}
				}
				// Adjust window size & inform WM about requirements.
				if((A->W.hints=XAllocSizeHints()) != NULL) {
					A->W.hints->min_width= A->W.hints->max_width= A->W.width;
					A->W.hints->min_height=A->W.hints->max_height=A->W.height;
					A->W.hints->flags=PMinSize|PMaxSize;
					XSetWMNormalHints(A->W.display, A->W.window, A->W.hints);
					XFree(A->W.hints);
				}
				XWindowAttributes xwa={0};
				XGetWindowAttributes(A->W.display, A->W.window, &xwa);
				if((xwa.width != A->W.width) || (xwa.height != A->W.height))
					XResizeWindow(A->W.display, A->W.window, A->W.width, A->W.height);

				// Prepare a Wallboard string with the Processor information.
				sprintf(A->L.string, OVERCLOCK, A->P.Features.BrandString, A->P.Platform.MaxNonTurboRatio * A->P.ClockSpeed);
				A->L.wbLength=strlen(A->L.string) + (A->P.Platform.MaxNonTurboRatio << 2);
				A->L.wallboard=calloc(A->L.wbLength, 1);
				memset(A->L.wallboard, 0x20, A->L.wbLength);
				memcpy(&A->L.wallboard[A->P.Platform.MaxNonTurboRatio << 1], A->L.string, strlen(A->L.string));
				A->L.wbLength=strlen(A->L.string) + (A->P.Platform.MaxNonTurboRatio << 1);
				// Store some Ratios into a string for future chart drawing.
				sprintf(A->L.bump, "%02d%02d%02d",	A->P.Platform.MinimumRatio,
									A->P.Platform.MaxNonTurboRatio,
									A->P.Turbo.MaxRatio_1C);
				// Prepare the chart.
				int i=0, j=A->W.extents.overall.width / A->P.Turbo.MaxRatio_1C;
				for(i=1; i <= A->P.Turbo.MaxRatio_1C; i++) {
					A->L.axes[i].x1 = A->W.margin.width + (j * i) + (A->W.extents.charWidth >> 1);
					A->L.axes[i].y1 = 3 + A->W.margin.height + A->W.extents.charHeight;
					A->L.axes[i].x2 = A->L.axes[i].x1;
					A->L.axes[i].y2 = A->W.margin.height + ((A->P.ThreadCount + 1) * A->W.extents.charHeight) - 3;
				}
			}
			else	noerr=false;

			XSelectInput(A->W.display, A->W.window, VisibilityChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);
		}
		else	noerr=false;
	}
	else	noerr=false;
	return(noerr);
}

// Center the layout of the current page.
void	CenterLayout(uARG *A) {
	A->L.Page[A->L.currentPage].hScroll=1 ;
	A->L.Page[A->L.currentPage].vScroll=1 ;
}

// Draw the layout background.
void	BuildLayout(uARG *A) {
	XSetBackground(A->W.display, A->W.gc, A->W.background);

	if(A->L.currentPage == CORE) {
		// Clear entirely the background.
		XSetForeground(A->W.display, A->W.gc, A->W.background);
		XFillRectangle(A->W.display, A->W.pixmap.B, A->W.gc, 0, 0, A->W.width, A->W.height);
		// Draw the axes.
		XSetForeground(A->W.display, A->W.gc, 0x666666);
		XDrawSegments(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->L.axes,
				A->P.Turbo.MaxRatio_1C + 1);

		// Draw the Core identifiers, the header, and the footer on the chart.
		XSetForeground(A->W.display, A->W.gc, A->W.foreground);
		int cpu=0;
		for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
			sprintf(A->L.string, CORE_NUM, cpu);
			XDrawString(	A->W.display, A->W.pixmap.B, A->W.gc,
					A->W.extents.charWidth >> 1,
					A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ),
					A->L.string, strlen(A->L.string) );
		}
		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.extents.charWidth >> 2,
				A->W.margin.height + A->W.extents.charHeight,
				"Core", 4);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + A->W.extents.overall.width + (A->W.extents.charWidth << 1),
				A->W.margin.height + A->W.extents.charHeight,
				"T.C", 3);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.extents.charWidth >> 2,
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				"Ratio", 5);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (5 << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				"5", 1);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MinimumRatio << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.bump[0], 2);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MaxNonTurboRatio << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.bump[2], 2);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Turbo.MaxRatio_1C << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.bump[4], 2);
	}
	else // Deal with other pages.
		{
		char items[4096]={0};
		int spacing=A->W.extents.charHeight;

		switch(A->L.currentPage) {
			case MENU: {
				strcpy(items, MENU_FORMAT);
				spacing=A->W.extents.charHeight  + (A->W.extents.charHeight >> 1);
			}
				break;
			case PROC: {
				const char  powered[2]={'N', 'Y'};
				sprintf(items, PROC_FORMAT,
					A->P.Features.BrandString,
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
					powered[A->P.Features.Std.ECX.EIST],
					powered[A->P.Features.Std.ECX.CNXT_ID],
					powered[A->P.Features.Std.ECX.FMA],
					powered[A->P.Features.Std.ECX.xTPR],
					powered[A->P.Features.Std.ECX.PDCM],
					powered[A->P.Features.Std.ECX.PCID],
					powered[A->P.Features.Std.ECX.DCA],
					powered[A->P.Features.Std.ECX.x2APIC],
					powered[A->P.Features.Std.ECX.TSCDEAD],
					powered[A->P.Features.Std.ECX.XSAVE],
					powered[A->P.Features.Std.ECX.OSXSAVE],
					powered[A->P.Features.Ext.EDX.XD_Bit],
					powered[A->P.Features.Ext.EDX.PG_1GB],
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
					powered[A->P.Features.Std.ECX.MONITOR],
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
			}
				break;
			case RAM: {
				strcpy(items, CHA_FORMAT);
				if(A->M != NULL) {
					unsigned cha=0;
					for(cha=0; cha < A->M->ChannelCount; cha++) {
						char timing[sizeof(HDSIZE)];
						sprintf(timing, CAS_FORMAT,
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
						strcat(items, timing);
					}
				}
				else
					strcpy(items, "Unknown\n");
			}
				break;
			case BIOS: {
				sprintf(items, BIOS_FORMAT, A->P.ClockSpeed);
			}
				break;
		}
		XSetForeground(A->W.display, A->W.gc, A->W.background);
		XFillRectangle(A->W.display, A->W.pixmap.B, A->W.gc, 0, 0, A->W.width, A->W.height);

		XSetForeground(A->W.display, A->W.gc, A->W.foreground);
		XDrawString(	A->W.display, A->W.pixmap.B, A->W.gc,
				A->W.margin.width + A->W.extents.charWidth,
				A->W.margin.height + A->W.extents.charHeight,
				A->L.Page[A->L.currentPage].title,
				strlen(A->L.Page[A->L.currentPage].title) );

		XSetForeground(A->W.display, A->W.gc, 0x666666);
		XDrawLine(	A->W.display, A->W.pixmap.B, A->W.gc,
				A->W.margin.width,
				A->W.margin.height + A->W.extents.charHeight + (A->W.extents.charHeight >> 1) - 1,
				A->W.width - (A->W.margin.width << 1),
				A->W.margin.height + A->W.extents.charHeight + (A->W.extents.charHeight >> 1) - 1);

		XRectangle R[]=	{      {0,
					0,
					A->W.width,
					A->W.height - (A->W.margin.height + A->W.extents.charHeight + (A->W.extents.charHeight >> 1))
				}	};
		XSetClipRectangles(	A->W.display, A->W.gc,
					0,
					A->W.margin.height + A->W.extents.charHeight + (A->W.extents.charHeight >> 1),
					R, 1, Unsorted);

		XSetForeground(A->W.display, A->W.gc, 0xf0f0f0);
		A->L.Page[A->L.currentPage].max=XPrint(	A->W.display, A->W.pixmap.B, A->W.gc,
							A->W.margin.width * A->L.Page[A->L.currentPage].hScroll,
							A->W.margin.height
							+ A->W.extents.charHeight
							+ (A->W.extents.charHeight >> 1)
							+ (A->W.extents.charHeight * A->L.Page[A->L.currentPage].vScroll),
							items,
							spacing);
		XSetClipMask(A->W.display, A->W.gc, None);
	}
}

// Release the Widget ressources.
void	CloseLayout(uARG *A) {
	free(A->L.wallboard);
	free(A->L.axes);
	free(A->L.usage);

	XUnloadFont(A->W.display, A->W.extents.font->fid);
	XFreeGC(A->W.display, A->W.gc);
	XFreePixmap(A->W.display, A->W.pixmap.B);
	XFreePixmap(A->W.display, A->W.pixmap.F);
	XDestroyWindow(A->W.display, A->W.window);
	XCloseDisplay(A->W.display);
}

// Fusion in one map the background and the foreground layouts.
void	MapLayout(uARG *A) {
	if((A->D.window != (XID)-1)
	&& XGetWindowAttributes(A->W.display, A->W.window, &A->W.attribs)
	&& XTranslateCoordinates(A->W.display, A->W.window, A->D.window,
				0, 0, &A->W.x, &A->W.y, &A->D.child) )
	{
		XCopyArea(A->W.display, A->D.window, A->W.pixmap.B, A->W.gc,
				A->W.x, A->W.y,
				A->W.attribs.width, A->W.attribs.height,
				0, 0);
	}
	XCopyArea(A->W.display, A->W.pixmap.B, A->W.pixmap.F, A->W.gc, 0, 0, A->W.width, A->W.height, 0, 0);
}

// Send the map to the display.
void	FlushLayout(uARG *A) {
	XCopyArea(A->W.display,A->W.pixmap.F,A->W.window,A->W.gc, 0, 0, A->W.width, A->W.height, 0, 0);
	XFlush(A->W.display);
}

// An activity pulse blinks during the calculation (red) or when in pause (yellow).
void	DrawPulse(uARG *A) {
	XSetForeground(A->W.display, A->W.gc, (A->W.pulse=!A->W.pulse) ? (A->PAUSE) ? 0xffff00 : 0xff0000 : A->W.foreground);
	XDrawArc(A->W.display, A->W.pixmap.F, A->W.gc,
		A->W.width - (A->W.extents.charWidth + (A->W.extents.charWidth >> 1)),
		A->W.height - (A->W.extents.charHeight - (A->W.extents.charWidth >> 2)),
		A->W.extents.charWidth,
		A->W.extents.charWidth,
		0, 360 << 8);
}

// Draw the layout foreground.
void	DrawLayout(uARG *A) {
	switch(A->L.currentPage) {
		case CORE: {
			XSetForeground(A->W.display, A->W.gc, A->W.foreground);
			// Scroll the wallboard.
			if(A->L.wbScroll < A->L.wbLength)
				A->L.wbScroll++;
			else
				A->L.wbScroll=0;
			XDrawString(	A->W.display, A->W.pixmap.F, A->W.gc,
					A->W.margin.width + (A->W.extents.charWidth << 1),
					A->W.extents.charHeight,
					&A->L.wallboard[A->L.wbScroll], (A->P.Platform.MaxNonTurboRatio << 1));
			// Draw the Core activity when C-State is C0
			int cpu=0;
			for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
				XSetForeground(A->W.display, A->W.gc, A->W.foreground);
				XDrawLine(	A->W.display, A->W.pixmap.F, A->W.gc,
						A->W.margin.width + (A->W.extents.overall.width * (1.0f - A->P.Core[cpu].State.C0)),
						3 + A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1) ),
						A->W.margin.width + (A->W.extents.overall.width * (1.0f - A->P.Core[cpu].State.C0)),
						A->W.margin.height + ( A->W.extents.charHeight * (cpu + 2) ) - 3 );

				A->L.usage[cpu].x=A->W.margin.width;
				A->L.usage[cpu].y=3 + A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1) );
				A->L.usage[cpu].width	= (A->W.extents.overall.width
							* (unsigned long long) (A->P.Core[cpu].UnhaltedRatio * A->P.Core[cpu].State.C0))
							/ A->P.Turbo.MaxRatio_1C;
				A->L.usage[cpu].height=A->W.extents.charHeight - 3;

				if(A->P.Core[cpu].UnhaltedRatio <= A->P.Platform.MinimumRatio)
					XSetForeground(A->W.display, A->W.gc, A->W.foreground);
				if(A->P.Core[cpu].UnhaltedRatio >  A->P.Platform.MinimumRatio)
					XSetForeground(A->W.display, A->W.gc, 0x009966);
				if(A->P.Core[cpu].UnhaltedRatio >= A->P.Platform.MaxNonTurboRatio)
					XSetForeground(A->W.display, A->W.gc, 0xffa500);
				if(A->P.Core[cpu].UnhaltedRatio >= A->P.Turbo.MaxRatio_4C)
					XSetForeground(A->W.display, A->W.gc, 0xff0000);

				XFillRectangle(A->W.display, A->W.pixmap.F, A->W.gc,
						A->L.usage[cpu].x,
						A->L.usage[cpu].y,
						A->L.usage[cpu].width,
						A->L.usage[cpu].height);

				XSetForeground(A->W.display, A->W.gc, 0xffffff);
				sprintf(A->L.string, CORE_FREQ, A->P.Core[cpu].OperatingFreq);
				XDrawString(A->W.display, A->W.pixmap.F, A->W.gc,
						A->W.margin.width + (A->W.extents.charWidth >> 2),
						A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ),
						A->L.string, strlen(A->L.string));

				XSetForeground(A->W.display, A->W.gc, A->W.foreground);
				sprintf(A->L.string, "%3d", A->P.Core[cpu].TjMax.Target - A->P.Core[cpu].Therm.DTS);
				XDrawString(A->W.display, A->W.pixmap.F, A->W.gc,
						A->W.margin.width + A->W.extents.overall.width + (A->W.extents.charWidth << 1),
						A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ),
						A->L.string, 3);
			}
			XSetForeground(A->W.display, A->W.gc, 0x666666);
			XDrawRectangles(A->W.display, A->W.pixmap.F, A->W.gc, A->L.usage, A->P.ThreadCount);
		}
		default:
			break;
	}
}

// Update the Widget name with the Top Core frequency and its temperature.
void	UpdateTitle(uARG *A) {
	sprintf(A->L.string, APP_TITLE,
		A->P.Top, A->P.Core[A->P.Top].UnhaltedFreq, A->P.Core[A->P.Top].TjMax.Target - A->P.Core[A->P.Top].Therm.DTS);
	XStoreName(A->W.display, A->W.window, A->L.string);
	XSetIconName(A->W.display, A->W.window, A->L.string);
}

// The drawing thread which paints the foreground.
static void *uDraw(void *uArg) {
	uARG *A=(uARG *) uArg;

	if(!A->PAUSE) {
		MapLayout(A);
		DrawLayout(A);
		if(A->W.activity)
			DrawPulse(A);
		UpdateTitle(A);
	} else
		DrawPulse(A);
	FlushLayout(A);

	return(NULL);
}

// the main thread which manages the X-Window events loop.
static void *uLoop(void *uArg) {
	uARG *A=(uARG *) uArg;

	XEvent	E={0};
	while(A->LOOP) {
		XNextEvent(A->W.display, &E);
		switch(E.type) {
			case Expose: {
				if(!E.xexpose.count) {
					while(XCheckTypedEvent(A->W.display, Expose, &E)) ;

					FlushLayout(A);
				}
			}
				break;
			case KeyPress:
				switch(XLookupKeysym(&E.xkey, 0)) {
					case XK_Escape:
						A->LOOP=false;
						break;
					case XK_Pause:
						A->PAUSE=!A->PAUSE;
						break;
					case XK_Return: {
						MapLayout(A);
						DrawLayout(A);
						FlushLayout(A);
					}
						break;
					case XK_Home:
						A->W.alwaysOnTop=true;
						break;
					case XK_End: {
						if(A->W.alwaysOnTop)
							XLowerWindow(A->W.display, A->W.window);
						A->W.alwaysOnTop=false;
					}
						break;
					case XK_p:
					case XK_P:
						A->W.activity=!A->W.activity;
						break;
					case XK_C:
					case XK_c: {
						CenterLayout(A);
						BuildLayout(A);
						MapLayout(A);
						FlushLayout(A);
					}
						break;
					case XK_Left: if(A->L.Page[A->L.currentPage].pageable
						      && A->L.Page[A->L.currentPage].hScroll < A->L.Page[A->L.currentPage].max.cols) {
								A->L.Page[A->L.currentPage].hScroll++ ;
								BuildLayout(A);
								MapLayout(A);
								FlushLayout(A);
						}
						break;
					case XK_Right: if(A->L.Page[A->L.currentPage].pageable
						       && A->L.Page[A->L.currentPage].hScroll > -A->L.Page[A->L.currentPage].max.cols) {
							A->L.Page[A->L.currentPage].hScroll-- ;
							BuildLayout(A);
							MapLayout(A);
							FlushLayout(A);
						}
						break;
					case XK_Up: if(A->L.Page[A->L.currentPage].pageable
						    && A->L.Page[A->L.currentPage].vScroll < A->L.Page[A->L.currentPage].max.rows) {
								A->L.Page[A->L.currentPage].vScroll++ ;
								BuildLayout(A);
								MapLayout(A);
								FlushLayout(A);
						}
						break;
					case XK_Down: if(A->L.Page[A->L.currentPage].pageable
						      && A->L.Page[A->L.currentPage].vScroll > -A->L.Page[A->L.currentPage].max.rows) {
							A->L.Page[A->L.currentPage].vScroll-- ;
							BuildLayout(A);
							MapLayout(A);
							FlushLayout(A);
						}
						break;
					case XK_Page_Up:
						if(A->L.currentPage < _COP_ - 1) {
							A->L.currentPage++ ;
							CenterLayout(A);
							BuildLayout(A);
							MapLayout(A);
							FlushLayout(A);
						}
						break;
					case XK_Page_Down:
						if(A->L.currentPage > 0) {
							A->L.currentPage-- ;
							CenterLayout(A);
							BuildLayout(A);
							MapLayout(A);
							FlushLayout(A);
						}
						break;
					case XK_KP_Add: {
						if(A->P.IdleTime > 50000)
							A->P.IdleTime-=25000;
						XSetForeground(A->W.display, A->W.gc, 0xffff00);
						sprintf(A->L.string, "[%d usecs]", A->P.IdleTime);
						XDrawImageString(A->W.display, A->W.window, A->W.gc,
								 A->W.width >> 1, A->W.height >> 1,
								 A->L.string, strlen(A->L.string) );
						}
						break;
					case XK_KP_Subtract: {
						A->P.IdleTime+=25000;
						sprintf(A->L.string, "[%d usecs]", A->P.IdleTime);
						XSetForeground(A->W.display, A->W.gc, 0xffff00);
						XDrawImageString(A->W.display, A->W.window, A->W.gc,
								 A->W.width >> 1, A->W.height >> 1,
								 A->L.string, strlen(A->L.string) );
						}
						break;
					case XK_F1:
					case XK_F2:
					case XK_F3:
					case XK_F4:
					case XK_F5: {
						A->L.currentPage=XLookupKeysym(&E.xkey, 0) - XK_F1;
						CenterLayout(A);
						BuildLayout(A);
						MapLayout(A);
						FlushLayout(A);
					}
						break;
				}
				break;
			case DestroyNotify:
				A->LOOP=false;
				break;
			case UnmapNotify:
				A->PAUSE=true;
				break;
			case MapNotify:
				A->PAUSE=false;
				break;
			case VisibilityNotify:
				switch (E.xvisibility.state) {
					case VisibilityUnobscured:
						break;
					case VisibilityPartiallyObscured:
					case VisibilityFullyObscured:
						if(A->W.alwaysOnTop)
							XRaiseWindow(A->W.display, A->W.window);
						break;
				}
				break;
		}
	}
	return(NULL);
}

// Apply the command line arguments.
int	Help(uARG *A, int argc, char *argv[]) {
	OPTION	options[] = {	{"-x", "%d", &A->W.x,             "Left position"       },
				{"-y", "%d", &A->W.y,             "Top position"        },
				{"-b", "%x", &A->W.background,    "Background color"    },
				{"-f", "%x", &A->W.foreground,    "Foreground color"    },
				{"-c", "%ud",&A->P.PerCore,       "Monitor per Thread/Core (0/1)"},
				{"-s", "%ld",&A->P.IdleTime,      "Idle time (usec)"    },
				{"-a", "%ud",&A->W.activity,      "Pulse activity (0/1)"},
				{"-t", "%ud",&A->W.alwaysOnTop,   "Always On Top (0/1)" },
				{"-D", "%lx",&A->D.window,        "Desktop Window id"   },
				{"-F", "%s", &A->W.extents.fname, "Font name"           },
		};
	const int s=sizeof(options)/sizeof(OPTION);
	int uid=geteuid(), i=0, j=0, noerr=true;

	if((argc - ((argc >> 1) << 1)) && (uid == 0)) {
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
		printf("\nRoot Access Granted [%s]\n\nReport bugs to webmaster@cyring.fr\n", (!uid)? "OK":"NO");
	}
	return(noerr);
}

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[]) {
	uARG	A= {
			LOOP: true,
			PAUSE: true,
			P: {
				Top:0,
				ArchID:-1,
				ClockSpeed:0,
				ThreadCount:0,
				Core:NULL,
				PerCore:false,
				IdleTime:750000,
			},
			W: {
				display:NULL,
				window:0,
				screen:NULL,
				pixmap: {
					B:0,
					F:0,
				},
				gc:0,
				x:0,
				y:0,
				width:300,
				height:150,
				margin: {
					width :4,
					height:13 + (13 >> 1),
				},
				alwaysOnTop: false,
				activity:false,
				pulse:false,
				extents: {
					fname:"Fixed",
					font:NULL,
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:6,
					charHeight:13,
				},
				background:0x333333,
				foreground:0x8fcefa,
				hints:NULL,
				attribs:{0},
			},
			D: {
				window:(XID)-1,
				name:{0},
				child:0,
			},
			L: {
				currentPage:CORE,
				Page: {
					{
						pageable: true,
						title: MENU_TITLE,
						max: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
					{
						pageable: false,
						title: NULL,
						max: {
							cols:0,
							rows:0,
						},
						hScroll:0,
						vScroll:0,
					},
					{
						pageable: true,
						title: PROC_TITLE,
						max: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
					{
						pageable: true,
						title: RAM_TITLE,
						max: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
					{
						pageable: true,
						title: BIOS_TITLE,
						max: {
							cols:0,
							rows:1,
						},
						hScroll:1,
						vScroll:1,
					},
				},
				string:{0},
				wbScroll:0,
				wbLength:0,
				wallboard:NULL,
				bump:{0},
				usage:NULL,
				axes:NULL,
			},
		};
	int	rc=0;

	if(Help(&A, argc, argv))
	{
		// Read the CPU Features.
		CPUID(&A.P.Features);
		// Find the Processor Architecture.
		for(A.P.ArchID=ARCHITECTURES; A.P.ArchID >=0 ; A.P.ArchID--)
				if(!(ARCH[A.P.ArchID].Signature.ExtFamily ^ A.P.Features.Std.EAX.ExtFamily)
				&& !(ARCH[A.P.ArchID].Signature.Family ^ A.P.Features.Std.EAX.Family)
				&& !(ARCH[A.P.ArchID].Signature.ExtModel ^ A.P.Features.Std.EAX.ExtModel)
				&& !(ARCH[A.P.ArchID].Signature.Model ^ A.P.Features.Std.EAX.Model))
					break;
		if(!A.P.PerCore) {
			if( (A.P.ThreadCount=Get_ThreadCount()) == 0)
				// Fallback to the CPUID fixed count of threads if unavailable from BIOS.
				A.P.ThreadCount=A.P.Features.ThreadCount;
		}
		else
			if( (A.P.ThreadCount=Get_CoreCount()) == 0)
				A.P.ThreadCount=ARCH[A.P.ArchID].MaxOfCores;
		// Allocate the Cores working structure.
		A.P.Core=malloc(A.P.ThreadCount * sizeof(struct THREADS));
		// Open once the MSR gate.
		if( Open_MSR(&A) )
		{
			// Read the bus clock frequency from the BIOS.
			if( (A.P.ClockSpeed=Get_ExternalClock()) == 0)
				// Fallback first to an estimated clock frequency.
				if((A.P.ClockSpeed=FallBack_Freq() / A.P.Platform.MaxNonTurboRatio) == 0)
					// Fallback at least to the default clock.
					if(A.P.ArchID != -1)
						A.P.ClockSpeed=ARCH[A.P.ArchID].ClockSpeed;

			// Read the Integrated Memory Controler information.
			A.M=IMC_Read_Info();

			// Initialize & run the Widget.
			if(XInitThreads() && OpenLayout(&A))
			{
				BuildLayout(&A);
				MapLayout(&A);
				DrawLayout(&A);
				FlushLayout(&A);
				XMapWindow(A.W.display, A.W.window);

				pthread_t TID_Cycle=0/*, TID_Draw=0*/;
				if(!pthread_create(&TID_Cycle, NULL, uCycle, &A)) {
					uLoop((void*) &A);
					pthread_join(TID_Cycle, NULL);
				}
				else rc=2;

				CloseLayout(&A);
			}
			else	rc=2;

			// Release the ressources.
			IMC_Free_Info(A.M);
			Close_MSR(&A);
		}
		else	rc=2;

		free(A.P.Core);
	}
	else	rc=1;
	return(rc);
}

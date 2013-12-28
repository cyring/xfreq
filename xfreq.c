/*
 * XFreq.c #0.08 by CyrIng
 *
 * Copyright (C) 2013 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "xfreq.h"


void	Open_MSR(uARG *A) {
	A->P.Core=malloc(A->P.Features.ThreadCount * sizeof(struct CORE));

	char	pathname[]="/dev/cpu/999/msr";
	int	cpu=0;
	for(cpu=0; cpu < A->P.Features.ThreadCount; cpu++) {
		sprintf(pathname, "/dev/cpu/%d/msr", cpu);
		A->P.Core[cpu].FD=open(pathname, O_RDONLY);
	}
}

void	Close_MSR(uARG *A) {
	int	cpu=0;
	for(cpu=0; cpu < A->P.Features.ThreadCount; cpu++) {
		close(A->P.Core[cpu].FD);
	}
	free(A->P.Core);
}

int	Read_MSR(uARG *A, int cpu, off_t offset, unsigned long long *msr) {
	ssize_t	retval=pread(A->P.Core[cpu].FD, msr, sizeof *msr, offset);
	return((retval != sizeof *msr) ? -1 : 0);
}

int	Read_SMBIOS(int structure, int instance, off_t offset, void *buf, size_t nbyte) {
	ssize_t	retval=0;
	char	pathname[]="/sys/firmware/dmi/entries/999-99/raw";
	int	fd=0, rc=-1;

	sprintf(pathname, "/sys/firmware/dmi/entries/%d-%d/raw", structure, instance);
	if( (fd=open(pathname, O_RDONLY)) != -1 ) {
		retval=pread(fd, buf, nbyte, offset);
		close(fd);
		rc=(retval != nbyte) ? -1 : 0;
	}
	return(rc);
}

PLATFORM Get_Platform_Info(uARG *A, int cpu) {
	PLATFORM msr={0};

	if( Read_MSR(A, cpu, MSR_PLATFORM_INFO, (unsigned long long *) &msr) != -1)
		return(msr);
	else
		return((PLATFORM) {0});
}

int	Get_Ratio(uARG *A, int cpu) {
	unsigned long long msr=0;

	if( Read_MSR(A, cpu, IA32_PERF_STATUS, &msr) != -1)
		return((int) msr);
	else
		return(0);
}

TURBO	Get_Turbo(uARG *A, int cpu) {
	TURBO msr={0};

	if( Read_MSR(A, cpu, MSR_TURBO_RATIO_LIMIT, (unsigned long long *) &msr) != -1)
		return(msr);
	else
		return((TURBO) {0});
}

int	Get_Temperature(uARG *A, int cpu) {
	TEMP	Temp={0};
	THERM	Therm={0};

	if((Read_MSR(A, cpu, MSR_TEMPERATURE_TARGET, (unsigned long long *) &Temp) != -1)
	&& (Read_MSR(A, cpu, IA32_THERM_STATUS, (unsigned long long *) &Therm) != -1))
		return(Temp.Target - Therm.DTS);
	else
		return(0);
}

int	External_Clock() {
	int	BCLK=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE, \
			SMBIOS_PROCINFO_INSTANCE, \
			SMBIOS_PROCINFO_EXTCLK, &BCLK, 1) != -1)
		return(BCLK);
	else
		return(0);
}

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
			} EAX,
			EBX,
			ECX,
			EDX;
		} Brand;
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
					features->BrandString[px]=Brand.EAX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					features->BrandString[px]=Brand.EBX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					features->BrandString[px]=Brand.ECX.Chr[jx];
				for(jx=0; jx<4; jx++, px++)
					features->BrandString[px]=Brand.EDX.Chr[jx];
		}
	}
}

void	BuildLayout(uARG *A) {
	A->L.usage=malloc(A->P.Features.ThreadCount * sizeof(XRectangle));
	A->L.axes=malloc(A->P.Turbo.MaxRatio_1C * sizeof(XSegment));

	sprintf(A->L.bclock, BCLOCK, A->P.ClockSpeed);
	sprintf(A->L.ratios, "%02d%02d%02d",	A->P.Platform.MinimumRatio,
						A->P.Platform.MaxNonTurboRatio,
						A->P.Turbo.MaxRatio_1C);

	int i=0, j=A->W.extents.overall.width / A->P.Turbo.MaxRatio_1C;
	for(i=0; i <= A->P.Turbo.MaxRatio_1C; i++) {
		A->L.axes[i].x1 = A->W.margin.width + (j * i) \
				+ A->W.extents.font->max_bounds.rbearing \
				- A->W.extents.font->min_bounds.lbearing;
		A->L.axes[i].y1 = A->W.margin.height \
				+ (A->W.extents.ascent + A->W.extents.descent);
		A->L.axes[i].x2 = A->L.axes[i].x1;
		A->L.axes[i].y2 = A->W.margin.height \
				+ ((A->P.Features.ThreadCount + 1) \
				* ( A->W.extents.ascent + A->W.extents.descent));
	}
}

void	CloseLayout(uARG *A) {
	free(A->L.axes);
	free(A->L.usage);
}

void	DrawLayout(uARG *A) {
	XSetForeground(A->W.display, A->W.gc, 0x666666);

	XDrawSegments(	A->W.display, \
			A->W.window, \
			A->W.gc, \
			A->L.axes, \
			A->P.Turbo.MaxRatio_1C + 1);

	XSetForeground(A->W.display, A->W.gc, A->W.foreground);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width, \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent), \
		"Core", 4);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width \
		+ ( A->W.extents.font->max_bounds.rbearing  \
		  - A->W.extents.font->min_bounds.lbearing) \
		* ( A->P.Turbo.MaxRatio_1C >> 1), \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent), \
		A->L.bclock, strlen(A->L.bclock));

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width \
		+ A->W.extents.overall.width \
		- ((A->W.extents.font->max_bounds.rbearing \
		  - A->W.extents.font->min_bounds.lbearing)), \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent), \
		"Temps", 5);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width, \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent) \
		* ( A->P.Features.ThreadCount + 1 + 1), \
		"Ratio", 5);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width \
		+ ( A->W.extents.font->max_bounds.rbearing  \
		  - A->W.extents.font->min_bounds.lbearing) \
		* ( A->P.Platform.MinimumRatio << 1), \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent) \
		* ( A->P.Features.ThreadCount + 1 + 1), \
		&A->L.ratios[0], 2);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width \
		+ ( A->W.extents.font->max_bounds.rbearing  \
		  - A->W.extents.font->min_bounds.lbearing) \
		* ( A->P.Platform.MaxNonTurboRatio << 1), \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent) \
		* ( A->P.Features.ThreadCount + 1 + 1), \
		&A->L.ratios[2], 2);

	XDrawString(A->W.display, \
		A->W.window, \
		A->W.gc, \
		A->W.margin.width \
		+ ( A->W.extents.font->max_bounds.rbearing  \
		  - A->W.extents.font->min_bounds.lbearing) \
		* ( A->P.Turbo.MaxRatio_1C << 1), \
		A->W.margin.height \
		+ ( A->W.extents.ascent \
		  + A->W.extents.descent) \
		* ( A->P.Features.ThreadCount + 1 + 1), \
		&A->L.ratios[4], 2);

	XFillRectangles(A->W.display, \
			A->W.window, \
			A->W.gc, \
			A->L.usage, \
			A->P.Features.ThreadCount);
	int cpu=0;
	for(cpu=0; cpu < A->P.Features.ThreadCount; cpu++) {
		sprintf(A->L.string, FREQ, cpu, A->P.Core[cpu].Freq);

		if(A->P.Core[cpu].Ratio >= A->P.Platform.MaxNonTurboRatio)
			XSetForeground(A->W.display, A->W.gc, 0xffff00);

		XDrawImageString(A->W.display, \
				A->W.window, \
				A->W.gc, \
				A->W.margin.width, \
				A->W.margin.height \
				+ ( A->W.extents.ascent \
				  + A->W.extents.descent) \
				* (cpu + 1 + 1), \
				A->L.string, strlen(A->L.string));

		XSetForeground(A->W.display, A->W.gc, A->W.foreground);

		sprintf(A->L.string, "%2d", A->P.Core[cpu].Temp);
		XDrawString(A->W.display, \
				A->W.window, \
				A->W.gc, \
				A->W.margin.width \
				+ A->W.extents.overall.width \
				+ ((A->W.extents.font->max_bounds.rbearing \
				  - A->W.extents.font->min_bounds.lbearing) << 1), \
				A->W.margin.height \
				+ ( A->W.extents.ascent \
				  + A->W.extents.descent) \
				* (cpu + 1 + 1), \
				A->L.string, 2);
	}
	XDrawRectangles(A->W.display, \
			A->W.window, \
			A->W.gc, \
			A->L.usage, \
			A->P.Features.ThreadCount);

	sprintf(A->L.string, TITLE, \
		A->P.Top, A->P.Core[A->P.Top].Freq);
	XStoreName(A->W.display, A->W.window, A->L.string);
}

static void *uExec(void *uArg) {
	uARG *A=(uARG *) uArg;

	int cpu=0, max=0;
	while(A->LOOP) {
		for(cpu=0, max=0; cpu < A->P.Features.ThreadCount; cpu++) {
			A->P.Core[cpu].Ratio=Get_Ratio(A, cpu);
			A->P.Core[cpu].Freq=A->P.Core[cpu].Ratio * A->P.ClockSpeed;
			if(max < A->P.Core[cpu].Ratio) {
				max=A->P.Core[cpu].Ratio;
				A->P.Top=cpu;
			}
			A->P.Core[cpu].Temp=Get_Temperature(A, cpu);

			A->L.usage[cpu].x=	A->W.margin.width;

			A->L.usage[cpu].y=	A->W.margin.height + 3 \
						+ ( A->W.extents.ascent \
						  + A->W.extents.descent) \
						* (cpu + 1);

			A->L.usage[cpu].width=	( A->W.extents.overall.width \
						* A->P.Core[cpu].Ratio) \
						/ A->P.Turbo.MaxRatio_1C;

			A->L.usage[cpu].height=	  A->W.extents.ascent \
						+ A->W.extents.descent - 3;
		}
		XClientMessageEvent E={type:ClientMessage, window:A->W.window, format:32};
		XLockDisplay(A->W.display);
		XSendEvent(A->W.display, A->W.window, 0, 0, (XEvent *)&E);
		XUnlockDisplay(A->W.display);

		usleep(750000);
	}
	return(NULL);
}

static void *uLoop(void *uArg) {
	uARG *A=(uARG *) uArg;

	XEvent	E={0};
	while(A->LOOP) {
		XLockDisplay(A->W.display);
		if(XPending(A->W.display) > 0) {
			XNextEvent(A->W.display, &E);
			switch(E.type) {
				case ClientMessage:
					if(E.xclient.send_event) {
						XClearArea(A->W.display, A->W.window, \
							A->W.margin.width, \
							A->W.margin.height \
							+ (A->W.extents.ascent + A->W.extents.descent), \
							A->W.extents.overall.width \
							+ ((A->W.extents.font->max_bounds.rbearing \
							-   A->W.extents.font->min_bounds.lbearing) << 2), \
							1 +(A->W.extents.ascent + A->W.extents.descent) \
								* (A->P.Features.ThreadCount), \
							false);
					}
					else break;
				case Expose: {
					if(!E.xexpose.count)
						DrawLayout(A);
				}
					break;
				case KeyPress:
					switch(XLookupKeysym(&E.xkey, 0)) {
						case XK_Escape:
							A->LOOP=false;
							break;
					}
					break;
				default:
					break;
			}
		}
		XUnlockDisplay(A->W.display);
		usleep(100000);
	}
	return(NULL);
}

void Help(OPTION *options, char *argv[])
{
	const int s=sizeof(options)/sizeof(OPTION);
	int i=0;
	printf("Usage: %s [OPTION...]\n\n", argv[0]);
	for(i=0; i < s; i++)
		printf("\t%s\t%s\n", options[i].argument, options[i].manual);
	printf("\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n\nReport bugs to webmaster@cyring.fr\n");
}

int main(int argc, char *argv[]) {
	uARG	A= {
			LOOP: true,
			W: {
				display:NULL,
				window:0,
				screen:NULL,
				gc:0,
				x:0,
				y:0,
				width:0,
				height:0,
				margin: {
					width :4,
					height:2,
					},
				extents: {
					fname:"7x13",
					font:NULL,
					overall:{0},
					dir:0,
					ascent:0,
					descent:0,
				},
				background:0x333333,
				foreground:0x8fcefa,
			},
			L: {
				string:{0},
				bclock:{0},
				ratios:{0},
			},
		};

	OPTION	options[] = {	{"-x", "%d", &A.W.x,             "Left position"},
				{"-y", "%d", &A.W.y,             "Top position" },
				{"-w", "%d", &A.W.margin.width,  "Margin-width" },
				{"-h", "%d", &A.W.margin.height, "Margin-height"},
				{"-F", "%s", &A.W.extents.fname, "Font name"},
				{"-b", "%x", &A.W.background,    "Background color"},
				{"-f", "%x", &A.W.foreground,    "Foreground color"},
		};
	const int s=sizeof(options)/sizeof(OPTION);
	
	if(argc - ((argc >> 1) << 1)) {
		int i=0, j=0;
		for(j=1; j < argc; j+=2) {
			for(i=0; i < s; i++)
				if(!strcmp(argv[j], options[i].argument)) {
					sscanf(argv[j+1], options[i].format, options[i].pointer);
					break;
				}
			if(i == s) {
				Help(options, argv);
				return(1);
			}
		}
	}
	else {
		Help(options, argv);
		return(1);
	}

	CPUID(&A.P.Features);
	Open_MSR(&A);

	A.P.Platform=Get_Platform_Info(&A, 0);
	A.P.Turbo=Get_Turbo(&A, 0);
	A.P.ClockSpeed=External_Clock();

	if( XInitThreads()
	&& (A.W.display=XOpenDisplay(NULL))
	&& (A.W.screen=DefaultScreenOfDisplay(A.W.display))
	&& (A.W.window=XCreateSimpleWindow(	A.W.display, \
						DefaultRootWindow(A.W.display), \
						A.W.x, A.W.y, 1, 1, \
						0, BlackPixelOfScreen(A.W.screen), \
						WhitePixelOfScreen(A.W.screen)))
	&& (A.W.gc=XCreateGC(A.W.display, A.W.window, 0, NULL)))
	{
		if((A.W.extents.font = XLoadQueryFont(A.W.display, A.W.extents.fname)))
			XSetFont(A.W.display, A.W.gc, A.W.extents.font->fid);

		XTextExtents(	A.W.extents.font, HDSIZE, A.P.Turbo.MaxRatio_1C << 1, \
				&A.W.extents.dir, &A.W.extents.ascent, \
				&A.W.extents.descent, &A.W.extents.overall);

		A.W.width=	(   A.W.margin.width << 1) \
				+   A.W.extents.overall.width \
				+ ((A.W.extents.font->max_bounds.rbearing \
				-   A.W.extents.font->min_bounds.lbearing) << 2);

		A.W.height=	(  A.W.margin.height << 1) \
				+ (A.W.extents.ascent + A.W.extents.descent) \
				* (A.P.Features.ThreadCount + 1 + 1);

		XResizeWindow(A.W.display, A.W.window, A.W.width, A.W.height);

		XSetWindowBackground(A.W.display, A.W.window, A.W.background);
		XSetBackground(A.W.display, A.W.gc, A.W.background);
		XSetForeground(A.W.display, A.W.gc, A.W.foreground);

		BuildLayout(&A);

		XSelectInput(A.W.display, A.W.window, ExposureMask | KeyPressMask);
		XMapWindow(A.W.display, A.W.window);

		pthread_t TID_Loop=0, TID_Exec=0;
		if(!pthread_create(&TID_Loop, NULL, uLoop, &A)
		&& !pthread_create(&TID_Exec, NULL, uExec, &A))
			pthread_join(TID_Loop, NULL);

		CloseLayout(&A);

		XUnloadFont(A.W.display, A.W.extents.font->fid);
		XFreeGC(A.W.display, A.W.gc);
		XDestroyWindow(A.W.display, A.W.window);
		XCloseDisplay(A.W.display);
	}
	Close_MSR(&A);

	return(0);
}

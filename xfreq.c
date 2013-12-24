/*
 * XFreq.c #0.05 by CyrIng
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

int	Read_MSR(int cpu, off_t offset, unsigned long long *msr) {
	ssize_t	retval=0;
	char	pathname[32]="";
	int		fd=0, rc=-1;

	sprintf(pathname, "/dev/cpu/%d/msr", cpu);
	if( (fd=open(pathname, O_RDONLY)) != -1 ) {
		retval=pread(fd, msr, sizeof *msr, offset);
		close(fd);
		rc=(retval != sizeof *msr) ? -1 : 0;
	}
	return(rc);
}

int	Read_SMBIOS(int structure, int instance, off_t offset, void *buf, size_t nbyte) {
	ssize_t	retval=0;
	char	pathname[]="/sys/firmware/dmi/entries/999-99/raw";
	int		fd=0, rc=-1;

	sprintf(pathname, "/sys/firmware/dmi/entries/%d-%d/raw", structure, instance);
	if( (fd=open(pathname, O_RDONLY)) != -1 ) {
		retval=pread(fd, buf, nbyte, offset);
		close(fd);
		rc=(retval != nbyte) ? -1 : 0;
	}
	return(rc);
}

PLATFORM Get_Platform_Info(int cpu) {
	PLATFORM msr={0};

	if( Read_MSR(cpu, MSR_PLATFORM_INFO, (unsigned long long *) &msr) != -1)
		return(msr);
	else
		return((PLATFORM) {0});
}

int	Get_Ratio(int cpu) {
	unsigned long long msr=0;

	if( Read_MSR(cpu, IA32_PERF_STATUS, &msr) != -1)
		return((int) msr);
	else
		return(0);
}

TURBO	Get_Turbo(int cpu) {
	TURBO msr={0};

	if( Read_MSR(cpu, MSR_TURBO_RATIO_LIMIT, (unsigned long long *) &msr) != -1)
		return(msr);
	else
		return((TURBO) {0});
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


static void *uExec(void *uArg) {
	uARG *A=(uARG *) uArg;

	int cpu=0, max=0;
	while(A->LOOP) {
		for(cpu=0, max=0; cpu < A->P.Features.ThreadCount; cpu++) {
			A->P.Core[cpu].Ratio=Get_Ratio(cpu);
			A->P.Core[cpu].Freq=A->P.Core[cpu].Ratio * A->P.ClockSpeed;
			if(max < A->P.Core[cpu].Ratio) {
				max=A->P.Core[cpu].Ratio;
				A->P.Top=cpu;
			}
		}
		XClientMessageEvent E={type:ClientMessage, window:A->W.window, format:32};
		XLockDisplay(A->W.display);
		XSendEvent(A->W.display, A->W.window, 0, 0, (XEvent *)&E);
		XUnlockDisplay(A->W.display);

		usleep(1500000);
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
							   A->W.margin.width, A->W.margin.height, \
							   A->W.width, A->W.height+1, false);
					}
					else break;
				case Expose: {
					if(!E.xexpose.count) {
						XDrawImageString(A->W.display, \
								A->W.window, \
								A->W.gc, \
								A->W.margin.width, \
								A->W.margin.height \
								+ ( \
									A->W.extents.ascent \
									+ A->W.extents.descent \
								), \
								A->L.header, A->L.cols);
						int cpu=0;
						for(cpu=0; cpu < A->P.Features.ThreadCount; cpu++) {
							sprintf(A->L.string, A->L.format,
								cpu, A->P.Core[cpu].Freq);

							XDrawImageString(A->W.display, \
									 A->W.window, \
									 A->W.gc, \
									 A->W.margin.width, \
									 A->W.margin.height \
									 + ( \
									     A->W.extents.ascent \
									   + A->W.extents.descent \
									   ) \
									 * (cpu + 1 + 1), \
									 A->L.string, strlen(A->L.string));

							A->W.rectangle[cpu].x=A->W.margin.width;
							A->W.rectangle[cpu].y=A->W.margin.height+3 \
									  + ( \
									      A->W.extents.ascent \
									    + A->W.extents.descent \
									    ) \
									  * (cpu + 1);
							A->W.rectangle[cpu].width=( \
										    A->W.width \
										  * A->P.Core[cpu].Ratio \
										  ) / 24;
							A->W.rectangle[cpu].height=A->W.extents.ascent \
									         + A->W.extents.descent-3;
						}
						XDrawRectangles(A->W.display, \
								A->W.window, \
								A->W.gc, \
								A->W.rectangle, \
								A->P.Features.ThreadCount);
						sprintf(A->L.string, TITLE, \
							A->P.Top, A->P.Core[A->P.Top].Freq);
						XStoreName(A->W.display, A->W.window, A->L.string);
					}
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

int main(int argc, char *argv[]) {
	if(argc >= 5) {
		uARG	A= {
			    LOOP: true,
			    W: {
			        display:NULL,
				window:0,
				screen:NULL,
				gc:0,
				x:atoi(argv[1]),
				y:atoi(argv[2]),
				width:atoi(argv[3]),
				height:atoi(argv[4]),
				margin: {
					width :0,
					height:0,
					},
				extents: {
					font:NULL,
					overall:{0},
					dir:0,
					ascent:0,
					descent:0,
				},
			    },
			    L: {
				hdmask:{0},
			        header:{0},
				string:{0},
				format:FORMAT,
			    },
			};

		CPUID(&A.P.Features);
		A.P.Platform=Get_Platform_Info(0);
		A.P.Turbo=Get_Turbo(0);
		A.P.ClockSpeed=External_Clock();
		A.P.Core=malloc(A.P.Features.ThreadCount * sizeof(struct CORE));

		sprintf(A.L.hdmask, HEADER,
			 A.P.Platform.MinimumRatio<<1,
			(A.P.Platform.MaxNonTurboRatio - A.P.Platform.MinimumRatio)<<1,
			(A.P.Turbo.MaxRatio_1C - A.P.Platform.MaxNonTurboRatio)<<1);
		sprintf(A.L.header, A.L.hdmask,
			A.P.Platform.MinimumRatio,
			A.P.Platform.MaxNonTurboRatio,
			A.P.Turbo.MaxRatio_1C);
		memmove(A.L.header, HDINFO, sizeof(HDINFO)-1);

		A.L.cols=strlen(A.L.header);
		A.L.rows=A.P.Features.ThreadCount + 1;	// + header line;

		if( XInitThreads()
		&& (A.W.display=XOpenDisplay(NULL))
		&& (A.W.screen=DefaultScreenOfDisplay(A.W.display))
		&& (A.W.window=XCreateSimpleWindow(A.W.display, \
						   DefaultRootWindow(A.W.display), \
						   A.W.x, A.W.y, A.W.width, A.W.height, \
						   0, BlackPixelOfScreen(A.W.screen), \
						   WhitePixelOfScreen(A.W.screen)))
		&& (A.W.gc=XCreateGC(A.W.display, A.W.window, 0, NULL)))
		{
			if(argc > 5)
				A.W.extents.font = XLoadQueryFont(A.W.display, argv[5]);
			else
				A.W.extents.font = XLoadQueryFont(A.W.display, "7x13");
			if(A.W.extents.font)
				XSetFont(A.W.display, A.W.gc, A.W.extents.font->fid);

			if(argc > 6) {
				XSetWindowBackground(A.W.display, A.W.window, strtod(argv[6], NULL));
				XSetBackground(A.W.display, A.W.gc, strtod(argv[6], NULL));
			}
			else
				XSetBackground(A.W.display, A.W.gc, WhitePixelOfScreen(A.W.screen));
			if(argc > 7)
				XSetForeground(A.W.display, A.W.gc, strtod(argv[7], NULL));
			else
				XSetForeground(A.W.display, A.W.gc, BlackPixelOfScreen(A.W.screen));

			if(argc > 8)
				A.W.margin.width=atoi(argv[8]);
			if(argc > 9)
				A.W.margin.height=atoi(argv[9]);

			XTextExtents(	A.W.extents.font, A.L.header, A.L.cols, \
					&A.W.extents.dir, &A.W.extents.ascent, \
					&A.W.extents.descent, &A.W.extents.overall);

			A.W.width=A.W.extents.overall.width;
			A.W.height=(A.W.extents.ascent + A.W.extents.descent) * A.L.rows;
			
			A.W.rectangle=malloc(A.P.Features.ThreadCount * sizeof(XRectangle));

			XSelectInput(A.W.display, A.W.window, ExposureMask | KeyPressMask);
			XMapWindow(A.W.display, A.W.window);

			pthread_t TID_Loop=0, TID_Exec=0;
			if(!pthread_create(&TID_Loop, NULL, uLoop, &A)
			&& !pthread_create(&TID_Exec, NULL, uExec, &A))
				pthread_join(TID_Loop, NULL);

			free(A.W.rectangle);

			XUnloadFont(A.W.display, A.W.extents.font->fid);
			XFreeGC(A.W.display, A.W.gc);
			XDestroyWindow(A.W.display, A.W.window);
			XCloseDisplay(A.W.display);
		}
		free(A.P.Core);
	}
	else {
		fprintf(stderr, "Usage  : %s <X> <Y> <Width> <Height> [OPTIONS]\n" \
				"Options:\n" \
				"\t<Font>\n" \
				"\t<Background>\n" \
				"\t<Foreground>\n" \
				"\t<Margin-width>\n" \
				"\t<Margin-height>\n", argv[0]);
		return(1);
	}
	return(0);
}

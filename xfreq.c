/*
 * XFreq.c #0.10 by CyrIng
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
	A->P.Core=malloc(A->P.ThreadCount * sizeof(struct CORE));

	char	pathname[]="/dev/cpu/999/msr";
	int	cpu=0;
	for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
		sprintf(pathname, "/dev/cpu/%d/msr", cpu);
		A->P.Core[cpu].FD=open(pathname, O_RDONLY);
		Read_MSR(A->P.Core[cpu].FD, MSR_PLATFORM_INFO, (PLATFORM *) &A->P.Platform);
		Read_MSR(A->P.Core[cpu].FD, MSR_TURBO_RATIO_LIMIT, (TURBO *) &A->P.Turbo);
		Read_MSR(A->P.Core[cpu].FD, MSR_TEMPERATURE_TARGET, (TJMAX *) &A->P.Core[cpu].TjMax);
	}
}

void	Close_MSR(uARG *A) {
	int	cpu=0;
	for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
		close(A->P.Core[cpu].FD);
	}
	free(A->P.Core);
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

int	Get_ExternalClock() {
	int	BCLK=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE, \
			SMBIOS_PROCINFO_INSTANCE, \
			SMBIOS_PROCINFO_EXTCLK, &BCLK, 1) != -1)
		return(BCLK);
	else
		return(0);
}

int	Get_ThreadCount() {
	short int ThreadCount=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE, \
			SMBIOS_PROCINFO_INSTANCE, \
			SMBIOS_PROCINFO_THREADS, &ThreadCount, 1) != -1)
		return(ThreadCount);
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
			} EAX, EBX, ECX, EDX;
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

int	BuildLayout(uARG *A) {
	GC tmpGC=0;

	if((A->W.display=XOpenDisplay(NULL))
	&& (A->W.screen=DefaultScreenOfDisplay(A->W.display))
	&& (A->W.window=XCreateSimpleWindow(	A->W.display, \
						DefaultRootWindow(A->W.display), \
						A->W.x, A->W.y, A->W.width, A->W.height, \
						0, BlackPixelOfScreen(A->W.screen), \
						A->W.background)))
	{
		if((tmpGC=XCreateGC(A->W.display, A->W.window, 0, NULL))
		&& (A->W.extents.font = XLoadQueryFont(A->W.display, A->W.extents.fname)))
		{
				XSetFont(A->W.display, tmpGC, A->W.extents.font->fid);

				XTextExtents(	A->W.extents.font, HDSIZE, A->P.Turbo.MaxRatio_1C << 1, \
						&A->W.extents.dir, &A->W.extents.ascent, \
						&A->W.extents.descent, &A->W.extents.overall);

				A->W.extents.charWidth=A->W.extents.font->max_bounds.rbearing - A->W.extents.font->min_bounds.lbearing;
				A->W.extents.charHeight=A->W.extents.ascent + A->W.extents.descent;

				A->W.width=(A->W.margin.width << 1) + A->W.extents.overall.width + (A->W.extents.charWidth << 2);
				A->W.height=(A->W.margin.height << 1) + A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1);

				XUnloadFont(A->W.display, A->W.extents.font->fid);
				XFreeGC(A->W.display, tmpGC);
		}
		else
			return(false);

		if((A->W.pixmap.B=XCreatePixmap(A->W.display, A->W.window, A->W.width, A->W.height, DefaultDepthOfScreen(A->W.screen)))
		&& (A->W.pixmap.F=XCreatePixmap(A->W.display, A->W.window, A->W.width, A->W.height, DefaultDepthOfScreen(A->W.screen)))
		&& (A->W.gc=XCreateGC(A->W.display, A->W.pixmap.F, 0, NULL))
		&& (A->W.extents.font = XLoadQueryFont(A->W.display, A->W.extents.fname)))
		{
			XSetFont(A->W.display, A->W.gc, A->W.extents.font->fid);

			A->L.usage=malloc(A->P.ThreadCount * sizeof(XRectangle));
			A->L.axes=malloc(A->P.Turbo.MaxRatio_1C * sizeof(XSegment));

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

			XSetBackground(A->W.display, A->W.gc, A->W.background);

			sprintf(A->L.bclock, BCLOCK, A->P.ClockSpeed);
			sprintf(A->L.ratios, "%02d%02d%02d",	A->P.Platform.MinimumRatio,
								A->P.Platform.MaxNonTurboRatio,
								A->P.Turbo.MaxRatio_1C);

			int i=0, j=A->W.extents.overall.width / A->P.Turbo.MaxRatio_1C;
			for(i=1; i <= A->P.Turbo.MaxRatio_1C; i++) {
				A->L.axes[i].x1 = A->W.margin.width + (j * i) + (A->W.extents.charWidth >> 1);
				A->L.axes[i].y1 = 3 + A->W.margin.height + A->W.extents.charHeight;
				A->L.axes[i].x2 = A->L.axes[i].x1;
				A->L.axes[i].y2 = A->W.margin.height + ((A->P.ThreadCount + 1) * A->W.extents.charHeight) - 3;
			}

			XSetForeground(A->W.display, A->W.gc, A->W.background);
			XFillRectangle(A->W.display, A->W.pixmap.B, A->W.gc, 0, 0, A->W.width, A->W.height);
			XSetForeground(A->W.display, A->W.gc, 0x666666);

			XDrawSegments(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->L.axes, \
				A->P.Turbo.MaxRatio_1C + 1);

			XSetForeground(A->W.display, A->W.gc, A->W.foreground);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width, \
				A->W.margin.height + A->W.extents.charHeight, \
				"Core", 4);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Turbo.MaxRatio_1C >> 1) ), \
				A->W.margin.height + A->W.extents.charHeight, \
				A->L.bclock, strlen(A->L.bclock));

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width + A->W.extents.overall.width - A->W.extents.charWidth, \
				A->W.margin.height + A->W.extents.charHeight, \
				"Temps", 5);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width, \
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ), \
				"Ratio", 5);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MinimumRatio << 1) ), \
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ), \
				&A->L.ratios[0], 2);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MaxNonTurboRatio << 1) ), \
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ), \
				&A->L.ratios[2], 2);

			XDrawString(	A->W.display, \
				A->W.pixmap.B, \
				A->W.gc, \
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Turbo.MaxRatio_1C << 1) ), \
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ), \
				&A->L.ratios[4], 2);

			XSelectInput(A->W.display, A->W.window, VisibilityChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);
			XMapWindow(A->W.display, A->W.window);
		}
		else
			return(false);
	}
	return(true);
}

void	CloseLayout(uARG *A) {
	free(A->L.axes);
	free(A->L.usage);

	XUnloadFont(A->W.display, A->W.extents.font->fid);
	XFreeGC(A->W.display, A->W.gc);
	XFreePixmap(A->W.display, A->W.pixmap.B);
	XFreePixmap(A->W.display, A->W.pixmap.F);
	XDestroyWindow(A->W.display, A->W.window);
	XCloseDisplay(A->W.display);
}

void	MapLayout(uARG *A) {
	if((A->D.window != (XID)-1)
	&& XGetWindowAttributes(A->W.display, A->W.window, &A->W.attribs)
	&& XTranslateCoordinates(A->W.display, A->W.window, A->D.window,
				0, 0, &A->W.x, &A->W.y, &A->D.child) )
	{
		XCopyArea(A->W.display, A->D.window, A->W.pixmap.B, A->W.gc, \
				A->W.x, A->W.y, \
				A->W.attribs.width, A->W.attribs.height, \
				0, 0);
	}
	{
		XCopyArea(A->W.display, A->W.pixmap.B, A->W.pixmap.F, A->W.gc, \
		0, 0, \
		A->W.width, A->W.height, \
		0, 0);
	}
}

void	DrawPulse(uARG *A) {
	XSetForeground(A->W.display, A->W.gc, (A->W.pulse=!A->W.pulse) ? 0xff0000:A->W.foreground);
	XDrawArc(A->W.display, A->W.window, A->W.gc, \
		A->W.width - (A->W.extents.charWidth + (A->W.extents.charWidth >> 1)), \
		A->W.height - (A->W.extents.charHeight - (A->W.extents.charWidth >> 1)), \
		A->W.extents.charWidth, \
		A->W.extents.charWidth, \
		0, 360 << 8);
}

void	DrawLayout(uARG *A) {
	int cpu=0;
	for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
			A->L.usage[cpu].x=A->W.margin.width;
			A->L.usage[cpu].y=3 + A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1) );
			A->L.usage[cpu].width=(A->W.extents.overall.width * A->P.Core[cpu].Ratio) / A->P.Turbo.MaxRatio_1C;
			A->L.usage[cpu].height=A->W.extents.charHeight - 3;

		if(A->P.Core[cpu].Ratio >= A->P.Platform.MinimumRatio)
			XSetForeground(A->W.display, A->W.gc, 0x009966);
		if(A->P.Core[cpu].Ratio >= A->P.Platform.MaxNonTurboRatio)
			XSetForeground(A->W.display, A->W.gc, 0xffa500);
		if(A->P.Core[cpu].Ratio >= A->P.Turbo.MaxRatio_4C)
			XSetForeground(A->W.display, A->W.gc, 0xffff00);

		XFillRectangle(A->W.display, \
				A->W.pixmap.F, \
				A->W.gc, \
				A->L.usage[cpu].x, \
				A->L.usage[cpu].y, \
				A->L.usage[cpu].width, \
				A->L.usage[cpu].height);

		XSetForeground(A->W.display, A->W.gc, A->W.foreground);

		sprintf(A->L.string, FREQ, cpu, A->P.Core[cpu].Freq);
		XDrawImageString(A->W.display, \
				A->W.pixmap.F, \
				A->W.gc, \
				A->W.margin.width, \
				A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ), \
				A->L.string, strlen(A->L.string));

		sprintf(A->L.string, "%2d", A->P.Core[cpu].TjMax.Target - A->P.Core[cpu].Therm.DTS);
		XDrawString(A->W.display, \
				A->W.pixmap.F, \
				A->W.gc, \
				A->W.margin.width + A->W.extents.overall.width + (A->W.extents.charWidth << 1), \
				A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ), \
				A->L.string, 2);
	}
	XSetForeground(A->W.display, A->W.gc, 0x666666);
	XDrawRectangles(A->W.display, \
			A->W.pixmap.F, \
			A->W.gc, \
			A->L.usage, \
			A->P.ThreadCount);

	sprintf(A->L.string, TITLE, \
		A->P.Top, A->P.Core[A->P.Top].Freq, A->P.Core[A->P.Top].TjMax.Target - A->P.Core[A->P.Top].Therm.DTS);
	XStoreName(A->W.display, A->W.window, A->L.string);
	XSetIconName(A->W.display, A->W.window, A->L.string);
}

static void *uExec(void *uArg) {
	uARG *A=(uARG *) uArg;

	int cpu=0, max=0;
	while(A->LOOP) {
		if(!A->PAUSE) {
			for(cpu=0, max=0; cpu < A->P.ThreadCount; cpu++) {
				Read_MSR(A->P.Core[cpu].FD, IA32_PERF_STATUS, (unsigned long long *) &A->P.Core[cpu].Ratio);
				A->P.Core[cpu].Freq=A->P.Core[cpu].Ratio * A->P.ClockSpeed;
				if(max < A->P.Core[cpu].Ratio) {
					max=A->P.Core[cpu].Ratio;
					A->P.Top=cpu;
				}
				Read_MSR(A->P.Core[cpu].FD, IA32_THERM_STATUS, (THERM *) &A->P.Core[cpu].Therm);
			}
//			XLockDisplay(A->W.display);
			XClientMessageEvent E={type:Expose, window:A->W.window, format:32};
			XSendEvent(A->W.display, A->W.window, 0, 0, (XEvent *)&E);
			XFlush(A->W.display);
//			XUnlockDisplay(A->W.display);
// 			printf("DEBUG:uExec[MSG_EVENT]\n");
		}
		usleep(A->P.IdleTime);
	}
	return(NULL);
}

static void *uLoop(void *uArg) {
	uARG *A=(uARG *) uArg;

	fd_set	fdset;
	int	fd=XConnectionNumber(A->W.display);
	struct	timeval timeout={tv_sec:0, tv_usec:A->P.IdleTime-1000};

	XEvent	E={0};
	while(A->LOOP) {
		FD_ZERO(&fdset);
		FD_SET(fd, &fdset);
		select(fd + 1, &fdset, NULL, NULL, &timeout);
		timeout.tv_sec=0; timeout.tv_usec=A->P.IdleTime-1000;

//		XLockDisplay(A->W.display);

		if(XPending(A->W.display) > 0) {
			XNextEvent(A->W.display, &E);
			switch(E.type) {
				case Expose: {
					if(E.xexpose.send_event) {
						MapLayout(A);
						DrawLayout(A);
//						printf("\tDEBUG:uLoop[EXPOSE:MSG_EVENT]\n");
					}
					if(!E.xexpose.count) {
						while(XCheckTypedEvent(A->W.display, Expose, &E)) ;

						XCopyArea(A->W.display,A->W.pixmap.F,A->W.window,A->W.gc, \
								0, \
								0, \
								A->W.width, \
								A->W.height, \
								0, \
								0);
// 						printf("\t\tDEBUG:uLoop[EXPOSE:COUNT]\n");
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
							XCopyArea(A->W.display,A->W.pixmap.F,A->W.window,A->W.gc, \
									0, \
									0, \
									A->W.width, \
									A->W.height, \
									0, \
									0);
							XFlush(A->W.display);
						}
							break;
					}
					break;
/*				case DestroyNotify:
					A->LOOP=false;
					break;
				case UnmapNotify:
					A->PAUSE=false;
					break;
				case MapNotify:
					A->PAUSE=true;
					break;
				case VisibilityNotify:
					switch (E.xvisibility.state) {
						case VisibilityUnobscured:
							A->PAUSE=false;
							break;
						case VisibilityPartiallyObscured:
							A->PAUSE=false;
							break;
						case VisibilityFullyObscured:
							A->PAUSE=true;
							break;
					}
					break;*/
			}
			if(A->W.activity) DrawPulse(A);
		}
//		XUnlockDisplay(A->W.display);
	}
	return(NULL);
}

int	Help(uARG *A, int argc, char *argv[]) {
	OPTION	options[] = {	{"-x", "%d", &A->W.x,             "Left position"},
				{"-y", "%d", &A->W.y,             "Top position" },
				{"-w", "%d", &A->W.margin.width,  "Margin-width" },
				{"-h", "%d", &A->W.margin.height, "Margin-height"},
				{"-F", "%s", &A->W.extents.fname, "Font name"},
				{"-b", "%x", &A->W.background,    "Background color"},
				{"-f", "%x", &A->W.foreground,    "Foreground color"},
				{"-D", "%lx",&A->D.window,        "Desktop Window id"},
				{"-s", "%ld",&A->P.IdleTime,      "Idle time (usec)"},
				{"-a", "%ld",&A->W.activity,      "Pulse activity (0/1)"},
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
		printf("\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n\nReport bugs to webmaster@cyring.fr\n");
	}
	return(noerr);
}

int main(int argc, char *argv[]) {
	uARG	A= {
			LOOP: true,
			PAUSE: false,
			P: {
				ThreadCount:0,
				Core:NULL,
				Top:0,
				ClockSpeed:0,
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
				width:360,
				height:180,
				margin: {
					width :4,
					height:2,
				},
				activity:false,
				pulse:false,
				extents: {
					fname:"7x13",
					font:NULL,
					overall:{0},
					dir:0,
					ascent:11,
					descent:2,
					charWidth:7,
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
				string:{0},
				bclock:{0},
				ratios:{0},
				usage:NULL,
				axes:NULL,
			},
		};
	if(Help(&A, argc, argv))
	{
		A.P.ThreadCount=Get_ThreadCount();
		CPUID(&A.P.Features);
		Open_MSR(&A);
		A.P.ClockSpeed=Get_ExternalClock();

		if(XInitThreads() && BuildLayout(&A))
		{
			pthread_t TID_Exec=0;
			if(!pthread_create(&TID_Exec, NULL, uExec, &A))
				uLoop((void*) &A);
			pthread_join(TID_Exec, NULL);

			CloseLayout(&A);
		}
		Close_MSR(&A);
	}
	return(0);
}

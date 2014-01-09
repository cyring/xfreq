/*
 * XFreq.c #0.11 by CyrIng
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
#include <sys/io.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "xfreq.h"

void	Open_MSR(uARG *A) {
	A->P.Core=malloc(A->P.ThreadCount * sizeof(struct THREADS));

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

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
			SMBIOS_PROCINFO_EXTCLK, &BCLK, 1) != -1)
		return(BCLK);
	else
		return(0);
}

int	Get_ThreadCount() {
	short int ThreadCount=0;

	if( Read_SMBIOS(SMBIOS_PROCINFO_STRUCTURE,
			SMBIOS_PROCINFO_INSTANCE,
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

void IMC_Free_Info(struct IMCINFO *imc)
{
	if(imc!=NULL)
	{
		if(imc->Channel!=NULL)
			free(imc->Channel);
		free(imc);
		imc = NULL;
	}
}


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
			if((tmpGC=XCreateGC(A->W.display, A->W.window, 0, NULL))) {
				if((A->W.extents.font = XLoadQueryFont(A->W.display, A->W.extents.fname)))
				{
					XSetFont(A->W.display, tmpGC, A->W.extents.font->fid);

					XTextExtents(	A->W.extents.font, HDSIZE, A->P.Turbo.MaxRatio_1C << 1,
							&A->W.extents.dir, &A->W.extents.ascent,
							&A->W.extents.descent, &A->W.extents.overall);

					A->W.extents.charWidth=A->W.extents.font->max_bounds.rbearing - A->W.extents.font->min_bounds.lbearing;
					A->W.extents.charHeight=A->W.extents.ascent + A->W.extents.descent;

					A->W.margin.width=(A->W.extents.charWidth >> 1);
					A->W.margin.height=(A->W.extents.charHeight >> 2);

					A->W.width=(A->W.margin.width << 1) + A->W.extents.overall.width + (A->W.extents.charWidth << 2);
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
			}
			else	noerr=false;

			XSelectInput(A->W.display, A->W.window, VisibilityChangeMask | ExposureMask | KeyPressMask | StructureNotifyMask);
		}
		else	noerr=false;
	}
	else	noerr=false;
	return(noerr);
}

void	CenterLayout(uARG *A) {
	A->L.Page[A->L.currentPage].hScroll=1 ;
	A->L.Page[A->L.currentPage].vScroll=1 ;
}

void	BuildLayout(uARG *A) {
	char items[1024]={0};
	int spacing=A->W.extents.charHeight;

	switch(A->L.currentPage) {
		case MENU: {
			strcpy(items, MENU_FORMAT);
			spacing=A->W.extents.charHeight  + (A->W.extents.charHeight >> 1);
		}
			break;
		case PROC: {
			sprintf(items, PROC_FORMAT,
				A->P.Features.BrandString,
				A->P.Features.Std.EAX.ExtFamily + A->P.Features.Std.EAX.Family,
				(A->P.Features.Std.EAX.ExtModel << 4) + A->P.Features.Std.EAX.Model,
				A->P.Features.Std.EAX.Stepping,
				A->P.Features.ThreadCount);
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
	XSetBackground(A->W.display, A->W.gc, A->W.background);

	if(A->L.currentPage != CORE) {
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
	else {
		sprintf(A->L.bclock, EXTCLK, A->P.ClockSpeed);
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
		XDrawSegments(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->L.axes,
				A->P.Turbo.MaxRatio_1C + 1);

		XSetForeground(A->W.display, A->W.gc, A->W.foreground);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width,
				A->W.margin.height + A->W.extents.charHeight,
				"Core", 4);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Turbo.MaxRatio_1C >> 1) ),
				A->W.margin.height + A->W.extents.charHeight,
				A->L.bclock, strlen(A->L.bclock));

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + A->W.extents.overall.width - A->W.extents.charWidth,
				A->W.margin.height + A->W.extents.charHeight,
				"Temps", 5);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width,
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				"Ratio", 5);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MinimumRatio << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.ratios[0], 2);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Platform.MaxNonTurboRatio << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.ratios[2], 2);

		XDrawString(	A->W.display,
				A->W.pixmap.B,
				A->W.gc,
				A->W.margin.width + ( A->W.extents.charWidth * (A->P.Turbo.MaxRatio_1C << 1) ),
				A->W.margin.height + ( A->W.extents.charHeight * (A->P.ThreadCount + 1 + 1) ),
				&A->L.ratios[4], 2);
	}
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
		XCopyArea(A->W.display, A->D.window, A->W.pixmap.B, A->W.gc,
				A->W.x, A->W.y,
				A->W.attribs.width, A->W.attribs.height,
				0, 0);
	}
	XCopyArea(A->W.display, A->W.pixmap.B, A->W.pixmap.F, A->W.gc, 0, 0, A->W.width, A->W.height, 0, 0);
}

void	FlushLayout(uARG *A) {
	XCopyArea(A->W.display,A->W.pixmap.F,A->W.window,A->W.gc, 0, 0, A->W.width, A->W.height, 0, 0);
	XFlush(A->W.display);
}

void	DrawPulse(uARG *A) {
	XSetForeground(A->W.display, A->W.gc, (A->W.pulse=!A->W.pulse) ? 0xff0000:A->W.foreground);
	XDrawArc(A->W.display, A->W.pixmap.F, A->W.gc,
		A->W.width - (A->W.extents.charWidth + (A->W.extents.charWidth >> 1)),
		A->W.height - (A->W.extents.charHeight - (A->W.extents.charWidth >> 2)),
		A->W.extents.charWidth,
		A->W.extents.charWidth,
		0, 360 << 8);
}

void	DrawLayout(uARG *A) {
	switch(A->L.currentPage) {
		case CORE: {
			int cpu=0;
			for(cpu=0; cpu < A->P.ThreadCount; cpu++) {
				A->L.usage[cpu].x=A->W.margin.width;
				A->L.usage[cpu].y=3 + A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1) );
				A->L.usage[cpu].width=(A->W.extents.overall.width * A->P.Core[cpu].Ratio) / A->P.Turbo.MaxRatio_1C;
				A->L.usage[cpu].height=A->W.extents.charHeight - 3;

				if(A->P.Core[cpu].Ratio < A->P.Platform.MinimumRatio)
					XSetForeground(A->W.display, A->W.gc, A->W.foreground);
				if(A->P.Core[cpu].Ratio >= A->P.Platform.MinimumRatio)
					XSetForeground(A->W.display, A->W.gc, 0x009966);
				if(A->P.Core[cpu].Ratio >= A->P.Platform.MaxNonTurboRatio)
					XSetForeground(A->W.display, A->W.gc, 0xffa500);
				if(A->P.Core[cpu].Ratio >= A->P.Turbo.MaxRatio_4C)
					XSetForeground(A->W.display, A->W.gc, 0xffff00);

				XFillRectangle(A->W.display, A->W.pixmap.F, A->W.gc,
						A->L.usage[cpu].x,
						A->L.usage[cpu].y,
						A->L.usage[cpu].width,
						A->L.usage[cpu].height);

				XSetForeground(A->W.display, A->W.gc, A->W.foreground);
				sprintf(A->L.string, CORE_FREQ, cpu, A->P.Core[cpu].Freq);
				XDrawImageString(A->W.display, A->W.pixmap.F, A->W.gc,
						A->W.margin.width,
						A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ),
						A->L.string, strlen(A->L.string));

				sprintf(A->L.string, "%2d", A->P.Core[cpu].TjMax.Target - A->P.Core[cpu].Therm.DTS);
				XDrawString(A->W.display, A->W.pixmap.F, A->W.gc,
						A->W.margin.width + A->W.extents.overall.width + (A->W.extents.charWidth << 1),
						A->W.margin.height + ( A->W.extents.charHeight * (cpu + 1 + 1) ),
						A->L.string, 2);
			}
			XSetForeground(A->W.display, A->W.gc, 0x666666);
			XDrawRectangles(A->W.display, A->W.pixmap.F, A->W.gc, A->L.usage, A->P.ThreadCount);
		}
		default:
			break;
	}
}

void	UpdateTitle(uARG *A) {
	sprintf(A->L.string, APP_TITLE,
		A->P.Top, A->P.Core[A->P.Top].Freq, A->P.Core[A->P.Top].TjMax.Target - A->P.Core[A->P.Top].Therm.DTS);
	XStoreName(A->W.display, A->W.window, A->L.string);
	XSetIconName(A->W.display, A->W.window, A->L.string);
}

static void *uExec(void *uArg) {
	uARG *A=(uARG *) uArg;

	int cpu=0, max=0;
	while(A->LOOP) {
		for(cpu=0, max=0; cpu < A->P.ThreadCount; cpu++) {
			Read_MSR(A->P.Core[cpu].FD, IA32_PERF_STATUS, (unsigned long long *) &A->P.Core[cpu].Ratio);
			A->P.Core[cpu].Freq=A->P.Core[cpu].Ratio * A->P.ClockSpeed;
			if(max < A->P.Core[cpu].Ratio) {
				max=A->P.Core[cpu].Ratio;
				A->P.Top=cpu;
			}
			Read_MSR(A->P.Core[cpu].FD, IA32_THERM_STATUS, (THERM *) &A->P.Core[cpu].Therm);
		}
		if(!A->PAUSE) {
			MapLayout(A);
			DrawLayout(A);
			if(A->W.activity)
				DrawPulse(A);
			FlushLayout(A);
			UpdateTitle(A);
		}
		usleep(A->P.IdleTime);
	}
	return(NULL);
}

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

int	Help(uARG *A, int argc, char *argv[]) {
	OPTION	options[] = {	{"-x", "%d", &A->W.x,             "Left position"},
				{"-y", "%d", &A->W.y,             "Top position" },
				{"-F", "%s", &A->W.extents.fname, "Font name"},
				{"-b", "%x", &A->W.background,    "Background color"},
				{"-f", "%x", &A->W.foreground,    "Foreground color"},
				{"-D", "%lx",&A->D.window,        "Desktop Window id"},
				{"-s", "%ld",&A->P.IdleTime,      "Idle time (usec)"},
				{"-a", "%ld",&A->W.activity,      "Pulse activity (0/1)"},
				{"-t", "%ld",&A->W.alwaysOnTop,   "Always On Top (0/1)"},
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

int main(int argc, char *argv[]) {
	uARG	A= {
			LOOP: true,
			PAUSE: true,
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
		A.M=IMC_Read_Info();

		if(XInitThreads() && OpenLayout(&A))
		{
			BuildLayout(&A);
			MapLayout(&A);
			DrawLayout(&A);
			FlushLayout(&A);
			XMapWindow(A.W.display, A.W.window);

			pthread_t TID_Exec=0;
			if(!pthread_create(&TID_Exec, NULL, uExec, &A))
				uLoop((void*) &A);
			pthread_join(TID_Exec, NULL);

			CloseLayout(&A);
		}
		Close_MSR(&A);
		return(0);
	}
	else
		return(1);
}
